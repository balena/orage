/* event-list.h
 *
 * Copyright (C) 2004 Mickaël Graf <korbinus at xfce.org>
 * Copyright (C) 2005 Juha Kautto <kautto.juha at kolumbus.fi>
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

#include <gtk/gtk.h>
#include <gdk/gdk.h>

typedef struct
{
    GtkWidget *elWindow;
    GtkWidget *elVbox;
    GtkWidget *elToolbar;
    GtkWidget *elCreate_toolbutton;
    GtkWidget *elPrevious_toolbutton;
    GtkWidget *elToday_toolbutton;
    GtkWidget *elNext_toolbutton;
    GtkWidget *elRefresh_toolbutton;
    GtkWidget *elCopy_toolbutton;
    GtkWidget *elClose_toolbutton;
    GtkWidget *elDelete_toolbutton;
    GtkWidget *elScrolledWindow;
    GtkWidget *elTreeView;
    GtkTooltips *elTooltips;
    GtkTreeSelection *elTreeSelection;
    GtkListStore    *elListStore;
    GtkTreeSortable *elTreeSortable;
    GtkAccelGroup *accel_group;
    GtkWidget *elSpin1, *elSpin2;
    GtkTreeViewColumn *elTime_treeviewcolumn;

    gboolean elToday;
} eventlist_win;

eventlist_win* create_eventlist_win(void);
void recreate_eventlist_win(eventlist_win *el);
void manage_eventlist_win(GtkCalendar *calendar, eventlist_win *el);
