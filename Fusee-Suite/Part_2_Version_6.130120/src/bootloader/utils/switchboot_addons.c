/*
 * Copyright (c) 2019 Mattytrog
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

//start_dir = dir to start browsing from
//required_extn = files of certain extension only
//browser_caption = Title in browser
//get_dir = will return dir or file. If disabled and a folder chosen, error displayed
//on fail, display error, automatically go to sd root

#include <string.h>
#include <stdlib.h>
#include "switchboot_addons.h"
#include "../config/ini.h"
#include "../config/config.h"
#include "../gfx/gfx.h"
#include "../gfx/tui.h"
#include "../libs/fatfs/ff.h"
#include "../utils/btn.h"
#include "../utils/util.h"
#include "../frontend/fe_emmc_tools.h"
#include "../mem/heap.h"
#include "../sec/se.h"
#include "../storage/nx_emmc.h"
#include "../storage/sdmmc.h"
#include "../utils/dirlist.h"

#define EMMC_BUF_ALIGNED 0xB5000000
#define SDXC_BUF_ALIGNED 0xB6000000
#define MIXD_BUF_ALIGNED 0xB7000000

#define NUM_SECTORS_PER_ITER 8192 // 4MB Cache.
#define OUT_FILENAME_SZ 80
#define HASH_FILENAME_SZ (OUT_FILENAME_SZ + 11) // 11 == strlen(".sha256sums")
#define SHA256_SZ 0x20

extern sdmmc_t sd_sdmmc;
extern sdmmc_storage_t sd_storage;
extern FATFS sd_fs;
bool noszchk;
extern bool sd_mount();
extern void sd_unmount();
extern int launch_payload(char *path, bool update);
extern void menu_autorcm();
extern void restore_license_dat();
extern int  sd_save_to_file(void *buf, u32 size, const char *filename);

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
		ments[5].type = MENT_DATA;
        ments[5].caption = "Regenerate SXOS licence";
        ments[5].data = "<sxos>";
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

	if (!strcmp(file_sec_untrimmed + strlen(file_sec_untrimmed) - 6, "<sxos>")) {
        restore_license_dat();
	  } else if (!strcmp(file_sec_untrimmed + strlen(file_sec_untrimmed) - 11, "<home_menu>")) {
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
	launch_payload("bootloader/payloads/samd21update.bin", false);
	} else if (btn & BTN_POWER){
		launch_payload("bootloader/payloads/samd21update.bin", false);
	}
	return;
}

void reboot_memloader(){

	if (sd_mount()){
	u8 fr = launch_payload("bootloader/memloader/tidy_memloader.bin", false);
	if (fr) gfx_printf("\ntidy_memloader.bin missing\n");
	}
	btn_wait();
	return;
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

static void _dump_emmc_extra(emmcPartType_t dumpType)
{
	int res = 0;
	u32 timer = 0;
	gfx_clear_partial_grey(0x00, 0, 1256);
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
	// Create SAFE folder
	emmc_path_impl(sdPath, "/SAFE", "", &storage);
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

				emmc_path_impl(sdPath, "/SAFE", "", &storage);
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

			sdmmc_storage_set_mmc_partition(&storage, i + 1);
			emmc_path_impl(sdPath, "/SAFE", "", &storage);
			emmc_path_impl(sdPath, "", bootPart.name, &storage);
			res = _dump_emmc_part(sdPath, &storage, &bootPart);
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

void dump_emmc_quick()	 { _dump_emmc_extra(PART_PRODBOOTS); }

static void _restore_emmc_extra(emmcPartType_t restoreType)
{
	//int res = 0;
	u32 timer = 0;
	gfx_clear_partial_grey(0x00, 0, 1256);
	gfx_con_setpos(0, 0);
	
	if ((restoreType & PART_BOOT) || (restoreType & PART_GP_ALL))
	{
		gfx_puts("The mode will only restore the ");
		if (restoreType & PART_BOOT)
			gfx_puts("boot ");
		gfx_puts("\npartitions that it can find.\n\n");
	}
	
	gfx_printf("Are you really sure?\n\n%k", 0xFFFFFFFF);
	gfx_con_getpos(&gfx_con.savedx, &gfx_con.savedy);
	u8 press = 5;
			while (press > 0){
			gfx_con_setpos(gfx_con.savedx, gfx_con.savedy);	
			gfx_printf("%k[VOL] - Cancel. Press [PWR] %d times...\n", 0xFFFFFF00, press);	
			u8 btn = btn_wait();
				if (btn & BTN_POWER){
					--press;
					msleep(100);
					
					
				} else goto out;
			}

	if (!sd_mount())
		goto out;

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
			emmc_path_impl(sdPath, "/SAFE", "", &storage);
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

			emmc_path_impl(sdPath, "/SAFE", "", &storage);
			emmc_path_impl(sdPath, "", prodinfoPart.name, &storage);
			_restore_emmc_part(sdPath, &storage, &prodinfoPart, false);
		}
	}

	gfx_putc('\n');
	timer = get_tmr_s() - timer;
	gfx_printf("Time taken: %dm %ds.\n", timer / 60, timer % 60);
	sdmmc_storage_end(&storage);

out:
	gfx_printf("\nPress any key");
	btn_wait();
	sd_unmount();
}
extern void restore_emmc_boot();
void restore_emmc_quick()     		{_restore_emmc_extra(PART_PRODBOOTS); }
void restore_emmc_quick_prodinfo()	{_restore_emmc_extra(PART_PRODINFO_ONLY);}
void restore_emmc_quick_noszchk()   {noszchk = true; restore_emmc_boot(); noszchk = false;}

/*if (allow_multi_part)
	{
		// Check to see if there is a combined file and if so then use that.
		if (f_stat(outFilename, &fno))
		{
			// If not, check if there are partial files and the total size matches.
			gfx_printf("No single file, checking for part files...\n");

			outFilename[sdPathLen++] = '.';

			// Stat total size of the part files.
			while ((u32)((u64)totalCheckFileSize >> (u64)9) != totalSectors)
			{
				_update_filename(outFilename, sdPathLen, 99, numSplitParts);

				gfx_con_setpos(gfx_con.savedx, gfx_con.savedy);
				gfx_printf("\nFilename: %s\n", outFilename);

				if (f_stat(outFilename, &fno))
				{
					WPRINTFARGS("Error (%d) file not found '%s'. Aborting...\n", res, outFilename);
					return 0;
				}
				else
					totalCheckFileSize += (u64)fno.fsize;

				numSplitParts++;
			}

			gfx_printf("\n%X sectors total.\n", (u32)((u64)totalCheckFileSize >> (u64)9));

			if ((u32)((u64)totalCheckFileSize >> (u64)9) != totalSectors)
			{
				if (!noszchk)
				{
				gfx_con.fntsz = 16;
				EPRINTF("Size of the SD Card backup does not match,\neMMC's selected part size.\n");
				f_close(&fp);
				return 0;
				} else 
				{
				gfx_con.fntsz = 16;
				EPRINTF("This is dangerous. Ensure you\nhave a backup before continuing\n");
				gfx_printf("\n[PWR] - Continue.  [VOL] - Cancel\n\n");
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
	gfx_con_setpos(gfx_con.savedx, gfx_con.savedy);
	gfx_printf("\nFilename: %s\n", outFilename);
	if (res)
	{
		if (res != FR_NO_FILE)
			EPRINTFARGS("Error (%d) while opening backup. Continuing...\n", res);
		else
			WPRINTFARGS("Error (%d) file not found. Continuing...\n", res);
		gfx_con.fntsz = 16;

		return 0;
	}
	else if (!use_multipart && (((u32)((u64)f_size(&fp) >> (u64)9)) != totalSectors)) // Check total restore size vs emmc size.
	{
		if (!noszchk){
		gfx_con.fntsz = 16;
		EPRINTF("Size of the SD Card backup does not match,\neMMC's selected part size.\n");
		f_close(&fp);
		return 0;
		} else {
			gfx_con.fntsz = 16;
			EPRINTF("This is dangerous. Ensure you\nhave a backup before continuing\n");
			gfx_printf("\n[PWR] - Continue.  [VOL] - Cancel\n\n");
			u32 btn = btn_wait();
			if (!(btn & BTN_POWER)){
				gfx_printf("Cancelled.\n\n");
				totalSectors = 0;
				}
			totalSectors = ((u32)((u64)f_size(&fp) >> (u64)9));
		}
	else
	{
		fileSize = (u64)f_size(&fp);
		gfx_printf("\nTotal restore size: %d MiB.\n\n",
			(u32)((use_multipart ? (u64)totalCheckFileSize : fileSize) >> (u64)9) >> SECTORS_TO_MIB_COEFF);
	}

	u8 *buf = (u8 *)MIXD_BUF_ALIGNED;

	u32 lba_curr = part->lba_start;
	u32 bytesWritten = 0;
	u32 prevPct = 200;
	int retryCount = 0;

	u32 num = 0;
	u32 pct = 0;

	gfx_con_getpos(&gfx_con.savedx, &gfx_con.savedy);

	while (totalSectors > 0)
	{*/
		// If we have more than one part, check the size for the split parts and make sure that the bytes written is not more than that.