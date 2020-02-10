/*
 * Copyright (c) 2018 naehrwert
 *
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
#include <math.h>
#include "../common/memory_map.h"
#include "libs/compr/blz.h"

#include "gfx/di.h"
#include "gfx/gfx.h"
#include "gfx/tui.h"
#include "gfx/logos.h"

#include "libs/fatfs/ff.h"
#include "mem/heap.h"
#include "mem/sdram.h"
#include "power/max77620.h"
#include "power/max17050.h"
#include "rtc/max77620-rtc.h"
#include "soc/fuse.h"
#include "soc/hw_init.h"
#include "soc/i2c.h"
#include "storage/sdmmc.h"
#include "utils/btn.h"
#include "utils/util.h"
#include "utils/rcm_usb.h"
#include "frontend/fe_emmc_tools.h"
sdmmc_t sd_sdmmc;
sdmmc_storage_t sd_storage;
FATFS sd_fs;
static bool sd_mounted;

bool sd_mount()
{
	if (sd_mounted)
		return true;

	if (sdmmc_storage_init_sd(&sd_storage, &sd_sdmmc, SDMMC_1, SDMMC_BUS_WIDTH_4, 11))
	{
		int res = 0;
		res = f_mount(&sd_fs, "", 1);
		if (res == FR_OK)
		{
			sd_mounted = 1;
			return true;
		}
	}

	return false;
}

void sd_unmount()
{
	if (sd_mounted)
	{
		f_mount(NULL, "", 1);
		sdmmc_storage_end(&sd_storage);
		sd_mounted = false;
	}
}

int sd_save_to_file(void *buf, u32 size, const char *filename)
{
	FIL fp;
	u32 res = 0;
	res = f_open(&fp, filename, FA_CREATE_ALWAYS | FA_WRITE);
	if (res)
	{
		EPRINTFARGS("Error (%d) creating file\n%s.\n", res, filename);
		return res;
	}

	f_write(&fp, buf, size, NULL);
	f_close(&fp);

	return 0;
}

void emmcsn_path_impl(char *path, char *sub_dir, char *filename, sdmmc_storage_t *storage)
{
	sdmmc_storage_t storage2;
	sdmmc_t sdmmc;
	char emmcSN[9];
	bool init_done = false;

	memcpy(path, "backup", 7);
	f_mkdir(path);

	if (!storage)
	{
		if (!sdmmc_storage_init_mmc(&storage2, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4))
			memcpy(emmcSN, "00000000", 9);
		else
		{
			init_done = true;
			itoa(storage2.cid.serial, emmcSN, 16);
		}
	}
	else
		itoa(storage->cid.serial, emmcSN, 16);

	u32 sub_dir_len = strlen(sub_dir);   // Can be a null-terminator.
	u32 filename_len = strlen(filename); // Can be a null-terminator.

	memcpy(path + strlen(path), "/", 2);
	memcpy(path + strlen(path), emmcSN, 9);
	f_mkdir(path);
	memcpy(path + strlen(path), sub_dir, sub_dir_len + 1);
	if (sub_dir_len)
		f_mkdir(path);
	memcpy(path + strlen(path), "/", 2);
	memcpy(path + strlen(path), filename, filename_len + 1);

	if (init_done)
		sdmmc_storage_end(&storage2);
}

void emmc_path_impl(char *path, char *sub_dir, char *filename, sdmmc_storage_t *storage)
{
	sdmmc_storage_t storage2;
	bool init_done = false;
	memcpy(path, "safe", 5);
	f_mkdir(path);
	

	if (storage)init_done = true;

	u32 sub_dir_len = strlen(sub_dir);   // Can be a null-terminator.
	u32 filename_len = strlen(filename); // Can be a null-terminator.

	memcpy(path + strlen(path), "/", 2);
	if (sub_dir_len){
	memcpy(path + strlen(path), sub_dir, sub_dir_len + 1);
	f_mkdir(path);
	memcpy(path + strlen(path), "/", 2);
	}
	memcpy(path + strlen(path), filename, filename_len + 1);

	if (init_done)
		sdmmc_storage_end(&storage2);
}

void check_power_off_from_hos()
{
	// Power off on AutoRCM wakeup from HOS shutdown. For modchips/dongles.
	u8 hosWakeup = i2c_recv_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_IRQTOP);
	if (hosWakeup & MAX77620_IRQ_TOP_RTC_MASK)
	{
		sd_unmount();

		// Stop the alarm, in case we injected too fast.
		max77620_rtc_stop_alarm();
		power_off();
	}
}

// This is a safe and unused DRAM region for our payloads.
#define RELOC_META_OFF      0x7C
#define PATCHED_RELOC_SZ    0x94
#define PATCHED_RELOC_STACK 0x40007000
#define PATCHED_RELOC_ENTRY 0x40010000
#define EXT_PAYLOAD_ADDR    0xC03C0000
#define RCM_PAYLOAD_ADDR    (EXT_PAYLOAD_ADDR + ALIGN(PATCHED_RELOC_SZ, 0x10))
#define COREBOOT_ADDR       (0xD0000000 - 0x100000)
#define CBFS_DRAM_EN_ADDR   0x4003e000
#define  CBFS_DRAM_MAGIC    0x4452414D // "DRAM"

void reloc_patcher(u32 payload_dst, u32 payload_src, u32 payload_size)
{
	memcpy((u8 *)payload_src, (u8 *)IPL_LOAD_ADDR, PATCHED_RELOC_SZ);

	volatile reloc_meta_t *relocator = (reloc_meta_t *)(payload_src + RELOC_META_OFF);

	relocator->start = payload_dst - ALIGN(PATCHED_RELOC_SZ, 0x10);
	relocator->stack = PATCHED_RELOC_STACK;
	relocator->end   = payload_dst + payload_size;
	relocator->ep    = payload_dst;

	if (payload_size == 0x7000)
	{
		memcpy((u8 *)(payload_src + ALIGN(PATCHED_RELOC_SZ, 0x10)), (u8 *)COREBOOT_ADDR, 0x7000); //Bootblock
		*(vu32 *)CBFS_DRAM_EN_ADDR = CBFS_DRAM_MAGIC;
	}
}

int launch_payload(const char *path)
{
		gfx_clear_grey(0x00);
	gfx_con_setpos(0, 0);
	if (!path)
		return 1;

	if (sd_mount())
	{
		FIL fp;
		if (f_open(&fp, path, FA_READ))
		{
			EPRINTF("Payload missing!\n");
			msleep(1000);
			sd_unmount();

			return 1;
		}

		// Read and copy the payload to our chosen address
		void *buf;
		u32 size = f_size(&fp);

		if (size < 0x30000)
			buf = (void *)RCM_PAYLOAD_ADDR;
		else
			buf = (void *)COREBOOT_ADDR;

		if (f_read(&fp, buf, size, NULL))
		{
			f_close(&fp);
			sd_unmount();

			return 1;
		}

		f_close(&fp);
		//free(path);

		sd_unmount();

		if (size < 0x30000)
		{
			reloc_patcher(PATCHED_RELOC_ENTRY, EXT_PAYLOAD_ADDR, ALIGN(size, 0x10));

			reconfig_hw_workaround(false, byte_swap_32(*(u32 *)(buf + size - sizeof(u32))));
		}
		else
		{
			reloc_patcher(PATCHED_RELOC_ENTRY, EXT_PAYLOAD_ADDR, 0x7000);
			reconfig_hw_workaround(true, 0);
		}

		void (*ext_payload_ptr)() = (void *)EXT_PAYLOAD_ADDR;

		msleep(100);

		// Launch our payload.
		(*ext_payload_ptr)();
	}

	return 1;
}

void browse_file()
{
	char *file_sec = NULL;
	if (sd_mount())
	{
		file_sec = file_browser("", ".bin", "Select A Payload", false, false, true);
		if (!file_sec) return;
	}

if (file_sec) launch_payload(file_sec);
	sd_unmount();
	return;
}

void new_payload_file(const char *old_file_sec)
{
	char * file_sec;
	file_sec = (char * ) calloc('0', 256);
	char *new_file_sec = malloc(256);
	if (sd_mount())
	{
		file_sec = file_browser("", "", "Select A Payload", false, false, true);
		if (!file_sec) return;
	}
	
	memcpy (new_file_sec, old_file_sec, strlen(old_file_sec) + 1);
	if((!f_stat(old_file_sec, NULL)))
	{
		while ((!f_stat(new_file_sec, NULL)))
		{
			memcpy (new_file_sec+strlen(new_file_sec), ".old", 5);
		}
	}
	u8 fr = 0;
	fr = f_rename(old_file_sec, new_file_sec);
	if(!fr) gfx_printf("Renamed old %s\n\nto %s\n\n", old_file_sec, new_file_sec);
	msleep(1000);
	fr = 0;
	fr = f_rename(file_sec, old_file_sec);
	if(fr) gfx_printf("Failed renaming\n%s\nto %s\n\n", file_sec, old_file_sec);
	else gfx_printf("Renamed \n%s\nto %s\n\n", file_sec, old_file_sec);
	gfx_printf("\n\nPress Any Key");
	btn_wait();
	return;
}

void disable_payload_file(const char *old_file_sec, const char *new_file_sec)
{
	u8 fr = 0;
	if (!sd_mount()) {fr = 1; goto out;}
	
	fr = f_rename(old_file_sec, new_file_sec);
out:
	if(fr) gfx_printf("Failed renaming %s\nto %s\n\nDoes it exist already?\nIs SD inserted correctly?\n", old_file_sec, new_file_sec);
	else gfx_printf("Renamed %s\n\nto %s\n", old_file_sec, new_file_sec);
	msleep(1000);
	gfx_printf("\n\nPress Any Key");
	btn_wait();
	return;
}

void enable_payload_file(const char *old_file_sec, const char *new_file_sec)
{
	u8 fr = 0;
	if (!sd_mount()) {fr = 1; goto out;}
	
	fr = f_rename(old_file_sec, new_file_sec);
out:
	if(fr) gfx_printf("Failed renaming %s\nto %s\n\nDoes it exist already?\nIs SD inserted correctly?\n", old_file_sec, new_file_sec);
	else gfx_printf("Renamed %s\n\nto %s\n", old_file_sec, new_file_sec);
	msleep(1000);
	gfx_printf("\n\nPress Any Key");
	btn_wait();
	return;
}

void delete_file()
{
	char * file_sec;
	file_sec = (char * ) calloc('0', 256);
	gfx_printf("This will permanently delete your file\n\nTo cancel, choose Exit\n\nPress Any Key");
	btn_wait();
	if (sd_mount())
	{
		file_sec = file_browser("", "", "This will permanently delete your file", false, false, true);
		if (!file_sec) return;
	}
	gfx_printf("Are you sure? [PWR] Yes, [VOL] Cancel");
	u8 btn = btn_wait(BTN_POWER | BTN_VOL_UP | BTN_VOL_DOWN);
		if(!(btn & BTN_POWER)) return;
	u8 fr = 0;
	fr = f_unlink(file_sec);
	if(!fr) gfx_printf("\nDeleted %s\n\n", file_sec);
	else gfx_printf("\nDelete %s\nfailed. Error %d\n\n", file_sec, fr);
	msleep(1000);
	gfx_printf("\n\nPress Any Key");
	btn_wait();
	return;
}

const char* path1 = ("payload.bin");
const char* path2 = ("payload1.bin");
const char* path3 = ("bootloader/update.bin");
const char* dis1 = ("payload.dis");
const char* dis2 = ("payload1.dis");
const char* dis3 = ("bootloader/update.dis");

void new_payload_bin_file(){new_payload_file (path1);}
void new_payload1_bin_file(){new_payload_file (path2);}
void new_update_bin_file(){f_mkdir("bootloader"); new_payload_file (path3);}
void disable_payload_bin_file(){disable_payload_file(path1, dis1);}
void disable_payload1_bin_file(){disable_payload_file(path2, dis2);}
void disable_update_bin_file(){disable_payload_file(path3, dis3);}
void enable_payload_bin_file(){disable_payload_file(dis1, path1);}
void enable_payload1_bin_file(){disable_payload_file(dis2, path2);}
void enable_update_bin_file(){disable_payload_file(dis3, path3);}

void payload_status()
{
if(!sd_mount()) {gfx_printf("\nSD not mounted\n"); btn_wait(); return;}
gfx_printf("%k\n%s (primary) ", 0xFFFFFF00, path1);
if(!f_stat(path1, NULL)) gfx_printf ("%kfound.%k\n", 0xFF00FF00, 0xFFFFFFFF);
	else gfx_printf ("%knot found.%k\n", 0xFFFF0000, 0xFFFFFFFF);

gfx_printf("%k\n%s (secondary) ", 0xFFFFFF00, path2);
if(!f_stat(path2, NULL)) gfx_printf ("%kfound.%k\n", 0xFF00FF00, 0xFFFFFFFF);
	else gfx_printf ("%knot found.%k\n", 0xFFFF0000, 0xFFFFFFFF);

gfx_printf("%k\n%s (last) ", 0xFFFFFF00, path3);
if(!f_stat(path3, NULL)) gfx_printf ("%kfound.%k\n", 0xFF00FF00, 0xFFFFFFFF);
	else gfx_printf ("%knot found.%k\n", 0xFFFF0000, 0xFFFFFFFF);

gfx_printf("%k\nDisabled Payloads:\n", 0xFF00FFFF);
gfx_printf("%k\n%s ", 0xFFFFFF00, dis1);
if(!f_stat(dis1, NULL)) gfx_printf ("%kfound.%k\n", 0xFF00FF00, 0xFFFFFFFF);
	else gfx_printf ("%knot found.%k\n", 0xFFFF0000, 0xFFFFFFFF);

gfx_printf("%k\n%s ", 0xFFFFFF00, dis2);
if(!f_stat(dis2, NULL)) gfx_printf ("%kfound.%k\n", 0xFF00FF00, 0xFFFFFFFF);
	else gfx_printf ("%knot found.%k\n", 0xFFFF0000, 0xFFFFFFFF);

gfx_printf("%k\n%s ", 0xFFFFFF00, dis3);
if(!f_stat(dis3, NULL)) gfx_printf ("%kfound.%k\n", 0xFF00FF00, 0xFFFFFFFF);
	else gfx_printf ("%knot found.%k\n", 0xFFFF0000, 0xFFFFFFFF);
btn_wait();
}


ment_t ment_restore[] = {
	MDEF_BACK(),
	MDEF_CHGLINE(),
	MDEF_CAPTION("---- Essential -----", 0xFF0AB9E6),
	MDEF_HANDLER("Restore safe folder BOOT0/1", restore_emmc_quick),
	MDEF_HANDLER("Restore safe folder PRODINFO", restore_emmc_quick_prodinfo),
	MDEF_CHGLINE(),
	MDEF_CAPTION("------ Full --------", 0xFF0AB9E6),
	MDEF_HANDLER("Restore eMMC BOOT0/1", restore_emmc_boot),
	MDEF_HANDLER("Restore eMMC RAW GPP", restore_emmc_rawnand),
	MDEF_CHGLINE(),
	MDEF_CAPTION("-- GPP Partitions --", 0xFF0AB9E6),
	MDEF_HANDLER("Restore GPP partitions", restore_emmc_gpp_parts),
	MDEF_CHGLINE(),
	MDEF_CAPTION("---- Dangerous -----", 0xFF0AB9E6),
	MDEF_HANDLER("Restore BOOT0/1 without size check", restore_emmc_quick_noszchk),
	MDEF_END()
};

menu_t menu_restore = { ment_restore, "Restore Options", 0, 0 };

ment_t ment_backup[] = {
	MDEF_BACK(),
	MDEF_CHGLINE(),
	MDEF_CAPTION("---- Essential -----", 0xFF0AB9E6),
	MDEF_HANDLER("Backup BOOT0/1/PRODINFO to safe folder", dump_emmc_quick),
	MDEF_CHGLINE(),
	MDEF_CAPTION("------ Full --------", 0xFF0AB9E6),
	MDEF_HANDLER("Backup eMMC BOOT0/1", dump_emmc_boot),
	MDEF_HANDLER("Backup eMMC RAW GPP", dump_emmc_rawnand),
	MDEF_CHGLINE(),
	MDEF_CAPTION("-- GPP Partitions --", 0xFF0AB9E6),
	MDEF_HANDLER("Backup eMMC SYS", dump_emmc_system),
	MDEF_HANDLER("Backup eMMC USER", dump_emmc_user),
	MDEF_END()
};

menu_t menu_backup = { ment_backup, "Backup Options", 0, 0 };

ment_t ment_plmanagement[] = {
	MDEF_BACK(),
	MDEF_CHGLINE(),
	MDEF_HANDLER("Payload Status", payload_status),
	MDEF_CHGLINE(),
	MDEF_HANDLER("Rename a file to payload.bin", new_payload_bin_file),
	MDEF_HANDLER("Rename a file to payload1.bin", new_payload1_bin_file),
	MDEF_HANDLER("Rename a file to bootloader/update.bin", new_update_bin_file),
	MDEF_CHGLINE(),
	MDEF_HANDLER("Disable payload.bin", disable_payload_bin_file),
	MDEF_HANDLER("Disable payload1.bin", disable_payload1_bin_file),
	MDEF_HANDLER("Disable bootloader/update.bin", disable_update_bin_file),
	MDEF_CHGLINE(),
	MDEF_HANDLER("Enable payload.bin", enable_payload_bin_file),
	MDEF_HANDLER("Enable payload1.bin", enable_payload1_bin_file),
	MDEF_HANDLER("Enable bootloader/update.bin", enable_update_bin_file),
	MDEF_CHGLINE(),
	MDEF_HANDLER("Delete file", delete_file),
	MDEF_END()
};

menu_t menu_plmanagement = { ment_plmanagement, "Payload Management", 0, 0 };

ment_t ment_main[] = {
	MDEF_BACK(),
	MDEF_CHGLINE(),
	MDEF_HANDLER("Browse for payload", browse_file),
	MDEF_MENU("Payload Management", &menu_plmanagement),
	MDEF_CHGLINE(),
	MDEF_MENU("Backup Options", &menu_backup),
	MDEF_MENU("Restore Options", &menu_restore),
	MDEF_CHGLINE(),
	MDEF_HANDLER("Regenerate SXOS license.dat", restore_license_dat),
	MDEF_CHGLINE(),
	MDEF_HANDLER("SAMD21 Update mode", restore_septprimary_dat),
	MDEF_CHGLINE(),
	MDEF_HANDLER("USB mount (Tidy_Memloader)", reboot_memloader),
	MDEF_END()
};

menu_t menu_main = { ment_main, "Fusee Suite SAMD21 prebootloader", 0, 0 };

void launch()
{
	tui_do_menu(&menu_main);
}

bool boot_payloads()
{
	
	if((!f_stat(path1, NULL))) launch_payload (path1);
	else if((!f_stat(path2, NULL))) launch_payload (path2);
		else if((!f_stat(path3, NULL))) launch_payload (path3);
	sd_unmount();
	return false;
}

void draw_rcm_asset(bool first_boot)
{
	gfx_clear_grey(0x00);
	
	while (true)
	{
		static const char information[] =
		"%kFusee_UF2 Information. V3_1219.\n"
		"\n"
		"%kFollowing Straps Detected:\n\n"
		"JOYCON     \n"
		"\n"
		"VOLUME+    \n"
		"\n"
		"USB VOLTAGE\n"
		"\n\n"
		"Chip-based autoRCM mode active.\n"
		"\n\n"
		"%kName your payload(s) as follows:\n"
		"\n"
		"Location: SD Root.\n\n"
		"payload.bin\n"
		"payload1.bin\n"
		"\n\nFor Kosmos users:\n\n"
		"Location /bootloader\n\n"
		"update.bin%k\n\n";

	gfx_con_setpos(0, 0);
	gfx_printf(information, 0xFFFFFF00, 0xFF00FF00, 0xFF00FFFF, 0xFFCCCCCC);
	gfx_set_rect_grey(CTRLASSET, 41, 404, 679,0);
	if(!first_boot)
	{
		u8 btn = btn_wait_timeout(1000, BTN_POWER | BTN_VOL_UP | BTN_VOL_DOWN);
		if(!(btn & BTN_VOL_DOWN)) break;
	} else {msleep(1000); btn_wait(); break;}
	}
	gfx_clear_grey(0x00);
	gfx_con_setpos(0, 0);
	
}

int draw_countdown_asset(int option)
{
	
	int drawn = 1;
	while (true)
	{
		if (drawn == 2)
			{
				gfx_con_setpos(32, 60);
				gfx_printf("PWR button held...2");
			}
		if (drawn == 4)
			{
				gfx_con_setpos(32, 60);
				gfx_printf("PWR button held...1");
			}
		
		u8 btn = btn_wait_timeout(500, BTN_POWER | BTN_VOL_UP | BTN_VOL_DOWN);
		if(!(btn & BTN_POWER)) break;
		++drawn;
		if(drawn > 5)break;
		
	}
	if (drawn > 5)
	{
		if(option== 1)
		{
		gfx_con_setpos(32, 60);
		gfx_printf("Powering off. Release button");
		msleep(1000);
		gfx_con_setpos(32, 60);
		gfx_printf("                            ");
		return 1;
		}
		else
		{
		gfx_con_setpos(32, 60);
		gfx_printf("Boot paused. Release button");
		msleep(1000);
		gfx_con_setpos(32, 60);
		gfx_printf("                           ");
		return 1;
		}
	}
	if(option== 1)
		{
			gfx_con_setpos(32, 60);
			gfx_printf("Button released. Attempting boot");
				msleep(1000);
			gfx_con_setpos(32, 60);
			gfx_printf("                                ");
		}
		else
		{
			gfx_con_setpos(32, 60);
			gfx_printf("Button released. Cancelled");
				msleep(1000);
			gfx_con_setpos(32, 60);
			gfx_printf("                          ");
		}
		
	return 0;
	
}



void draw_sd_asset(bool onoff)
{
	if (onoff)gfx_set_rect_rgb(SDASSET, 32,38, 32,1210);
	else gfx_set_rect_rgb(NOSDASSET, 32,38, 32,1210);
}

void draw_ln_asset(bool onoff)
{
	if (onoff)gfx_set_rect_grey(LNASSET, 17,11, 36,100);
	else gfx_set_rect_grey(LNOFFASSET, 17,11, 36,100);
}

#define batt_size 40
void fill_batt_asset(int segcol, int battPercentage)
{
	
	int co = 0; int bk = 0;
	int battsegy = 35;
	//manual logarithmic(ish). Hardcoded for speed
	
	if(battPercentage > 1 && battPercentage < 6) {co = 1; bk = (40 - co); segcol +=2;}
	if(battPercentage > 5 && battPercentage < 11) {co = 2; bk = (40 - co); segcol +=2;}
	if(battPercentage > 10 && battPercentage < 16) {co = 4; bk = (40 - co);segcol +=2;}
	if(battPercentage > 15 && battPercentage < 21) {co = 6; bk = (40 - co);}
	if(battPercentage > 20 && battPercentage < 26) {co = 8; bk = (40 - co);}
	if(battPercentage > 25 && battPercentage < 31) {co = 10; bk = (40 - co);}
	if(battPercentage > 30 && battPercentage < 36) {co = 12; bk = (40 - co);}
	if(battPercentage > 35 && battPercentage < 41) {co = 14; bk = (40 - co);}
	if(battPercentage > 40 && battPercentage < 46) {co = 16; bk = (40 - co);}
	if(battPercentage > 45 && battPercentage < 51) {co = 18; bk = (40 - co);}
	if(battPercentage > 50 && battPercentage < 56) {co = 20; bk = (40 - co);}
	if(battPercentage > 55 && battPercentage < 61) {co = 22; bk = (40 - co);}
	if(battPercentage > 60 && battPercentage < 66) {co = 24; bk = (40 - co);}
	if(battPercentage > 65 && battPercentage < 71) {co = 26; bk = (40 - co);}
	if(battPercentage > 70 && battPercentage < 76) {co = 28; bk = (40 - co);}
	if(battPercentage > 75 && battPercentage < 81) {co = 30; bk = (40 - co);}
	if(battPercentage > 80 && battPercentage < 86) {co = 32; bk = (40 - co);}
	if(battPercentage > 85 && battPercentage < 91) {co = 34; bk = (40 - co);}
	if(battPercentage > 90 && battPercentage < 96) {co = 36; bk = (40 - co);}
	if(battPercentage > 95 && battPercentage < 100) {co = 38; bk = (40 - co);}
	if(battPercentage > 99) co = 40;

	for (int j = 1; j <= co; j++)
		{
			battsegy += 1;
			if (segcol == 0) gfx_set_rect_rgb(BATTDKGNASSET, 18,1, 36,battsegy);
			if (segcol == 1) gfx_set_rect_rgb(BATTGRNASSET, 18,1, 36,battsegy);
			if (segcol == 2) gfx_set_rect_rgb(BATTDKRDASSET, 18,1, 36,battsegy);
			if (segcol == 3) gfx_set_rect_rgb(BATTREDASSET, 18,1, 36,battsegy);
			
			
		}
	if(co!=40)
	{		
		for (int j = co; j <= bk; j++)
			{
			battsegy += 1;
			gfx_set_rect_rgb(BATTBLKASSET, 18,1, 36,battsegy);
			}
		battsegy = 35;
	}
}
 
 void draw_outline_assets(int option)
{
	u8 battoly = 32;
		
		gfx_set_rect_grey(BATTBTMASSET, 26,4, 32,battoly);
		battoly +=3;
		for (int i = 0; i<batt_size; ++i)
		{
			++battoly;
			gfx_set_rect_grey(BATTSIDASSET, 26,1, 32,battoly);
		}
		gfx_set_rect_grey(BATTTOPASSET, 26,8, 32,battoly+1);
		if(option == 1) gfx_set_rect_grey(CTRLASSET, 41, 404, 679,0);
		
	
	
}
#define BATT_THRESHDIS 5
int draw_boot_logo(int delayMs)
{
	u8 btn;
	static bool battCharging; static int flatBatt = 0; static int tickTimer = 0; //init global timer
	static int battPercentage, battPercent, Current;
	u8 * NINTENDOASSET = (void *)malloc(0x733F);
	blz_uncompress_srcdest(NINTENDOASSET_blz, SZ_NINTENDOASSET_blz, NINTENDOASSET, SZ_NINTENDOASSET);
	
	
	max17050_get_property(MAX17050_RepSOC, (int *)&battPercent);
	battPercentage = ((battPercent >> 8) & 0xFF);
	if (battPercentage < BATT_THRESHDIS) flatBatt = 0;//batt is flat
		else flatBatt = 1;	
	
	if(flatBatt == 0) // enter charging lock loop
	{
		
		draw_outline_assets(0);//draw batt outline
		display_backlight_brightness(64, 1000); // backlight on
		
		while (battPercentage < BATT_THRESHDIS)
		{
			max17050_get_property(MAX17050_Current, &Current);
			max17050_get_property(MAX17050_RepSOC, (int *)&battPercent);
			battPercentage = ((battPercent >> 8) & 0xFF);
			if(Current>0) {battCharging = 1; tickTimer = 0;}
			else battCharging = 0;
			++tickTimer; // start timer 1/4 sec
			
			fill_batt_asset(battCharging, battPercentage);
			draw_ln_asset(battCharging);
			msleep(1000);
			if((!battCharging) && tickTimer > 5) {check_power_off_from_hos(); power_off();}
		}
		tickTimer = 0; // reset tick
	}
	
	if(flatBatt == 1)
	{
		gfx_set_rect_grey(NINTENDOASSET, 80, 329, 314,460);
		display_backlight_brightness(64, 1000); // backlight on
		btn = btn_wait_timeout(0, BTN_POWER | BTN_VOL_UP | BTN_VOL_DOWN);
			if (btn & BTN_POWER)
			{
				msleep(100); //delay unhook pwr btn			
				if (draw_countdown_asset(0)) {flatBatt = 2;}
			}
	}
	msleep(delayMs);
	display_backlight_brightness(0, 1000);
	gfx_clear_grey(0x00);
	return flatBatt;
}


void boot_batt()
{
	//first, determine if battery is too flat  to boot
	static int oldbattPercentage, battPercentage, battPercent, Current;
	 static bool battCharging; static int directBoot = false;
	static int tickTimer = 0; //init global timer
	u8 btn;
	directBoot = draw_boot_logo(1000);
start:	
	
		
		
		
	
	if(directBoot == 1)
		{
			//go for boot
			display_backlight_brightness(0, 1000);
			if(sd_mount()) boot_payloads();
			display_backlight_brightness(64, 1000);
			directBoot = 2;
		}

	if(directBoot == 2)
	{//main loop
	/*if(sd_mount())
		{	
			if (f_stat("safe", NULL))
			{
				gfx_clear_grey(0x00);
				gfx_con_setpos(0,0);
				display_backlight_brightness(64, 1000); // backlight on
				gfx_printf("Safe folder not found on this SD card\n\nPerforming initial safety backup\n\nof BOOT0/1/PRODINFO.\n\nThese backups are saved in safe\n\nfolder on SD root");
				msleep(5000);
				dump_emmc_quick();
				gfx_clear_grey(0x00);
				draw_rcm_asset(true);
				gfx_clear_grey(0x00);
				gfx_con_setpos(0,0);
				
			}
			display_backlight_brightness(0, 1000); // backlight on
			sd_unmount();
		}*/
		
		draw_outline_assets(1);//draw batt outline
		display_backlight_brightness(64, 1000); // backlight on
		//gather values, populate display
		
		while(true)
		{
			max17050_get_property(MAX17050_Current, &Current);
			max17050_get_property(MAX17050_RepSOC, (int *)&battPercent);
			battPercentage = ((battPercent >> 8) & 0xFF);
			if(Current>0) 
			{
				battCharging = 1;
				tickTimer = 0;
			}
			else 
			{
				battCharging = 0;
				++tickTimer; // start timer 1/4 sec
			}
			if (oldbattPercentage != battPercentage) fill_batt_asset(1, battPercentage);
			draw_ln_asset(battCharging);
			if(sd_mount()) {draw_sd_asset(true); sd_unmount();}
				else draw_sd_asset(false);
			
			btn = btn_wait_timeout(250, BTN_POWER | BTN_VOL_UP | BTN_VOL_DOWN);
			if (btn & BTN_VOL_UP){launch(); gfx_clear_grey(0x00); tickTimer = 0; oldbattPercentage = 0; draw_outline_assets(0);}//draw batt outline
			if (btn & BTN_VOL_DOWN){draw_rcm_asset(false); tickTimer = 0; oldbattPercentage = 0; draw_outline_assets(0);}
			if (btn & BTN_POWER) 
			{
				msleep(100); //delay unhook pwr btn			
				if (draw_countdown_asset(1)) {display_backlight_brightness(0, 1000); check_power_off_from_hos(); power_off();}
				else {sd_mount(); boot_payloads(); check_power_off_from_hos(); power_off();}
			}
			if(!battCharging && tickTimer >= 60) {display_backlight_brightness(0, 1000); check_power_off_from_hos(); power_off();}
			if(tickTimer > 60) tickTimer = 0;
			 if(oldbattPercentage) oldbattPercentage = battPercentage;
		}
	}
	
	//if(bootFailed) {display_backlight_brightness(0, 1000); check_power_off_from_hos(); power_off();}
	goto start;	
}

extern void pivot_stack(u32 stack_top);
void ipl_main()
{
	// Do initial HW configuration. This is compatible with consecutive reruns without a reset.
	config_hw();

	//Pivot the stack so we have enough space.
	pivot_stack(IPL_STACK_TOP);

	//Tegra/Horizon configuration goes to 0x80000000+, package2 goes to 0xA9800000, we place our heap in between.
	heap_init(IPL_HEAP_START);

	display_init();

	u32 *fb = display_init_framebuffer();
	gfx_init_ctxt(fb, 1280, 720, 720);

	gfx_con_init();

	display_backlight_pwm_init();

	while (true)
	boot_batt();
	
	while (true)
	;
}
