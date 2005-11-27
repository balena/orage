/* mainbox.c
 *
 * Copyright (C) 2004-2005 Mickaël Graf <korbinus at xfce.org>
 * (C) 2003 Edscott Wilson Garcia <edscott at users.sourceforge.net>
 * (C) 2005 Juha Kautto <kautto.juha at kolumbus.fi>
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
#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/netk-trayicon.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4mcs/mcs-client.h>

#include "mainbox.h"
#include "functions.h"
#include "xfce_trayicon.h"
#include "about-xfcalendar.h"
#include "event-list.h"
#include "appointment.h"
#include "ical-code.h"
#include "reminder.h"


#define LEN_BUFFER 1024
#define CHANNEL  "orage"
#define RCDIR    "xfce4" G_DIR_SEPARATOR_S "xfcalendar"

void apply_settings(void);

extern CalWin *xfcal;
extern gboolean normalmode;
extern gint pos_x, pos_y;
extern gint event_win_size_x, event_win_size_y;

gboolean
xfcalendar_mark_appointments()
{
    guint year, month, day;

    if (xfical_file_open()){
        gtk_calendar_get_date(GTK_CALENDAR(xfcal->mCalendar), &year, &month
                            , &day);
        xfical_mark_calendar(GTK_CALENDAR(xfcal->mCalendar), year, month+1); 
        xfical_file_close();
    }

    return TRUE;
}

gboolean
mWindow_delete_event_cb(GtkWidget *widget, GdkEvent *event,
			gpointer user_data)
{

  CalWin *xfcal = (CalWin *)user_data;

  xfcalendar_toggle_visible(xfcal);

  return(TRUE);
}

void
mFile_newApp_activate_cb(GtkMenuItem *menuitem, 
			 gpointer user_data)
{
  appt_win *app;
  struct tm *t;
  time_t tt;
  char cur_date[9];

  tt=time(NULL);
  t=localtime(&tt);
  g_snprintf(cur_date, 9, "%04d%02d%02d", t->tm_year+1900
            , t->tm_mon+1, t->tm_mday);
  app = create_appt_win("NEW", cur_date, NULL);  
  gtk_widget_show(app->appWindow);
}

void
mFile_openArchive_activate_cb (GtkMenuItem *menuitem,
            gpointer user_data)
{
    CalWin *xfcal = (CalWin *) user_data;
    GtkWidget *file_chooser;
    XfceFileFilter *filter;
    gchar *archive_path;

    /* Create file chooser */
    file_chooser = xfce_file_chooser_new (_("Select a file..."),
                                            GTK_WINDOW (xfcal->mWindow),
                                            XFCE_FILE_CHOOSER_ACTION_OPEN,
                                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                            GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                            NULL);
    /* Add filters */
    filter = xfce_file_filter_new ();
	xfce_file_filter_set_name(filter, _("Calendar files"));
	xfce_file_filter_add_pattern(filter, "*.ics");
	xfce_file_chooser_add_filter(XFCE_FILE_CHOOSER(file_chooser), filter);
	xfce_file_filter_set_name(filter, _("All Files"));
	xfce_file_filter_add_pattern(filter, "*");
	xfce_file_chooser_add_filter(XFCE_FILE_CHOOSER(file_chooser), filter);

	if(gtk_dialog_run(GTK_DIALOG(file_chooser)) == GTK_RESPONSE_ACCEPT) {
		archive_path = xfce_file_chooser_get_filename(XFCE_FILE_CHOOSER(file_chooser));

        if(archive_path){
            set_ical_path (archive_path);
            gtk_widget_set_sensitive (xfcal->mFile_closeArchive, TRUE);
        }
    }

    gtk_widget_destroy(file_chooser);
    xfcalendar_mark_appointments();
}

void
mFile_closeArchive_activate_cb (GtkMenuItem *menuitem,
            gpointer user_data)
{
    CalWin *xfcal = (CalWin *) user_data;

    set_default_ical_path ();
    gtk_widget_set_sensitive (xfcal->mFile_closeArchive, FALSE);
    xfcalendar_mark_appointments();
}

void
mFile_close_activate_cb (GtkMenuItem *menuitem, 
			 gpointer user_data)
{
  CalWin *xfcal = (CalWin *)user_data;

  gtk_widget_hide(xfcal->mWindow);
}

void
mFile_quit_activate_cb (GtkMenuItem *menuitem, 
			gpointer user_data)
{
  gtk_main_quit();
}

void 
mEdit_preferences_activate_cb(GtkMenuItem *menuitem,
            gpointer user_data)
{
  mcs_client_show(GDK_DISPLAY(), DefaultScreen(GDK_DISPLAY()), CHANNEL);
}

void
mView_ViewSelectedDate_activate_cb(GtkMenuItem *menuitem,
            gpointer user_data)
{
    CalWin *xfcal = (CalWin *) user_data;
    eventlist_win *el;

    el = create_eventlist_win();
    manage_eventlist_win(GTK_CALENDAR(xfcal->mCalendar), el);
}

void
mView_selectToday_activate_cb(GtkMenuItem *menuitem,
            gpointer user_data)
{
  CalWin *xfcal = (CalWin *)user_data;
  struct tm *t;
  time_t tt;

  tt=time(NULL);
  t=localtime(&tt);
    xfcalendar_select_date (GTK_CALENDAR (xfcal->mCalendar), t->tm_year+1900, t->tm_mon, t->tm_mday);

}

void
mHelp_help_activate_cb (GtkMenuItem *menuitem, gpointer user_data)
{
    xfce_exec("xfhelp4 xfce4-user-guide.html", FALSE, FALSE, NULL);
}

void
mHelp_about_activate_cb (GtkMenuItem *menuitem, 
			 gpointer user_data)
{
  create_wAbout((GtkWidget *)menuitem, user_data);
}

void
mCalendar_scroll_event_cb (GtkWidget     *calendar,
			  GdkEventScroll *event)
{
  guint year, month, day;
  gtk_calendar_get_date(GTK_CALENDAR(calendar), &year, &month, &day);

   switch(event->direction)
    {
	case GDK_SCROLL_UP:
	    if(--month == -1){
	      month = 11;
	      --year;
	    }
	    gtk_calendar_select_month(GTK_CALENDAR(calendar), month, year);
	    break;
	case GDK_SCROLL_DOWN:
	    if(++month == 12){
	      month = 0;
	      ++year;
	    }
	    gtk_calendar_select_month(GTK_CALENDAR(calendar), month, year);
	    break;
	default:
	  g_print("get scroll event!!!");
    }
}

void
mCalendar_day_selected_double_click_cb(GtkCalendar *calendar,
                                        gpointer user_data)
{
    CalWin *xfcal = (CalWin *) user_data;
    eventlist_win *el;

    el = create_eventlist_win();
    manage_eventlist_win(GTK_CALENDAR(xfcal->mCalendar), el);
}

void
mCalendar_month_changed_cb(GtkCalendar *calendar,
			    gpointer user_data)
{
  /*
  CalWin *xfcal = (CalWin *)user_data;
  */

  xfcalendar_mark_appointments();
}

void 
xfcalendar_init_settings(CalWin *xfcal)
{
  gchar *fpath;
  FILE *fp;
  char buf[LEN_BUFFER];

    xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    fpath = xfce_resource_save_location(XFCE_RESOURCE_CONFIG,
                        RCDIR G_DIR_SEPARATOR_S "oragerc", FALSE);

    if ((fp = fopen(fpath, "r")) == NULL) {
        fp = fopen(fpath, "w");
        if (fp == NULL)
            g_warning("Unable to create %s", fpath);
        else {
            fclose(fp);
            apply_settings();
        }
    }  
    else {
        fgets(buf, LEN_BUFFER, fp); /* [Main Window Position] */
        fgets(buf, LEN_BUFFER, fp); 
        if (sscanf(buf, "X=%i, Y=%i", &pos_x, &pos_y) != 2) {
            g_warning("Unable to read position from: %s", fpath);
        }
        fgets(buf, LEN_BUFFER, fp); /* [Event Window Size] */
        fgets(buf, LEN_BUFFER, fp); 
        if (sscanf(buf, "X=%i, Y=%i", &event_win_size_x, &event_win_size_y) 
            != 2) {
            g_warning("Unable to read size from: %s", fpath);
        }
        fgets(buf, LEN_BUFFER, fp); /* [TIMEZONE] */
        fgets(buf, LEN_BUFFER, fp); 
        if (strncmp(buf, "UTC", 3) == 0) 
            xfical_set_local_timezone("UTC");
        else if (strncmp(buf, "floating", 8) == 0)
            xfical_set_local_timezone(NULL);
        else {/* real timezone */
            if (strlen(buf) && buf[strlen(buf)-1] == '\n')
                buf[strlen(buf)-1]='\0'; /* remove '\n' */
            xfical_set_local_timezone(buf);
        }
    }
}

void
xfcalendar_toggle_visible ()
{

  GdkEventClient gev;
  Window xwindow;
  GdkAtom atom;
                                                                                
  memset(&gev, 0, sizeof(gev));
                                                                                
  atom = gdk_atom_intern("_XFCE_CALENDAR_RUNNING", FALSE);
  xwindow = XGetSelectionOwner(GDK_DISPLAY(), gdk_x11_atom_to_xatom(atom));

  gev.type = GDK_CLIENT_EVENT;
  gev.window = NULL;
  gev.send_event = TRUE;
  gev.message_type = gdk_atom_intern("_XFCE_CALENDAR_TOGGLE_HERE", FALSE);
  gev.data_format = 8;
                                                                                
  gdk_event_send_client_message((GdkEvent *)&gev, (GdkNativeWindow)xwindow);
}

void create_mainWin()
{
    GtkWidget *menu_separator;
    GdkPixbuf *xfcalendar_logo = xfce_themed_icon_load ("xfcalendar", 48);

    xfcal->mAccel_group = gtk_accel_group_new ();

    gtk_window_set_title (GTK_WINDOW(xfcal->mWindow), _("Orage"));
    gtk_window_set_position (GTK_WINDOW (xfcal->mWindow), GTK_WIN_POS_NONE);
    gtk_window_set_resizable (GTK_WINDOW (xfcal->mWindow), FALSE);
    gtk_window_set_destroy_with_parent (GTK_WINDOW (xfcal->mWindow), TRUE);

    if (xfcalendar_logo != NULL) {
      gtk_window_set_icon (GTK_WINDOW (xfcal->mWindow), xfcalendar_logo);
      g_object_unref (xfcalendar_logo);
    }

    /* Build the vertical box */
    xfcal->mVbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (xfcal->mWindow), xfcal->mVbox);

    /* Build the menu */
    xfcal->mMenubar = gtk_menu_bar_new ();
    gtk_box_pack_start (GTK_BOX (xfcal->mVbox),
                        xfcal->mMenubar,
                        FALSE,
                        FALSE,
                        0);

    /* File menu */
    xfcal->mFile_menu = xfcalendar_menu_new(_("_File"), xfcal->mMenubar);

    /*xfcal->mFile_newApp = xfcalendar_menu_item_new_with_mnemonic(_("_New appointment"), xfcal->mFile_menu);*/
    xfcal->mFile_newApp = xfcalendar_image_menu_item_new_from_stock ("gtk-new", xfcal->mFile_menu, xfcal->mAccel_group);

    menu_separator = xfcalendar_separator_menu_item_new (xfcal->mFile_menu);
 
    xfcal->mFile_openArchive = xfcalendar_menu_item_new_with_mnemonic (_("Open archive file..."), xfcal->mFile_menu);
    xfcal->mFile_closeArchive = xfcalendar_menu_item_new_with_mnemonic (_("Close archive file"), xfcal->mFile_menu);
    gtk_widget_set_sensitive (xfcal->mFile_closeArchive, FALSE);

    menu_separator = xfcalendar_separator_menu_item_new (xfcal->mFile_menu);

    xfcal->mFile_close = xfcalendar_image_menu_item_new_from_stock ("gtk-close", xfcal->mFile_menu, xfcal->mAccel_group);

    xfcal->mFile_quit = xfcalendar_image_menu_item_new_from_stock ("gtk-quit", xfcal->mFile_menu, xfcal->mAccel_group);

    /* Edit menu */
    xfcal->mEdit_menu = xfcalendar_menu_new(_("_Edit"), xfcal->mMenubar);

    xfcal->mEdit_preferences = xfcalendar_image_menu_item_new_from_stock("gtk-preferences", xfcal->mEdit_menu, xfcal->mAccel_group);

    /* View menu */
    xfcal->mView_menu = xfcalendar_menu_new (_("_View"), xfcal->mMenubar);

    xfcal->mView_ViewSelectedDate = xfcalendar_menu_item_new_with_mnemonic (_("View selected _date"), xfcal->mView_menu);

    menu_separator = xfcalendar_separator_menu_item_new (xfcal->mView_menu);

    xfcal->mView_selectToday = xfcalendar_menu_item_new_with_mnemonic(_("Select _Today"), xfcal->mView_menu);

    /* Help menu */
    xfcal->mHelp_menu = xfcalendar_menu_new(_("_Help"), xfcal->mMenubar);
    xfcal->mHelp_help = xfcalendar_image_menu_item_new_from_stock ("gtk-help", xfcal->mHelp_menu, xfcal->mAccel_group);
    xfcal->mHelp_about = xfcalendar_image_menu_item_new_from_stock ("gtk-about", xfcal->mHelp_menu, xfcal->mAccel_group);

    /* Build the calendar */
    xfcal->mCalendar = gtk_calendar_new ();
    gtk_box_pack_start (GTK_BOX (xfcal->mVbox), xfcal->mCalendar, TRUE, TRUE, 0);
    gtk_calendar_set_display_options (GTK_CALENDAR (xfcal->mCalendar),
                                  GTK_CALENDAR_SHOW_HEADING
                                  | GTK_CALENDAR_SHOW_DAY_NAMES
                                  | GTK_CALENDAR_SHOW_WEEK_NUMBERS);

    /* Signals */
    g_signal_connect ((gpointer) xfcal->mWindow, "delete_event",
	  	    G_CALLBACK (mWindow_delete_event_cb),
		    (gpointer) xfcal);

    g_signal_connect ((gpointer) xfcal->mFile_newApp, "activate",
		    G_CALLBACK (mFile_newApp_activate_cb),
		    (gpointer) xfcal);

    g_signal_connect ((gpointer) xfcal->mFile_openArchive, "activate",
            G_CALLBACK (mFile_openArchive_activate_cb),
            (gpointer) xfcal);

    g_signal_connect ((gpointer) xfcal->mFile_closeArchive, "activate",
            G_CALLBACK (mFile_closeArchive_activate_cb),
            (gpointer) xfcal);

    g_signal_connect ((gpointer) xfcal->mFile_close, "activate",
		    G_CALLBACK (mFile_close_activate_cb),
		    (gpointer) xfcal);

    g_signal_connect ((gpointer) xfcal->mFile_quit, "activate",
		    G_CALLBACK (mFile_quit_activate_cb),
		    (gpointer) xfcal);

    g_signal_connect ((gpointer) xfcal->mEdit_preferences, "activate",
		    G_CALLBACK (mEdit_preferences_activate_cb),
		    NULL);

    g_signal_connect ((gpointer) xfcal->mView_ViewSelectedDate, "activate",
		    G_CALLBACK (mView_ViewSelectedDate_activate_cb),
		    (gpointer) xfcal);

    g_signal_connect ((gpointer) xfcal->mView_selectToday, "activate",
		    G_CALLBACK (mView_selectToday_activate_cb),
		    (gpointer) xfcal);

    g_signal_connect ((gpointer) xfcal->mHelp_help, "activate",
                      G_CALLBACK (mHelp_help_activate_cb),
                      NULL);

    g_signal_connect ((gpointer) xfcal->mHelp_about, "activate",
		    G_CALLBACK (mHelp_about_activate_cb),
		    (gpointer) xfcal);

    g_signal_connect ((gpointer) xfcal->mCalendar, "scroll_event",
		    G_CALLBACK (mCalendar_scroll_event_cb),
		    NULL);

    g_signal_connect ((gpointer) xfcal->mCalendar, "day_selected_double_click",
		    G_CALLBACK (mCalendar_day_selected_double_click_cb),
            (gpointer) xfcal);

    g_signal_connect ((gpointer) xfcal->mCalendar, "month-changed",
		    G_CALLBACK (mCalendar_month_changed_cb),
		    (gpointer) xfcal);

    gtk_window_add_accel_group (GTK_WINDOW (xfcal->mWindow), xfcal->mAccel_group);


    xfcalendar_init_settings (xfcal);
                                                                                
    if (pos_x || pos_y)
        gtk_window_move (GTK_WINDOW (xfcal->mWindow), pos_x, pos_y);
    gtk_window_stick (GTK_WINDOW (xfcal->mWindow));

    xfcalendar_mark_appointments();

}
