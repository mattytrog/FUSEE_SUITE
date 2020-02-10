/*
 * Copyright (c) 2018 naehrwert
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

#include "di.h"
#include "tui.h"
#include "../utils/btn.h"
#include "../config/config.h"
#include "../power/max17050.h"
#include "../utils/util.h"

#ifdef MENU_LOGO_ENABLE
extern u8 *Kc_MENU_LOGO;
#define X_MENU_LOGO       119
#define Y_MENU_LOGO        57
#define X_POS_MENU_LOGO   577
#define Y_POS_MENU_LOGO  1179
#endif //MENU_LOGO_ENABLE

void tui_pbar(int x, int y, u32 val, u32 fgcol, u32 bgcol)
{
	u32 cx, cy;
	if (val > 200)
		val = 200;

	gfx_con_getpos(&cx, &cy);

	gfx_con_setpos(x, y);

	gfx_printf("%k%3d%%%k", fgcol, val, bgcol);
}

void *tui_do_menu(menu_t *menu)
{
	int idx = 0, prev_idx = 0, cnt = 0x7FFFFFFF;

	gfx_clear_partial_grey(0x00, 0, 1256);

#ifdef MENU_LOGO_ENABLE
	gfx_set_rect_rgb(Kc_MENU_LOGO,
		X_MENU_LOGO, Y_MENU_LOGO, X_POS_MENU_LOGO, Y_POS_MENU_LOGO);
#endif //MENU_LOGO_ENABLE

	while (true)
	{
		gfx_con_setcol(0xFFCCCCCC, 1, 0xFF000000);
		gfx_con_setpos(menu->x, menu->y);
		gfx_printf("[%s]\n\n", menu->caption);

		// Skip caption or seperator lines selection.
		while (menu->ents[idx].type == MENT_CAPTION ||
			menu->ents[idx].type == MENT_CHGLINE)
		{
			if (prev_idx <= idx || (!idx && prev_idx == cnt - 1))
			{
				idx++;
				if (idx > (cnt - 1))
				{
					idx = 0;
					prev_idx = 0;
				}
			}
			else
			{
				idx--;
				if (idx < 0)
				{
					idx = cnt - 1;
					prev_idx = cnt;
				}
			}
		}
		prev_idx = idx;

		// Draw the menu.
		for (cnt = 0; menu->ents[cnt].type != MENT_END; cnt++)
		{
			if (cnt == idx)
				gfx_con_setcol(0xFF000000, 1, 0xFFCCCCCC);
			else
				gfx_con_setcol(0xFFCCCCCC, 1, 0xFF000000);
			if (menu->ents[cnt].type == MENT_CAPTION)
				gfx_printf("%k %s", menu->ents[cnt].color, menu->ents[cnt].caption);
			else if (menu->ents[cnt].type != MENT_CHGLINE)
				gfx_printf(" %s", menu->ents[cnt].caption);
			if(menu->ents[cnt].type == MENT_MENU)
				gfx_printf("%k...", 0xFF0099EE);
			gfx_printf(" \n");
		}
		gfx_con_setcol(0xFFCCCCCC, 1, 0xFF000000);
		gfx_putc('\n');

		// Wait for user command.
		u32 btn = btn_wait();

		if (btn & BTN_VOL_DOWN && idx < (cnt - 1))
			idx++;
		else if (btn & BTN_VOL_DOWN && idx == (cnt - 1))
		{
			idx = 0;
			prev_idx = -1;
		}
		if (btn & BTN_VOL_UP && idx > 0)
			idx--;
		else if (btn & BTN_VOL_UP && idx == 0)
		{
			idx = cnt - 1;
			prev_idx = cnt;
		}
		if (btn & BTN_POWER)
		{
			ment_t *ent = &menu->ents[idx];
			switch (ent->type)
			{
			case MENT_HANDLER:
				ent->handler(ent->data);
				break;
			case MENT_MENU:
				return tui_do_menu(ent->menu);
				break;
			case MENT_DATA:
				return ent->data;
				break;
			case MENT_BACK:
				return NULL;
				break;
			case MENT_HDLR_RE:
				ent->handler(ent);
				if (!ent->data)
					return NULL;
				break;
			default:
				break;
			}
			gfx_con.fntsz = 16;
			gfx_clear_partial_grey(0x00, 0, 1256);
#ifdef MENU_LOGO_ENABLE
			gfx_set_rect_rgb(Kc_MENU_LOGO,
				X_MENU_LOGO, Y_MENU_LOGO, X_POS_MENU_LOGO, Y_POS_MENU_LOGO);
#endif //MENU_LOGO_ENABLE
		}
	}

	return NULL;
}
