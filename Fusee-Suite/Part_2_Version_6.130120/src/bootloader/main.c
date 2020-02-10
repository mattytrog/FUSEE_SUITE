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
//#define LOGO_RED
#define LOGO_BLK


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


//todo realign msg array. OOgly.
char msg[50][75] =
{
"START_OF_VARIABLES",
"Entering menu",
"Release button",
"Cancelled. Skipped. ",
"Booting",
"",
"found.",
"not found.",
"Disabled Payloads:",
"",
"",
"",
"",
"",
"",
"",
"Press Any Key",
"Failed. Error ",
"Success.",
"Please select a payload number\nwith VOL+ and VOL-.\nPress PWR to confirm",
"This will rename a payload to a number\n",
"that you can choose with your chip.\n", //21
"Your chip will always look for this\n",
"until you change via VOL+ button\n",
"This will permanently delete your file\n\n",
"To cancel, choose Exit\n\n",
"Renamed:",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"                                      ",	
"",
"",
"",
};

char samdsettings[50][38] =
{
"ANIM00",
"SCREENSAVER1",
"payload.bin",
"payload1.bin",
"bootloader/update.bin",
"payload.dis",
"payload1.dis",
"bootloader/update.dis",
"JOYCON     ",
"VOLUME+    ",
"USB VOLTAGE",
"Chip-based autoRCM mode active.",
"Boot Animation 1 selected.",
"Screensaver is active.",
"                                      ", //11
{0xE6},
{0x00},
{0x12},
{0xFF},
{0xFF},
{0xFF},
};

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


		FIL fp;
		f_open(&fp, path, FA_READ);
		

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
	gfx_clear_color(0xFF000000);
	gfx_con_setpos(0, 0);
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
	if(!fr) gfx_printf("%s %s\n\nto %s\n\n", msg[26], old_file_sec, new_file_sec);
	msleep(1000);
	fr = 0;
	fr = f_rename(file_sec, old_file_sec);
	if(fr) gfx_printf("Failed renaming\n%s\nto %s\n\n", file_sec, old_file_sec);
	else gfx_printf("Renamed \n%s\nto %s\n\n", file_sec, old_file_sec);
	gfx_printf("\n\n%s", msg[16]);
	btn_wait();
	return;
}

void set_x_payload()
{
	char *file_sec = NULL;
	u8 fr = 0;
	int payloadnumber = 0;
	if (sd_mount())
	{
		file_sec = file_browser("", ".bin", "Select A Payload", false, false, true);
		if (!file_sec) return;
	}

if (file_sec) {
	gfx_printf("\n%k%s%s%s%s%k\n", 0xFFFFFF00, msg[20], msg[21], msg[22], msg[23],0xFF00FFFF);
	gfx_printf("%k%s\n\n\n",0xFFFFFFFF, msg[19]);
	gfx_con_getpos(&gfx_con.savedx, &gfx_con.savedy);
	}
	while(true)
	{
		gfx_con_setpos(gfx_con.savedx, gfx_con.savedy);
		if (payloadnumber !=0) gfx_printf("%kpayload%d.bin   %k", 0xFF00FF00, payloadnumber,  0xFFFFFFFF);
		else gfx_printf("%kCancel        ", 0xFF00FFFF);
		u8 btn = btn_wait(BTN_POWER | BTN_VOL_UP | BTN_VOL_DOWN);
		if(btn & BTN_POWER) break;
		if(btn & BTN_VOL_UP) ++payloadnumber;
		if(btn & BTN_VOL_DOWN) --payloadnumber;
		if (payloadnumber>8)payloadnumber = 0;
		if (payloadnumber<0)payloadnumber = 8;
	}
	if(payloadnumber == 0) goto out;
		else if(payloadnumber == 1) fr = f_rename (file_sec, "payload1.bin");
		else if(payloadnumber == 2) fr = f_rename (file_sec, "payload2.bin");
		else if(payloadnumber == 3) fr = f_rename (file_sec, "payload3.bin");
		else if(payloadnumber == 4) fr = f_rename (file_sec, "payload4.bin");
		else if(payloadnumber == 5) fr = f_rename (file_sec, "payload5.bin");
		else if(payloadnumber == 6) fr = f_rename (file_sec, "payload6.bin");
		else if(payloadnumber == 7) fr = f_rename (file_sec, "payload7.bin");
		else if(payloadnumber == 8) fr = f_rename (file_sec, "payload8.bin");
	if(fr) {gfx_printf("\n%k%s %d%k", 0xFFFF0000, msg[17], fr, 0xFFFFFFFF); payloadnumber = 0; goto out;}
	else gfx_printf("\n%k%s%k", 0xFF00FF00, msg[18], 0xFFFFFFFF);
	msleep(500);
	
out:
gfx_printf("\n\n%k%s%k", 0xFFFFFF00, msg[16], 0xFFFFFFFF);
btn_wait();
	if(payloadnumber == 0)
	sd_unmount();
	return;
}

void disable_payload_file(const char *old_file_sec, const char *new_file_sec)
{
	u8 fr = 0;
	if (!sd_mount()) {fr = 1; goto out;}
	
	fr = f_rename(old_file_sec, new_file_sec);
out:
	if(fr) gfx_printf("\n%s %d", msg[17], fr);
	else gfx_printf("\n%s\n\n", msg[18]);
	msleep(1000);
	gfx_printf("\n\n%s", msg[16]);
	btn_wait();
	return;
}

void enable_payload_file(const char *old_file_sec, const char *new_file_sec)
{
	u8 fr = 0;
	if (!sd_mount()) {fr = 1; goto out;}
	
	fr = f_rename(old_file_sec, new_file_sec);
out:
	if(fr) gfx_printf("\n%s %d", msg[17], fr);
	else gfx_printf("\n%s\n\n", msg[18]);
	msleep(1000);
	gfx_printf("\n\n%s", msg[16]);
	btn_wait();
	return;
}

void delete_file()
{
	char * file_sec;
	file_sec = (char * ) calloc('0', 256);
	gfx_printf("%s%s%s", msg[24], msg[25], msg[16]);
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
	if(!fr) gfx_printf("\n%s\n\n", msg[18]);
	else gfx_printf("\n%s %d", msg[17], fr);
	msleep(1000);
	gfx_printf("\n\n%s", msg[16]);
	btn_wait();
	return;
}

const char* path1 = (samdsettings[2]);
const char* path2 = (samdsettings[3]);
const char* path3 = (samdsettings[4]);
const char* dis1 = (samdsettings[5]);
const char* dis2 = (samdsettings[6]);
const char* dis3 = (samdsettings[7]);

void new_payload_bin_file(){new_payload_file (path1);}
void new_payload1_bin_file(){new_payload_file (path2);}
void new_update_bin_file(){f_mkdir("bootloader"); new_payload_file (path3);}
void new_payloadx_bin_file(){set_x_payload();}
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
if(!f_stat(path1, NULL)) gfx_printf ("%k%s%k\n", 0xFF00FF00, msg[6],0xFFFFFFFF);
	else gfx_printf ("%k%s%k\n", 0xFFFF0000,  msg[7],0xFFFFFFFF);

gfx_printf("%k\n%s (secondary) ", 0xFFFFFF00, path2);
if(!f_stat(path2, NULL)) gfx_printf ("%k%s%k\n", 0xFF00FF00, msg[6],0xFFFFFFFF);
	else gfx_printf ("%k%s%k\n", 0xFFFF0000,  msg[7],0xFFFFFFFF);

gfx_printf("%k\n%s (last) ", 0xFFFFFF00, path3);
if(!f_stat(path3, NULL)) gfx_printf ("%k%s%k\n", 0xFF00FF00, msg[6],0xFFFFFFFF);
	else gfx_printf ("%k%s%k\n", 0xFFFF0000,  msg[7],0xFFFFFFFF);
	
gfx_printf("%k\n%s\n", 0xFF00FFFF, msg[8]);
gfx_printf("%k\n%s ", 0xFFFFFF00, dis1);
if(!f_stat(dis1, NULL)) gfx_printf ("%k%s%k\n", 0xFF00FF00, msg[6],0xFFFFFFFF);
	else gfx_printf ("%k%s%k\n", 0xFFFF0000,  msg[7],0xFFFFFFFF);

gfx_printf("%k\n%s ", 0xFFFFFF00, dis2);
if(!f_stat(dis2, NULL)) gfx_printf ("%k%s%k\n", 0xFF00FF00, msg[6],0xFFFFFFFF);
	else gfx_printf ("%k%s%k\n", 0xFFFF0000,  msg[7],0xFFFFFFFF);

gfx_printf("%k\n%s ", 0xFFFFFF00, dis3);
if(!f_stat(dis3, NULL)) gfx_printf ("%k%s%k\n", 0xFF00FF00, msg[6],0xFFFFFFFF);
	else gfx_printf ("%k%s%k\n", 0xFFFF0000,  msg[7],0xFFFFFFFF);
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
	MDEF_CHGLINE(),
	MDEF_CAPTION("----- Status -----", 0xFF0AB9E6),
	MDEF_HANDLER("Payload Status", payload_status),
	MDEF_CHGLINE(),
	MDEF_CAPTION("-- Rename File To---", 0xFF0AB9E6),
	MDEF_HANDLER(samdsettings[2], new_payload_bin_file),
	MDEF_HANDLER(samdsettings[3], new_payload1_bin_file),
	MDEF_HANDLER("payloadx.bin (1 - 8) for SAMD select", new_payloadx_bin_file),
	MDEF_HANDLER("bootloader/update.bin", new_update_bin_file),
	MDEF_CHGLINE(),
	MDEF_CHGLINE(),
	MDEF_CAPTION("----- Disable -----", 0xFF0AB9E6),
	MDEF_HANDLER(samdsettings[2], disable_payload_bin_file),
	MDEF_HANDLER(samdsettings[3], disable_payload1_bin_file),
	MDEF_HANDLER(samdsettings[4], disable_update_bin_file),
	MDEF_CHGLINE(),
	MDEF_CHGLINE(),
	MDEF_CAPTION("----- Enable -----", 0xFF0AB9E6),
	MDEF_HANDLER(samdsettings[2], enable_payload_bin_file),
	MDEF_HANDLER(samdsettings[3], enable_payload1_bin_file),
	MDEF_HANDLER(samdsettings[4], enable_update_bin_file),
	MDEF_CHGLINE(),
	MDEF_CHGLINE(),
	MDEF_CAPTION("----- Delete -----", 0xFF0AB9E6),
	MDEF_HANDLER("Delete file", delete_file),
	MDEF_END()
};

menu_t menu_plmanagement = { ment_plmanagement, "Payload Management", 0, 0 };

ment_t ment_main[] = {
	MDEF_BACK(),
	MDEF_CHGLINE(),
	MDEF_CHGLINE(),
	MDEF_CAPTION("----- Browse -----", 0xFF0AB9E6),
	MDEF_HANDLER("Browse for payload", browse_file),
	MDEF_CHGLINE(),
	MDEF_CAPTION("---- Payloads ----", 0xFF0AB9E6),
	MDEF_MENU("Payload Management", &menu_plmanagement),
	MDEF_CHGLINE(),
	MDEF_CHGLINE(),
	MDEF_CAPTION("--Backup/Restore--", 0xFF0AB9E6),
	MDEF_MENU("Backup Options", &menu_backup),
	MDEF_MENU("Restore Options", &menu_restore),
	MDEF_CHGLINE(),
	MDEF_CHGLINE(),
	MDEF_CAPTION("------ SXOS ------", 0xFF0AB9E6),
	MDEF_HANDLER("Regenerate SXOS license.dat", restore_license_dat),
	MDEF_CHGLINE(),
	MDEF_CHGLINE(),
	MDEF_CAPTION("----- Update -----", 0xFF0AB9E6),
	MDEF_HANDLER("SAMD21 Update mode", restore_septprimary_dat),
	MDEF_CHGLINE(),
	MDEF_CHGLINE(),
	MDEF_CAPTION("------ Mount -----", 0xFF0AB9E6),
	MDEF_HANDLER("USB mount (Tidy_Memloader)", reboot_memloader),
	MDEF_CHGLINE(),
	MDEF_CHGLINE(),
	MDEF_CAPTION("------ Power -----", 0xFF0AB9E6),
	MDEF_HANDLER("Power Off", power_off),
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

	gfx_clear_color(0xFF000000);
	
	while (true)
	{
		static const char information[] =
		"%K%kFusee_UF2 Information. V6_130120.\n"
		"\n\n\n"
		"%kFollowing Straps Detected:\n\n\n\n\n"
		"%k%s\n"
		"\n\n"
		"%s\n"
		"\n\n"
		"%s\n"
		"\n\n"
		"%s\n"
		"\n\n\n\n%s\n\n%s\n%s\n"
		"\n"
		"%kName your payload(s) as follows:\n"
		"\n"
		"Location: SD Root.\n\n"
		"%s\n"
		"%s\n"
		"%s\n"
		"\n\nFor Kosmos users:\n\n"
		"%s%k\n\n";

	gfx_con_setpos(0, 0);
	gfx_printf(information,  0xFF000000, 0xFFFFFF00, 0xFF00FF00, 0xFFFFFF00, samdsettings[8], samdsettings[9], samdsettings[10], samdsettings[11], samdsettings[12],samdsettings[13],samdsettings[14], 0xFF00FFFF, samdsettings[2], samdsettings[3], samdsettings[4], samdsettings[4], 0xFFCCCCCC);
	//gfx_set_rect_grey(CTRLASSET, 41, 404, 679,0);
	if(!first_boot)
	{
		u8 btn = btn_wait_timeout(1000, BTN_POWER | BTN_VOL_UP | BTN_VOL_DOWN);
		if(!(btn & BTN_VOL_DOWN)) break;
	} else {msleep(1000); btn_wait(); break;}
	}
	gfx_clear_color(0xFF000000);
	gfx_con_setpos(0, 0);
	
}

//char * animated = ("REDanimated"); // char for uf2 purposes. Easier to srch...
//char * animated = ("REDstaticol");
//char * animated = ("BLKanimated");

//variable = general_countdown_asset("booting payloadbin", "booting payloadbin", "Selected. Release button", "Cancelled. Skipped", 0xFF0000);
int general_countdown_asset(const char * text1, const char * optional1, const char * text3, const char * text4, const u32 bgcolor, const u32 fgcolor)
{
	
	int drawn = 1;
	int res = 0;
	
	while (true)
	{
		if (drawn == 39)
			{
				gfx_con_setpos(32, 60);
				gfx_printf("%K%s", bgcolor, msg[38]);
			}
		if (drawn == 40)
			{
				gfx_con_setpos(32, 60);
				gfx_printf("%K%k%s %s...2", bgcolor, fgcolor, text1, optional1);
			}
		if (drawn == 79)
			{
				gfx_con_setpos(32, 60);
				gfx_printf("%K%s", bgcolor, msg[38]);
			}
		if (drawn == 80)
			{
				gfx_con_setpos(32, 60);
				gfx_printf("%K%k%s %s...1", bgcolor, fgcolor, text1, optional1);
			}
		gfx_con_setpos(112 , 85);
		gfx_printf("%K%k | ", bgcolor, fgcolor);
		gfx_con_setpos(192 , 85);
		gfx_printf("%K%k | ", bgcolor, fgcolor);
		gfx_con_setpos(256 , 85);
		gfx_printf("%K%k | ", bgcolor, fgcolor);
		gfx_con_setpos(16+(drawn*2) , 85);
		gfx_printf("%K%k >", bgcolor, fgcolor);
		
		u8 btn = btn_wait_timeout(20, BTN_POWER | BTN_VOL_UP | BTN_VOL_DOWN);
		
		if((btn & BTN_POWER) || (btn & BTN_VOL_UP)|| (btn & BTN_VOL_DOWN)) ++drawn;
		if(btn & BTN_POWER) res = 1;
		if(btn & BTN_VOL_UP) res = 2;
		if(btn & BTN_VOL_DOWN) res = 3;
		if((!(btn & BTN_POWER)) && (!(btn & BTN_VOL_UP)) && (!(btn & BTN_VOL_DOWN))) break;
		if(drawn > 120)break;
		
	}
	if (drawn > 120)
	{
		gfx_con_setpos(32, 60);
		gfx_printf("%s", (msg[38]));
		gfx_con_setpos(32, 60);
		gfx_printf("%K%k%s", bgcolor, fgcolor, text3);
		msleep(1000);
		gfx_con_setpos(32, 60);
		gfx_printf("%s", (msg[38]));
		goto out;
	}
	else
	{
	
		gfx_con_setpos(32, 60);
		gfx_printf("%K%k%s",bgcolor, fgcolor, text4);
		msleep(1000);
		gfx_con_setpos(32, 60);
		gfx_printf("%s", (msg[38]));
		res = 0;
	}
out:		
	return res;
	
}

bool sdupdated = false;
bool sdoldonoff = false;

void draw_sd_asset(bool sdonoff)
{
	if (sdonoff != sdoldonoff) sdupdated = false;
	if(sdupdated) return;
	u8 * SDASSET = (void *)malloc(0xE40);
	
	if(!sdupdated)
	{
		if (sdonoff){blz_uncompress_sdasset(0x00, 0xFF, 0x00, SDASSET_blz, SZ_SDASSET_blz, SDASSET, SZ_SDASSET); sdupdated = true;}
		else {blz_uncompress_sdasset(0xFF, 0x00, 0x00, SDASSET_blz, SZ_SDASSET_blz, SDASSET, SZ_SDASSET); sdupdated = true;}
		gfx_set_rect_rgb(SDASSET, 32,38, 32,1210);
	}
	sdoldonoff = sdonoff;
	
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

bool assetsupdated = false;
 
 void draw_outline_assets()
{
	if(assetsupdated) return;
	u8 * BTNASSET = (void *)malloc(0x32A0);
	u8 battoly = 32;
		
		gfx_set_rect_grey(BATTBTMASSET, 26,4, 32,battoly);
		battoly +=3;
		for (int i = 0; i<batt_size; ++i)
		{
			++battoly;
			gfx_set_rect_grey(BATTSIDASSET, 26,1, 32,battoly);
		}
		gfx_set_rect_grey(BATTTOPASSET, 26,8, 32,battoly+1);
		blz_uncompress_srcdest(BUTTONASSET_blz, SZ_BUTTONASSET_blz, BTNASSET, SZ_BUTTONASSET);
		gfx_set_rect_grey(BTNASSET, 32, 405, 688,0);
		assetsupdated = true;
		sdupdated = false;
		
	
	
}

#define BATT_THRESHDIS 5

const char * animated = (samdsettings[0]);//this is for arduino. char array easier to find than bits!

void screensaver()

{
	u8 * ASSET = (void *)malloc(0x16698);
	u8 * CLR_SCREEN = (void *)malloc(0xF);
	u32 SCREENSAVER_TIMER = 0;
	static int Current;
	u32 colour32 = 0; static bool flipx = false; static bool flipy = false;
	blz_uncompress_bootlogo(0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, NINTENDOASSET_blz, SZ_NINTENDOASSET_blz, ASSET, SZ_NINTENDOASSET);
		//swap col byte 3 for 1 and 1 for 3
		memcpy (CLR_SCREEN+0, ASSET + 2, 3);
		memcpy (CLR_SCREEN+1, ASSET + 1, 3);
		memcpy (CLR_SCREEN+2, ASSET + 0, 3);
		
		colour32 = (*(u32 *)CLR_SCREEN);
		gfx_clear_color(colour32);
		//display_backlight_brightness(64, 1000); // backlight on
		u32 xcord = 462; u32 ycord = 305; int dirx = 2; int diry = 1;
		u8 btn;
	while (true)
	{
		gfx_set_rect_rgb(ASSET, 90, 340, ycord,xcord);
		if(flipx) xcord = xcord + dirx;
		else xcord = xcord - dirx;
		
		if(flipy) ycord = ycord + diry;
		else ycord = ycord - diry;
		if(xcord >= 940) {flipx = false; ++dirx;}
		if(xcord <= 0) {flipx = true; ++dirx;}
		if(ycord >= 630) {flipy = false; ++diry;}
		if(ycord <= 0) {flipy = true; ++diry;}
		if(dirx>2) dirx = 1;
		if(diry>2) diry = 1;
		btn = btn_wait_timeout(0, BTN_POWER | BTN_VOL_UP | BTN_VOL_DOWN);
		if ((btn & BTN_POWER) || (btn & BTN_VOL_UP) || (btn & BTN_VOL_DOWN)) break;	
		++SCREENSAVER_TIMER;
		if(SCREENSAVER_TIMER == 5000) {display_backlight_brightness(0, 1000); check_power_off_from_hos(); power_off();}
		if(SCREENSAVER_TIMER == 4000)
		{
			max17050_get_property(MAX17050_Current, &Current);
			if(Current>0) SCREENSAVER_TIMER = 0;
		}
			
			
	}
	gfx_clear_color(0xFF000000);
		
		
}


int draw_boot_logo(int delayMs)
{
	
	u8 btn;
	u8 current_anim;
	static bool battCharging; static int flatBatt = 0; static int tickTimer = 0; //init global timer
	static int battPercentage, battPercent, Current;
	u8 * ASSET = (void *)malloc(0x16698);
	u8 * CLR_SCREEN = (void *)malloc(0xF);
	char TXT_COLOUR[3];
	u8 skipboot = 0;
	u32 colour32 = 0;
	bool logo_interrupt = false;


		blz_uncompress_bootlogo(*(u8*)samdsettings[18], *(u8*)samdsettings[19], *(u8*)samdsettings[20], *(u8*)samdsettings[15], *(u8*)samdsettings[16], *(u8*)samdsettings[17], NINTENDOASSET_blz, SZ_NINTENDOASSET_blz, ASSET, SZ_NINTENDOASSET);
		//swap col byte 3 for 1 and 1 for 3
		memcpy (CLR_SCREEN+0, ASSET + 2, 3);
		memcpy (CLR_SCREEN+1, ASSET + 1, 3);
		memcpy (CLR_SCREEN+2, ASSET + 0, 3);
		TXT_COLOUR[0] = *(u8*)samdsettings[18];
		TXT_COLOUR[1] = *(u8*)samdsettings[19];
		TXT_COLOUR[2] = *(u8*)samdsettings[20];
	
		colour32 = (*(u32 *)CLR_SCREEN);
	
	max17050_get_property(MAX17050_RepSOC, (int *)&battPercent);
	battPercentage = ((battPercent >> 8) & 0xFF);
	if (battPercentage < BATT_THRESHDIS) flatBatt = 0;//batt is flat
		else flatBatt = 1;	
	
	if(flatBatt == 0) // enter charging lock loop
	{
		
		draw_outline_assets();//draw batt outline
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
		flatBatt = 1;
	}
	
	if(flatBatt == 1)
	{
		u32 movegfy; u32 limitgfy;
		
		if((!strcmp(animated, "ANIM04"))) current_anim = 4;
		if((!strcmp(animated, "ANIM03"))) current_anim = 3;
		if((!strcmp(animated, "ANIM02"))) current_anim = 2;
		if((!strcmp(animated, "ANIM01"))) current_anim = 1;
		
		
		if(current_anim == 1) {movegfy = 305; limitgfy = 305;}
		if(current_anim == 2) {movegfy = 150; limitgfy = 305;}
		if(current_anim == 3) {movegfy = 1; limitgfy = 340;}
		if(current_anim == 4) {movegfy = 1; limitgfy = 90;}
		
		gfx_clear_color(colour32);
		
		while (movegfy <=limitgfy)
		{
		
		
		
		
		
		
		
		
		btn = btn_wait_timeout(0, BTN_POWER | BTN_VOL_UP | BTN_VOL_DOWN);
			if (btn & BTN_POWER)
			{
				logo_interrupt = true;
				gfx_clear_color(colour32);
				gfx_set_rect_rgb(ASSET, 90, 340, 305,462);
				display_backlight_brightness(64, 1000); // backlight on
				msleep(100); //delay unhook pwr btn
				skipboot=(general_countdown_asset(msg[1], "", msg[2], msg[3], colour32, *(u32 *)TXT_COLOUR));
				if(skipboot == 1) {flatBatt = 2; break;}
		
			}
			if (btn & BTN_VOL_UP)
			{
				logo_interrupt = true;
				gfx_clear_color(colour32);
				gfx_set_rect_rgb(ASSET, 90, 340, 305,462);
				display_backlight_brightness(64, 1000); // backlight on
				msleep(100); //delay unhook pwr btn
				skipboot=(general_countdown_asset(msg[4], samdsettings[3], msg[2], msg[3], colour32, *(u32 *)TXT_COLOUR));
				if(skipboot == 2) {flatBatt = 3; ; break;}
		
			}
			
			if (btn & BTN_VOL_DOWN)
			{
				logo_interrupt = true;
				gfx_clear_color(colour32);
				gfx_set_rect_rgb(ASSET, 90, 340, 305,462);
				display_backlight_brightness(64, 1000); // backlight on
				msleep(100); //delay unhook pwr btn
				skipboot=(general_countdown_asset(msg[4], samdsettings[4], msg[2], msg[3], colour32, *(u32 *)TXT_COLOUR));
				if(skipboot == 3) {flatBatt = 4; break;}
		
			}
			if((!strcmp(animated, "ANIM00"))) logo_interrupt= true;
			if(!logo_interrupt)
			{
				//gfx_set_rect_rgb(ASSET, 89, 329, movegfy,462);
				if(current_anim == 1) gfx_set_rect_rgb(ASSET, 90, 340, 305 ,462);
				if(current_anim == 2) gfx_set_rect_rgb(ASSET, 90, 340, movegfy ,462);
				if(current_anim == 3) gfx_set_rect_rgb(ASSET, 90, movegfy, 305 ,462);
				if(current_anim == 4) gfx_set_rect_rgb(ASSET, movegfy, 340, 305 ,462);
				display_backlight_brightness(64, 1000); // backlight on
			}
			
		if(logo_interrupt) break;
		else movegfy = (-(~movegfy));
		}
			
			
	}
	if(!logo_interrupt)
	{
		msleep(delayMs);
		display_backlight_brightness(0, 3000); msleep(250);
	}
	else
	{
		display_backlight_brightness(0, 1000);
	}
	gfx_clear_color(0xFF000000);
	return flatBatt;
}

u8 ssaver = 0;
void boot_batt()
{
	//first, determine if battery is too flat  to boot
	static int oldbattPercentage, battPercentage, battPercent, Current;
	 static bool battCharging; static int directBoot = 0;
	static int tickTimer = 0; //init global timer
	static int nextTick = 50; //init global timer
	u8 btn;
	
	directBoot = draw_boot_logo(1000);
start:		
	
	if(directBoot == 1)
		{
			//go for boot
			display_backlight_brightness(0, 1000);
			if(sd_mount())
			{
				launch_payload (path1); launch_payload (path2); launch_payload (path3);
				directBoot = 2;
			} else directBoot = 2;
		}
	if(directBoot == 3)
		{
			if(sd_mount())
			{
				launch_payload (path2);
				directBoot = 2;
			}else directBoot = 2;
		}
		
	if(directBoot == 4)
		{
			if(sd_mount())
			{
				launch_payload (path3);
				directBoot = 2;
			}else directBoot = 2;
		}
		
		
	if(directBoot == 2)
	{//main loop
	/*if(sd_mount())
		{	
			if (f_stat("safe", NULL))
			{
				gfx_clear_color(0xFF000000);
				gfx_con_setpos(0,0);
				display_backlight_brightness(64, 1000); // backlight on
				gfx_printf("Safe folder not found on this SD card\n\nPerforming initial safety backup\n\nof BOOT0/1/PRODINFO.\n\nThese backups are saved in safe\n\nfolder on SD root");
				msleep(5000);
				dump_emmc_quick();
				gfx_clear_color(0xFF000000);
				draw_rcm_asset(true);
				gfx_clear_color(0xFF000000);
				gfx_con_setpos(0,0);
				
			}
			display_backlight_brightness(0, 1000); // backlight on
			sd_unmount();
		}*/
		
		draw_outline_assets();//draw batt outline
		
		//gather values, populate display
		if((!strcmp(samdsettings[1], "SCREENSAVER1"))) ssaver = 1;
		while(true)
		{
			max17050_get_property(MAX17050_Current, &Current);
			max17050_get_property(MAX17050_RepSOC, (int *)&battPercent);
			battPercentage = ((battPercent >> 8) & 0xFF);
			if(Current>0) 
			{
				battCharging = 1;
				//tickTimer = 0;
			}
			else 
			{
				battCharging = 0;
				 // start timer 1/4 sec
			}
			++tickTimer;
			if (oldbattPercentage != battPercentage) fill_batt_asset(1, battPercentage);
			draw_ln_asset(battCharging);
			
			if (tickTimer >= nextTick)
			{
				if(sd_mount()) {draw_sd_asset(true); sd_unmount();}
				else draw_sd_asset(false);
				nextTick = tickTimer + 50;
			}
			
			btn = btn_wait_timeout(0, BTN_POWER | BTN_VOL_UP | BTN_VOL_DOWN);
			if (btn & BTN_VOL_UP){gfx_clear_color(0xFF000000); display_backlight_brightness(64, 1000); launch(); gfx_clear_color(0xFF000000); msleep(250); tickTimer = 0; nextTick = 50; oldbattPercentage = 0; assetsupdated = false; draw_outline_assets();}//draw batt outline
			if (btn & BTN_VOL_DOWN){gfx_clear_color(0xFF000000); display_backlight_brightness(64, 1000); draw_rcm_asset(false); tickTimer = 0;  nextTick = 50; oldbattPercentage = 0; assetsupdated = false; draw_outline_assets();}
			if (btn & BTN_POWER) 
			{	
				display_backlight_brightness(64, 1000);
				if (general_countdown_asset("Powering off", "", "Release PWR button", "Attempting boot   ", 0xFF000000, 0xFFFFFFFF) == 1){display_backlight_brightness(0, 1000); check_power_off_from_hos(); power_off();}
				else {sd_mount(); boot_payloads(); check_power_off_from_hos(); power_off();}
			}
			if((battCharging) && (tickTimer >= 1500)) {tickTimer = 650;  nextTick = 700;}
			if((!battCharging) && (tickTimer >= 1500)) {display_backlight_brightness(0, 1000); check_power_off_from_hos(); power_off();}
			if(tickTimer >= 700) 
			{
				display_backlight_brightness(8, 3000);
				if(ssaver)
				{
					screensaver();
					tickTimer = 0; nextTick = 50;
					oldbattPercentage = 0; 
					assetsupdated = false; 
					draw_outline_assets();
				}
			}
			if(tickTimer < 100) {display_backlight_brightness(64, 1000);}
			//if(tickTimer > 200) tickTimer = 0;
			if(oldbattPercentage) oldbattPercentage = battPercentage;
			 //display_backlight_brightness(64, 1000); // backlight on
			 
			 msleep(20);
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
