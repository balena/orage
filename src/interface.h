/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2006-2008 Juha Kautto  (juha at xfce.org)
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

typedef struct _intf_win
{
    GtkWidget *main_window;
    GtkWidget *main_vbox;
    GtkWidget *menubar;
    GtkWidget *filemenu;
    GtkWidget *filemenu_close;
    GtkWidget *toolbar;
    GtkWidget *close_button;
    GtkWidget *notebook;

        /* Import/export tab */
    GtkWidget *iea_notebook_page;
    GtkWidget *iea_tab_label;
    /* import */
    GtkWidget *iea_imp_frame;
    GtkWidget *iea_imp_entry;
    GtkWidget *iea_imp_open_button;
    GtkWidget *iea_imp_save_button;
    /* export */
    GtkWidget *iea_exp_frame;
    GtkWidget *iea_exp_entry;
    GtkWidget *iea_exp_open_button;
    GtkWidget *iea_exp_save_button;
    GtkWidget *iea_exp_add_all_rb;
    GtkWidget *iea_exp_add_id_rb;
    GtkWidget *iea_exp_id_entry;
#ifdef HAVE_ARCHIVE
    /* archive */
    GtkWidget *iea_arc_frame;
    GtkWidget *iea_arc_button1;
    GtkWidget *iea_arc_button2;
#endif

        /* Orage files tab */
    GtkWidget *fil_notebook_page;
    GtkWidget *fil_tab_label;
    /* Orage calendar file */
    GtkWidget *orage_file_frame;
    GtkWidget *orage_file_entry;
    GtkWidget *orage_file_open_button;
    GtkWidget *orage_file_save_button;
    GtkWidget *orage_file_rename_rb;
    GtkWidget *orage_file_copy_rb;
    GtkWidget *orage_file_move_rb;
    /* archive file */
    GtkWidget *archive_file_frame;
    GtkWidget *archive_file_entry;
    GtkWidget *archive_file_open_button;
    GtkWidget *archive_file_save_button;
    GtkWidget *archive_file_rename_rb;
    GtkWidget *archive_file_copy_rb;
    GtkWidget *archive_file_move_rb;

        /* Foreign tab */
    GtkWidget *for_notebook_page;
    GtkWidget *for_tab_label;
    GtkWidget *for_tab_main_vbox;
    /* add new file */
    GtkWidget *for_new_frame;
    GtkWidget *for_new_entry;
    GtkWidget *for_new_open_button;
    GtkWidget *for_new_save_button;
    GtkWidget *for_new_read_only;
    /* currrent files */
    GtkWidget *for_cur_frame;
    GtkWidget *for_cur_table;

    GtkTooltips *tooltips;
    GtkAccelGroup *accelgroup;
} intf_win;  /* interface = export/import window */

void orage_external_interface(CalWin *xfcal);

gboolean orage_foreign_files_check(gpointer user_data);
gboolean orage_foreign_file_add(gchar *filename, gboolean read_only);
gboolean orage_foreign_file_remove(gchar *filename);
gboolean orage_import_file(gchar *entry_filename);

#endif /* !__INTERFACE_H__ */
