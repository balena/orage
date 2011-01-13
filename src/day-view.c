/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2007-2011 Juha Kautto  (juha at xfce.org)
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
#include <time.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include "orage-i18n.h"
#include "functions.h"
#include "day-view.h"
#include "ical-code.h"
#include "parameters.h"
#include "event-list.h"
#include "appointment.h"

static void do_appt_win(char *mode, char *uid, day_win *dw)
{
    appt_win *apptw;

    apptw = create_appt_win(mode, uid);
    if (apptw) {
        /* we started this, so keep track of it */
        dw->apptw_list = g_list_prepend(dw->apptw_list, apptw);
        /* inform the appointment that we are interested in it */
        apptw->dw = dw;
    }
};

static void set_scroll_position(day_win *dw)
{
    GtkAdjustment *v_adj;

    v_adj = gtk_scrolled_window_get_vadjustment(
            GTK_SCROLLED_WINDOW(dw->scroll_win));
    if (dw->scroll_pos > 0) /* we have old value */
        gtk_adjustment_set_value(v_adj, dw->scroll_pos);
    else if (dw->scroll_pos < 0)
        /* default: let's try to start roughly from line 8 = 8 o'clock */
        gtk_adjustment_set_value(v_adj, v_adj->upper/3);
}

static gboolean scroll_position_timer(gpointer user_data)
{
    set_scroll_position((day_win *)user_data);
    return(FALSE); /* only once */
}

static void get_scroll_position(day_win *dw)
{
    GtkAdjustment *v_adj;

    v_adj = gtk_scrolled_window_get_vadjustment(
            GTK_SCROLLED_WINDOW(dw->scroll_win));
    dw->scroll_pos = gtk_adjustment_get_value(v_adj);
}

static GtkWidget *build_line(day_win *dw, gint left_x, gint top_y
        , gint width, gint height, GtkWidget *hour_line)
{
    GdkColormap *pic1_cmap;
    GdkVisual *pic1_vis;
    GdkPixmap *pic1;
    GdkGC *pic1_gc;
    GtkWidget *new_hour_line;
    gint depth = 16;
    gboolean first = FALSE;

    /*
     * GdkPixbuf *scaled;
    scaled = gdk_pixbuf_scale_simple (pix, w, h, GDK_INTERP_BILINEAR);
    */
     
    pic1_cmap = gdk_colormap_get_system();
    pic1_vis = gdk_colormap_get_visual(pic1_cmap);
    depth = pic1_vis->depth;
    if (hour_line == NULL) {
        pic1 = gdk_pixmap_new(NULL, width, height, depth);
        gdk_drawable_set_colormap(pic1, pic1_cmap);
        first = TRUE;
    }
    else
        gtk_image_get_pixmap(GTK_IMAGE(hour_line), &pic1, NULL);
    pic1_gc = gdk_gc_new(pic1);
    if (first) {
        gdk_gc_set_foreground(pic1_gc, &dw->line_color);
        gdk_draw_rectangle(pic1, pic1_gc, TRUE, left_x, top_y, width, height);
    }
    else {
        gdk_draw_rectangle(pic1, pic1_gc, TRUE, left_x, top_y, width, height);
    }
    
    new_hour_line = gtk_image_new_from_pixmap(pic1, NULL);
    g_object_unref(pic1_gc);
    g_object_unref(pic1);
    return(new_hour_line);
}

static void close_window(day_win *dw)
{
    appt_win *apptw;
    GList *apptw_list;

    /* need to clean the appointment list and inform all appointments that
     * we are not interested anymore (= should not get updated) */
    apptw_list = dw->apptw_list;
    for (apptw_list = g_list_first(apptw_list);
         apptw_list != NULL;
         apptw_list = g_list_next(apptw_list)) {
        apptw = (appt_win *)apptw_list->data;
        if (apptw) /* appointment window is still alive */
            apptw->dw = NULL; /* not interested anymore */
        else
            orage_message(110, "close_window: not null appt window");
    }
    g_list_free(dw->apptw_list);

    gtk_widget_destroy(dw->Window);
    gtk_object_destroy(GTK_OBJECT(dw->Tooltips));
    g_free(dw);
    dw = NULL;
}

static gboolean on_Window_delete_event(GtkWidget *w, GdkEvent *e
        , gpointer user_data)
{
    close_window((day_win *)user_data);
    return(FALSE);
}

static void on_File_close_activate_cb(GtkMenuItem *mi, gpointer user_data)
{
    close_window((day_win *)user_data);
}

static void on_Close_clicked(GtkButton *b, gpointer user_data)
{
    close_window((day_win *)user_data);
}

static void create_new_appointment(day_win *dw)
{
    char *s_date, a_day[10];

    s_date = (char *)gtk_button_get_label(GTK_BUTTON(dw->StartDate_button));
    strcpy(a_day, orage_i18_date_to_icaldate(s_date));

    do_appt_win("NEW", a_day, dw);
}

static void on_File_newApp_activate_cb(GtkMenuItem *mi, gpointer user_data)
{
    create_new_appointment((day_win *)user_data);
}

static void on_Create_toolbutton_clicked_cb(GtkButton *mi, gpointer user_data)
{
    create_new_appointment((day_win *)user_data);
}

static void on_View_refresh_activate_cb(GtkMenuItem *mi, gpointer user_data)
{
    refresh_day_win((day_win *)user_data);
}

static void on_Refresh_clicked(GtkButton *b, gpointer user_data)
{
    refresh_day_win((day_win *)user_data);
}

static void changeSelectedDate(day_win *dw, gint day)
{
    struct tm tm_date;

    tm_date = orage_i18_date_to_tm_date(
            gtk_button_get_label(GTK_BUTTON(dw->StartDate_button)));
    orage_move_day(&tm_date, day);
    gtk_button_set_label(GTK_BUTTON(dw->StartDate_button)
            , orage_tm_date_to_i18_date(&tm_date));
    refresh_day_win(dw);
}

static void go_to_today(day_win *dw)
{
    gtk_button_set_label(GTK_BUTTON(dw->StartDate_button)
            , orage_localdate_i18());
    refresh_day_win(dw);
}

static void on_Today_clicked(GtkButton *b, gpointer user_data)
{
    go_to_today((day_win *)user_data);
}

static void on_Go_today_activate_cb(GtkMenuItem *mi, gpointer user_data)
{
    go_to_today((day_win *)user_data);
}

static void on_Go_previous_activate_cb(GtkMenuItem *mi, gpointer user_data)
{
    changeSelectedDate((day_win *)user_data, -1);
}

static void on_Previous_clicked(GtkButton *b, gpointer user_data)
{
    changeSelectedDate((day_win *)user_data, -1);
}

static void on_Go_next_activate_cb(GtkMenuItem *mi, gpointer user_data)
{
    changeSelectedDate((day_win *)user_data, 1);
}

static void on_Next_clicked(GtkButton *b, gpointer user_data)
{
    changeSelectedDate((day_win *)user_data, 1);
}

static void build_menu(day_win *dw)
{
    GtkWidget *menu_separator;

    dw->Menubar = gtk_menu_bar_new();
    gtk_box_pack_start(GTK_BOX(dw->Vbox), dw->Menubar, FALSE, FALSE, 0);

    /********** File menu **********/
    dw->File_menu = orage_menu_new(_("_File"), dw->Menubar);
    dw->File_menu_new = orage_image_menu_item_new_from_stock("gtk-new"
            , dw->File_menu, dw->accel_group);

    menu_separator = orage_separator_menu_item_new(dw->File_menu);

    dw->File_menu_close = orage_image_menu_item_new_from_stock("gtk-close"
        , dw->File_menu, dw->accel_group);

    /********** View menu **********/
    dw->View_menu = orage_menu_new(_("_View"), dw->Menubar);
    dw->View_menu_refresh = orage_image_menu_item_new_from_stock ("gtk-refresh"
            , dw->View_menu, dw->accel_group);
    gtk_widget_add_accelerator(dw->View_menu_refresh
            , "activate", dw->accel_group
            , GDK_r, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(dw->View_menu_refresh
            , "activate", dw->accel_group
            , GDK_Return, 0, 0);
    gtk_widget_add_accelerator(dw->View_menu_refresh
            , "activate", dw->accel_group
            , GDK_KP_Enter, 0, 0);

    /********** Go menu   **********/
    dw->Go_menu = orage_menu_new(_("_Go"), dw->Menubar);
    dw->Go_menu_today = orage_image_menu_item_new_from_stock("gtk-home"
            , dw->Go_menu, dw->accel_group);
    gtk_widget_add_accelerator(dw->Go_menu_today
            , "activate", dw->accel_group
            , GDK_Home, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
    dw->Go_menu_prev = orage_image_menu_item_new_from_stock("gtk-go-back"
            , dw->Go_menu, dw->accel_group);
    gtk_widget_add_accelerator(dw->Go_menu_prev
            , "activate", dw->accel_group
            , GDK_Left, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
    dw->Go_menu_next = orage_image_menu_item_new_from_stock("gtk-go-forward"
            , dw->Go_menu, dw->accel_group);
    gtk_widget_add_accelerator(dw->Go_menu_next
            , "activate", dw->accel_group
            , GDK_Right, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);

    g_signal_connect((gpointer)dw->File_menu_new, "activate"
            , G_CALLBACK(on_File_newApp_activate_cb), dw);
    g_signal_connect((gpointer)dw->File_menu_close, "activate"
            , G_CALLBACK(on_File_close_activate_cb), dw);
    g_signal_connect((gpointer)dw->View_menu_refresh, "activate"
            , G_CALLBACK(on_View_refresh_activate_cb), dw);
    g_signal_connect((gpointer)dw->Go_menu_today, "activate"
            , G_CALLBACK(on_Go_today_activate_cb), dw);
    g_signal_connect((gpointer)dw->Go_menu_prev, "activate"
            , G_CALLBACK(on_Go_previous_activate_cb), dw);
    g_signal_connect((gpointer)dw->Go_menu_next, "activate"
            , G_CALLBACK(on_Go_next_activate_cb), dw);
}

static void build_toolbar(day_win *dw)
{
    GtkWidget *toolbar_separator;
    int i = 0;

    dw->Toolbar = gtk_toolbar_new();
    gtk_box_pack_start(GTK_BOX(dw->Vbox), dw->Toolbar, FALSE, FALSE, 0);
    gtk_toolbar_set_tooltips(GTK_TOOLBAR(dw->Toolbar), TRUE);

    dw->Create_toolbutton = orage_toolbar_append_button(dw->Toolbar
            , "gtk-new", dw->Tooltips, _("New"), i++);

    toolbar_separator = orage_toolbar_append_separator(dw->Toolbar, i++);

    dw->Previous_toolbutton = orage_toolbar_append_button(dw->Toolbar
            , "gtk-go-back", dw->Tooltips, _("Back"), i++);
    dw->Today_toolbutton = orage_toolbar_append_button(dw->Toolbar
            , "gtk-home", dw->Tooltips, _("Today"), i++);
    dw->Next_toolbutton = orage_toolbar_append_button(dw->Toolbar
            , "gtk-go-forward", dw->Tooltips, _("Forward"), i++);

    toolbar_separator = orage_toolbar_append_separator(dw->Toolbar, i++);

    dw->Refresh_toolbutton = orage_toolbar_append_button(dw->Toolbar
            , "gtk-refresh", dw->Tooltips, _("Refresh"), i++);

    toolbar_separator = orage_toolbar_append_separator(dw->Toolbar, i++);

    dw->Close_toolbutton = orage_toolbar_append_button(dw->Toolbar
            , "gtk-close", dw->Tooltips, _("Close"), i++);

    g_signal_connect((gpointer)dw->Create_toolbutton, "clicked"
            , G_CALLBACK(on_Create_toolbutton_clicked_cb), dw);
    g_signal_connect((gpointer)dw->Previous_toolbutton, "clicked"
            , G_CALLBACK(on_Previous_clicked), dw);
    g_signal_connect((gpointer)dw->Today_toolbutton, "clicked"
            , G_CALLBACK(on_Today_clicked), dw);
    g_signal_connect((gpointer)dw->Next_toolbutton, "clicked"
            , G_CALLBACK(on_Next_clicked), dw);
    g_signal_connect((gpointer)dw->Refresh_toolbutton, "clicked"
            , G_CALLBACK(on_Refresh_clicked), dw);
    g_signal_connect((gpointer)dw->Close_toolbutton, "clicked"
             , G_CALLBACK(on_Close_clicked), dw);
}

static gboolean upd_day_view(day_win *dw)
{
    static guint day_cnt=-1;
    guint day_cnt_n;

    /* we only need to do this if it is really a new day count. We may get
     * many of these while spin button is changing day count and it is enough
     * to show only the last one, which is visible */
    day_cnt_n = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dw->day_spin));
    if (day_cnt != day_cnt_n) { /* need really do it */
        refresh_day_win(dw);
        day_cnt = day_cnt_n;
    }
    dw->upd_timer = 0;
    return(FALSE); /* we do this only once */
}

static void on_spin_changed(GtkSpinButton *b, gpointer *user_data)
{
    day_win *dw = (day_win *)user_data;

    /* refresh_day_win is rather heavy (=slow), so doing it here 
     * is not a good idea. We can't keep up with repeated quick presses 
     * if we do the whole thing here. So let's throw it to background 
     * and do it later. */
    if (dw->upd_timer) {
        g_source_remove(dw->upd_timer);       
    }
    dw->upd_timer = g_timeout_add(500, (GtkFunction)upd_day_view, dw);
}

static void on_Date_button_clicked_cb(GtkWidget *button, gpointer *user_data)
{
    day_win *dw = (day_win *)user_data;

    if (orage_date_button_clicked(button, dw->Window))
        refresh_day_win(dw);
}

static void header_button_clicked_cb(GtkWidget *button, gpointer *user_data)
{
    gchar *start_date;

    start_date = (char *)gtk_button_get_label(GTK_BUTTON(button));
    create_el_win(start_date);
}

static void on_button_press_event_cb(GtkWidget *widget
        , GdkEventButton *event, gpointer *user_data)
{
    day_win *dw = (day_win *)user_data;
    gchar *uid;

    if (event->type == GDK_2BUTTON_PRESS) {
        uid = g_object_get_data(G_OBJECT(widget), "UID");
        do_appt_win("UPDATE", uid, dw);
    }
}

static void on_arrow_left_press_event_cb(GtkWidget *widget
        , GdkEventButton *event, gpointer *user_data)
{
    changeSelectedDate((day_win *)user_data, -1);
}

static void on_arrow_right_press_event_cb(GtkWidget *widget
        , GdkEventButton *event, gpointer *user_data)
{
    changeSelectedDate((day_win *)user_data, 1);
}

static void add_row(day_win *dw, xfical_appt *appt)
{
    gint row, start_row, end_row;
    gint col, start_col, end_col, first_col, last_col;
    gint height, start_height, end_height;
    gchar *tip, *start_date, *end_date, *tmp_title, *tmp_note;
    GtkWidget *ev, *lab, *hb;
    struct tm tm_start, tm_end, tm_first;
    GdkColor *color;

    /* First clarify timings */
    tm_start = orage_icaltime_to_tm_time(appt->starttimecur, FALSE);
    tm_end   = orage_icaltime_to_tm_time(appt->endtimecur, FALSE);
    tm_first = orage_icaltime_to_tm_time(dw->a_day, FALSE);
    start_col = orage_days_between(&tm_first, &tm_start)+1;
    end_col   = orage_days_between(&tm_first, &tm_end)+1;

    if (start_col < 1) {
        col = 1;
        row = 0;
    }
    else {
        col = start_col;
        row = tm_start.tm_hour;
    }

    /* then add the appointment */
    tmp_title = orage_process_text_commands(
            appt->title ? appt->title : _("Unknown"));
    tmp_note = orage_process_text_commands(appt->note);
    ev = gtk_event_box_new();
    lab = gtk_label_new(tmp_title);
    gtk_container_add(GTK_CONTAINER(ev), lab);

    if (appt->starttimecur[8] != 'T') { /* whole day event */
        gtk_widget_modify_bg(ev, GTK_STATE_NORMAL, &dw->bg2);
        if (dw->header[col] == NULL) { /* first data */
            hb = gtk_hbox_new(TRUE, 3);
            dw->header[col] = hb;
        }
        else {
            hb = dw->header[col];
            /* FIXME: set some real bar here to make it visible that we
             * have more than 1 appointment here
             */
        }
        tip = g_strdup_printf("%s\n%s - %s\n%s"
                , tmp_title, appt->starttimecur, appt->endtimecur, tmp_note);
    }
    else {
        if ((color = orage_category_list_contains(appt->categories)) != NULL)
            gtk_widget_modify_bg(ev, GTK_STATE_NORMAL, color);
        else if ((row % 2) == 1)
            gtk_widget_modify_bg(ev, GTK_STATE_NORMAL, &dw->bg1);
        if (dw->element[row][col] == NULL) {
            hb = gtk_hbox_new(TRUE, 3);
            dw->element[row][col] = hb;
        }
        else {
            hb = dw->element[row][col];
            /* FIXME: set some real bar here to make it visible that we
             * have more than 1 appointment here
             */
        }
        if (orage_days_between(&tm_start, &tm_end) == 0)
            tip = g_strdup_printf("%s\n%02d:%02d-%02d:%02d\n%s"
                    , tmp_title, tm_start.tm_hour, tm_start.tm_min
                    , tm_end.tm_hour, tm_end.tm_min, tmp_note);
        else {
    /* we took the date in unnormalized format, so we need to do that now */
            tm_start.tm_year -= 1900;
            tm_start.tm_mon -= 1;
            tm_end.tm_year -= 1900;
            tm_end.tm_mon -= 1;
            start_date = g_strdup(orage_tm_date_to_i18_date(&tm_start));
            end_date = g_strdup(orage_tm_date_to_i18_date(&tm_end));
            tip = g_strdup_printf("%s\n%s %02d:%02d - %s %02d:%02d\n%s"
                    , tmp_title
                    , start_date, tm_start.tm_hour, tm_start.tm_min
                    , end_date, tm_end.tm_hour, tm_end.tm_min, tmp_note);
            g_free(start_date);
            g_free(end_date);
        }
    }
    gtk_tooltips_set_tip(dw->Tooltips, ev, tip, NULL);
    /*
    gtk_box_pack_start(GTK_BOX(hb2), ev, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hb), hb2, TRUE, TRUE, 0);
    */
    gtk_box_pack_start(GTK_BOX(hb), ev, TRUE, TRUE, 0);
    g_object_set_data_full(G_OBJECT(ev), "UID", g_strdup(appt->uid), g_free);
    g_signal_connect((gpointer)ev, "button-press-event"
            , G_CALLBACK(on_button_press_event_cb), dw);
    g_free(tip);
    g_free(tmp_title);
    g_free(tmp_note);

    /* and finally draw the line to show how long the appointment is,
     * but only if it is Busy type event (=availability != 0) 
     * and it is not whole day event */
    if (appt->availability && appt->starttimecur[8] == 'T') {
        height = dw->StartDate_button_req.height;
        /*
         * same_date = !strncmp(start_ical_time, end_ical_time, 8);
         * */
        if (start_col < 1)
            first_col = 1;
        else
            first_col = start_col;
        if (end_col > dw->days)
            last_col = dw->days;
        else
            last_col = end_col;
        for (col = first_col; col <= last_col; col++) {
            if (col == start_col)
                start_row = tm_start.tm_hour;
            else
                start_row = 0;
            if (col == end_col)
                end_row   = tm_end.tm_hour;
            else
                end_row   = 23;
            for (row = start_row; row <= end_row; row++) {
                if (row == tm_start.tm_hour && col == start_col)
                    start_height = tm_start.tm_min*height/60;
                else
                    start_height = 0;
                if (row == tm_end.tm_hour && col == end_col)
                    end_height = tm_end.tm_min*height/60;
                else
                    end_height = height;
                dw->line[row][col] = build_line(dw, 1, start_height
                        , 2, end_height-start_height, dw->line[row][col]);
            }
        }
    }
}

static void app_rows(day_win *dw, xfical_type ical_type, gchar *file_type)
{
    GList *appt_list=NULL, *tmp;
    xfical_appt *appt;

    xfical_get_each_app_within_time(dw->a_day, dw->days
            , ical_type, file_type, &appt_list);
    for (tmp = g_list_first(appt_list);
         tmp != NULL;
         tmp = g_list_next(tmp)) {
        appt = (xfical_appt *)tmp->data;
        if (appt->priority < g_par.priority_list_limit) { 
            add_row(dw, appt);
        }
        xfical_appt_free(appt);
    }
    g_list_free(appt_list);
}

static void app_data(day_win *dw)
{
    xfical_type ical_type;
    gchar *s_date, file_type[8];
    gint i;

    ical_type = XFICAL_TYPE_EVENT;
    s_date = (char *)gtk_button_get_label(GTK_BUTTON(dw->StartDate_button));
    strcpy(dw->a_day, orage_i18_date_to_icaldate(s_date));
    dw->days = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dw->day_spin));

    /* first search base orage file */
    if (!xfical_file_open(TRUE))
        return;
    strcpy(file_type, "O00.");
    app_rows(dw, ical_type, file_type);
    /* then process all foreign files */
    for (i = 0; i < g_par.foreign_count; i++) {
        g_sprintf(file_type, "F%02d.", i);
        app_rows(dw, ical_type, file_type);
    }
    xfical_file_close(TRUE);
}


static void fill_days(day_win *dw, gint days)
{
    gint row, col, height, width;
    GtkWidget *ev, *hb;
    GtkWidget *marker;

    height = dw->StartDate_button_req.height;
    width = dw->StartDate_button_req.width;

    /* first clear the structure */
    for (col = 1; col <  days+1; col++) {
        dw->header[col] = NULL;
        for (row = 0; row < 24; row++) {
            dw->element[row][col] = NULL;
    /* gdk_draw_rectangle(, , , left_x, top_y, width, height); */
            dw->line[row][col] = build_line(dw, 0, 0, 3, height, NULL);
        }
    }

    app_data(dw);

    for (col = 1; col < days+1; col++) {
        hb = gtk_hbox_new(FALSE, 0);
        marker = build_line(dw, 0, 0, 2, height, NULL);
        gtk_box_pack_start(GTK_BOX(hb), marker, FALSE, FALSE, 0);
        /* check if we have full day events and put them to header */
        if (dw->header[col]) {
            gtk_box_pack_start(GTK_BOX(hb), dw->header[col], TRUE, TRUE, 0);
            gtk_widget_set_size_request(hb, width, -1);
        }
        else {
            ev = gtk_event_box_new();
            gtk_widget_modify_bg(ev, GTK_STATE_NORMAL, &dw->bg2);
            gtk_box_pack_start(GTK_BOX(hb), ev, TRUE, TRUE, 0);
        }
        gtk_table_attach(GTK_TABLE(dw->dtable_h), hb, col, col+1, 1, 2
                 , (GTK_FILL), (0), 0, 0);

        /* check rows */
        for (row = 0; row < 24; row++) {
            hb = gtk_hbox_new(FALSE, 0);
            if (row == 0)
                gtk_widget_set_size_request(hb, width, -1);
            if (dw->element[row][col]) {
                gtk_box_pack_start(GTK_BOX(hb), dw->line[row][col]
                        , FALSE, FALSE, 0);
                gtk_box_pack_start(GTK_BOX(hb), dw->element[row][col]
                        , TRUE, TRUE, 0);
                gtk_widget_set_size_request(hb, width, -1);
            }
            else {
                ev = gtk_event_box_new();
                /*
                name = gtk_label_new(" ");
                gtk_container_add(GTK_CONTAINER(ev), name);
                */
                if ((row % 2) == 1)
                    gtk_widget_modify_bg(ev, GTK_STATE_NORMAL, &dw->bg1);
                gtk_box_pack_start(GTK_BOX(hb), dw->line[row][col]
                        , FALSE, FALSE, 0);
                gtk_box_pack_start(GTK_BOX(hb), ev, TRUE, TRUE, 0);
            }
            gtk_table_attach(GTK_TABLE(dw->dtable), hb, col, col+1, row, row+1
                     , (GTK_FILL), (0), 0, 0);
        }
    }
}

static void build_day_view_header(day_win *dw, char *start_date)
{
    GtkWidget *hbox, *label;

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(dw->Vbox), hbox, FALSE, FALSE, 10);

    label = gtk_label_new(_("Start"));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 10);

    /* start date button */
    dw->StartDate_button = gtk_button_new();
    gtk_box_pack_start(GTK_BOX(hbox), dw->StartDate_button, FALSE, FALSE, 0);

    label = gtk_label_new(_("       Number of days to show"));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 10);

    /* show days spin = how many days to show */
    dw->day_spin = gtk_spin_button_new_with_range(1, MAX_DAYS, 1);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(dw->day_spin), TRUE);
    gtk_box_pack_start(GTK_BOX(hbox), dw->day_spin, FALSE, FALSE, 0);

    /* initial values */
    gtk_button_set_label(GTK_BUTTON(dw->StartDate_button)
            , (const gchar *)start_date);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(dw->day_spin), 7);

    /* sizes */
    gtk_widget_size_request(dw->StartDate_button, &dw->StartDate_button_req);
    dw->StartDate_button_req.width += dw->StartDate_button_req.width/10;
    label = gtk_label_new("00");
    gtk_widget_size_request(label, &dw->hour_req);

    g_signal_connect((gpointer)dw->day_spin, "value-changed"
            , G_CALLBACK(on_spin_changed), dw);
    g_signal_connect((gpointer)dw->StartDate_button, "clicked"
            , G_CALLBACK(on_Date_button_clicked_cb), dw);
}

static void build_day_view_colours(day_win *dw)
{
    GtkStyle *def_style;
    GdkColormap *pic1_cmap;

    def_style = gtk_widget_get_default_style();
    pic1_cmap = gdk_colormap_get_system();
    dw->bg1 = def_style->bg[GTK_STATE_NORMAL];
    dw->bg1.red +=  (dw->bg1.red < 64000 ? 1000 : -1000);
    dw->bg1.green += (dw->bg1.green < 64000 ? 1000 : -1000);
    dw->bg1.blue += (dw->bg1.blue < 64000 ? 1000 : -1000);
    gdk_colormap_alloc_color(pic1_cmap, &dw->bg1, FALSE, TRUE);

    dw->bg2 = def_style->bg[GTK_STATE_NORMAL];
    dw->bg2.red +=  (dw->bg2.red > 1000 ? -1000 : 1000);
    dw->bg2.green += (dw->bg2.green > 1000 ? -1000 : 1000);
    dw->bg2.blue += (dw->bg2.blue > 2000 ? -2000 : 2000);
    gdk_colormap_alloc_color(pic1_cmap, &dw->bg2, FALSE, TRUE);

    if (!gdk_color_parse("white", &dw->line_color)) {
        dw->line_color.red =  239 * (65535/255);
        dw->line_color.green = 235 * (65535/255);
        dw->line_color.blue = 230 * (65535/255);
    }
    gdk_colormap_alloc_color(pic1_cmap, &dw->line_color, FALSE, TRUE);

    if (!gdk_color_parse("red", &dw->fg_sunday)) {
        g_warning("color parse failed: red\n");
        dw->fg_sunday.red = 255 * (65535/255);
        dw->fg_sunday.green = 10 * (65535/255);
        dw->fg_sunday.blue = 10 * (65535/255);
    }
    gdk_colormap_alloc_color(pic1_cmap, &dw->fg_sunday, FALSE, TRUE);

    if (!gdk_color_parse("gold", &dw->bg_today)) {
        g_warning("color parse failed: gold\n");
        dw->bg_today.red = 255 * (65535/255);
        dw->bg_today.green = 215 * (65535/255);
        dw->bg_today.blue = 115 * (65535/255);
    }
    gdk_colormap_alloc_color(pic1_cmap, &dw->bg_today, FALSE, TRUE);
}

static void fill_hour(day_win *dw, gint col, gint row, char *text)
{
    GtkWidget *name, *ev;

    ev = gtk_event_box_new();
    name = gtk_label_new(text);
    gtk_container_add(GTK_CONTAINER(ev), name);
    if ((row % 2) == 1)
        gtk_widget_modify_bg(ev, GTK_STATE_NORMAL, &dw->bg1);
    gtk_widget_set_size_request(ev, dw->hour_req.width
            , dw->StartDate_button_req.height);
    gtk_table_attach(GTK_TABLE(dw->dtable), ev, col, col+1, row, row+1
         , (GTK_FILL), (0), 0, 0);
}

static void fill_hour_arrow(day_win *dw, gint col)
{
    GtkWidget *arrow, *ev;

    ev = gtk_event_box_new();
    if (col == 0) {
        arrow = gtk_arrow_new(GTK_ARROW_LEFT, GTK_SHADOW_NONE);
        g_signal_connect((gpointer)ev, "button-press-event"
                , G_CALLBACK(on_arrow_left_press_event_cb), dw);
    }
    else {
        arrow = gtk_arrow_new(GTK_ARROW_RIGHT, GTK_SHADOW_NONE);
        g_signal_connect((gpointer)ev, "button-press-event"
                , G_CALLBACK(on_arrow_right_press_event_cb), dw);
    }
    gtk_container_add(GTK_CONTAINER(ev), arrow);
    gtk_widget_set_size_request(ev, dw->hour_req.width
            , dw->StartDate_button_req.height);
    gtk_table_attach(GTK_TABLE(dw->dtable_h), ev, col, col+1, 0, 1
            , (GTK_FILL), (0), 0, 0);
}

static void build_day_view_table(day_win *dw)
{
    gint days;   /* number of days to show */
    int year, month, day;
    gint i, sunday;
    GtkWidget *label, *button;
    char text[5+1], *date, *today;
    struct tm tm_date;
    gint monthdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    GtkWidget *vp;
    
    orage_category_get_list();
    days = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dw->day_spin));
    tm_date = orage_i18_date_to_tm_date(
            gtk_button_get_label(GTK_BUTTON(dw->StartDate_button)));

    /****** header of day table = days columns ******/
    dw->scroll_win_h = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(dw->scroll_win_h)
            , GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
    gtk_box_pack_start(GTK_BOX(dw->Vbox), dw->scroll_win_h
            , TRUE, TRUE, 0);
    dw->day_view_vbox = gtk_vbox_new(FALSE, 0);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(dw->scroll_win_h)
            , dw->day_view_vbox);
    /*
    gtk_container_add(GTK_CONTAINER(dw->scroll_win_h), dw->day_view_vbox);
    */
    /* row 1= day header buttons 
     * row 2= full day events after the buttons */
    dw->dtable_h = gtk_table_new(2, days+2, FALSE);
    gtk_box_pack_start(GTK_BOX(dw->day_view_vbox), dw->dtable_h
            , FALSE, FALSE, 0);

    sunday = tm_date.tm_wday; /* 0 = Sunday */
    if (sunday) /* index to next sunday */
        sunday = 7-sunday;
    year = tm_date.tm_year + 1900;
    month = tm_date.tm_mon;
    day = tm_date.tm_mday;
    if (((tm_date.tm_year%4) == 0) && (((tm_date.tm_year%100) != 0) 
            || ((tm_date.tm_year%400) == 0)))
        ++monthdays[1];
    today = g_strdup(orage_localdate_i18());

    fill_hour_arrow(dw, 0);
    for (i = 1; i <  days+1; i++) {
        date = orage_tm_date_to_i18_date(&tm_date);
        button = gtk_button_new();
        gtk_button_set_label(GTK_BUTTON(button), date);
        if (strcmp(today, date) == 0) {
            gtk_widget_modify_bg(button, GTK_STATE_NORMAL, &dw->bg_today);
        }
        if ((i-1)%7 == sunday) {
            label = gtk_bin_get_child(GTK_BIN(button));
            gtk_widget_modify_fg(label, GTK_STATE_NORMAL, &dw->fg_sunday);
        }
        gtk_widget_set_size_request(button, dw->StartDate_button_req.width, -1);
        g_signal_connect((gpointer)button, "clicked"
                , G_CALLBACK(header_button_clicked_cb), dw);
        gtk_table_attach(GTK_TABLE(dw->dtable_h), button, i, i+1, 0, 1
                , (GTK_FILL), (0), 0, 0);

        if (++tm_date.tm_mday == (monthdays[tm_date.tm_mon]+1)) {
            if (++tm_date.tm_mon == 12) {
                ++tm_date.tm_year;
                tm_date.tm_mon = 0;
            }
            tm_date.tm_mday = 1;
        }
        /* some rare locales show weekday in the default date, so we need to 
         * make it correct. Safer would be to call mktime() */
        tm_date.tm_wday = (tm_date.tm_wday+1)%7;
    }
    fill_hour_arrow(dw, days+1);
    g_free(today);

    /****** body of day table ******/
    dw->scroll_win = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(dw->scroll_win)
            , GTK_SHADOW_NONE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(dw->scroll_win)
            , GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
    gtk_scrolled_window_set_placement(GTK_SCROLLED_WINDOW(dw->scroll_win)
            , GTK_CORNER_TOP_LEFT);
    gtk_box_pack_start(GTK_BOX(dw->day_view_vbox), dw->scroll_win
            , TRUE, TRUE, 0);
    vp = gtk_viewport_new(NULL, NULL);
    gtk_viewport_set_shadow_type(GTK_VIEWPORT(vp), GTK_SHADOW_NONE);
    gtk_container_add(GTK_CONTAINER(dw->scroll_win), vp);
    dw->dtable = gtk_table_new(24, days+2, FALSE);
    gtk_container_add(GTK_CONTAINER(vp), dw->dtable);

    /* hours column = hour rows */
    for (i = 0; i < 24; i++) {
        g_sprintf(text, "%02d", i);
        /* ev is needed for background colour */
        fill_hour(dw, 0, i, text);
        fill_hour(dw, days+1, i, text);
    }
    fill_days(dw, days);
}

void refresh_day_win(day_win *dw)
{
    get_scroll_position(dw);
    gtk_widget_destroy(dw->scroll_win_h);
    build_day_view_table(dw);
    gtk_widget_show_all(dw->scroll_win_h);
    /* I was not able to get this work without the timer. Ugly yes, but
     * it works and does not hurt - much */
    g_timeout_add(100, (GtkFunction)scroll_position_timer, (gpointer)dw);
}

day_win *create_day_win(char *start_date)
{
    day_win *dw;

    /* initialisation + main window + base vbox */
    dw = g_new0(day_win, 1);
    dw->scroll_pos = -1; /* not set */
    dw->Tooltips = gtk_tooltips_new();
    dw->accel_group = gtk_accel_group_new();

    dw->Window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(dw->Window), 690, 390);
    gtk_window_set_title(GTK_WINDOW(dw->Window), _("Orage - day view"));
    gtk_window_add_accel_group(GTK_WINDOW(dw->Window), dw->accel_group);

    dw->Vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(dw->Window), dw->Vbox);
    g_signal_connect((gpointer)dw->Window, "delete_event"
            , G_CALLBACK(on_Window_delete_event), dw);

    build_menu(dw);
    build_toolbar(dw);
    build_day_view_colours(dw);
    build_day_view_header(dw, start_date);
    build_day_view_table(dw);
    gtk_widget_show_all(dw->Window);
    set_scroll_position(dw);

    return(dw);
}
