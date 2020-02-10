/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018 Rajko Stojadinovic
 * Copyright (c) 2018-2019 CTCaer
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <stdlib.h>

#include "fe_emmc_tools.h"
#include "../../common/memory_map.h"
#include "../config/config.h"
#include "../config/ini.h"
#include "../gfx/gfx.h"
#include "../gfx/tui.h"
#include "../libs/fatfs/ff.h"
#include "../mem/heap.h"
#include "../sec/se.h"
#include "../storage/nx_emmc.h"
#include "../storage/sdmmc.h"
#include "../utils/btn.h"
#include "../utils/util.h"

#define NUM_SECTORS_PER_ITER 8192 // 4MB Cache.
#define OUT_FILENAME_SZ 128
#define SHA256_SZ 0x20

extern sdmmc_t sd_sdmmc;
extern sdmmc_storage_t sd_storage;
extern FATFS sd_fs;

extern bool sd_mount();
extern void sd_unmount();
extern void emmcsn_path_impl(char *path, char *sub_dir, char *filename, sdmmc_storage_t *storage);
extern void emmc_path_impl(char *path, char *sub_dir, char *filename, sdmmc_storage_t *storage);

bool noszchk;
#pragma GCC push_options
#pragma GCC optimize ("Os")

int _dump_emmc_verify(sdmmc_storage_t *storage, u32 lba_curr, char *outFilename, emmc_part_t *part)
{
	FIL fp;
	u8 sparseShouldVerify = 4;
	u32 btn = 0;
	u32 prevPct = 200;
	u32 sdFileSector = 0;
	int res = 0;

	u8 hashEm[SHA256_SZ];
	u8 hashSd[SHA256_SZ];

	if (f_open(&fp, outFilename, FA_READ) == FR_OK)
	{
		u32 totalSectorsVer = (u32)((u64)f_size(&fp) >> (u64)9);

		u8 *bufEm = (u8 *)EMMC_BUF_ALIGNED;
		u8 *bufSd = (u8 *)SDXC_BUF_ALIGNED;

		u32 pct = (u64)((u64)(lba_curr - part->lba_start) * 100u) / (u64)(part->lba_end - part->lba_start);
		tui_pbar(0, gfx_con.y, pct, 0xFFFFFF00, 0xFF00FF00);

		u32 num = 0;
		while (totalSectorsVer > 0)
		{
			num = MIN(totalSectorsVer, NUM_SECTORS_PER_ITER);

			// Check every time or every 4.
			// Every 4 protects from fake sd, sector corruption and frequent I/O corruption.
			// Full provides all that, plus protection from extremely rare I/O corruption.
			if (!(sparseShouldVerify % 4))
			{
				if (!sdmmc_storage_read(storage, lba_curr, num, bufEm))
				{
					gfx_con.fntsz = 16;
					EPRINTF("\nVerification failed\n");

					f_close(&fp);
					return 1;
				}
				f_lseek(&fp, (u64)sdFileSector << (u64)9);
				if (f_read(&fp, bufSd, num << 9, NULL))
				{
					gfx_con.fntsz = 16;
					EPRINTF("\nVerification failed\n");

					f_close(&fp);
					return 1;
				}

				se_calc_sha256(hashEm, bufEm, num << 9);
				se_calc_sha256(hashSd, bufSd, num << 9);
				res = memcmp(hashEm, hashSd, 0x10);

				if (res)
				{
					gfx_con.fntsz = 16;
					EPRINTF("\nVerification failed\n");

					f_close(&fp);
					return 1;
				}
			}

			pct = (u64)((u64)(lba_curr - part->lba_start) * 100u) / (u64)(part->lba_end - part->lba_start);
			if (pct != prevPct)
			{
				tui_pbar(0, gfx_con.y, pct, 0xFFFFFF00, 0xFF00FF00);
				prevPct = pct;
			}

			lba_curr += num;
			totalSectorsVer -= num;
			sdFileSector += num;
			sparseShouldVerify++;

			btn = btn_wait_timeout(0, BTN_VOL_DOWN | BTN_VOL_UP);
			if ((btn & BTN_VOL_DOWN) && (btn & BTN_VOL_UP))
			{
				gfx_con.fntsz = 16;
				WPRINTF("\n\nVerification was cancelled!");
				gfx_con.fntsz = 16;
				msleep(1000);

				f_close(&fp);

				return 0;
			}
		}
		f_close(&fp);

		tui_pbar(0, gfx_con.y, pct, 0xFFCCCCCC, 0xFF555555);

		return 0;
	}
	else
	{
		gfx_con.fntsz = 16;
		EPRINTF("\nFile not found.\n\nVerification failed..\n");
		return 1;
	}
}

void _update_filename(char *outFilename, u32 sdPathLen, u32 numSplitParts, u32 currPartIdx)
{
	if (numSplitParts >= 10 && currPartIdx < 10)
	{
		outFilename[sdPathLen] = '0';
		itoa(currPartIdx, &outFilename[sdPathLen + 1], 10);
	}
	else
		itoa(currPartIdx, &outFilename[sdPathLen], 10);
}

int _dump_emmc_part(char *sd_path, sdmmc_storage_t *storage, emmc_part_t *part)
{
	static const u32 FAT32_FILESIZE_LIMIT = 0xFFFFFFFF;
	//static const u32 SECTORS_TO_MIB_COEFF = 11;

	u32 multipartSplitSize = (1u << 31);
	u32 totalSectors = part->lba_end - part->lba_start + 1;
	u32 currPartIdx = 0;
	u32 numSplitParts = 0;
	u32 maxSplitParts = 0;
	u32 btn = 0;
	bool isSmallSdCard = false;
	bool partialDumpInProgress = false;
	int res = 0;
	char *outFilename = sd_path;
	u32 sdPathLen = strlen(sd_path);

	FIL partialIdxFp;
	char partialIdxFilename[12];
	strcpy(partialIdxFilename, "partial.idx");

	gfx_con.fntsz = 16;
	// 1GB parts for sd cards 8GB and less.
	if ((sd_storage.csd.capacity >> (20 - sd_storage.csd.read_blkbits)) <= 8192)
		multipartSplitSize = (1u << 30);
	// Maximum parts fitting the free space available.
	maxSplitParts = (sd_fs.free_clst * sd_fs.csize) / (multipartSplitSize / NX_EMMC_BLOCKSIZE);

	// Check if the USER partition or the RAW eMMC fits the sd card free space.
	if (totalSectors > (sd_fs.free_clst * sd_fs.csize))
	{
		isSmallSdCard = true;

		gfx_printf("%k\nNot enough space.%k\n", 0xFFFFBA00, 0xFFCCCCCC);

		if (!maxSplitParts)
		{
			gfx_con.fntsz = 16;
			EPRINTF("Not enough space for Partial Backup.");

			return 0;
		}
	}
	// Check if we are continuing a previous raw eMMC or USER partition backup in progress.
	if (f_open(&partialIdxFp, partialIdxFilename, FA_READ) == FR_OK && totalSectors > (FAT32_FILESIZE_LIMIT / NX_EMMC_BLOCKSIZE))
	{
		gfx_printf("%kPartial Backup. Continuing...%k\n", 0xFFAEFD14, 0xFFCCCCCC);

		partialDumpInProgress = true;
		// Force partial dumping, even if the card is larger.
		isSmallSdCard = true;

		f_read(&partialIdxFp, &currPartIdx, 4, NULL);
		f_close(&partialIdxFp);

		if (!maxSplitParts)
		{
			gfx_con.fntsz = 16;
			EPRINTF("Not enough space for Partial Backup.");

			return 0;
		}

		// Increase maxSplitParts to accommodate previously backed up parts.
		maxSplitParts += currPartIdx;
	}
	else if (isSmallSdCard)
		gfx_printf("%kPartial Backup enabled.%k\n", 0xFFFFBA00, 0xFFCCCCCC);

	// Check if filesystem is FAT32 or the free space is smaller and backup in parts.
	if (((sd_fs.fs_type != FS_EXFAT) && totalSectors > (FAT32_FILESIZE_LIMIT / NX_EMMC_BLOCKSIZE)) || isSmallSdCard)
	{
		u32 multipartSplitSectors = multipartSplitSize / NX_EMMC_BLOCKSIZE;
		numSplitParts = (totalSectors + multipartSplitSectors - 1) / multipartSplitSectors;

		outFilename[sdPathLen++] = '.';

		// Continue from where we left, if Partial Backup in progress.
		_update_filename(outFilename, sdPathLen, numSplitParts, partialDumpInProgress ? currPartIdx : 0);
	}

	FIL fp;
	
	if (!f_open(&fp, outFilename, FA_READ))
	{
		f_close(&fp);
		gfx_con.fntsz = 16;
		gfx_con_setpos(0, (gfx_con.savedy+16));
		WPRINTF("Exists.[VOL] skip / [PWR] Overwrite");
		msleep(500);

		if (!(btn_wait() & BTN_POWER))
			return 0;
		gfx_con.fntsz = 16;
	//gfx_con_getpos(&gfx_con.savedx, &gfx_con.savedy);
	}
	//gfx_con_setpos(gfx_con.savedx, gfx_con.savedy);
	res = f_open(&fp, outFilename, FA_CREATE_ALWAYS | FA_WRITE);
	if (res)
	{
		gfx_con.fntsz = 16;
		EPRINTFARGS("Error (%d) creating file.\n", res);

		return 0;
	}
	
	u8 *buf = (u8 *)MIXD_BUF_ALIGNED;

	u32 lba_curr = part->lba_start;
	u32 lbaStartPart = part->lba_start;
	u32 bytesWritten = 0;
	u32 prevPct = 200;
	int retryCount = 0;

	// Continue from where we left, if Partial Backup in progress.
	if (partialDumpInProgress)
	{
		lba_curr += currPartIdx * (multipartSplitSize / NX_EMMC_BLOCKSIZE);
		totalSectors -= currPartIdx * (multipartSplitSize / NX_EMMC_BLOCKSIZE);
		lbaStartPart = lba_curr; // Update the start LBA for verification.
	}
	u64 totalSize = (u64)((u64)totalSectors << 9);
	if (!isSmallSdCard && (sd_fs.fs_type == FS_EXFAT || totalSize <= FAT32_FILESIZE_LIMIT))
		f_lseek(&fp, totalSize);
	else
		f_lseek(&fp, MIN(totalSize, multipartSplitSize));
	f_lseek(&fp, 0);

	u32 num = 0;
	u32 pct = 0;
	while (totalSectors > 0)
	{
		if (numSplitParts != 0 && bytesWritten >= multipartSplitSize)
		{
			f_close(&fp);
			memset(&fp, 0, sizeof(fp));
			currPartIdx++;


				// Verify part.
				if (_dump_emmc_verify(storage, lbaStartPart, outFilename, part))
				{
					EPRINTF("\nPress any key and try again...\n");

					return 0;
				}

			_update_filename(outFilename, sdPathLen, numSplitParts, currPartIdx);

			// Always create partial.idx before next part, in case a fatal error occurs.
			if (isSmallSdCard)
			{
				// Create partial backup index file.
				if (f_open(&partialIdxFp, partialIdxFilename, FA_CREATE_ALWAYS | FA_WRITE) == FR_OK)
				{
					f_write(&partialIdxFp, &currPartIdx, 4, NULL);
					f_close(&partialIdxFp);
				}
				else
				{
					gfx_con.fntsz = 16;
					EPRINTF("\nError creating partial.idx file.\n");

					return 0;
				}

				// More parts to backup that do not currently fit the sd card free space or fatal error.
				if (currPartIdx >= maxSplitParts)
				{
					gfx_puts("\n\n1. Press any key to unmount SD Card.\n\
						2. Move files to free space.\n\
						   Don\'t move partial.idx file!\n\
						3. Re-insert SD Card.\n\
						4. Select the SAME option again.\n");
					gfx_con.fntsz = 16;

					return 1;
				}
			}

			// Create next part.
			//gfx_con_setpos(gfx_con.savedx, gfx_con.savedy);
			gfx_printf("Filename: %s\n\n", outFilename);
			lbaStartPart = lba_curr;
			res = f_open(&fp, outFilename, FA_CREATE_ALWAYS | FA_WRITE);
			if (res)
			{
				gfx_con.fntsz = 16;
				EPRINTFARGS("Error (%d) creating file.\n", res);

				return 0;
			}

			bytesWritten = 0;

			totalSize = (u64)((u64)totalSectors << 9);
			f_lseek(&fp, MIN(totalSize, multipartSplitSize));
			f_lseek(&fp, 0);
		}

		retryCount = 0;
		num = MIN(totalSectors, NUM_SECTORS_PER_ITER);
		while (!sdmmc_storage_read(storage, lba_curr, num, buf))
		{
			EPRINTF("Error reading from eMMC, retrying...");

			msleep(150);
			if (retryCount >= 3)
			{
				gfx_con.fntsz = 16;
				EPRINTF("\nFailed reading from eMMC. Aborting.\n");
				EPRINTF("\nPress any key and try again...\n");

				f_close(&fp);
				f_unlink(outFilename);

				return 0;
			}
		}
		res = f_write(&fp, buf, NX_EMMC_BLOCKSIZE * num, NULL);
		if (res)
		{
			gfx_con.fntsz = 16;
			EPRINTFARGS("\nFatal error (%d) writing to SD Card", res);
			EPRINTF("\nPress any key and try again...\n");

			f_close(&fp);
			f_unlink(outFilename);

			return 0;
		}
		pct = (u64)((u64)(lba_curr - part->lba_start) * 100u) / (u64)(part->lba_end - part->lba_start);
		if (pct != prevPct)
		{
			tui_pbar(0, gfx_con.y, pct, 0xFFCCCCCC, 0xFF555555);
			prevPct = pct;
		}

		lba_curr += num;
		totalSectors -= num;
		bytesWritten += num * NX_EMMC_BLOCKSIZE;

		// Force a flush after a lot of data if not splitting.
		if (numSplitParts == 0 && bytesWritten >= multipartSplitSize)
		{
			f_sync(&fp);
			bytesWritten = 0;
		}

		btn = btn_wait_timeout(0, BTN_VOL_DOWN | BTN_VOL_UP);
		if ((btn & BTN_VOL_DOWN) && (btn & BTN_VOL_UP))
		{
			gfx_con.fntsz = 16;
			WPRINTF("\nThe backup was cancelled!");
			EPRINTF("\nPress any key...");
			btn_wait();

			f_close(&fp);
			f_unlink(outFilename);

			return 0;
		}
	}
	tui_pbar(0, gfx_con.y, 100, 0xFFCCCCCC, 0xFF555555);

	// Backup operation ended successfully.
	f_close(&fp);

		// Verify last part or single file backup.
		if (_dump_emmc_verify(storage, lbaStartPart, outFilename, part))
		{
			EPRINTF("\nPress any key and try again...\n");

			return 0;
		}
		else
			tui_pbar(0, gfx_con.y, 100, 0xFFFFFF00, 0xFF00FF00);


	gfx_con.fntsz = 16;
	// Remove partial backup index file if no fatal errors occurred.
	if (isSmallSdCard)
	{
		f_unlink(partialIdxFilename);
		gfx_printf("%k\nFiles ready for joining.", 0xFFCCCCCC);
	}
	gfx_puts("\n");

	return 1;
}

typedef enum
{
	PART_BOOT =				(1 << 0),
	PART_SYSTEM =			(1 << 1),
	PART_USER =				(1 << 2),
	PART_RAW =				(1 << 3),
	PART_PRODBOOTS =		(1 << 4),
	PART_PRODINFO_ONLY =	(1 << 5),
	PART_GP_ALL =			(1 << 7),
	
} emmcPartType_t;

void _dump_emmc_selected(emmcPartType_t dumpType)
{
	int res = 0;
	u32 timer = 0;
	gfx_clear_grey(0x00);
	gfx_con_setpos(0, 0);

	if (!sd_mount())
		goto out;

	gfx_puts("Checking for available free space...\n\n");
	// Get SD Card free space for Partial Backup.
	f_getfree("", &sd_fs.free_clst, NULL);
	
	sdmmc_storage_t storage;
	sdmmc_t sdmmc;
	if (!sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4))
	{
		EPRINTF("Failed to init eMMC.");
		goto out;
	}

	int i = 0;
	char sdPath[OUT_FILENAME_SZ];
	// Create Restore folders, if they do not exist.

	timer = get_tmr_s();
	if (dumpType & PART_PRODBOOTS)
		{
			emmc_part_t prodinfoPart;
			memset(&prodinfoPart, 0, sizeof(prodinfoPart));
			prodinfoPart.lba_start = 34;
			prodinfoPart.lba_end = 8192 - 1;
			strcpy(prodinfoPart.name, "PRODINFO");
			{
				gfx_printf("%k%02d: %s (%07X-%07X)%k\n", 0xFF00FF00, i++,
					prodinfoPart.name, prodinfoPart.lba_start, prodinfoPart.lba_end, 0xFFFFFFFF);
					gfx_con_getpos(&gfx_con.savedx, &gfx_con.savedy);
				emmc_path_impl(sdPath, "", prodinfoPart.name, &storage);
				res = _dump_emmc_part(sdPath, &storage, &prodinfoPart);
			}
			const u32 BOOT_PART_SIZE = storage.ext_csd.boot_mult << 17;

		
		emmc_part_t bootPart;
		memset(&bootPart, 0, sizeof(bootPart));
		bootPart.lba_start = 0;
		bootPart.lba_end = (BOOT_PART_SIZE / NX_EMMC_BLOCKSIZE) - 1;
			for (i = 0; i < 2; i++)
			{
			memcpy(bootPart.name, "BOOT", 5);
			bootPart.name[4] = (u8)('0' + i);
			bootPart.name[5] = 0;

			gfx_printf("%k%02d: %s (%07X-%07X)%k\n", 0xFF00FF00, i,
				bootPart.name, bootPart.lba_start, bootPart.lba_end, 0xFFFFFFFF);
			gfx_con_getpos(&gfx_con.savedx, &gfx_con.savedy);
			sdmmc_storage_set_mmc_partition(&storage, i + 1);

			emmc_path_impl(sdPath, "", bootPart.name, &storage);
			res = _dump_emmc_part(sdPath, &storage, &bootPart);
			}
		}
	if (dumpType & PART_BOOT)
	{
		const u32 BOOT_PART_SIZE = storage.ext_csd.boot_mult << 17;

		emmc_part_t bootPart;
		memset(&bootPart, 0, sizeof(bootPart));
		bootPart.lba_start = 0;
		bootPart.lba_end = (BOOT_PART_SIZE / NX_EMMC_BLOCKSIZE) - 1;
		for (i = 0; i < 2; i++)
		{
			strcpy(bootPart.name, "BOOT");
			bootPart.name[4] = (u8)('0' + i);
			bootPart.name[5] = 0;

			gfx_printf("%k%02d: %s (%07X-%07X)%k\n", 0xFF00DDFF, i,
				bootPart.name, bootPart.lba_start, bootPart.lba_end, 0xFFCCCCCC);
			gfx_con_getpos(&gfx_con.savedx, &gfx_con.savedy);
			sdmmc_storage_set_mmc_partition(&storage, i + 1);

			emmcsn_path_impl(sdPath, "", bootPart.name, &storage);
			res = _dump_emmc_part(sdPath, &storage, &bootPart);
		}
	}

	if ((dumpType & PART_SYSTEM) || (dumpType & PART_USER) || (dumpType & PART_RAW))
	{
		sdmmc_storage_set_mmc_partition(&storage, 0);

		if ((dumpType & PART_SYSTEM) || (dumpType & PART_USER))
		{
			LIST_INIT(gpt);
			nx_emmc_gpt_parse(&gpt, &storage);
			LIST_FOREACH_ENTRY(emmc_part_t, part, &gpt, link)
			{
				if ((dumpType & PART_USER) == 0 && !strcmp(part->name, "USER"))
					continue;
				if ((dumpType & PART_SYSTEM) == 0 && strcmp(part->name, "USER"))
					continue;

				gfx_printf("%k%02d: %s (%07X-%07X)%k\n", 0xFF00DDFF, i++,
					part->name, part->lba_start, part->lba_end, 0xFFCCCCCC);
				gfx_con_getpos(&gfx_con.savedx, &gfx_con.savedy);
				emmcsn_path_impl(sdPath, "/partitions", part->name, &storage);
				res = _dump_emmc_part(sdPath, &storage, part);
				// If a part failed, don't continue.
				if (!res)
					break;
			}
			nx_emmc_gpt_free(&gpt);
		}

		if (dumpType & PART_RAW)
		{
			// Get GP partition size dynamically.
			const u32 RAW_AREA_NUM_SECTORS = storage.sec_cnt;

			emmc_part_t rawPart;
			memset(&rawPart, 0, sizeof(rawPart));
			rawPart.lba_start = 0;
			rawPart.lba_end = RAW_AREA_NUM_SECTORS - 1;
			strcpy(rawPart.name, "rawnand.bin");
			{
				gfx_printf("%k%02d: %s (%07X-%07X)%k\n", 0xFF00DDFF, i++,
					rawPart.name, rawPart.lba_start, rawPart.lba_end, 0xFFCCCCCC);
				gfx_con_getpos(&gfx_con.savedx, &gfx_con.savedy);
				emmcsn_path_impl(sdPath, "", rawPart.name, &storage);
				res = _dump_emmc_part(sdPath, &storage, &rawPart);
			}
		}
	}

	gfx_putc('\n');
	timer = get_tmr_s() - timer;
	gfx_printf("Time taken: %dm %ds.\n", timer / 60, timer % 60);
	sdmmc_storage_end(&storage);
	if (res)
		gfx_printf("\nFinished! Press any key...\n");

out:
	sd_unmount();
	btn_wait();
}

void dump_emmc_system()  { _dump_emmc_selected(PART_SYSTEM); }
void dump_emmc_user()    { _dump_emmc_selected(PART_USER); }
void dump_emmc_boot()    { _dump_emmc_selected(PART_BOOT); }
void dump_emmc_rawnand() { _dump_emmc_selected(PART_RAW); }
void dump_emmc_quick()	 { _dump_emmc_selected(PART_PRODBOOTS); }

int _restore_emmc_part(char *sd_path, sdmmc_storage_t *storage, emmc_part_t *part, bool allow_multi_part)
{
	//static const u32 SECTORS_TO_MIB_COEFF = 11;

	u32 totalSectors = part->lba_end - part->lba_start + 1;
	u32 currPartIdx = 0;
	u32 numSplitParts = 0;
	u32 lbaStartPart = part->lba_start;
	int res = 0;
	char *outFilename = sd_path;
	u32 sdPathLen = strlen(sd_path);
	u64 fileSize = 0;
	u64 totalCheckFileSize = 0;
	gfx_con.fntsz = 16;

	FIL fp;
	FILINFO fno;

	//gfx_con_getpos(&gfx_con.savedx, &gfx_con.savedy);

	bool use_multipart = false;

	if (allow_multi_part)
	{
		// Check to see if there is a combined file and if so then use that.
		if (f_stat(outFilename, &fno))
		{
			// If not, check if there are partial files and the total size matches.
			gfx_printf("No single file. Checking for partial.\n");

			outFilename[sdPathLen++] = '.';

			// Stat total size of the part files.
			while ((u32)((u64)totalCheckFileSize >> (u64)9) != totalSectors)
			{
				_update_filename(outFilename, sdPathLen, 99, numSplitParts);

				
				gfx_printf("\nFilename: %s\n", outFilename);

				if (f_stat(outFilename, &fno))
				{
					WPRINTFARGS("Error (%d) file not found.\n", res);
					return 0;
				}
				else
					totalCheckFileSize += (u64)fno.fsize;
				
				//gfx_con_setpos(gfx_con.savedx, gfx_con.savedy);
				numSplitParts++;
			}

			if ((u32)((u64)totalCheckFileSize >> (u64)9) != totalSectors)
			{
				if (!noszchk){
					gfx_con.fntsz = 16;
					EPRINTF("Size mismatch!\n");
					f_close(&fp);
					return 0;
					} else 
					{
						gfx_con.fntsz = 16;
						EPRINTF("This is dangerous. Ensure you\nhave a backup before continuing\n");
						gfx_printf("\n[PWR] Continue.  [VOL] Cancel\n\n");
						u32 btn = btn_wait();
						if (!(btn & BTN_POWER)){
							gfx_printf("Cancelled.\n\n");
							totalSectors = 0;
							}
						totalSectors = ((u32)((u64)totalCheckFileSize >> (u64)9));
					}
			}
			else
			{
				use_multipart = true;
				_update_filename(outFilename, sdPathLen, numSplitParts, 0);
			}
		}
	}

	res = f_open(&fp, outFilename, FA_READ);
	gfx_printf("Filename: %s\n", outFilename);
	if (res)
	{
		if (res != FR_NO_FILE)
			EPRINTFARGS("Error (%d) opening backup.\n", res);
		else
			WPRINTFARGS("Error (%d) file not found.\n", res);
		gfx_con.fntsz = 16;

		return 0;
	}
	else if (!use_multipart && (((u32)((u64)f_size(&fp) >> (u64)9)) != totalSectors)) // Check total restore size vs emmc size.
	{
		if (!noszchk){
		gfx_con.fntsz = 16;
		EPRINTF("Size mismatch!\n");
		f_close(&fp);
		return 0;
		} else {
			gfx_con.fntsz = 16;
			EPRINTF("This is dangerous. Ensure you\nhave a backup before continuing\n");
			gfx_printf("\n[PWR] Continue.  [VOL] Cancel\n\n");
			u32 btn = btn_wait();
			if (!(btn & BTN_POWER)){
				gfx_printf("Cancelled.\n\n");
				totalSectors = 0;
				}
			totalSectors = ((u32)((u64)f_size(&fp) >> (u64)9));
		}
	}
	else
	{
		fileSize = (u64)f_size(&fp);
	}

	u8 *buf = (u8 *)MIXD_BUF_ALIGNED;

	u32 lba_curr = part->lba_start;
	u32 bytesWritten = 0;
	u32 prevPct = 200;
	int retryCount = 0;

	u32 num = 0;
	u32 pct = 0;

	//gfx_con_getpos(&gfx_con.savedx, &gfx_con.savedy);

	while (totalSectors > 0)
	{
		// If we have more than one part, check the size for the split parts and make sure that the bytes written is not more than that.
		if (numSplitParts != 0 && bytesWritten >= fileSize)
		{
			// If we have more bytes written then close the file pointer and increase the part index we are using
			f_close(&fp);
			memset(&fp, 0, sizeof(fp));
			currPartIdx++;

			if (_dump_emmc_verify(storage, lbaStartPart, outFilename, part))
				{
					EPRINTF("\nPress any key and try again...\n");

					return 0;
				}

			_update_filename(outFilename, sdPathLen, numSplitParts, currPartIdx);

			// Read from next part.

			lbaStartPart = lba_curr;

			// Try to open the next file part
			res = f_open(&fp, outFilename, FA_READ);
			if (res)
			{
				gfx_con.fntsz = 16;
				EPRINTFARGS("Error (%d) opening file.\n", res);

				return 0;
			}
			fileSize = (u64)f_size(&fp);
			bytesWritten = 0;
		}

		retryCount = 0;
		num = MIN(totalSectors, NUM_SECTORS_PER_ITER);

		res = f_read(&fp, buf, NX_EMMC_BLOCKSIZE * num, NULL);
		if (res)
		{
			gfx_con.fntsz = 16;
			EPRINTFARGS("\nFatal error (%d) SD Card", res);
			EPRINTF("\nYour device may not boot. Press any key to retry.\n");

			f_close(&fp);
			return 0;
		}
		while (!sdmmc_storage_write(storage, lba_curr, num, buf))
		{
			EPRINTFARGS("Error writing to eMMC (try %d), retrying...",++retryCount);

			msleep(150);
			if (retryCount >= 3)
			{
				gfx_con.fntsz = 16;
				EPRINTF("\nFailed writing from eMMC. Aborting..\n");
				EPRINTF("\nYour device may not boot. Press any key to retry.\n");

				f_close(&fp);
				return 0;
			}
		}
		pct = (u64)((u64)(lba_curr - part->lba_start) * 100u) / (u64)(part->lba_end - part->lba_start);
		if (pct != prevPct)
		{
			tui_pbar(0, gfx_con.y, pct, 0xFFCCCCCC, 0xFF555555);
			prevPct = pct;
		}

		lba_curr += num;
		totalSectors -= num;
		bytesWritten += num * NX_EMMC_BLOCKSIZE;
	}
	tui_pbar(0, gfx_con.y, 100, 0xFFCCCCCC, 0xFF555555);

	// Restore operation ended successfully.
	f_close(&fp);

	if (_dump_emmc_verify(storage, lbaStartPart, outFilename, part))
		{
			EPRINTF("\nPress any key and try again...\n");

			return 0;
		}
		else
			tui_pbar(0, gfx_con.y, 100, 0xFFFFFF00, 0xFF00FF00);

	gfx_con.fntsz = 16;
	gfx_puts("\n\n");

	return 1;
}

static void _restore_emmc_selected(emmcPartType_t restoreType)
{
	int res = 0;
	u32 timer = 0;
	gfx_clear_grey(0x00);
	gfx_con_setpos(0, 0);

	gfx_printf("%kThis may render your device\ninoperative!\n\n", 0xFFFFDD00);
	gfx_printf("Are you really sure?\n%k", 0xFFCCCCCC);
	if ((restoreType & PART_BOOT) || (restoreType & PART_GP_ALL))
	{
		gfx_puts("Only found partitions are restored\n");
	}
	gfx_con_getpos(&gfx_con.savedx, &gfx_con.savedy);

	u8 failsafe_wait = 10;
	while (failsafe_wait > 0)
	{
		gfx_con_setpos(gfx_con.savedx, gfx_con.savedy);
		gfx_printf("%kWait... (%ds)    %k", 0xFF888888, failsafe_wait, 0xFFCCCCCC);
		msleep(1000);
		failsafe_wait--;
	}

	gfx_puts("\n[PWR] Continue.\n[VOL] skip/menu.\n\n");

	u32 btn = btn_wait();
	if (!(btn & BTN_POWER))
		goto out;

	if (!sd_mount())
		goto out;
	gfx_clear_grey(0x00);
	gfx_con_setpos(0,0);
	sdmmc_storage_t storage;
	sdmmc_t sdmmc;
	if (!sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4))
	{
		EPRINTF("Failed to init eMMC.");
		goto out;
	}

	int i = 0;
	char sdPath[OUT_FILENAME_SZ];

	timer = get_tmr_s();
	if (restoreType & PART_BOOT)
	{
		const u32 BOOT_PART_SIZE = storage.ext_csd.boot_mult << 17;

		emmc_part_t bootPart;
		memset(&bootPart, 0, sizeof(bootPart));
		bootPart.lba_start = 0;
		bootPart.lba_end = (BOOT_PART_SIZE / NX_EMMC_BLOCKSIZE) - 1;
		for (i = 0; i < 2; i++)
		{
			strcpy(bootPart.name, "BOOT");
			bootPart.name[4] = (u8)('0' + i);
			bootPart.name[5] = 0;

			gfx_printf("%k%02d: %s (%07X-%07X)%k\n", 0xFF00DDFF, i,
				bootPart.name, bootPart.lba_start, bootPart.lba_end, 0xFFCCCCCC);

			sdmmc_storage_set_mmc_partition(&storage, i + 1);

			emmcsn_path_impl(sdPath, "", bootPart.name, &storage);
			res = _restore_emmc_part(sdPath, &storage, &bootPart, false);
		}
	}

	if (restoreType & PART_GP_ALL)
	{
		sdmmc_storage_set_mmc_partition(&storage, 0);

		LIST_INIT(gpt);
		nx_emmc_gpt_parse(&gpt, &storage);
		LIST_FOREACH_ENTRY(emmc_part_t, part, &gpt, link)
		{
			gfx_printf("%k%02d: %s (%07X-%07X)%k\n", 0xFF00DDFF, i++,
				part->name, part->lba_start, part->lba_end, 0xFFCCCCCC);

			emmcsn_path_impl(sdPath, "/partitions", part->name, &storage);
			res = _restore_emmc_part(sdPath, &storage, part, false);
		}
		nx_emmc_gpt_free(&gpt);
	}

	if (restoreType & PART_RAW)
	{
		// Get GP partition size dynamically.
		const u32 RAW_AREA_NUM_SECTORS = storage.sec_cnt;

		emmc_part_t rawPart;
		memset(&rawPart, 0, sizeof(rawPart));
		rawPart.lba_start = 0;
		rawPart.lba_end = RAW_AREA_NUM_SECTORS - 1;
		strcpy(rawPart.name, "rawnand.bin");
		{
			gfx_printf("%k%02d: %s (%07X-%07X)%k\n", 0xFF00DDFF, i++,
				rawPart.name, rawPart.lba_start, rawPart.lba_end, 0xFFCCCCCC);

			emmcsn_path_impl(sdPath, "", rawPart.name, &storage);
			res = _restore_emmc_part(sdPath, &storage, &rawPart, true);
		}
	}
	
	if (restoreType & PART_PRODBOOTS)
	{
		// Get GP partition size dynamically.
		//const u32 RAW_AREA_NUM_SECTORS = storage.sec_cnt;
		
		const u32 BOOT_PART_SIZE = storage.ext_csd.boot_mult << 17;

		emmc_part_t bootPart;
		memset(&bootPart, 0, sizeof(bootPart));
		bootPart.lba_start = 0;
		bootPart.lba_end = (BOOT_PART_SIZE / NX_EMMC_BLOCKSIZE) - 1;
		for (i = 0; i < 2; i++)
		{
			memcpy(bootPart.name, "BOOT", 4);
			bootPart.name[4] = (u8)('0' + i);
			bootPart.name[5] = 0;

			gfx_printf("%k%02d: %s (%07X-%07X)%k\n", 0xFF00FF00, i,
				bootPart.name, bootPart.lba_start, bootPart.lba_end, 0xFFFFFFFF);

			sdmmc_storage_set_mmc_partition(&storage, i + 1);

			emmc_path_impl(sdPath, "", bootPart.name, &storage);
			_restore_emmc_part(sdPath, &storage, &bootPart, false);
		}
	}
	
	if (restoreType & PART_PRODINFO_ONLY)
	{
		// Get GP partition size dynamically.
		//const u32 RAW_AREA_NUM_SECTORS = storage.sec_cnt;

		emmc_part_t prodinfoPart;
		memset(&prodinfoPart, 0, sizeof(prodinfoPart));
		prodinfoPart.lba_start = 34;
		prodinfoPart.lba_end = 8192 - 1;
		strcpy(prodinfoPart.name, "PRODINFO");
		{
			gfx_printf("%k%02d: %s (%07X-%07X)%k\n", 0xFF00FF00, i++,
				prodinfoPart.name, prodinfoPart.lba_start, prodinfoPart.lba_end, 0xFFFFFFFF);

			emmc_path_impl(sdPath, "", prodinfoPart.name, &storage);
			_restore_emmc_part(sdPath, &storage, &prodinfoPart, false);
		}
	}

	gfx_putc('\n');
	timer = get_tmr_s() - timer;
	gfx_printf("Time taken: %dm %ds.\n", timer / 60, timer % 60);
	sdmmc_storage_end(&storage);
	if (res)
		gfx_printf("\nFinished! Press any key...\n");

out:
	sd_unmount();
	btn_wait();
}

void restore_emmc_boot()      { _restore_emmc_selected(PART_BOOT); }
void restore_emmc_rawnand()   { _restore_emmc_selected(PART_RAW); }
void restore_emmc_gpp_parts() { _restore_emmc_selected(PART_GP_ALL); }
void restore_emmc_quick()     		{_restore_emmc_selected(PART_PRODBOOTS); }
void restore_emmc_quick_prodinfo()	{_restore_emmc_selected(PART_PRODINFO_ONLY);}
void restore_emmc_quick_noszchk()   {noszchk = true; restore_emmc_boot(); noszchk = false;}

char * file_browser(const char * start_dir,
  const char * required_extn,
    const char * browser_caption,
      const bool get_dir,
        const bool goto_root_on_fail,
          const bool ASCII_order) {

  u8 max_entries = 55;
  char * browse_dir = (char * ) calloc('0', 256);
  DIR dir;
  FILINFO fno;

  if (start_dir) memcpy(browse_dir + 0, start_dir, strlen(start_dir) + 1);

  char * filelist;
  char * file_sec;
  char * file_sec_untrimmed;
  char * select_display = malloc(256);
  u32 i = 0;
  u8 trimsize = 0;
  u8 trimlen;
  u8 err;
  bool home_selected;
  bool select_item;
  bool back_selected;
  bool directory_selected;
  ment_t * ments = (ment_t * ) malloc(sizeof(ment_t) * (max_entries + 9));

  start:
    while (true) {
      filelist = (char * ) calloc(max_entries, 256);

      file_sec = (char * ) calloc('0', 256);
      file_sec_untrimmed = (char * ) calloc('0', 256);
      trimlen = 0;
      i = 0;
      err = 0;
      home_selected = false;
      select_item = false;
      back_selected = false;
      directory_selected = false;
      gfx_clear_grey(0x00);
      gfx_con_setpos(0, 0);

      //j = 0, k = 0;

      memcpy(select_display + 0, "SD:", 4);
      memcpy(select_display + strlen(select_display), browse_dir, strlen(browse_dir) + 1);
      if (!f_opendir( & dir, browse_dir)) {
        for (;;) {
          int res = f_readdir( & dir, & fno);
          if (res || !fno.fname[0]) {

            break;
          }

          if (fno.fattrib & AM_DIR) {
            memcpy(fno.fname + strlen(fno.fname), " <Dir>", 7);
            memcpy(filelist + ((i + 9) * 256), fno.fname, strlen(fno.fname) + 1);
          }

          if (!(fno.fattrib & AM_DIR)) memcpy(filelist + ((i + 9) * 256), fno.fname, strlen(fno.fname) + 1);

          i++;
          if ((i + 9) > (max_entries - 1))
            break;
        }
        f_closedir( & dir);
      }

      if ((i + 9) == 9) {
        err = 1;
        goto out;
      }

      //alphabetical order. Not worth it due to size
      if (ASCII_order) {
        u32 j = 0, k = 0;
        char * temp = (char * ) calloc(1, 256);
        k = i;
        for (i = 0; i < k - 1; i++) {
          for (j = i + 1; j < k; j++) {
            if (strcmp( & filelist[(i + 9) * 256], & filelist[(j + 9) * 256]) > 0) {
              memcpy(temp, & filelist[(i + 9) * 256], strlen( & filelist[(i + 9) * 256]) + 1);
              memcpy( & filelist[(i + 9) * 256], & filelist[(j + 9) * 256], strlen( & filelist[(j + 9) * 256]) + 1);
              memcpy( & filelist[(j + 9) * 256], temp, strlen(temp) + 1);
            }
          }
        }
        free(temp);
      }
      i = 0;
      if (filelist) {
        // Build configuration menu.
        ments[0].type = MENT_CAPTION;
        ments[0].caption = "Select Path:";
        ments[0].color = 0xFFCCCCCC;
        ments[1].type = MENT_CHGLINE;
        ments[2].type = MENT_DATA;
        ments[2].caption = select_display;
        ments[2].data = "<selected_item>";
        //ments[0].handler = exit_loop;
        ments[3].type = MENT_CHGLINE;
        ments[4].type = MENT_CHGLINE;
		ments[5].type = MENT_CHGLINE;
        ments[6].type = MENT_DATA;
        ments[6].caption = "Exit";
        ments[6].data = "<home_menu>";
        
        ments[7].type = MENT_DATA;
        ments[7].caption = "Parent (..)";
        ments[7].data = "<go_back>";
        ments[8].type = MENT_CHGLINE;

        while (true) {
          if (i > max_entries || !filelist[(i + 9) * 256])
            break;
          ments[i + 9].type = INI_CHOICE;
          ments[i + 9].caption = & filelist[(i + 9) * 256];
          ments[i + 9].data = & filelist[(i + 9) * 256];

          i++;
        }
      }

      if (!browser_caption) browser_caption = (char * )
      "Select A File";
      if (i > 0) {
        memset( & ments[i + 9], 0, sizeof(ment_t));
        menu_t menu = {
          ments,
          browser_caption,
          0,
          0
        };
        //free(file_sec_untrimmed);
        file_sec_untrimmed = (char * ) tui_do_menu( & menu);
      }

      trimsize = 0;

	  if (!strcmp(file_sec_untrimmed + strlen(file_sec_untrimmed) - 11, "<home_menu>")) {
        home_selected = true;
        break;
      } else if (!strcmp(file_sec_untrimmed + strlen(file_sec_untrimmed) - 15, "<selected_item>")) {
        select_item = true;
        trimsize = 15;
        break;
      } else if (!strcmp(file_sec_untrimmed + strlen(file_sec_untrimmed) - 9, "<go_back>")) {
        back_selected = true;
        trimsize = 9;
        break;
      } else if (!strcmp(file_sec_untrimmed + strlen(file_sec_untrimmed) - 6, " <Dir>")) {
        directory_selected = true;
        trimsize = 6;
        break;
      } else break;

      if (i == 0) break;
    }
  trimlen = (strlen(file_sec_untrimmed) - trimsize);
  memcpy(file_sec + 0, file_sec_untrimmed, trimlen);
  if (strlen(browse_dir) == 1) memcpy(browse_dir + 0, "\0", 2);
  recheck:
  
    if (back_selected) {

      memcpy(browse_dir + strlen(browse_dir), file_sec_untrimmed, trimlen);
      char * back_trim_len = strrchr(browse_dir, '/');
      memcpy(browse_dir + (strlen(browse_dir) - strlen(back_trim_len)), "\0", 2);
      if (strlen(back_trim_len) == 1024) memcpy(browse_dir + 0, "/", 2);
      goto start;
    }

  else if (directory_selected) {

    memcpy(browse_dir + strlen(browse_dir), "/", 2);
    memcpy(browse_dir + strlen(browse_dir), file_sec, strlen(file_sec) + 1);
    goto start;
  } else if (select_item) {
    err = 3;
    goto out;
  } else {
    memcpy(browse_dir + strlen(browse_dir), "/", 2);
    memcpy(browse_dir + strlen(browse_dir), file_sec, strlen(file_sec) + 1);
    if (required_extn) {
      if (strcmp(required_extn, browse_dir + strlen(browse_dir) - strlen(required_extn)))
        err = 2;
    }
  }

  if (home_selected) {
    err = 0;
    browse_dir = NULL;
  }

  out:
    gfx_clear_grey(0x00);;
  gfx_con_setpos(0, 0);

  if (err == 1) {
    if (goto_root_on_fail) {
      gfx_printf("Empty folder. Please choose another");
      msleep(2000);
      back_selected = true;
      goto recheck;
    } else {
      gfx_printf("Invalid Selection. Exiting");
      msleep(2000);
      return NULL;
    }
  }
  if (err == 2) {
    gfx_printf("File extension incorrect");
    msleep(2000);
    back_selected = true;
    goto recheck;
  }
  if (err == 3) {
    if (!get_dir) {
      gfx_printf("Directory chosen. Choose a file instead");
      msleep(2000);
      back_selected = true;
      goto recheck;
    } else return browse_dir;
  }

  return browse_dir;
}

extern int sd_save_to_file(void *buf, u32 size, const char *filename);
extern int launch_payload(const char *path);

void restore_license_dat(){
	gfx_clear_grey(0x00);
	gfx_con_setpos(0, 0);
	unsigned char license_dat[256] = { 
	0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53,
	0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53,
	0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53,
	0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53,
	0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53,
	0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53,
	0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53,
	0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53,
	0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53,
	0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53,
	0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53,
	0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53,
	0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53,
	0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53,
	0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53,
	0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53,
	0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53,
	0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53,
	0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53,
	0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53,
	0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53, 0x53, 0x58, 0x4F, 0x53,
	0x53, 0x58, 0x4F, 0x53
};
	if (!sd_mount()){
		return;
	}
	//does it exist?
	if (f_stat("license.dat", NULL) == FR_OK){
		gfx_printf("\nSXOS license.dat exists!\n\nCancelled.");
		msleep(3000);
		return;
	} else {
		gfx_printf("\nPlease wait\n\n");
	sd_save_to_file(license_dat, sizeof(license_dat), "license.dat");
		gfx_printf("Success. Written %d bytes", sizeof(license_dat));
	sd_unmount();
	msleep(3000);
	}
	return;

}

void restore_septprimary_dat(){
	gfx_clear_grey(0x00);
	gfx_con_setpos(0, 0);
	unsigned char dummy_bin[928] = {
	0x00, 0x00, 0xA0, 0xE3, 0x74, 0x10, 0x9F, 0xE5, 0x50, 0x00, 0x81, 0xE5,
	0xB4, 0x01, 0x81, 0xE5, 0x40, 0x08, 0x81, 0xE5, 0x68, 0x00, 0x9F, 0xE5,
	0x68, 0x10, 0x9F, 0xE5, 0x00, 0x00, 0x81, 0xE5, 0x04, 0x00, 0xA0, 0xE3,
	0x60, 0x10, 0x9F, 0xE5, 0x60, 0x20, 0x9F, 0xE5, 0x23, 0x00, 0x00, 0xEB,
	0x05, 0x00, 0xA0, 0xE3, 0x58, 0x10, 0x9F, 0xE5, 0x58, 0x20, 0x9F, 0xE5,
	0x1F, 0x00, 0x00, 0xEB, 0x06, 0x00, 0xA0, 0xE3, 0x50, 0x10, 0x9F, 0xE5,
	0x50, 0x20, 0x9F, 0xE5, 0x1B, 0x00, 0x00, 0xEB, 0x4C, 0x00, 0x9F, 0xE5,
	0x4C, 0x10, 0x9F, 0xE5, 0x00, 0x20, 0xA0, 0xE3, 0x48, 0x30, 0x9F, 0xE5,
	0x02, 0x40, 0x90, 0xE7, 0x02, 0x40, 0x81, 0xE7, 0x04, 0x20, 0x82, 0xE2,
	0x03, 0x00, 0x52, 0xE1, 0xFA, 0xFF, 0xFF, 0x1A, 0x34, 0x00, 0x9F, 0xE5,
	0x10, 0xFF, 0x2F, 0xE1, 0x1B, 0x00, 0x00, 0xEA, 0x00, 0xE4, 0x00, 0x70,
	0x30, 0x4C, 0x00, 0x40, 0x08, 0xF2, 0x00, 0x60, 0xDC, 0x15, 0x00, 0x00,
	0x20, 0xE0, 0x00, 0x00, 0xEE, 0x4A, 0x00, 0x00, 0x5B, 0xE0, 0x00, 0x00,
	0x88, 0x4E, 0x00, 0x00, 0x18, 0xE0, 0x00, 0x00, 0x00, 0xF1, 0x03, 0x40,
	0x40, 0x00, 0x01, 0x40, 0xC0, 0x02, 0x00, 0x00, 0x10, 0x10, 0x10, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x14, 0x30, 0x9F, 0xE5, 0x00, 0x01, 0xA0, 0xE1, 0xA1, 0x10, 0xA0, 0xE1,
	0x01, 0x18, 0xA0, 0xE1, 0x02, 0x10, 0x81, 0xE1, 0x00, 0x10, 0x83, 0xE7,
	0x1E, 0xFF, 0x2F, 0xE1, 0x00, 0xDC, 0x01, 0x60, 0xF8, 0xB5, 0xC0, 0x46,
	0xF8, 0xBC, 0x08, 0xBC, 0x9E, 0x46, 0x70, 0x47, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xC0, 0x9F, 0xE5, 0x1C, 0xFF, 0x2F, 0xE1, 0x05, 0x01, 0x01, 0x40,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xD0, 0x1F, 0xE5, 0x9F, 0x00, 0x00, 0xEA,
	0x00, 0x00, 0x01, 0x40, 0x06, 0x48, 0x07, 0x4B, 0x10, 0xB5, 0x83, 0x42,
	0x04, 0xD0, 0x06, 0x4B, 0x00, 0x2B, 0x01, 0xD0, 0x00, 0xF0, 0x0A, 0xF8,
	0x10, 0xBC, 0x01, 0xBC, 0x00, 0x47, 0xC0, 0x46, 0xD8, 0x02, 0x01, 0x40,
	0xD8, 0x02, 0x01, 0x40, 0x00, 0x00, 0x00, 0x00, 0x18, 0x47, 0xC0, 0x46,
	0x08, 0x48, 0x09, 0x49, 0x09, 0x1A, 0x89, 0x10, 0xCB, 0x0F, 0x59, 0x18,
	0x10, 0xB5, 0x49, 0x10, 0x04, 0xD0, 0x06, 0x4B, 0x00, 0x2B, 0x01, 0xD0,
	0x00, 0xF0, 0x0A, 0xF8, 0x10, 0xBC, 0x01, 0xBC, 0x00, 0x47, 0xC0, 0x46,
	0xD8, 0x02, 0x01, 0x40, 0xD8, 0x02, 0x01, 0x40, 0x00, 0x00, 0x00, 0x00,
	0x18, 0x47, 0xC0, 0x46, 0x10, 0xB5, 0x08, 0x4C, 0x23, 0x78, 0x00, 0x2B,
	0x09, 0xD1, 0xFF, 0xF7, 0xC9, 0xFF, 0x06, 0x4B, 0x00, 0x2B, 0x02, 0xD0,
	0x05, 0x48, 0x00, 0xE0, 0x00, 0xBF, 0x01, 0x23, 0x23, 0x70, 0x10, 0xBC,
	0x01, 0xBC, 0x00, 0x47, 0xE0, 0x02, 0x01, 0x40, 0x00, 0x00, 0x00, 0x00,
	0xD8, 0x02, 0x01, 0x40, 0x06, 0x4B, 0x10, 0xB5, 0x00, 0x2B, 0x03, 0xD0,
	0x05, 0x49, 0x06, 0x48, 0x00, 0xE0, 0x00, 0xBF, 0xFF, 0xF7, 0xC4, 0xFF,
	0x10, 0xBC, 0x01, 0xBC, 0x00, 0x47, 0xC0, 0x46, 0x00, 0x00, 0x00, 0x00,
	0xE4, 0x02, 0x01, 0x40, 0xD8, 0x02, 0x01, 0x40, 0x82, 0x20, 0x56, 0x4B,
	0xC0, 0x00, 0x1A, 0x58, 0x55, 0x49, 0x11, 0x40, 0x80, 0x22, 0x12, 0x02,
	0x0A, 0x43, 0xD0, 0x21, 0x10, 0xB5, 0x1A, 0x50, 0x81, 0x22, 0x58, 0x58,
	0xD2, 0x05, 0x02, 0x43, 0x5A, 0x50, 0xAB, 0x22, 0x90, 0x39, 0x92, 0x00,
	0x99, 0x50, 0x80, 0x21, 0x18, 0x3A, 0xC9, 0x02, 0x99, 0x50, 0xC0, 0x21,
	0x70, 0x32, 0x49, 0x05, 0x99, 0x50, 0x4A, 0x4A, 0x11, 0x68, 0x13, 0x68,
	0x5B, 0x1A, 0x02, 0x2B, 0xFB, 0xD9, 0x80, 0x23, 0x47, 0x48, 0x01, 0x68,
	0xDB, 0x00, 0x19, 0x43, 0x01, 0x60, 0x01, 0x21, 0x45, 0x4C, 0x20, 0x68,
	0x88, 0x43, 0x20, 0x60, 0x44, 0x4C, 0x20, 0x68, 0x18, 0x43, 0x20, 0x60,
	0x43, 0x4C, 0x20, 0x68, 0x88, 0x43, 0x20, 0x60, 0x42, 0x4C, 0x20, 0x68,
	0x18, 0x43, 0x20, 0x60, 0x41, 0x4C, 0x20, 0x68, 0x88, 0x43, 0x20, 0x60,
	0x40, 0x4C, 0x20, 0x68, 0x18, 0x43, 0x20, 0x60, 0x3F, 0x4C, 0x20, 0x68,
	0x88, 0x43, 0x20, 0x60, 0x3E, 0x48, 0x04, 0x68, 0x23, 0x43, 0x03, 0x60,
	0x3D, 0x48, 0x03, 0x68, 0x8B, 0x43, 0x03, 0x60, 0x04, 0x20, 0x3C, 0x49,
	0x0B, 0x68, 0x03, 0x43, 0x0B, 0x60, 0x01, 0x21, 0x3A, 0x4B, 0x49, 0x42,
	0x19, 0x60, 0x11, 0x68, 0x2C, 0x4A, 0x13, 0x68, 0x5B, 0x1A, 0x02, 0x2B,
	0xFB, 0xD9, 0xAA, 0x22, 0x40, 0x21, 0x27, 0x4B, 0x92, 0x00, 0x99, 0x50,
	0xC0, 0x21, 0x58, 0x32, 0x49, 0x05, 0x99, 0x50, 0x80, 0x20, 0xA4, 0x21,
	0xC0, 0x02, 0x89, 0x00, 0x58, 0x50, 0xD1, 0x39, 0xFF, 0x39, 0x59, 0x61,
	0x2E, 0x49, 0x19, 0x61, 0x2E, 0x49, 0x99, 0x61, 0xD8, 0x21, 0x2E, 0x48,
	0x89, 0x00, 0x58, 0x50, 0x2D, 0x48, 0x04, 0x31, 0x58, 0x50, 0x2D, 0x48,
	0xE4, 0x39, 0x58, 0x50, 0x18, 0x31, 0x5A, 0x50, 0x00, 0x22, 0xA1, 0x39,
	0xFF, 0x39, 0x5A, 0x50, 0x04, 0x31, 0x5A, 0x50, 0xE8, 0x21, 0x89, 0x00,
	0x5A, 0x50, 0x04, 0x31, 0x5A, 0x50, 0x26, 0x49, 0x5A, 0x50, 0xD0, 0x21,
	0x25, 0x48, 0x5A, 0x58, 0x02, 0x40, 0x5A, 0x50, 0x82, 0x21, 0xC9, 0x00,
	0x5A, 0x58, 0x0E, 0x48, 0x02, 0x40, 0xA4, 0x20, 0x5A, 0x50, 0x80, 0x21,
	0x40, 0x00, 0x1A, 0x58, 0xD2, 0x00, 0x09, 0x06, 0xD2, 0x08, 0x0A, 0x43,
	0x1A, 0x50, 0x38, 0x30, 0x1A, 0x58, 0xD2, 0x00, 0xD2, 0x08, 0x0A, 0x43,
	0x1A, 0x50, 0xD4, 0x20, 0xC0, 0x00, 0x1A, 0x58, 0xD2, 0x00, 0xD2, 0x08,
	0x11, 0x43, 0x19, 0x50, 0xFE, 0xE7, 0xC0, 0x46, 0x00, 0x60, 0x00, 0x60,
	0xFF, 0x3F, 0xFF, 0xFF, 0x10, 0x50, 0x00, 0x60, 0xA0, 0x10, 0x2D, 0x70,
	0x88, 0x10, 0x2D, 0x70, 0xA0, 0x11, 0x2D, 0x70, 0x88, 0x11, 0x2D, 0x70,
	0xA0, 0x12, 0x2D, 0x70, 0x88, 0x12, 0x2D, 0x70, 0xA0, 0x13, 0x2D, 0x70,
	0x88, 0x13, 0x2D, 0x70, 0xA0, 0x14, 0x2D, 0x70, 0x88, 0x14, 0x2D, 0x70,
	0xF8, 0x0C, 0x20, 0x54, 0x8C, 0x00, 0x34, 0x54, 0x30, 0x01, 0x00, 0x80,
	0x00, 0x02, 0xF0, 0x01, 0x08, 0x08, 0x40, 0x80, 0xFC, 0x00, 0x20, 0x40,
	0x80, 0x07, 0x00, 0x23, 0x54, 0x05, 0x00, 0x00, 0xFF, 0xFF, 0x7F, 0x1F,
	0xF8, 0xB5, 0xC0, 0x46, 0xF8, 0xBC, 0x08, 0xBC, 0x9E, 0x46, 0x70, 0x47,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x9F, 0xE5, 0x1C, 0xFF, 0x2F, 0xE1,
	0x05, 0x01, 0x01, 0x40, 0x00, 0x00, 0x00, 0x00, 0xDD, 0x00, 0x01, 0x40,
	0xAD, 0x00, 0x01, 0x40
};

	if (!sd_mount()){
		return;
	}
	//does it exist?
	if (f_stat("bootloader/payloads/samd21update.bin", NULL) == FR_OK){
		gfx_printf("\nDummy payload exists!\n\n");
	} else {
		gfx_printf("\nPlease wait\n\n");
		sd_save_to_file(dummy_bin, sizeof(dummy_bin), "bootloader/payloads/samd21update.bin");
		gfx_printf("Success. Written %d bytes\n\n", sizeof(dummy_bin));
		}
		gfx_printf("\nTwice press [RESET] on your chip\nand connect to USB.\n\nHold [PWR] for 12 seconds when you\nhave finished updating UF2 files.\n\nPress [POWER] to enter update mode.");
		u8 btn = btn_wait();
	if (btn & BTN_VOL_UP){
	msleep(5000);
	launch_payload("bootloader/payloads/samd21update.bin");
	} else if (btn & BTN_POWER){
		launch_payload("bootloader/payloads/samd21update.bin");
	}
	return;
}

void reboot_memloader(){

	if (sd_mount()){
	u8 fr = launch_payload("bootloader/memloader/tidy_memloader.bin");
	if (fr) gfx_printf("\ntidy_memloader.bin missing\n");
	}
	btn_wait();
	return;
}
#pragma GCC pop_options

