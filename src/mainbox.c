/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2005-2007 Juha Kautto  (juha at xfce.org)
 * Copyright (c) 2004-2005 Mickael Graf (korbinus at xfce.org)
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "orage-i18n.h"
#include "functions.h"
#include "mainbox.h"
#include "about-xfcalendar.h"
#include "ical-code.h"
#include "event-list.h"
#include "appointment.h"
#include "interface.h"
#include "parameters.h"
#include "tray_icon.h"


gboolean
orage_mark_appointments()
{
    guint year, month, day;

    if (!xfical_file_open(TRUE))
        return(FALSE);
    gtk_calendar_get_date(GTK_CALENDAR(g_par.xfcal->mCalendar)
            , &year, &month, &day);
    xfical_mark_calendar(GTK_CALENDAR(g_par.xfcal->mCalendar)
            , year, month+1); 
    xfical_file_close(TRUE);
    return(TRUE);
}

static void
mFile_newApp_activate_cb(GtkMenuItem *menuitem, gpointer user_data)
{
    appt_win *app;
    struct tm *t;
    char cur_date[9];

    t = orage_localtime();
    g_snprintf(cur_date, 9, "%04d%02d%02d", t->tm_year+1900
            , t->tm_mon+1, t->tm_mday);
    app = create_appt_win("NEW", cur_date, NULL);  
}

static void
mFile_interface_activate_cb(GtkMenuItem *menuitem, gpointer user_data)
{
    CalWin *cal = (CalWin *)user_data;

    orage_external_interface(cal);
}

static void
mFile_close_activate_cb(GtkMenuItem *menuitem, gpointer user_data)
{
    CalWin *cal = (CalWin *)user_data;

    gtk_widget_hide(cal->mWindow);
}

static void
mFile_quit_activate_cb(GtkMenuItem *menuitem, gpointer user_data)
{
    gtk_main_quit();
}

static void 
mEdit_preferences_activate_cb(GtkMenuItem *menuitem, gpointer user_data)
{
    show_parameters();
}

static void
mView_ViewSelectedDate_activate_cb(GtkMenuItem *menuitem, gpointer user_data)
{
    create_el_win(NULL);
}

static void
mView_ViewSelectedWeek_activate_cb(GtkMenuItem *menuitem, gpointer user_data)
{
    CalWin *cal = (CalWin *)user_data;

    create_day_win(orage_cal_to_i18_date(GTK_CALENDAR(cal->mCalendar)));
}

static void
mView_selectToday_activate_cb(GtkMenuItem *menuitem, gpointer user_data)
{
    CalWin *cal = (CalWin *)user_data;

    orage_select_today(GTK_CALENDAR(cal->mCalendar));
}

static void
mHelp_help_activate_cb(GtkMenuItem *menuitem, gpointer user_data)
{
    gchar *helpdoc;

    helpdoc = g_strconcat("xfbrowser4 ", PACKAGE_DATA_DIR
           , G_DIR_SEPARATOR_S, "orage"
           , G_DIR_SEPARATOR_S, "doc"
           , G_DIR_SEPARATOR_S, "C"
           , G_DIR_SEPARATOR_S, "orage.html", NULL);
    orage_exec(helpdoc, NULL, NULL);
    /*
    xfce_exec(helpdoc, FALSE, FALSE, NULL);
    xfce_exec("xfhelp4 xfce4-user-guide.html", FALSE, FALSE, NULL);
    */
}

static void
mHelp_about_activate_cb(GtkMenuItem *menuitem, gpointer user_data)
{
    create_wAbout((GtkWidget *)menuitem, user_data);
}

static void
mCalendar_scroll_event_cb(GtkWidget *calendar, GdkEventScroll *event)
{
    guint year, month, day;
    gtk_calendar_get_date(GTK_CALENDAR(calendar), &year, &month, &day);

    switch(event->direction) {
        case GDK_SCROLL_UP:
            if (--month == -1) {
                month = 11;
                --year;
            }
            gtk_calendar_select_month(GTK_CALENDAR(calendar), month, year);
            break;
        case GDK_SCROLL_DOWN:
            if (++month == 12) {
                month = 0;
                ++year;
            }
            gtk_calendar_select_month(GTK_CALENDAR(calendar), month, year);
            break;
        default:
            g_warning("Got unknown scroll event!!!");
    }
}

static void
mCalendar_day_selected_double_click_cb(GtkCalendar *cdar, gpointer user_data)
{
    if (g_par.show_days)
        create_day_win(orage_cal_to_i18_date(cdar));
    else
        create_el_win(NULL);
}

static gboolean
upd_calendar(GtkCalendar *calendar)
{
    static guint year=-1, month=-1;
    guint year_n, month_n, day_n;

    /* we only need to do this if it is really a new month. We may get
     * many of these while calender is changing months and it is enough
     * to show only the last one, which is visible */
    gtk_calendar_get_date(calendar, &year_n, &month_n, &day_n);
    if (month != month_n || year != year_n) { /* need really do it */
        orage_mark_appointments();
        year = year_n;
        month = month_n;
    }
    return(FALSE); /* we do this only once */
}

void
mCalendar_month_changed_cb(GtkCalendar *calendar, gpointer user_data)
{
    /* orage_mark_appointments is rather heavy (=slow), so doing
     * it here is not a good idea. We can't keep up with the autorepeat
     * speed if we do the whole thing here. bug 2080 proofs it. so let's
     * throw it to background and do it later */
    g_timeout_add(500, (GtkFunction)upd_calendar, calendar);
}

static void build_menu(void)
{
    GtkWidget *menu_separator;
    CalWin *cal = g_par.xfcal;

    cal->mMenubar = gtk_menu_bar_new();
    gtk_box_pack_start(GTK_BOX(cal->mVbox), cal->mMenubar, FALSE, FALSE, 0);

    /* File menu */
    cal->mFile_menu = orage_menu_new(_("_File"), cal->mMenubar);

    cal->mFile_newApp = orage_image_menu_item_new_from_stock("gtk-new"
            , cal->mFile_menu, cal->mAccel_group);

    menu_separator = orage_separator_menu_item_new(cal->mFile_menu);

    cal->mFile_interface = 
            orage_menu_item_new_with_mnemonic(_("_Exchange data")
                    , cal->mFile_menu);

    menu_separator = orage_separator_menu_item_new(cal->mFile_menu);

    cal->mFile_close = orage_image_menu_item_new_from_stock("gtk-close"
            , cal->mFile_menu, cal->mAccel_group);
    cal->mFile_quit = orage_image_menu_item_new_from_stock("gtk-quit"
            , cal->mFile_menu, cal->mAccel_group);

    /* Edit menu */
    cal->mEdit_menu = orage_menu_new(_("_Edit"), cal->mMenubar);

    cal->mEdit_preferences = 
            orage_image_menu_item_new_from_stock("gtk-preferences"
                    , cal->mEdit_menu, cal->mAccel_group);

    /* View menu */
    cal->mView_menu = orage_menu_new(_("_View"), cal->mMenubar);

    cal->mView_ViewSelectedDate = 
            orage_menu_item_new_with_mnemonic(_("View selected _date")
                    , cal->mView_menu);
    cal->mView_ViewSelectedWeek = 
            orage_menu_item_new_with_mnemonic(_("View selected _week")
                    , cal->mView_menu);

    menu_separator = orage_separator_menu_item_new(cal->mView_menu);

    cal->mView_selectToday = 
            orage_menu_item_new_with_mnemonic(_("Select _Today")
                    , cal->mView_menu);

    /* Help menu */
    cal->mHelp_menu = orage_menu_new(_("_Help"), cal->mMenubar);
    cal->mHelp_help = orage_image_menu_item_new_from_stock("gtk-help"
            , cal->mHelp_menu, cal->mAccel_group);
    cal->mHelp_about = orage_image_menu_item_new_from_stock("gtk-about"
            , cal->mHelp_menu, cal->mAccel_group);

    gtk_widget_show_all(g_par.xfcal->mMenubar);

    /* Signals */
    g_signal_connect((gpointer) cal->mFile_newApp, "activate"
            , G_CALLBACK(mFile_newApp_activate_cb),(gpointer) cal);
    g_signal_connect((gpointer) cal->mFile_interface, "activate"
            , G_CALLBACK(mFile_interface_activate_cb),(gpointer) cal);
    g_signal_connect((gpointer) cal->mFile_close, "activate"
            , G_CALLBACK(mFile_close_activate_cb),(gpointer) cal);
    g_signal_connect((gpointer) cal->mFile_quit, "activate"
            , G_CALLBACK(mFile_quit_activate_cb),(gpointer) cal);
    g_signal_connect((gpointer) cal->mEdit_preferences, "activate"
            , G_CALLBACK(mEdit_preferences_activate_cb), NULL);
    g_signal_connect((gpointer) cal->mView_ViewSelectedDate, "activate"
            , G_CALLBACK(mView_ViewSelectedDate_activate_cb),(gpointer) cal);
    g_signal_connect((gpointer) cal->mView_ViewSelectedWeek, "activate"
            , G_CALLBACK(mView_ViewSelectedWeek_activate_cb),(gpointer) cal);
    g_signal_connect((gpointer) cal->mView_selectToday, "activate"
            , G_CALLBACK(mView_selectToday_activate_cb),(gpointer) cal);
    g_signal_connect((gpointer) cal->mHelp_help, "activate"
            , G_CALLBACK(mHelp_help_activate_cb), NULL);
    g_signal_connect((gpointer) cal->mHelp_about, "activate"
            , G_CALLBACK(mHelp_about_activate_cb),(gpointer) cal);
}

void build_mainWin()
{
    GdkPixbuf *orage_logo;
    CalWin *cal = g_par.xfcal;

    /* using static icon here since this dynamic icon is not updated
     * when date changes. Could be added, but not worth it.
     * Dynamic icon is used in systray and about windows */
    orage_logo = orage_create_icon(cal, TRUE, 48, 48); 
    /* orage_logo = xfce_themed_icon_load("xfcalendar", 48); */
    /* orage_logo = create_icon(cal, 48, 48); */
    cal->mAccel_group = gtk_accel_group_new();

    gtk_window_set_title(GTK_WINDOW(cal->mWindow), _("Orage"));
    gtk_window_set_position(GTK_WINDOW(cal->mWindow), GTK_WIN_POS_NONE);
    gtk_window_set_resizable(GTK_WINDOW(cal->mWindow), FALSE);
    gtk_window_set_destroy_with_parent(GTK_WINDOW(cal->mWindow), TRUE);

    if (orage_logo != NULL) {
        gtk_window_set_icon(GTK_WINDOW(cal->mWindow), orage_logo);
        gtk_window_set_default_icon(orage_logo);
        g_object_unref(orage_logo);
    }

    /* Build the vertical box */
    cal->mVbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(cal->mWindow), cal->mVbox);
    gtk_widget_show(cal->mVbox);

    /* Build the menu */
    build_menu();

    /* Build the calendar */
    cal->mCalendar = gtk_calendar_new();
    gtk_box_pack_start(GTK_BOX(cal->mVbox), cal->mCalendar, TRUE, TRUE, 0);
    gtk_calendar_set_display_options(GTK_CALENDAR(cal->mCalendar)
            , GTK_CALENDAR_SHOW_HEADING | GTK_CALENDAR_SHOW_DAY_NAMES
            | GTK_CALENDAR_SHOW_WEEK_NUMBERS);
    gtk_widget_show(cal->mCalendar);

    /* Signals */
    g_signal_connect((gpointer) cal->mCalendar, "scroll_event"
            , G_CALLBACK(mCalendar_scroll_event_cb), NULL);

    g_signal_connect((gpointer) cal->mCalendar, "day_selected_double_click"
            , G_CALLBACK(mCalendar_day_selected_double_click_cb)
            , (gpointer) cal);

    g_signal_connect((gpointer) cal->mCalendar, "month-changed"
            , G_CALLBACK(mCalendar_month_changed_cb), (gpointer) cal);

    gtk_window_add_accel_group(GTK_WINDOW(cal->mWindow), cal->mAccel_group);

    if (g_par.pos_x || g_par.pos_y)
        gtk_window_move(GTK_WINDOW(cal->mWindow), g_par.pos_x, g_par.pos_y);
    /*
    gtk_window_stick(GTK_WINDOW(cal->mWindow));
    */
}
