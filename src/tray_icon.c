/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2006-2013 Juha Kautto  (juha at xfce.org)
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <time.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include <gdk/gdkevents.h>
#include <gdk/gdkx.h>

#include "orage-i18n.h"
#include "functions.h"
#include "mainbox.h"
#include "about-xfcalendar.h"
#include "ical-code.h"
#include "event-list.h"
#include "appointment.h"
#include "parameters.h"
#include "tray_icon.h"

#define ORAGE_TRAYICON ((GtkStatusIcon *)g_par.trayIcon)


static void on_Today_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    struct tm *t;
    CalWin *xfcal = (CalWin *)user_data;
    el_win *el;

    t = orage_localtime();
    orage_select_date(GTK_CALENDAR(xfcal->mCalendar), t->tm_year+1900
            , t->tm_mon, t->tm_mday);
    el = create_el_win(NULL);
}

static void on_preferences_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    show_parameters();
}

static void on_new_appointment_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    struct tm *t;
    char cur_date[9];

    t = orage_localtime();
    g_snprintf(cur_date, 9, "%04d%02d%02d", t->tm_year+1900
               , t->tm_mon+1, t->tm_mday);
    create_appt_win("NEW", cur_date);  
}

static void on_about_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    create_wAbout((GtkWidget *)menuitem, user_data);
}

static void on_globaltime_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    GError *error = NULL;

    if (!orage_exec("globaltime", FALSE, &error))
        g_message("%s: start of %s failed: %s", "Orage", "globaltime"
                , error->message);
}

static gboolean button_press_cb(GtkStatusIcon *status_icon
        , GdkEventButton *event, gpointer user_data)
{
    GdkAtom atom;
    GdkEventClient gev;
    Window xwindow;

    if (event->type != GDK_BUTTON_PRESS) /* double or triple click */
        return(FALSE); /* ignore */
    else if (event->button == 2) {
        /* send message to program to check if it is running */
        atom = gdk_atom_intern("_XFCE_GLOBALTIME_RUNNING", FALSE);
        if ((xwindow = XGetSelectionOwner(GDK_DISPLAY(),
                gdk_x11_atom_to_xatom(atom))) != None) { /* yes, then toggle */
            gev.type = GDK_CLIENT_EVENT;
            gev.window = NULL;
            gev.send_event = TRUE;
            gev.message_type = gdk_atom_intern("_XFCE_GLOBALTIME_TOGGLE_HERE"
                    , FALSE);
            gev.data_format = 8;

            if (!gdk_event_send_client_message((GdkEvent *) &gev,
                    (GdkNativeWindow)xwindow))
                 g_message("%s: send message to %s failed", "Orage"
                         , "globaltime");

            return(TRUE);
        }
        else { /* not running, let's try to start it. Need to reset TZ! */
            on_globaltime_activate(NULL, NULL);
            return(TRUE);
        }
    }

    return(FALSE);
}

static void toggle_visible_cb(GtkStatusIcon *status_icon, gpointer user_data)
{
    orage_toggle_visible();
}

static void show_menu(GtkStatusIcon *status_icon, guint button
        , guint activate_time, gpointer user_data)
{
    gtk_menu_popup((GtkMenu *)user_data, NULL, NULL, NULL, NULL
            , button, activate_time);
}

static gboolean format_line(PangoLayout *pl, struct tm *t, char *data
        , char *font, char *color)
{
    gchar ts[200];
    gchar row[20];
    gchar *row_format = "<span foreground=\"%s\" font_desc=\"%s\">%s</span>";
    gchar *strftime_failed = "format_line: strftime %s failed";

    if (ORAGE_STR_EXISTS(data)) {
        if (strftime(row, 19, data, t) == 0) {
            g_warning(strftime_failed, data);
            return(FALSE);
        }
        else {
            g_snprintf(ts, 199, row_format, color, font, row);
            pango_layout_set_markup(pl, ts, -1);
            pango_layout_set_alignment(pl, PANGO_ALIGN_CENTER);
            return(TRUE);
        }
    }
    else
        return(FALSE);
}

static void create_own_icon_pango_layout(gint line
        , GdkPixmap *pic, GdkGC *pic_gc
        , struct tm *t, gint width, gint height)
{
    CalWin *xfcal = (CalWin *)g_par.xfcal;
    PangoRectangle real_rect, log_rect;
    gint x_size, y_size, x_offset, y_offset;
    PangoLayout *pl;

    pl = gtk_widget_create_pango_layout(xfcal->mWindow, "x");
    switch (line) {
        case 1:
            if (format_line(pl, t, g_par.own_icon_row1_data
                    , g_par.own_icon_row1_font, g_par.own_icon_row1_color)) {
                pango_layout_get_extents(pl, &real_rect, &log_rect);
                x_size = PANGO_PIXELS(log_rect.width);
                x_offset = (width - x_size)/2 + g_par.own_icon_row1_x;
                y_offset = g_par.own_icon_row1_y;
                gdk_draw_layout(pic, pic_gc, x_offset, y_offset, pl);
            }
            break;
        case 2:
            if (format_line(pl, t, g_par.own_icon_row2_data
                    , g_par.own_icon_row2_font, g_par.own_icon_row2_color)) {
                pango_layout_get_extents(pl, &real_rect, &log_rect);
                x_size = PANGO_PIXELS(log_rect.width);
                x_offset = (width - x_size)/2 + g_par.own_icon_row2_x;
                y_offset = g_par.own_icon_row2_y;
                gdk_draw_layout(pic, pic_gc, x_offset, y_offset, pl);
            }
            break;
        case 3:
            if (format_line(pl, t, g_par.own_icon_row3_data
                    , g_par.own_icon_row3_font, g_par.own_icon_row3_color)) {
                pango_layout_get_extents(pl, &real_rect, &log_rect);
                x_size = PANGO_PIXELS(log_rect.width);
                x_offset = (width - x_size)/2 + g_par.own_icon_row3_x;
                y_offset = g_par.own_icon_row3_y;
                gdk_draw_layout(pic, pic_gc, x_offset, y_offset, pl);
            }
            break;
        default:
            g_warning("create_own_icon_pango_layout: wrong line number %d", line);
    }
    g_object_unref(pl);
}

static void create_icon_pango_layout(gint line, GdkPixmap *pic, GdkGC *pic_gc
        , struct tm *t, gint width, gint height)
{
    CalWin *xfcal = (CalWin *)g_par.xfcal;
    PangoRectangle real_rect, log_rect;
    gint x_size, y_size, x_offset, y_offset;
    gboolean first_try = TRUE, done = FALSE;
    char line1_font[] = "Ariel 24";
    char line3_font[] = "Ariel bold 26";
    char color[] = "black";
    char def[] = "Orage";
    PangoLayout *pl;

    pl = gtk_widget_create_pango_layout(xfcal->mWindow, "x");
    while (!done) {
        switch (line) {
            case 1:
                /* Try different methods: 
                   ^ should make the string upper case, but it does not exist 
                   in all implementations and it may not work well. 
                   %A may produce too long string, so then on second try we 
                   try shorter %a. 
                   And finally last default is string orage. */
                if (first_try) {
                    if (!format_line(pl, t, "%^A", line1_font, color)) {
                        if (!format_line(pl, t, "%A", line1_font, color)) {
                            /* we should never come here */
                            format_line(pl, t, def, line1_font, color);
                        }
                    }
                }
                else {
                    if (!format_line(pl, t, "%^a", line1_font, color)) {
                        if (!format_line(pl, t, "%a", line1_font, color)) {
                            /* we should never come here */
                            format_line(pl, t, def, line1_font, color);
                        }
                    }
                }
                break;
            case 2:
                /* this %d should always work, so no need to try other */
                format_line(pl, t, "%d", "Sans bold 72", "red");
                break;
            case 3:
                if (first_try) {
                    if (!format_line(pl, t, "%^B", line3_font, color)) {
                        if (!format_line(pl, t, "%B", line3_font, color)) {
                            format_line(pl, t, def, line3_font, color);
                        }
                    }
                }
                else {
                    if (!format_line(pl, t, "%^b", line3_font, color)) {
                        if (!format_line(pl, t, "%b", line3_font, color)) {
                            format_line(pl, t, def, line3_font, color);
                        }
                    }
                }
                break;
            default:
                g_warning("create_icon_pango_layout: wrong line number %d", line);
        }
        pango_layout_get_extents(pl, &real_rect, &log_rect);
        /* real_rect is smaller than log_rect. real is the minimal rectangular
           to cover the text while log has some room around it. It is safer to
           use log since it is hard to position real.
           */
        /* x_offset, y_offset = free space left after this layout 
           divided equally to start and end */
        x_size = PANGO_PIXELS(log_rect.width);
        y_size = PANGO_PIXELS(log_rect.height);
        x_offset = (width - x_size)/2;
        if (line == 1)
            y_offset = 4; /* we have 1 pixel border line */
        else if (line == 2)
            y_offset = (height - y_size + 2)/2;
        else if (line == 3)
            y_offset = (height - y_size);
        /*
    g_print("\n\norage row %d offset. First trial %d\n"
            "width=%d x-offset=%d\n"
            "\t(real pixel text width=%d logical pixel text width=%d)\n"
            "height=%d y-offset=%d\n"
            "\t(real pixel text height=%d logical pixel text height=%d)\n"
            "\n" 
            , line, first_try
            , width, x_offset
            , PANGO_PIXELS(real_rect.width), x_size
            , height, y_offset
            , PANGO_PIXELS(real_rect.height), y_size
            ); 
            */
        if (x_offset < 0) { /* it does not fit */
            if (first_try) {
                first_try = FALSE;
            }
            else {
                orage_message(110, "trayicon: row %d does not fit in dynamic icon", line);
                done = TRUE; /* failed */
            }
        }
        else {
            done = TRUE; /* success */
        }
    }
    gdk_draw_layout(pic, pic_gc, x_offset, y_offset, pl);
    g_object_unref(pl);
    /*
    gdk_draw_rectangle(pic, pic_gc2, FALSE
            , x_offset+(x_size-PANGO_PIXELS(real_rect.width))/2
            , y_offset+(y_size-PANGO_PIXELS(real_rect.height))/2
            , PANGO_PIXELS(real_rect.width), PANGO_PIXELS(real_rect.height));
    gdk_draw_rectangle(pic, pic_gc2, FALSE
            , x_offset, y_offset, x_size, y_size);
            */
}

static GdkPixmap *create_icon_pixmap(GdkColormap *pic_cmap
        , int width, int height, int depth)
{
    GdkPixmap *pic;
    GdkGC *pic_gc1, *pic_gc2;
    GdkColor color;
    gint red = 239, green = 235, blue = 230;

    /* let's build Orage standard icon pixmap */
    pic = gdk_pixmap_new(NULL, width, height, depth);
    gdk_drawable_set_colormap(pic, pic_cmap);
    pic_gc1 = gdk_gc_new(pic);
    pic_gc2 = gdk_gc_new(pic);
    color.red = red * (65535/255);
    color.green = green * (65535/255);
    color.blue = blue * (65535/255);
    color.pixel = (gulong)(red*65536 + green*256 + blue);
    gdk_colormap_alloc_color(pic_cmap, &color, FALSE, TRUE);
    gdk_gc_set_foreground(pic_gc1, &color);
    gdk_draw_rectangle(pic, pic_gc1, TRUE, 0, 0, width, height);
    gdk_draw_line(pic, pic_gc2, 4, height-1, width-1, height-1);
    gdk_draw_line(pic, pic_gc2, width-1, 4, width-1, height-1);
    gdk_draw_line(pic, pic_gc2, 2, height-3, width-3, height-3);
    gdk_draw_line(pic, pic_gc2, width-3, 2, width-3, height-3);
    gdk_draw_rectangle(pic, pic_gc2, FALSE, 0, 0, width-5, height-5);
    g_object_unref(pic_gc1);
    g_object_unref(pic_gc2);
    return(pic);
}

static GdkPixmap *create_own_icon_pixmap(GdkColormap *pic_cmap
        , int width, int height, int depth)
{
    GdkPixmap *pic;

    pic = gdk_pixmap_colormap_create_from_xpm(NULL, pic_cmap, NULL, NULL
            , g_par.own_icon_file);
    if (pic == NULL) { /* failed, let's use standard instead */
        orage_message(110, "trayicon: xpm file %s not valid pixmap"
                , g_par.own_icon_file);
        pic = create_icon_pixmap(pic_cmap, width, height, depth);
    }
    return(pic);
}

GdkPixbuf *orage_create_icon(gboolean static_icon, gint size)
{
    CalWin *xfcal = (CalWin *)g_par.xfcal;
    GtkIconTheme *icon_theme = NULL;
    GdkPixbuf *pixbuf, *pixbuf2;
    GdkPixmap *pic;
    GdkGC *pic_gc;
    GdkColormap *pic_cmap;
    GdkVisual *pic_vis;
    struct tm *t;
    gint width = 160, height = 160, depth = 16; /* size of icon */
    gint r_width = 0, r_height = 0; /* usable size of icon */

    icon_theme = gtk_icon_theme_get_default();
    if (static_icon || !g_par.use_dynamic_icon) { /* load static icon */
        pixbuf = gtk_icon_theme_load_icon(icon_theme, "xfcalendar", size
                , GTK_ICON_LOOKUP_USE_BUILTIN, NULL);
    }
    else { /***** dynamic icon build starts now *****/
        t = orage_localtime();
        pic_cmap = gdk_colormap_get_system();
        pic_vis = gdk_colormap_get_visual(pic_cmap);
        depth = pic_vis->depth;

        if (g_par.use_own_dynamic_icon) {
            pic = create_own_icon_pixmap(pic_cmap, width, height, depth);
            pic_gc = gdk_gc_new(pic);
            create_own_icon_pango_layout(1, pic, pic_gc, t, width, height);
            create_own_icon_pango_layout(2, pic, pic_gc, t, width, height);
            create_own_icon_pango_layout(3, pic, pic_gc, t, width, height);
        }
        else { /* standard dynamic icon */
            pic = create_icon_pixmap(pic_cmap, width, height, depth);
            pic_gc = gdk_gc_new(pic);
            /* We draw borders so we can't use the whole space */
            r_width = width - 6; 
            r_height = height - 6;

            /* weekday */
            create_icon_pango_layout(1, pic, pic_gc, t, r_width, r_height);

            /* day */
            create_icon_pango_layout(2, pic, pic_gc, t, r_width, r_height);

            /* month */
            create_icon_pango_layout(3, pic, pic_gc, t, r_width, r_height);
        }

        pixbuf = gdk_pixbuf_get_from_drawable(NULL, pic, pic_cmap
                , 0, 0, 0, 0, width, height);
        if (size) {
            pixbuf2 = gdk_pixbuf_scale_simple(pixbuf, size, size
                    , GDK_INTERP_BILINEAR);
            g_object_unref(pixbuf);
            pixbuf = pixbuf2;
        }

        if (pixbuf == NULL) {
            g_warning("orage_create_icon: dynamic icon creation failed\n");
            pixbuf = gtk_icon_theme_load_icon(icon_theme, "xfcalendar", size
                    , GTK_ICON_LOOKUP_USE_BUILTIN, NULL);
        }
        g_object_unref(pic_gc);
        g_object_unref(pic);
    }
  
    if (pixbuf == NULL) {
        g_warning("orage_create_icon: static icon creation failed, using stock ABOUT icon\n");
        /* dynamic icon also tries static before giving up */
        pixbuf = gtk_icon_theme_load_icon(icon_theme, GTK_STOCK_ABOUT, size
                , GTK_ICON_LOOKUP_USE_BUILTIN, NULL);
    }
    return(pixbuf);
}

static GtkWidget *create_TrayIcon_menu(void)
{
    CalWin *xfcal = (CalWin *)g_par.xfcal;
    GtkWidget *trayMenu;
    GtkWidget *menuItem;

    trayMenu = gtk_menu_new();

    menuItem = gtk_image_menu_item_new_with_mnemonic(_("Today"));
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuItem)
            , gtk_image_new_from_stock(GTK_STOCK_HOME, GTK_ICON_SIZE_MENU));
    g_signal_connect(menuItem, "activate"
            , G_CALLBACK(on_Today_activate), xfcal);
    gtk_menu_shell_append(GTK_MENU_SHELL(trayMenu), menuItem);

    menuItem = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(trayMenu), menuItem);
    menuItem = gtk_image_menu_item_new_with_label(_("New appointment"));
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuItem)
            , gtk_image_new_from_stock(GTK_STOCK_NEW, GTK_ICON_SIZE_MENU));
    g_signal_connect(menuItem, "activate"
            , G_CALLBACK(on_new_appointment_activate), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(trayMenu), menuItem);
  
    menuItem = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(trayMenu), menuItem);
    menuItem = gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, NULL);
    g_signal_connect(menuItem, "activate"
            , G_CALLBACK(on_preferences_activate), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(trayMenu), menuItem);

    menuItem = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(trayMenu), menuItem);
    menuItem = gtk_image_menu_item_new_with_label(_("About Orage"));
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuItem)
            , gtk_image_new_from_stock(GTK_STOCK_ABOUT, GTK_ICON_SIZE_MENU));
    g_signal_connect(menuItem, "activate"
            , G_CALLBACK(on_about_activate), xfcal);
    gtk_menu_shell_append(GTK_MENU_SHELL(trayMenu), menuItem);

    menuItem = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(trayMenu), menuItem);
    menuItem = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
    g_signal_connect(menuItem, "activate", G_CALLBACK(gtk_main_quit), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(trayMenu), menuItem);
  
    menuItem = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(trayMenu), menuItem);
    menuItem = gtk_image_menu_item_new_with_label(_("Globaltime"));
    g_signal_connect(menuItem, "activate"
            , G_CALLBACK(on_globaltime_activate), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(trayMenu), menuItem);

    gtk_widget_show_all(trayMenu);
    return(trayMenu);
}

static void destroy_TrayIcon(GtkStatusIcon *trayIcon)
{
    g_object_unref(trayIcon);
}

GtkStatusIcon* create_TrayIcon(GdkPixbuf *orage_logo)
{
    CalWin *xfcal = (CalWin *)g_par.xfcal;
    GtkWidget *trayMenu;
    GtkStatusIcon *trayIcon = NULL;

    /*
     * Create the tray icon popup menu
     */
    trayMenu = create_TrayIcon_menu();

    /*
     * Create the tray icon
     */
    trayIcon = gtk_status_icon_new_from_pixbuf(orage_logo);
    g_object_ref(trayIcon);
    g_object_ref_sink(trayIcon);

    g_signal_connect(G_OBJECT(trayIcon), "button-press-event",
    			   G_CALLBACK(button_press_cb), xfcal);
    g_signal_connect(G_OBJECT(trayIcon), "activate",
    			   G_CALLBACK(toggle_visible_cb), xfcal);
    g_signal_connect(G_OBJECT(trayIcon), "popup_menu",
    			   G_CALLBACK(show_menu), trayMenu);
    return(trayIcon);
}

void refresh_TrayIcon(void)
{
    GdkPixbuf *orage_logo;

    orage_logo = orage_create_icon(FALSE, 0);
    if (!orage_logo) {
        g_warning("refresh_TrayIcon: failed to load icon.");
        return;
    }
    if (g_par.show_systray) { /* refresh tray icon */
        if (ORAGE_TRAYICON && gtk_status_icon_is_embedded(ORAGE_TRAYICON)) {
            gtk_status_icon_set_visible(ORAGE_TRAYICON, FALSE);
            destroy_TrayIcon(ORAGE_TRAYICON);
        }
        g_par.trayIcon = create_TrayIcon(orage_logo);
        gtk_status_icon_set_visible(ORAGE_TRAYICON, TRUE);
    }
    gtk_window_set_default_icon(orage_logo);
    /* main window is always active so we need to change it's icon also */
    gtk_window_set_icon(GTK_WINDOW(((CalWin *)g_par.xfcal)->mWindow)
            , orage_logo);
    g_object_unref(orage_logo);
}
