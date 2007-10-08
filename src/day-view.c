/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2007      Juha Kautto  (juha at xfce.org)
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
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include "orage-i18n.h"
#include "functions.h"
#include "day-view.h"
#include "ical-code.h"
#include "parameters.h"

static void refresh_day_view_table(day_win *dw);

static GtkWidget *build_line(gint left_x, gint top_y, gint width, gint height
        , GtkWidget *hour_line)
{
    GdkPixmap *pic1;
    GdkGC *pic1_gc1, *pic1_gc2;
    GdkColormap *pic1_cmap;
    GdkColor color;
    GdkVisual *pic1_vis;
    GtkWidget *new_hour_line;
    gint depth = 16;
    gint red = 239, green = 235, blue = 230;
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
    pic1_gc1 = gdk_gc_new(pic1);
    pic1_gc2 = gdk_gc_new(pic1);
    color.red = red * (65535/255);
    color.green = green * (65535/255);
    color.blue = blue * (65535/255);
    color.pixel = (gulong)(red*65536 + green*256 + blue);
    gdk_colormap_alloc_color(pic1_cmap, &color, FALSE, TRUE);
    gdk_gc_set_foreground(pic1_gc1, &color);
    /* gdk_draw_rectangle(, , , left_x, top_y, width, height); */
    if (first)
        gdk_draw_rectangle(pic1, pic1_gc1, TRUE, left_x, top_y, width, height);
    else
        gdk_draw_rectangle(pic1, pic1_gc2, TRUE, left_x, top_y, width, height);
    
    new_hour_line = gtk_image_new_from_pixmap(pic1, NULL);
    g_object_unref(pic1_gc1);
    g_object_unref(pic1_gc2);
    g_object_unref(pic1);
    return(new_hour_line);
}

static void close_window(day_win *dw)
{
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

static void build_menu(day_win *dw)
{
    dw->Menubar = gtk_menu_bar_new();
    gtk_box_pack_start(GTK_BOX(dw->Vbox), dw->Menubar, FALSE, FALSE, 0);

    /* File menu */
    dw->File_menu = orage_menu_new(_("_File"), dw->Menubar);
    dw->File_menu_close = orage_image_menu_item_new_from_stock("gtk-close"
        , dw->File_menu, dw->accel_group);

    g_signal_connect((gpointer)dw->File_menu_close, "activate"
        , G_CALLBACK(on_File_close_activate_cb), dw);
}

static void on_Close_clicked(GtkButton *b, gpointer user_data)
{
    close_window((day_win *)user_data);
}

static void build_toolbar(day_win *dw)
{
    int i = 0;

    dw->Toolbar = gtk_toolbar_new();
    gtk_box_pack_start(GTK_BOX(dw->Vbox), dw->Toolbar, FALSE, FALSE, 0);
    gtk_toolbar_set_tooltips(GTK_TOOLBAR(dw->Toolbar), TRUE);

    dw->Close_toolbutton = orage_toolbar_append_button(dw->Toolbar
             , "gtk-close", dw->Tooltips, _("Close"), i++);

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
        refresh_day_view_table(dw);
        day_cnt = day_cnt_n;
    }
    return(FALSE); /* we do this only once */
}

static void on_spin_changed(GtkSpinButton *b, gpointer *user_data)
{
    day_win *dw = (day_win *)user_data;

    /* refresh_day_view_table is rather heavy (=slow), so doing it here 
     * is not a good idea. We can't keep up with repeated quick presses 
     * if we do the whole thing here. So let's throw it to background 
     * and do it later. */
    g_timeout_add(500, (GtkFunction)upd_day_view, dw);
}

static void on_Date_button_clicked_cb(GtkWidget *button, gpointer *user_data)
{
    day_win *dw = (day_win *)user_data;

    if (orage_date_button_clicked(button, dw->Window))
        refresh_day_view_table(dw);
}

static void header_button_clicked_cb(GtkWidget *button, gpointer *user_data)
{
    day_win *dw = (day_win *)user_data;
    gchar *start_date;

    start_date = (char *)gtk_button_get_label(GTK_BUTTON(button));
    create_el_win(start_date);
}

static void on_button_press_event_cb(GtkWidget *widget
        , GdkEventButton *event, gpointer *user_data)
{
    day_win *dw = (day_win *)user_data;
    gchar *uid;

    g_print("pressed button %d\n", event->button);
    uid = g_object_get_data(G_OBJECT(widget), "UID");
    create_appt_win("UPDATE", uid, NULL);
}

static void event_button_clicked_cb(GtkWidget *button, gpointer *user_data)
{
    day_win *dw = (day_win *)user_data;
    gchar *uid;

    uid = g_object_get_data(G_OBJECT(button), "UID");
    create_appt_win("UPDATE", uid, NULL);
}

static void add_row(day_win *dw, xfical_appt *appt, char *a_day, gint days)
{
    gint row, start_row, end_row;
    gint col, start_col, end_col, first_col, last_col;
    gint height, start_height, end_height;
    gchar *text, *tip, *uid, *start_date, *end_date;
    GtkWidget *ev, *lab, *hb;
    struct tm tm_start, tm_end, tm_first;
    GDate *g_start, *g_end, *g_first;

    tm_start = orage_icaltime_to_tm_time(appt->starttimecur, FALSE);
    tm_end   = orage_icaltime_to_tm_time(appt->endtimecur, FALSE);
    tm_first = orage_icaltime_to_tm_time(a_day, FALSE);

    g_start = g_date_new_dmy(tm_start.tm_mday, tm_start.tm_mon
            , tm_start.tm_year);
    g_end   = g_date_new_dmy(tm_end.tm_mday, tm_end.tm_mon
            , tm_end.tm_year);
    g_first = g_date_new_dmy(tm_first.tm_mday, tm_first.tm_mon
            , tm_first.tm_year);
    col = g_date_days_between(g_first, g_start)+1;  /* col 0 == hour headers */
    if (col < 1) {
        col = 1;
        row = 0;
    }
    else 
        row = tm_start.tm_hour;

    text = g_strdup(appt->title ? appt->title : _("Unknown"));
    ev = gtk_event_box_new();
    lab = gtk_label_new(text);
    gtk_container_add(GTK_CONTAINER(ev), lab);

    if (appt->starttimecur[8] != 'T') {
        gtk_widget_modify_bg(ev, GTK_STATE_NORMAL, &dw->bg2);
        if (dw->header[col] == NULL)
            hb = gtk_hbox_new(TRUE, 0);
        else
            hb = dw->header[col];
        tip = g_strdup_printf("%s\n%s - %s\n%s"
                , appt->title
                , appt->starttimecur
                , appt->endtimecur
                , appt->note);
        gtk_tooltips_set_tip(dw->Tooltips, ev, tip, NULL);
        gtk_box_pack_start(GTK_BOX(hb), ev, TRUE, TRUE, 0);
        g_object_set_data_full(G_OBJECT(ev), "UID", g_strdup(appt->uid)
                , g_free);
        g_signal_connect((gpointer)ev, "button-press-event"
                , G_CALLBACK(on_button_press_event_cb), dw);
        dw->header[col] = hb;
        g_free(tip);
        g_free(text);
        g_date_free(g_start);
        g_date_free(g_end);
        g_date_free(g_first);
        return;
    }

    if (dw->element[row][col] == NULL)
        hb = gtk_hbox_new(TRUE, 0);
    else
        hb = dw->element[row][col];
    if ((row % 2) == 1)
        gtk_widget_modify_bg(ev, GTK_STATE_NORMAL, &dw->bg1);
    if (g_date_days_between(g_start, g_end) == 0)
        tip = g_strdup_printf("%s\n%02d:%02d-%02d:%02d\n%s"
                , appt->title
                , tm_start.tm_hour, tm_start.tm_min
                , tm_end.tm_hour, tm_end.tm_min
                , appt->note);
    else {
        /* we took the date in unnormalized format, so we need to do that now */
        tm_start.tm_year -= 1900;
        tm_start.tm_mon -= 1;
        tm_end.tm_year -= 1900;
        tm_end.tm_mon -= 1;
        start_date = g_strdup(orage_tm_date_to_i18_date(&tm_start));
        end_date = g_strdup(orage_tm_date_to_i18_date(&tm_end));
        tip = g_strdup_printf("%s\n%s %02d:%02d - %s %02d:%02d\n%s"
                , appt->title
                , start_date, tm_start.tm_hour, tm_start.tm_min
                , end_date, tm_end.tm_hour, tm_end.tm_min
                , appt->note);
        g_free(start_date);
        g_free(end_date);
    }
    gtk_tooltips_set_tip(dw->Tooltips, ev, tip, NULL);
    gtk_box_pack_start(GTK_BOX(hb), ev, TRUE, TRUE, 0);
    g_object_set_data_full(G_OBJECT(ev), "UID", g_strdup(appt->uid), g_free);
    g_signal_connect((gpointer)ev, "button-press-event"
            , G_CALLBACK(on_button_press_event_cb), dw);
    dw->element[row][col] = hb;
    g_free(tip);
    g_free(text);
    height = dw->StartDate_button_req.height;
    /*
     * same_date = !strncmp(start_ical_time, end_ical_time, 8);
     * */
    start_col = g_date_days_between(g_first, g_start)+1;
    if (start_col < 1)
        first_col = 1;
    else
        first_col = start_col;
    end_col   = g_date_days_between(g_first, g_end)+1;
    if (end_col > days)
        last_col = days;
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
            dw->line[row][col] = build_line(1, start_height
                    , 2, end_height-start_height, dw->line[row][col]);
        }
    }
    g_date_free(g_start);
    g_date_free(g_end);
    g_date_free(g_first);
}

static void app_rows(day_win *dw, char *a_day , xfical_type ical_type
        , gchar *file_type)
{
    xfical_appt *appt;
    int days = 1;

    program_log("\tapp_rows start");
    days = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dw->day_spin));
    /* xfical_appt_get_next_on_day uses extra days so to show 7 days we need
     * to pass days=6, which means 6 days in addition to the one */
    for (appt = xfical_appt_get_next_on_day(a_day, TRUE, days-1
                , ical_type , file_type);
         appt;
         appt = xfical_appt_get_next_on_day(a_day, FALSE, days-1
                , ical_type , file_type)) {
        add_row(dw, appt, a_day, days);
        xfical_appt_free(appt);
    }
    program_log("\tapp_rows end");
}

static void app_data(day_win *dw)
{
    xfical_type ical_type;
    gchar file_type[8];
    gint i;
    gchar a_day[9]; /* yyyymmdd */

    ical_type = XFICAL_TYPE_EVENT;
    strcpy(a_day, orage_i18_date_to_icaltime(gtk_button_get_label(
            GTK_BUTTON(dw->StartDate_button))));

    /* first search base orage file */
    program_log("\tapp_data before open");
    if (!xfical_file_open(TRUE))
        return;
    program_log("\tapp_data after open");
    strcpy(file_type, "O00.");
    app_rows(dw, a_day, ical_type, file_type);
    /* then process all foreign files */
    for (i = 0; i < g_par.foreign_count; i++) {
        app_rows(dw, a_day, ical_type, file_type);
    }
    xfical_file_close(TRUE);
}


static void fill_days(day_win *dw, gint days)
{
    gint row, col, height, width;
    GtkWidget *name, *ev, *hb;
    GtkWidget *marker;

    program_log("fill_days started");
    height = dw->StartDate_button_req.height;
    width = dw->StartDate_button_req.width;

    for (col = 1; col <  days+1; col++) {
        dw->header[col] = NULL;
        for (row = 0; row < 24; row++) {
            dw->element[row][col] = NULL;
    /* gdk_draw_rectangle(, , , left_x, top_y, width, height); */
            dw->line[row][col] = build_line(0, 0, 3, height, NULL);
        }
    }
    program_log("fill_days init done");
    /* FIXME: the next line is heavy. it takes almost 100 % of time */
    app_data(dw);
    program_log("fill_days data done");
    for (col = 1; col < days+1; col++) {
        hb = gtk_hbox_new(FALSE, 0);
        marker = build_line(0, 0, 2, height, NULL);
        gtk_box_pack_start(GTK_BOX(hb), marker, FALSE, FALSE, 0);
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
    program_log("fill_days done");
}

static void build_day_view_header(day_win *dw, char *start_date)
{
    GtkWidget *hbox, *label, *space_label;

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(dw->Vbox), hbox, FALSE, FALSE, 10);

    label = gtk_label_new(_("Start"));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 10);

    /* start date button */
    dw->StartDate_button = gtk_button_new();
    gtk_box_pack_start(GTK_BOX(hbox), dw->StartDate_button, FALSE, FALSE, 0);

    space_label = gtk_label_new("  ");
    gtk_box_pack_start(GTK_BOX(hbox), space_label, FALSE, FALSE, 0);

    space_label = gtk_label_new("     ");
    gtk_box_pack_start(GTK_BOX(hbox), space_label, FALSE, FALSE, 0);

    label = gtk_label_new(_("Show"));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 10);

    /* show days spin = how many days to show */
    dw->day_spin = gtk_spin_button_new_with_range(1, MAX_DAYS, 1);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(dw->day_spin), TRUE);
    gtk_widget_set_size_request(dw->day_spin, 40, -1);
    gtk_box_pack_start(GTK_BOX(hbox), dw->day_spin, FALSE, FALSE, 0);
    label = gtk_label_new(_("days"));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

    space_label = gtk_label_new("   ");
    gtk_box_pack_start(GTK_BOX(hbox), space_label, FALSE, FALSE, 0);

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

static void build_day_view_bgs(day_win *dw)
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
    dw->bg2.blue += (dw->bg2.blue > 1000 ? -1000 : 1000);
    gdk_colormap_alloc_color(pic1_cmap, &dw->bg1, FALSE, TRUE);
}

static void build_day_view_table(day_win *dw)
{
    gint days;   /* number of days to show */
    int year, month, day;
    gint i, j, sunday;
    GtkWidget *name, *label, *ev;
    char text[5+1], *date;
    struct tm tm_date;
    guint monthdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    GtkWidget *vp;
    GdkColor fg;
    GdkColormap *pic1_cmap;

    pic1_cmap = gdk_colormap_get_system();
    fg.red = 255 * (65535/255);
    fg.green = 10 * (65535/255);
    fg.blue = 10 * (65535/255);
    fg.pixel = (gulong)(fg.red*65536 + fg.green*256 +fg.blue);
    gdk_colormap_alloc_color(pic1_cmap, &fg, FALSE, TRUE);

    days = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dw->day_spin));
    tm_date = orage_i18_date_to_tm_date(
            gtk_button_get_label(GTK_BUTTON(dw->StartDate_button)));
    sunday = tm_date.tm_wday; /* 0 = Sunday */
    if (sunday)
        sunday = 7-sunday;
    
    /* header of day table = days columns */
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
    dw->dtable_h = gtk_table_new(2, days+2, FALSE);
    gtk_box_pack_start(GTK_BOX(dw->day_view_vbox), dw->dtable_h
            , FALSE, FALSE, 0);
    year = tm_date.tm_year + 1900;
    month = tm_date.tm_mon;
    day = tm_date.tm_mday;
    if (((tm_date.tm_year%4) == 0) && (((tm_date.tm_year%100) != 0) 
            || ((tm_date.tm_year%400) == 0)))
        ++monthdays[1];
    name = gtk_label_new(" ");
    gtk_widget_set_size_request(name, dw->hour_req.width, -1);
    gtk_table_attach(GTK_TABLE(dw->dtable_h), name, 0, 1, 0, 1
             , (GTK_FILL), (0), 0, 0);
    for (i = 1; i <  days+1; i++) {
        date = orage_tm_date_to_i18_date(&tm_date);
        name = gtk_button_new();
        gtk_button_set_label(GTK_BUTTON(name), date);
        if ((i-1)%7 == sunday) {
            label = gtk_bin_get_child(GTK_BIN(name));
            gtk_widget_modify_fg(label, GTK_STATE_NORMAL, &fg);
        }
        gtk_widget_set_size_request(name, dw->StartDate_button_req.width, -1);
        g_signal_connect((gpointer)name, "clicked"
                , G_CALLBACK(header_button_clicked_cb), dw);
        gtk_table_attach(GTK_TABLE(dw->dtable_h), name, i, i+1, 0, 1
                , (GTK_FILL), (0), 0, 0);

        if (++tm_date.tm_mday == (monthdays[tm_date.tm_mon]+1)) {
            if (++tm_date.tm_mon == 12) {
                ++tm_date.tm_year;
                tm_date.tm_mon = 0;
            }
            tm_date.tm_mday = 1;
        }
    }
    name = gtk_label_new(" ");
    gtk_widget_set_size_request(name, dw->hour_req.width, -1);
    gtk_table_attach(GTK_TABLE(dw->dtable_h), name, days+1, days+2, 0, 1
             , (GTK_FILL), (0), 0, 0);

    /* body of day table */
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
        ev = gtk_event_box_new();
        g_sprintf(text, "%02d", i);
        name = gtk_label_new(text);
        gtk_container_add(GTK_CONTAINER(ev), name);
        if ((i % 2) == 1)
            gtk_widget_modify_bg(ev, GTK_STATE_NORMAL, &dw->bg1);
        gtk_widget_set_size_request(ev, dw->hour_req.width
                , dw->StartDate_button_req.height);
        gtk_table_attach(GTK_TABLE(dw->dtable), ev, 0, 1, i, i+1
             , (GTK_FILL), (0), 0, 0);
        ev = gtk_event_box_new();
        name = gtk_label_new(text);
        gtk_container_add(GTK_CONTAINER(ev), name);
        if ((i % 2) == 1)
            gtk_widget_modify_bg(ev, GTK_STATE_NORMAL, &dw->bg1);
        gtk_widget_set_size_request(ev, dw->hour_req.width
                , dw->StartDate_button_req.height);
        gtk_table_attach(GTK_TABLE(dw->dtable), ev, days+1, days+2, i, i+1
             , (GTK_FILL), (0), 0, 0);
    }
    fill_days(dw, days);
}

static void set_scroll_position(day_win *dw)
{
    GtkAdjustment *v_adj;
    
    /* let's try to start roughly from line 8 = 8 o'clock */
    v_adj = gtk_scrolled_window_get_vadjustment(
            GTK_SCROLLED_WINDOW(dw->scroll_win));
    gtk_adjustment_set_value(v_adj, v_adj->upper/3);
}

static void refresh_day_view_table(day_win *dw)
{
    gtk_widget_destroy(dw->scroll_win_h);
    build_day_view_table(dw);
    gtk_widget_show_all(dw->scroll_win_h);
    set_scroll_position(dw);
}

day_win *create_day_win(char *start_date)
{
    day_win *dw;

    program_log("create_day_win started");
    /* initialisation + main window + base vbox */
    dw = g_new(day_win, 1);
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
    program_log("create_day_win toolbar done");
    build_day_view_bgs(dw);
    build_day_view_header(dw, start_date);
    program_log("create_day_win header done");
    build_day_view_table(dw);
    program_log("create_day_win table done");
    gtk_widget_show_all(dw->Window);
    set_scroll_position(dw);
    program_log("create_day_win done");

    return(dw);
}
