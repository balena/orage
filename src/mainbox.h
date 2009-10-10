/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2005-2008 Juha Kautto  (juha at xfce.org)
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

#ifndef __MAINBOX_H__
#define __MAINBOX_H__

typedef struct _CalWin
{
    GtkAccelGroup *mAccel_group;
    GtkTooltips   *Tooltips;

    GtkWidget *mWindow;
    GtkWidget *mVbox;

    GtkWidget *mMenubar;
    GtkWidget *mFile_menu;
    GtkWidget *mFile_newApp;
    GtkWidget *mFile_interface;
    GtkWidget *mFile_close;
    GtkWidget *mFile_quit;
    GtkWidget *mEdit_menu;
    GtkWidget *mEdit_preferences;
    GtkWidget *mView_menu;
    GtkWidget *mView_ViewSelectedDate;
    GtkWidget *mView_ViewSelectedWeek;
    GtkWidget *mView_selectToday;
    GtkWidget *mHelp_menu;
    GtkWidget *mHelp_help;
    GtkWidget *mHelp_about;

    GtkWidget *mCalendar;

    GtkWidget *mTodo_vbox;
    GtkWidget *mTodo_label;
    GtkWidget *mTodo_scrolledWin;
    GtkWidget *mTodo_rows_vbox;
    GtkWidget *mEvent_vbox;
    GtkWidget *mEvent_label;
    GtkWidget *mEvent_scrolledWin;
    GtkWidget *mEvent_rows_vbox;

    GdkColor mRed, mBlue;
} CalWin;

void build_mainWin();
gboolean orage_mark_appointments();
void build_mainbox_info();
void build_mainbox_event_box();
void mCalendar_month_changed_cb(GtkCalendar *calendar, gpointer user_data);

#endif /* !__MAINBOX_H__ */
