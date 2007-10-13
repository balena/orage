/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2005-2007 Juha Kautto  (juha at xfce.org)
 * Copyright (c) 2004-2006 Mickael Graf (korbinus at xfce.org)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the 
       Free Software Foundation
       51 Franklin Street, 5th Floor
       Boston, MA 02110-1301 USA

 */

#ifndef __DAY_VIEW_H__
#define __DAY_VIEW_H__

#define MAX_DAYS 40

typedef struct _day_win
{
    GtkAccelGroup *accel_group;
    GtkTooltips   *Tooltips;

    GtkWidget *Window;
    GtkWidget *Vbox;

    GtkWidget *StartDate_button;
    GtkRequisition StartDate_button_req;
    GtkWidget *day_spin;

    GtkWidget *Menubar;
    GtkWidget *File_menu;
    GtkWidget *File_menu_close;

    GtkWidget *Toolbar;
    GtkWidget *Close_toolbutton;

    GtkWidget *day_view_vbox;
    GtkWidget *scroll_win_h;
    GtkWidget *dtable_h; /* header of day table */
    GtkWidget *scroll_win;
    GtkWidget *dtable;   /* day table */
    GtkRequisition hour_req;

    GtkWidget *header[MAX_DAYS];
    GtkWidget *element[24][MAX_DAYS];
    GtkWidget *line[24][MAX_DAYS];

    guint upd_timer;

    GdkColor bg1, bg2;
} day_win;

day_win *create_day_win(char *start_date);

#endif /* !__DAY_VIEW_H__ */
