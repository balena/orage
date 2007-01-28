/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2005-2007 Juha Kautto  (juha at xfce.org)
 * Copyright (c) 2003-2005 Mickael Graf (korbinus at xfce.org)
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

#include <libxfcegui4/libxfcegui4.h>

#include "mainbox.h"
#include "functions.h"
#include "interface.h"
#include "event-list.h"
#include "appointment.h"
#include "ical-code.h"
#include "parameters.h"

#define FILETYPE_SIZE 2


/*
enum {
  TARGET_STRING,
  TARGET_ROOTWIN
};
static GtkTargetEntry target_table[] = {
  { "STRING",     0, TARGET_STRING },
  { "text/plain", 0, TARGET_STRING },
  { "application/x-rootwindow-drop", 0, TARGET_ROOTWIN }
};
*/
enum {
    DRAG_TARGET_URILIST = 0
   ,DRAG_TARGET_STRING
};
static const GtkTargetEntry file_drag_targets[] =
{
    { "text/uri-list", 0, DRAG_TARGET_URILIST }
};
static int file_drag_target_count = 1;
static const GtkTargetEntry uid_drag_targets[] =
{
    { "STRING", 0, DRAG_TARGET_STRING }
};
static int uid_drag_target_count = 1;


void on_exp_file_button_clicked_cb(GtkButton *button, gpointer user_data)
{
    intf_win *intf_w = (intf_win *)user_data;
    GtkWidget *chooser;
    GtkFileFilter *filter;
    gchar *entry_filename;
    gchar *cal_file;
    const gchar *filetype[FILETYPE_SIZE] = {"*.ics", "*.vcs"};
    int i;

    entry_filename = g_strdup(gtk_entry_get_text(
            (GtkEntry *)intf_w->exp_file_entry));
    chooser = gtk_file_chooser_dialog_new(_("Select a file...")
            , GTK_WINDOW (intf_w->main_window)
            , GTK_FILE_CHOOSER_ACTION_SAVE
            , GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL
            , GTK_STOCK_OK, GTK_RESPONSE_ACCEPT
            , NULL);

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, _("Calendar Files"));
    for (i = 0; i < FILETYPE_SIZE; i++) {
        gtk_file_filter_add_pattern(filter, filetype[i]);
    }
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, _("All Files"));
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);

    if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
        cal_file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
        if (cal_file) {
            gtk_entry_set_text(GTK_ENTRY(intf_w->exp_file_entry), cal_file);
            gtk_widget_grab_focus(intf_w->exp_file_entry);
            gtk_editable_set_position(GTK_EDITABLE(intf_w->exp_file_entry), -1);
        }
    }

    gtk_widget_destroy(chooser);
    g_free(entry_filename);
}

void on_imp_file_button_clicked_cb(GtkButton *button, gpointer user_data)
{
    intf_win *intf_w = (intf_win *)user_data;
    GtkWidget *chooser;
    GtkFileFilter *filter;
    gchar *entry_filename;
    gchar *cal_file;
    const gchar *filetype[FILETYPE_SIZE] = {"*.ics", "*.vcs"};
    int i;

    entry_filename = g_strdup(gtk_entry_get_text(
            (GtkEntry *)intf_w->imp_file_entry));
    chooser = gtk_file_chooser_dialog_new(_("Select a file...")
            , GTK_WINDOW (intf_w->main_window)
            , GTK_FILE_CHOOSER_ACTION_OPEN
            , GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL
            , GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT
            , NULL);

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, _("Calendar Files"));
    for (i = 0; i < FILETYPE_SIZE; i++) {
        gtk_file_filter_add_pattern(filter, filetype[i]);
    }
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, _("All Files"));
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);

    if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
        cal_file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
        if (cal_file) {
            gtk_entry_set_text(GTK_ENTRY(intf_w->imp_file_entry), cal_file);
            gtk_widget_grab_focus(intf_w->imp_file_entry);
            gtk_editable_set_position(GTK_EDITABLE(intf_w->imp_file_entry), -1);
        }
    }

    gtk_widget_destroy(chooser);
    g_free(entry_filename);
}

void on_archive_button_clicked_cb(GtkButton *button, gpointer user_data)
{
    intf_win *intf_w = (intf_win *)user_data;
    xfical_archive();
}

void on_unarchive_button_clicked_cb(GtkButton *button, gpointer user_data)
{
    g_print("on_unarchive_button_clicked_cb: start\n");
    xfical_unarchive();
}

gboolean orage_import_file(gchar *entry_filename) 
{
    return(xfical_import_file(entry_filename));
}

gboolean orage_export_file(gchar *entry_filename, gint count, gchar *uids) 
{
    return(xfical_export_file(entry_filename, count, uids));
}

void save_intf_import(intf_win *intf_w)
{
    gchar *entry_filename, *filename, *filename_end;
    gboolean more_files;

    entry_filename = g_strdup(gtk_entry_get_text(
            (GtkEntry *)intf_w->imp_file_entry));
    if (entry_filename != NULL && entry_filename[0] != 0) {
        more_files = TRUE;
        for (filename = entry_filename; more_files; ) {
            filename_end = g_strstr_len((const gchar *)filename
                    , strlen(filename), ",");
            if (filename_end != NULL)
                *filename_end = 0; /* filename ends here */
            /* FIXME: proper messages to screen */
            if (orage_import_file(filename))
                g_message("Orage **: Import done %s", filename);
            else
                g_warning("import failed file=%s\n", filename);
            if (filename_end != NULL) { /* we have more files */
                filename = filename_end+1; /* next file starts here */
                more_files = TRUE;
            }
            else {
                more_files = FALSE;
            }
        }
    }
    else
        g_warning("save_button_clicked: filename MISSING");
    g_free(entry_filename);
}

void save_intf_export(intf_win *intf_w)
{
    gchar *entry_filename, *entry_uids;
    gint app_count = 0; /* 0 = all, 1 = one */

    entry_filename = g_strdup(gtk_entry_get_text(
            (GtkEntry *)intf_w->exp_file_entry));
    entry_uids = g_strdup(gtk_entry_get_text(
            (GtkEntry *)intf_w->exp_id_entry));
    if (entry_filename != NULL && entry_filename[0] != 0) {
        /* FIXME: proper messages to screen */
        if (gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(intf_w->exp_add_all_rb))) {
            app_count = 0;
        }
        else if (gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(intf_w->exp_add_id_rb))) {
            app_count = 1;
        }
        else {
            g_warning("UNKNOWN select appointment %s\n");
        }

        if (orage_export_file(entry_filename, app_count, entry_uids))
            g_message("Orage **: Export done %s", entry_filename);
        else
            g_warning("export failed file=%s\n", entry_filename);
    }
    else
        g_warning("save_button_clicked: filename MISSING");
    g_free(entry_filename);
    g_free(entry_uids);
}

void save_intf(gpointer user_data)
{
    intf_win *intf_w = (intf_win *)user_data;
    gint page_num;
    GtkNotebook *notebook;
    GtkWidget *notebook_page, *tab_label;
    const gchar *label_str;

    notebook = GTK_NOTEBOOK(intf_w->notebook);
    page_num = gtk_notebook_get_current_page(notebook);
    notebook_page = gtk_notebook_get_nth_page(notebook, page_num);
    tab_label = gtk_notebook_get_tab_label(notebook, notebook_page);
    label_str = gtk_label_get_text(GTK_LABEL(tab_label));

    if (!strcmp(label_str, _("Import"))) {
        save_intf_import(intf_w);
    }
    else if (!strcmp(label_str, _("Export"))) {
        save_intf_export(intf_w);
    }
    else
        g_warning("orage_import_file: unknown tab label");
}

void close_intf_w(gpointer user_data)
{
    intf_win *intf_w = (intf_win *)user_data;

    gtk_widget_destroy(intf_w->main_window);
    g_free(intf_w);
}

void save_button_clicked(GtkButton *button, gpointer user_data)
{
    save_intf(user_data);
}

void filemenu_save_activated(GtkMenuItem *menuitem, gpointer user_data)
{
    save_intf(user_data);
}

void close_button_clicked(GtkButton *button, gpointer user_data)
{
    close_intf_w(user_data);
}

void filemenu_close_activated(GtkMenuItem *menuitem, gpointer user_data)
{
    close_intf_w(user_data);
}

void create_menu(intf_win *intf_w)
{
    GtkWidget *menu_separator;

    /* Menu bar */
    intf_w->menubar = gtk_menu_bar_new();
    gtk_box_pack_start(GTK_BOX(intf_w->main_vbox), intf_w->menubar
            , FALSE, FALSE, 0);

    /* File menu */
    intf_w->filemenu = orage_menu_new(_("_File"), intf_w->menubar);

    intf_w->filemenu_save =
            orage_image_menu_item_new_from_stock("gtk-save"
                    , intf_w->filemenu, intf_w->accelgroup);

    menu_separator = orage_separator_menu_item_new( intf_w->filemenu);
    
    intf_w->filemenu_close = 
            orage_image_menu_item_new_from_stock("gtk-close" 
                    , intf_w->filemenu, intf_w->accelgroup);

    g_signal_connect((gpointer)intf_w->filemenu_save, "activate"
            , G_CALLBACK(filemenu_save_activated), intf_w);
    g_signal_connect((gpointer)intf_w->filemenu_close, "activate"
            , G_CALLBACK(filemenu_close_activated), intf_w);
}

void create_toolbar(intf_win *intf_w)
{
    gint i = 0;
    GtkWidget *toolbar_separator;

    /* Toolbar */
    intf_w->toolbar = gtk_toolbar_new();
    gtk_box_pack_start(GTK_BOX(intf_w->main_vbox), intf_w->toolbar
            , FALSE, FALSE, 0);
    gtk_toolbar_set_tooltips(GTK_TOOLBAR(intf_w->toolbar), TRUE);

    /* Buttons */
    intf_w->save_button = orage_toolbar_append_button(intf_w->toolbar
            , "gtk-save", intf_w->tooltips, _("Save"), i++);

    toolbar_separator = orage_toolbar_append_separator(intf_w->toolbar
            , i++);

    intf_w->close_button = orage_toolbar_append_button(intf_w->toolbar
            , "gtk-close", intf_w->tooltips, _("Close"), i++);

    g_signal_connect((gpointer)intf_w->save_button, "clicked"
            , G_CALLBACK(save_button_clicked), intf_w);
    g_signal_connect((gpointer)intf_w->close_button, "clicked"
            , G_CALLBACK(close_button_clicked), intf_w);
}

void handle_file_drag_data(GtkWidget *widget, GdkDragContext *context
        , GtkSelectionData *data, guint time, gboolean imp)
{
    gchar *contents;
    gsize length;
    gchar **file_list;
    gchar *file;
    gint i, pos;
    GError *error = NULL;

    if (data->length < 0) {
        g_warning("File drag failed");
        gtk_drag_finish(context, FALSE, FALSE, time);
        return;
    }
    file_list = g_uri_list_extract_uris((gchar *)data->data);
    for (i = 0; file_list[i] != NULL; i++) {
        if ((file = g_filename_from_uri(file_list[i], NULL, &error)) == NULL) {
            g_warning("Dragging g_filename_from_uri %s failed %s"
                    , file_list[i], error->message);
            g_error_free(error);
            return;
        }
        if (i == 0) { /* first file (often the only file */
            gtk_entry_set_text(GTK_ENTRY(widget), file);
            gtk_editable_set_position(GTK_EDITABLE(widget), -1);
            pos = gtk_editable_get_position(GTK_EDITABLE(widget));
        }
        else {
            if (imp) { /* quite ok to have more files */
                gtk_editable_insert_text(GTK_EDITABLE(widget), ",", 1, &pos);
                gtk_editable_insert_text(GTK_EDITABLE(widget), file
                        , strlen(file), &pos);
            }
            else { /* export to only 1 file, ignoring the rest */
                g_warning("Exporting only to one file, ignoring drag file %d (%s)\n"
                        , i+1, file_list[i]);
            }
        }
    }
    gtk_drag_finish(context, TRUE, FALSE, time);
}

void imp_file_drag_data_received(GtkWidget *widget, GdkDragContext *context
        , gint x, gint y, GtkSelectionData *data, guint info, guint time)
{
    handle_file_drag_data(widget, context, data, time, TRUE);
}

void exp_file_drag_data_received(GtkWidget *widget, GdkDragContext *context
        , gint x, gint y, GtkSelectionData *data, guint info, guint time)
{
    handle_file_drag_data(widget, context, data, time, FALSE);
}

void uid_drag_data_received(GtkWidget *widget, GdkDragContext *context
        , gint x, gint y, GtkSelectionData *data, guint info, guint time)
{
    if (data->length < 0) {
        g_warning("UID drag failed");
        gtk_drag_finish(context, FALSE, FALSE, time);
        return;
    }
    gtk_drag_finish(context, TRUE, FALSE, time);
}

/*
gboolean drag_motion(GtkWidget *widget, GdkDragContext *context
        , gint x, gint y, guint time)
{
g_print ("drag_motion\n");

    if (context->actions & GDK_ACTION_COPY) {
g_print ("drag_motion: accept\n");
        gdk_drag_status(context, GDK_ACTION_COPY, time);
        gtk_drag_highlight(widget);
        return(TRUE);
    }
    else {
g_print ("drag_motion: fail\n");
        gdk_drag_status(context, 0, time);
        return(FALSE);
    }
}

void drag_leave(GtkWidget *widget, GdkDragContext *context, guint time)
{
g_print ("drag_leave\n");
    gtk_drag_unhighlight(widget);
}
*/

gboolean drag_drop(GtkWidget *widget, GdkDragContext *context
        , gint x, gint y, guint time)
{
    gtk_drag_get_data(widget, context
            , GDK_POINTER_TO_ATOM(context->targets->data), time);

    return(TRUE);
}

void drag_and_drop_init(intf_win *intf_w)
{
    /*
    g_signal_connect(intf_w->imp_file_entry, "drag_motion"
            , G_CALLBACK(drag_motion), NULL);
    g_signal_connect(intf_w->imp_file_entry, "drag_leave"
            , G_CALLBACK(drag_leave), NULL);
            */
    gtk_drag_dest_set(intf_w->imp_file_entry
            , GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT
            , file_drag_targets, file_drag_target_count, GDK_ACTION_COPY);
    g_signal_connect(intf_w->imp_file_entry, "drag_drop"
            , G_CALLBACK(drag_drop), NULL);
    g_signal_connect(intf_w->imp_file_entry, "drag_data_received"
            , G_CALLBACK(imp_file_drag_data_received), NULL);

    gtk_drag_dest_set(intf_w->exp_file_entry
            , GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT
            , file_drag_targets, file_drag_target_count, GDK_ACTION_COPY);
    g_signal_connect(intf_w->exp_file_entry, "drag_drop"
            , G_CALLBACK(drag_drop), NULL);
    g_signal_connect(intf_w->exp_file_entry, "drag_data_received"
            , G_CALLBACK(exp_file_drag_data_received), NULL);

    gtk_drag_dest_set(intf_w->exp_id_entry
            , GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT
            , uid_drag_targets, uid_drag_target_count, GDK_ACTION_COPY);
    g_signal_connect(intf_w->exp_id_entry, "drag_drop"
            , G_CALLBACK(drag_drop), NULL);
    g_signal_connect(intf_w->exp_id_entry, "drag_data_received"
            , G_CALLBACK(uid_drag_data_received), NULL);
}

void exp_add_all_rb_clicked(GtkWidget *button, gpointer *user_data)
{
    intf_win *intf_w = (intf_win *)user_data;

    gtk_widget_set_sensitive(intf_w->exp_id_entry, FALSE);
}

void exp_add_id_rb_clicked(GtkWidget *button, gpointer *user_data)
{
    intf_win *intf_w = (intf_win *)user_data;

    gtk_widget_set_sensitive(intf_w->exp_id_entry, TRUE);
}

void create_export_tab(intf_win *intf_w)
{
    GtkWidget *hbox, *label;
    gchar *file;

    intf_w->exp_table = orage_table_new(3, 20);
    intf_w->exp_notebook_page = intf_w->exp_table;
    intf_w->exp_tab_label = gtk_label_new(_("Export"));
    gtk_notebook_append_page(GTK_NOTEBOOK(intf_w->notebook)
            , intf_w->exp_notebook_page, intf_w->exp_tab_label);

    label = gtk_label_new(_("Write to file:"));
    hbox = gtk_hbox_new(FALSE, 0);
    intf_w->exp_file_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), intf_w->exp_file_entry, TRUE, TRUE, 0);
    intf_w->exp_file_button = gtk_button_new_from_stock("gtk-find");
    gtk_box_pack_start(GTK_BOX(hbox), intf_w->exp_file_button, FALSE, TRUE, 0);
    orage_table_add_row(intf_w->exp_table, label, hbox, 0
            , (GTK_FILL) , (GTK_FILL));
    g_signal_connect((gpointer)intf_w->exp_file_button, "clicked"
            , G_CALLBACK(on_exp_file_button_clicked_cb), intf_w);
    file = g_build_filename(g_get_home_dir(), "orage_export.ics", NULL);
    gtk_entry_set_text(GTK_ENTRY(intf_w->exp_file_entry), file);
    g_free(file);

    label = gtk_label_new(_("Select"));
    intf_w->exp_add_all_rb = gtk_radio_button_new_with_label(NULL
            , _("All appointments"));
    orage_table_add_row(intf_w->exp_table, label, intf_w->exp_add_all_rb, 1
            , (GTK_FILL), (GTK_FILL));
    g_signal_connect((gpointer)intf_w->exp_add_all_rb, "clicked"
            , G_CALLBACK(exp_add_all_rb_clicked), intf_w);

    hbox = gtk_hbox_new(FALSE, 0);
    intf_w->exp_add_id_rb = gtk_radio_button_new_with_mnemonic_from_widget(
            GTK_RADIO_BUTTON(intf_w->exp_add_all_rb)
            , _("Named appointments: "));
    gtk_box_pack_start(GTK_BOX(hbox), intf_w->exp_add_id_rb, TRUE, TRUE, 0);
    intf_w->exp_id_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), intf_w->exp_id_entry, TRUE, TRUE, 0);
    gtk_widget_set_sensitive(intf_w->exp_id_entry, FALSE);
    orage_table_add_row(intf_w->exp_table, NULL, hbox, 2
            , (GTK_FILL), (GTK_FILL));
    g_signal_connect((gpointer)intf_w->exp_add_id_rb, "clicked"
            , G_CALLBACK(exp_add_id_rb_clicked), intf_w);
    gtk_tooltips_set_tip(intf_w->tooltips, intf_w->exp_id_entry
            , _("Orage appointment UIDs separated by commas."), NULL);
    gtk_tooltips_set_tip(intf_w->tooltips, intf_w->exp_add_id_rb
            , _("You can easily drag these from event-list window."), NULL);
}

void create_import_tab(intf_win *intf_w)
{
    GtkWidget *hbox, *label;

    intf_w->imp_table = orage_table_new(1, 20);
    intf_w->imp_notebook_page = intf_w->imp_table;
    intf_w->imp_tab_label = gtk_label_new(_("Import"));
    gtk_notebook_append_page(GTK_NOTEBOOK(intf_w->notebook)
            , intf_w->imp_notebook_page, intf_w->imp_tab_label);

    label = gtk_label_new(_("Read file:"));
    hbox = gtk_hbox_new(FALSE, 0);
    intf_w->imp_file_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), intf_w->imp_file_entry, TRUE, TRUE, 0);
    intf_w->imp_file_button = gtk_button_new_from_stock("gtk-find");
    gtk_box_pack_start(GTK_BOX(hbox), intf_w->imp_file_button, FALSE, TRUE, 0);
    orage_table_add_row(intf_w->imp_table, label, hbox, 0
            , (GTK_FILL), (GTK_FILL));

    g_signal_connect((gpointer)intf_w->imp_file_button, "clicked"
            , G_CALLBACK(on_imp_file_button_clicked_cb), intf_w);
    gtk_tooltips_set_tip(intf_w->tooltips, intf_w->imp_file_entry
            , _("Separate filenames with comma(,).\n NOTE: comma is not valid character in filenames for Orage."), NULL);
}

void create_archive_tab(intf_win *intf_w)
{
    GtkWidget *label;
    gchar *file;
    char *str;

    intf_w->arc_table = orage_table_new(3, 20);
    intf_w->arc_notebook_page = intf_w->arc_table;
    intf_w->arc_tab_label = gtk_label_new(_("Archive"));
    gtk_notebook_append_page(GTK_NOTEBOOK(intf_w->notebook)
            , intf_w->arc_notebook_page, intf_w->arc_tab_label);

    str = g_strdup_printf(_("Archive now (threshold: %d months)")
            , g_par.archive_limit);
    label = gtk_label_new(str);
    intf_w->arc_button1 = gtk_button_new_from_stock("gtk-execute");
    orage_table_add_row(intf_w->arc_table, label, intf_w->arc_button1, 0
            , (GTK_FILL) , (GTK_FILL));
    gtk_tooltips_set_tip(intf_w->tooltips, intf_w->arc_button1
            , _("You can change archive threshold in parameters"), NULL);
    g_signal_connect((gpointer)intf_w->arc_button1, "clicked"
            , G_CALLBACK(on_archive_button_clicked_cb), intf_w);

    label = gtk_label_new(_("Revert archive now:"));
    intf_w->arc_button2 = gtk_button_new_from_stock("gtk-execute");
    orage_table_add_row(intf_w->arc_table, label, intf_w->arc_button2, 1
            , (GTK_FILL) , (GTK_FILL));
    gtk_tooltips_set_tip(intf_w->tooltips, intf_w->arc_button2
            , _("Return all archived events into main orage file and remove arch file\nThis is usefull for example when doing export and moving orage\nappointments to another system."), NULL);
    g_signal_connect((gpointer)intf_w->arc_button2, "clicked"
            , G_CALLBACK(on_unarchive_button_clicked_cb), intf_w);
}

void orage_external_interface(CalWin *xfcal)
{
    intf_win *intf_w = g_new(intf_win, 1);

     /* main window creation and base elements */
    intf_w->main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(intf_w->main_window)
            , _("Exchange data - Orage"));
    gtk_window_set_default_size(GTK_WINDOW(intf_w->main_window), 300, 200);

    intf_w->tooltips = gtk_tooltips_new();
    intf_w->accelgroup = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(intf_w->main_window)
            , intf_w->accelgroup);

    intf_w->main_vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(intf_w->main_window), intf_w->main_vbox);

    create_menu(intf_w);
    create_toolbar(intf_w);

     /* create tabs */
    intf_w->notebook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(intf_w->main_vbox), intf_w->notebook);
    gtk_container_set_border_width(GTK_CONTAINER(intf_w->notebook), 5);

    create_import_tab(intf_w);
    create_export_tab(intf_w);
    create_archive_tab(intf_w);

    gtk_widget_show_all(intf_w->main_window);
    drag_and_drop_init(intf_w);
}
