/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2006-2007 Juha Kautto  (juha at xfce.org)
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

#ifndef __INTERFACE_H__
#define __INTERFACE_H__

typedef struct
{
    GtkWidget *main_window;
    GtkWidget *main_vbox;
    GtkWidget *menubar;
    GtkWidget *filemenu;
    GtkWidget *filemenu_save;
    GtkWidget *filemenu_close;
    GtkWidget *toolbar;
    GtkWidget *save_button;
    GtkWidget *close_button;
    GtkWidget *notebook;
    GtkWidget *exp_table;
    GtkWidget *exp_notebook_page;
    GtkWidget *exp_tab_label;
    GtkWidget *exp_file_entry;
    GtkWidget *exp_file_button;
    GtkWidget *exp_add_all_rb;
    GtkWidget *exp_add_id_rb;
    GtkWidget *exp_id_entry;
    GtkWidget *imp_table;
    GtkWidget *imp_notebook_page;
    GtkWidget *imp_tab_label;
    GtkWidget *imp_file_entry;
    GtkWidget *imp_file_button;
    GtkWidget *arc_table;
    GtkWidget *arc_notebook_page;
    GtkWidget *arc_tab_label;
    GtkWidget *arc_button1;
    GtkWidget *arc_button2;

    GtkTooltips *tooltips;
    GtkAccelGroup *accelgroup;
} intf_win;  /* interface = export/import window */

void orage_external_interface(CalWin *xfcal);

#endif /* !__INTERFACE_H__ */
