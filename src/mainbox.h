/* mainbox.h
 *
 * (C) 2004 Mickaël Graf
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

typedef struct _CalWin
{
    GtkWidget *mWindow;
    GtkWidget *mVbox;
    GtkWidget *mMenubar;
    GtkWidget *mFile;
    GtkWidget *mFile_menu;
    GtkWidget *mFile_newApp;
    GtkWidget *mFile_close;
    GtkWidget *mFile_separator;
    GtkWidget *mFile_quit;
    GtkWidget *mEdit;
    GtkWidget *mEdit_menu;
    GtkWidget *mEdit_preferences;
    GtkWidget *mView;
    GtkWidget *mView_menu;
    GtkWidget *mView_ViewSelectedDate;
    GtkWidget *mView_separator;
    GtkWidget *mView_selectToday;
    GtkWidget *mHelp;
    GtkWidget *mHelp_menu;
    GtkWidget *mHelp_about;
    GtkWidget *mCalendar;
    GtkAccelGroup *mAccel_group;

} CalWin;

void create_mainWin();
void xfcalendar_toggle_visible();
gboolean xfcalendar_mark_appointments();
