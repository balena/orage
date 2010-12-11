/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2006-2008 Juha Kautto  (juha at xfce.org)
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

#define ORAGE_TRAYICON ((GtkStatusIcon *)g_par.trayIcon)

void orage_toggle_visible();


void on_Today_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    struct tm *t;
    CalWin *xfcal = (CalWin *)user_data;
    el_win *el;

    t = orage_localtime();
    orage_select_date(GTK_CALENDAR(xfcal->mCalendar), t->tm_year+1900
            , t->tm_mon, t->tm_mday);
    el = create_el_win(NULL);
}

void on_preferences_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    show_parameters();
}

void on_new_appointment_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    struct tm *t;
    char cur_date[9];

    t = orage_localtime();
    g_snprintf(cur_date, 9, "%04d%02d%02d", t->tm_year+1900
               , t->tm_mon+1, t->tm_mday);
    create_appt_win("NEW", cur_date);  
}

void on_about_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    create_wAbout((GtkWidget *)menuitem, user_data);
}

void on_globaltime_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    GError *error = NULL;

    if (!orage_exec("globaltime", FALSE, &error))
        g_message("%s: start of %s failed: %s", "Orage", "globaltime"
                , error->message);
}

gboolean button_press_cb(GtkStatusIcon *status_icon, GdkEventButton *event
        , gpointer user_data)
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

void toggle_visible_cb(GtkStatusIcon *status_icon, gpointer user_data)
{
    orage_toggle_visible();
}

void show_menu(GtkStatusIcon *status_icon, guint button, guint activate_time
        , gpointer user_data)
{
    gtk_menu_popup((GtkMenu *)user_data, NULL, NULL, NULL, NULL
            , button, activate_time);
}

void create_icon_pango_layout(gint line, PangoLayout *pl, struct tm *t
        , gint width, gint height
        , gint *x_offset, gint *y_offset, gint *y_size)
{
    gchar ts[200], row[20];
    PangoRectangle real_rect, log_rect;
    gint x_size;
    gboolean first_try = TRUE, done = FALSE;
    gchar *row1_1_data = "%^A";
    gchar *row1_1b_data = "%A";
    gchar *row1_2_data = "%^a";
    gchar *row1_2b_data = "%a";
    gchar *row1_color = "black";
    gchar *row1_font = "Ariel 24";
    gchar *row2_color = "red";
    gchar *row2_font = "Sans bold 72";
    gchar *row3_1_data = "%^B";
    gchar *row3_1b_data = "%B";
    gchar *row3_2_data = "%^b";
    gchar *row3_2b_data = "%b";
    gchar *row3_color = "black";
    gchar *row3_font = "Ariel bold 26";
    gchar *row_format = "<span foreground=\"%s\" font_desc=\"%s\">%s</span>";
    gchar *strfttime_failed = "create_icon_pango_layout: strftime %s failed";

    while (!done) {
        switch (line) {
            case 1:
                if (first_try) {
                    if (strftime(row, 19, row1_1_data, t) == 0) {
                        g_warning(strfttime_failed, row1_1_data);
                        if (strftime(row, 19, row1_1b_data, t) == 0) {
                            g_warning(strfttime_failed, row1_1b_data);
                            g_sprintf(row, "orage");
                        }
                    }
                }
                else { /* we failed once, let's try shorter string */
                    if (strftime(row, 19, row1_2_data, t) == 0) {
                        g_warning(strfttime_failed, row1_2_data);
                        if (strftime(row, 19, row1_2b_data, t) == 0) {
                            g_warning(strfttime_failed, row1_2b_data);
                            g_sprintf(row, "orage");
                        }
                    }
                }
                g_snprintf(ts, 199, row_format, row1_color, row1_font, row);
                break;
            case 2:
                g_snprintf(row, 19, "%02d", t->tm_mday);
                g_snprintf(ts, 199, row_format, row2_color, row2_font, row);
                break;
            case 3:
                if (first_try) {
                    if (strftime(row, 19, row3_1_data, t) == 0) {
                        g_warning(strfttime_failed, row3_1_data);
                        if (strftime(row, 19, row3_1b_data, t) == 0) {
                            g_warning(strfttime_failed, row3_1b_data);
                            g_sprintf(row, "orage");
                        }
                    }
                }
                else { /* we failed once, let's try shorter string */
                    if (strftime(row, 19, row3_2_data, t) == 0) {
                        g_warning(strfttime_failed, row3_2_data);
                        if (strftime(row, 19, row3_2b_data, t) == 0) {
                            g_warning(strfttime_failed, row3_2b_data);
                            g_sprintf(row, "orage");
                        }
                    }
                }
                g_snprintf(ts, 199, row_format, row3_color, row3_font, row);
                break;
            default:
                g_warning("create_icon_pango_layout: wrong line number %d", line);
        }
        pango_layout_set_markup(pl, ts, -1);
        pango_layout_set_alignment(pl, PANGO_ALIGN_CENTER);
        pango_layout_get_extents(pl, &real_rect, &log_rect);
        /* real_rect is smaller than log_rect. real is the minimal rectangular
           to cover the text while log has some room around it. It is safer to
           use log since it is hard to position real.
           */
        /* x_offset, y_offset = free space left after this layout 
           divided equally to start and end */
        x_size = PANGO_PIXELS(log_rect.width);
        *y_size = PANGO_PIXELS(log_rect.height);
        *x_offset = (width - x_size)/2;
        if (line == 1)
            *y_offset = 4; /* we have 1 pixel border line */
        else if (line == 2)
            *y_offset = (height - *y_size + 2)/2;
        else if (line == 3)
            *y_offset = (height - *y_size);
        /*
    g_print("\n\norage row %d offset. First trial %d\n"
            "width=%d x-offset=%d\n"
            "\t(real pixel text width=%d logical pixel text width=%d)\n"
            "height=%d y-offset=%d\n"
            "\t(real pixel text height=%d logical pixel text height=%d)\n"
            "\n" 
            , line, first_try
            , width, *x_offset
            , PANGO_PIXELS(real_rect.width), x_size
            , height, *y_offset
            , PANGO_PIXELS(real_rect.height), *y_size
            ); 
            */
        if (*x_offset < 0) { /* it does not fit */
            if (first_try) {
                first_try = FALSE;
            }
            else {
                orage_message(110, "trayicon: row %d does not fit in dynamic icon", line);
                done = TRUE; /* failed */
            }
        }
        else 
            done = TRUE;
    }
    /*
    gdk_draw_rectangle(pic1, pic1_gc2, FALSE
            , *x_offset+(x_size-PANGO_PIXELS(real_rect.width))/2
            , *y_offset+(*y_size-PANGO_PIXELS(real_rect.height))/2
            , PANGO_PIXELS(real_rect.width), PANGO_PIXELS(real_rect.height));
    gdk_draw_rectangle(pic1, pic1_gc2, FALSE
            , *x_offset, *y_offset, x_size, *y_size);
            */
}

GdkPixbuf *orage_create_icon(gboolean static_icon, gint size)
{
    CalWin *xfcal = (CalWin *)g_par.xfcal;
    GtkIconTheme *icon_theme = NULL;
    GdkPixbuf *pixbuf, *pixbuf2;
    GdkPixmap *pic1;
    GdkGC *pic1_gc1, *pic1_gc2;
    GdkColormap *pic1_cmap;
    GdkColor color;
    GdkVisual *pic1_vis;
    PangoLayout *pl_day, *pl_weekday, *pl_month;
    gint width = 0, height = 0, depth = 16;
    gint red = 239, green = 235, blue = 230;
    struct tm *t;
    gint x_used = 0, y_used = 0;
    gint x_offset_day = 0, y_offset_day = 0, y_size_day = 0;
    gint x_offset_weekday = 0, y_offset_weekday = 0, y_size_weekday = 0;
    gint x_offset_month = 0, y_offset_month = 0, y_size_month = 0;

    icon_theme = gtk_icon_theme_get_default();
    if (static_icon || !g_par.use_dynamic_icon) {
        pixbuf = gtk_icon_theme_load_icon(icon_theme, "xfcalendar", size
                , GTK_ICON_LOOKUP_USE_BUILTIN, NULL);
        return(pixbuf);
    }

    /***** dynamic icon build starts now *****/
    t = orage_localtime();
    width = 160; 
    height = 160;
    pic1_cmap = gdk_colormap_get_system();
    pic1_vis = gdk_colormap_get_visual(pic1_cmap);
    depth = pic1_vis->depth;
    pic1 = gdk_pixmap_new(NULL, width, height, depth);
    gdk_drawable_set_colormap(pic1, pic1_cmap);
    /*
    pic1 = gdk_pixmap_colormap_create_from_xpm(NULL, pic1_cmap, NULL, NULL
            , "/home/juha/ohjelmat/Orage/dev/orage/icons/orage.xpm");
    */
    /* pic1_cmap = gtk_widget_get_colormap(xfcal->mWindow); */
    pic1_gc1 = gdk_gc_new(pic1);
    pic1_gc2 = gdk_gc_new(pic1);
    color.red = red * (65535/255);
    color.green = green * (65535/255);
    color.blue = blue * (65535/255);
    color.pixel = (gulong)(red*65536 + green*256 + blue);
    /*
    gdk_color_alloc(pic1_cmap, &color);
    */
    gdk_colormap_alloc_color(pic1_cmap, &color, FALSE, TRUE);
    gdk_gc_set_foreground(pic1_gc1, &color);
    /*
    gdk_draw_rectangle(pic1, pic1_gc1, TRUE, 0, 0, width, height);
    gdk_draw_rectangle(pic1, pic1_gc2, FALSE, 0, 0, width-1, height-1);
    gdk_draw_rectangle(pic1, pic1_gc2, FALSE, 0, 0, width-3, height-3);
    gdk_draw_rectangle(pic1, pic1_gc2, FALSE, 0, 0, width-5, height-5);
    */
    gdk_draw_rectangle(pic1, pic1_gc1, TRUE, 0, 0, width, height);
    gdk_draw_line(pic1, pic1_gc2, 4, height-1, width-1, height-1);
    gdk_draw_line(pic1, pic1_gc2, width-1, 4, width-1, height-1);
    gdk_draw_line(pic1, pic1_gc2, 2, height-3, width-3, height-3);
    gdk_draw_line(pic1, pic1_gc2, width-3, 2, width-3, height-3);
    gdk_draw_rectangle(pic1, pic1_gc2, FALSE, 0, 0, width-5, height-5);
    x_used = 6;
    y_used = 6;

    /* gdk_draw_line(pic1, pic1_gc1, 10, 20, 30, 38); */

    /* create any valid pango layout to get things started */
    pl_day = gtk_widget_create_pango_layout(xfcal->mWindow, "x");
    pl_weekday = pango_layout_copy(pl_day);
    pl_month = pango_layout_copy(pl_day);

    /* weekday */
    create_icon_pango_layout(1, pl_weekday, t
            , width - x_used, height - y_used
            , &x_offset_weekday, &y_offset_weekday, &y_size_weekday);

    /* day */
    create_icon_pango_layout(2, pl_day, t
            , width - x_used, height - y_used
            , &x_offset_day, &y_offset_day, &y_size_day);

    /* month */
    create_icon_pango_layout(3, pl_month, t
            , width - x_used, height - y_used
            , &x_offset_month, &y_offset_month, &y_size_month);

    gdk_draw_layout(pic1, pic1_gc1, x_offset_weekday, y_offset_weekday
            , pl_weekday);
    gdk_draw_layout(pic1, pic1_gc1, x_offset_day, y_offset_day
            , pl_day);
    gdk_draw_layout(pic1, pic1_gc1, x_offset_month, y_offset_month
            , pl_month);
    pixbuf = gdk_pixbuf_get_from_drawable(NULL, pic1, pic1_cmap
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

    g_object_unref(pic1_gc1);
    g_object_unref(pic1_gc2);
    g_object_unref(pl_day);
    g_object_unref(pl_weekday);
    g_object_unref(pl_month);
    g_object_unref(pic1);
  
    return(pixbuf);
}

GtkWidget *create_TrayIcon_menu()
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

void destroy_TrayIcon(GtkStatusIcon *trayIcon)
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

void refresh_TrayIcon()
{
    GdkPixbuf *orage_logo;

    orage_logo = orage_create_icon(FALSE, 0);
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
