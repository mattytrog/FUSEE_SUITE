/*
 * Copyright (c) 2018 Rajko Stojadinovic
 * Copyright (c) 2018 CTCaer
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

#ifndef _FE_EMMC_TOOLS_H_
#define _FE_EMMC_TOOLS_H_
#include "../utils/types.h"

char * file_browser(const char * start_dir,
  const char * required_extn,
    const char * browser_caption,
      const bool get_dir,
        const bool goto_root_on_fail,
          const bool ASCII_order);
void dump_emmc_system();
void dump_emmc_user();
void dump_emmc_boot();
void dump_emmc_rawnand();
void dump_emmc_quick();

void restore_emmc_boot();
void restore_emmc_rawnand();
void restore_emmc_gpp_parts();
void restore_license_dat();
void restore_septprimary_dat();
void reboot_memloader();
void restore_emmc_quick();
void restore_emmc_quick_prodinfo();
void restore_emmc_quick_noszchk();

#endif
