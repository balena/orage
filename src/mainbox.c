/* mainbox.c
 *
 * Copyright (C) 2004-2005 Mickaël Graf <korbinus@xfce.org>
 * Parts of the code below are copyright (C) 2003 Edscott Wilson Garcia <edscott@users.sourceforge.net>
 *                                       (C) 2005 Juha Kautto <kautto.juha at kolumbus.fi>
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
#include "support.h"
#include "xfce_trayicon.h"
#include "about-xfcalendar.h"
#include "interface.h"
#include "callbacks.h"

extern gboolean open_ical_file(void);
extern void close_ical_file(void);
extern int getnextday_ical_app(int year, int month, int day);

#define LEN_BUFFER 1024
#define CHANNEL  "xfcalendar"
#define RCDIR    "xfce4" G_DIR_SEPARATOR_S "xfcalendar"

extern gboolean normalmode;
extern gint pos_x, pos_y;
static GtkCalendar *cal;

char today[8];

gboolean
xfcalendar_mark_appointments (CalWin *xfcal)
{
    guint year, month, day;

    gtk_calendar_get_date(GTK_CALENDAR(xfcal->mCalendar), &year, &month, &day);

    if (open_ical_file()){
        day = -1;
        while (day = getnextday_ical_app(year, month+1, day)){
            gtk_calendar_mark_day(cal, day);
        }
        close_ical_file();
    }

    return TRUE;
}

gboolean
mWindow_delete_event_cb(GtkWidget *widget, GdkEvent *event,
			gpointer user_data)
{

  CalWin *xfcal = (CalWin *)user_data;

  xfcal->show_Calendar = FALSE;
  xfcalendar_toggle_visible(xfcal);

  return(TRUE);

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
mSettings_preferences_activate_cb      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

  mcs_client_show(GDK_DISPLAY(), DefaultScreen(GDK_DISPLAY()), CHANNEL);

}

void
mSettings_selectToday_activate_cb      (GtkMenuItem     *menuitem,
					gpointer         user_data)
{

  CalWin *xfcal = (CalWin *)user_data;
  struct tm *t;
  time_t tt;

  tt=time(NULL);
  t=localtime(&tt);

  gtk_calendar_select_month(GTK_CALENDAR(xfcal->mCalendar), t->tm_mon, t->tm_year+1900);
  gtk_calendar_select_day(GTK_CALENDAR(xfcal->mCalendar), t->tm_mday);

}

void
mHelp_about_activate_cb (GtkMenuItem *menuitem, 
			 gpointer user_data)
{
  create_wAbout((GtkWidget *)menuitem, user_data);
}

void
mCalendar_sroll_event_cb (GtkWidget     *calendar,
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
mCalendar_day_selected_double_click_cb (GtkCalendar *calendar,
                                        gpointer user_data)
{

  GtkWidget *appointment;
  appointment = create_wAppointment();
  manageAppointment(calendar, appointment);
  gtk_widget_show(appointment);

}

void
mCalendar_month_changed_cb (GtkCalendar *calendar,
			    gpointer user_data)
{

  CalWin *xfcal = (CalWin *)user_data;

  gtk_calendar_clear_marks(GTK_CALENDAR(xfcal->mCalendar));
  xfcalendar_mark_appointments(xfcal);
}

void 
xfcalendar_init_settings (CalWin *xfcal)
{
  gchar *fpath;
  FILE *fp;
  char buf[LEN_BUFFER];

  xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  /* default */
  xfcal->show_Calendar = TRUE;
  xfcal->show_Taskbar = TRUE;
  xfcal->show_Pager = TRUE;
  xfcal->display_Options = GTK_CALENDAR_SHOW_HEADING 
    | GTK_CALENDAR_SHOW_DAY_NAMES 
    | GTK_CALENDAR_SHOW_WEEK_NUMBERS;

  fpath = xfce_resource_save_location(XFCE_RESOURCE_CONFIG,
                        RCDIR G_DIR_SEPARATOR_S "xfcalendarrc", FALSE);

  if ((fp = fopen(fpath, "r")) == NULL){
    fp = fopen(fpath, "w");
    if (fp == NULL)
      g_warning("Unable to create %s", fpath);
    else {
      fprintf(fp, "[Session Visibility]\n");
      fclose(fp);
    }
  }  else{
    fgets(buf, LEN_BUFFER, fp); /* [Window Position] */
    if (fscanf(fp, "X=%i, Y=%i", &pos_x, &pos_y) != 2)
      {
      g_warning("Unable to read position from: %s", fpath);
      }
  }
}

gboolean
xfcalendar_alarm_clock(gpointer user_data)
{
    CalWin *xfcal = (CalWin *)user_data;
    struct tm *t;
    char key[8], loc_date[8], selected_date[8];
    guint year, month, day;
    time_t tt;
    static char start_key[8]={0,0,0,0,0,0,0,0};
    char a_day[10];
    char *text;

  tt=time(NULL);
  t=localtime(&tt);
  g_snprintf(key, 8, "%03d%02d%02d", t->tm_year, t->tm_mon, t->tm_mday);
#ifdef DEBUG
  printf("at alarm %s==%s\n",key,start_key);
#endif

  /* See if the day just changed and the former current date was selected */

  /* Just for avoiding to rewrite the routine in case key has its format  
   * changed in the future.
   */
  strcpy(loc_date, key);

  if (strcmp (loc_date, today) != 0)
    {
      /* Get the selected data */
      gtk_calendar_get_date (GTK_CALENDAR (xfcal->mCalendar),
			     &year, &month, &day); 
      /* build a string of it */
      g_snprintf(selected_date, 8, "%03d%02d%02d", year-1900, month, day);

      /* if the selected date is the former today variable, changed 
       * the selected data; otherwise do nothing for keeping the date
       * selected by the user. */
      if (strcmp (selected_date, today) == 0)
	{
	  /* change today variable */
	  strcpy(today, selected_date);

	  /* select the relevant month and day in the calendar */
	  gtk_calendar_select_month(GTK_CALENDAR(xfcal->mCalendar), 
				    t->tm_mon, t->tm_year+1900);
	  gtk_calendar_select_day(GTK_CALENDAR(xfcal->mCalendar), 
				  t->tm_mday);
	}
    }

  /* Check if any appointement for the date */
    if (!strlen(start_key) || strcmp(start_key,key)!=0){
        if (open_ical_file()){
            strcpy(start_key, key);
            sprintf(a_day, "%04d%02d%02d", t->tm_year+1900, t->tm_mon+1, t->tm_mday);
            if (get_ical_app(&text, a_day)){ /* data found */
                if (strlen(text)) pretty_window(text);
            }
            close_ical_file();
        }
    }
    return TRUE;	
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

void create_mainWin(CalWin *xfcal)
{
  struct tm *t;
  time_t tt;

  GdkPixbuf *xfcalendar_logo = xfce_themed_icon_load ("xfcalendar", 48);

  xfcal->mAccel_group = gtk_accel_group_new ();

  gtk_window_set_title (GTK_WINDOW(xfcal->mWindow), _("Xfcalendar"));
  gtk_window_set_position (GTK_WINDOW (xfcal->mWindow), GTK_WIN_POS_NONE);
  gtk_window_set_resizable (GTK_WINDOW (xfcal->mWindow), FALSE);
  gtk_window_set_destroy_with_parent (GTK_WINDOW (xfcal->mWindow), TRUE);

  if(xfcalendar_logo != NULL){
    gtk_window_set_icon(GTK_WINDOW (xfcal->mWindow), xfcalendar_logo);
    g_object_unref(xfcalendar_logo);
  }

  /* Build the vertical box */
  xfcal->mVbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (xfcal->mVbox);
  gtk_container_add (GTK_CONTAINER (xfcal->mWindow), xfcal->mVbox);

  /* Build the menu */
  xfcal->mMenubar = gtk_menu_bar_new ();
  gtk_widget_show (xfcal->mMenubar);
  gtk_box_pack_start (GTK_BOX (xfcal->mVbox),
		      xfcal->mMenubar,
		      FALSE,
		      FALSE,
		      0);

  /* File menu */
  xfcal->mFile = gtk_menu_item_new_with_mnemonic (_("_File"));
  gtk_widget_show (xfcal->mFile);
  gtk_container_add (GTK_CONTAINER (xfcal->mMenubar), xfcal->mFile);

  xfcal->mFile_menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (xfcal->mFile), xfcal->mFile_menu);

  xfcal->mFile_close = gtk_menu_item_new_with_mnemonic (_("_Close window"));
  gtk_widget_show(xfcal->mFile_close);
  gtk_container_add(GTK_CONTAINER (xfcal->mFile_menu), xfcal->mFile_close);

  xfcal->mFile_separator = gtk_separator_menu_item_new ();
  gtk_widget_show (xfcal->mFile_separator);
  gtk_container_add (GTK_CONTAINER (xfcal->mFile_menu), xfcal->mFile_separator);

  xfcal->mFile_quit =  gtk_image_menu_item_new_from_stock ("gtk-quit", 
							   xfcal->mAccel_group);
  gtk_widget_show (xfcal->mFile_quit);
  gtk_container_add (GTK_CONTAINER (xfcal->mFile_menu), xfcal->mFile_quit);

  /* Settings menu */
  xfcal->mSettings =  gtk_menu_item_new_with_mnemonic(_("_Settings"));
  gtk_widget_show (xfcal->mSettings);
  gtk_container_add (GTK_CONTAINER (xfcal->mMenubar), xfcal->mSettings);

  xfcal->mSettings_menu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(xfcal->mSettings), 
			    xfcal->mSettings_menu);

  xfcal->mSettings_preferences = gtk_menu_item_new_with_mnemonic(_("_Preferences"));
  gtk_widget_show (xfcal->mSettings_preferences);
  gtk_container_add (GTK_CONTAINER (xfcal->mSettings_menu), 
		     xfcal->mSettings_preferences);

  xfcal->mSettings_separator = gtk_separator_menu_item_new();
  gtk_widget_show (xfcal->mSettings_separator);
  gtk_container_add (GTK_CONTAINER (xfcal->mSettings_menu), 
		     xfcal->mSettings_separator);


  xfcal->mSettings_selectToday = gtk_menu_item_new_with_mnemonic(_("Select _Today"));
  gtk_widget_show (xfcal->mSettings_selectToday);
  gtk_container_add (GTK_CONTAINER (xfcal->mSettings_menu), 
		     xfcal->mSettings_selectToday);

  /* Help menu */

  xfcal->mHelp = gtk_menu_item_new_with_mnemonic (_("_Help"));
  gtk_widget_show (xfcal->mHelp);
  gtk_container_add (GTK_CONTAINER (xfcal->mMenubar), xfcal->mHelp);

  xfcal->mHelp_menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (xfcal->mHelp), xfcal->mHelp_menu);

  xfcal->mHelp_about = gtk_menu_item_new_with_mnemonic (_("_About"));
  gtk_widget_show (xfcal->mHelp_about);
  gtk_container_add (GTK_CONTAINER (xfcal->mHelp_menu), xfcal->mHelp_about);

  /* Build the calendar */
  xfcal->mCalendar = gtk_calendar_new ();
  gtk_widget_show (xfcal->mCalendar);
  gtk_box_pack_start (GTK_BOX (xfcal->mVbox), xfcal->mCalendar, TRUE, TRUE, 0);
  gtk_calendar_display_options (GTK_CALENDAR (xfcal->mCalendar),
                                GTK_CALENDAR_SHOW_HEADING
                                | GTK_CALENDAR_SHOW_DAY_NAMES
                                | GTK_CALENDAR_SHOW_WEEK_NUMBERS);

  /* Signals */
  g_signal_connect ((gpointer) xfcal->mWindow, "delete_event",
		    G_CALLBACK (mWindow_delete_event_cb),
		    (gpointer) xfcal);

  g_signal_connect ((gpointer) xfcal->mFile_close, "activate",
		    G_CALLBACK (mFile_close_activate_cb),
		    (gpointer) xfcal);

  g_signal_connect ((gpointer) xfcal->mFile_quit, "activate",
		    G_CALLBACK (mFile_quit_activate_cb),
		    (gpointer) xfcal);

  g_signal_connect ((gpointer) xfcal->mSettings_preferences, "activate",
		    G_CALLBACK (mSettings_preferences_activate_cb),
		    NULL);

  g_signal_connect ((gpointer) xfcal->mSettings_selectToday, "activate",
		    G_CALLBACK (mSettings_selectToday_activate_cb),
		    (gpointer) xfcal);

  g_signal_connect ((gpointer) xfcal->mHelp_about, "activate",
		    G_CALLBACK (mHelp_about_activate_cb),
		    (gpointer) xfcal);

  g_signal_connect ((gpointer) xfcal->mCalendar, "scroll_event",
		    G_CALLBACK (mCalendar_sroll_event_cb),
		    NULL);

  g_signal_connect ((gpointer) xfcal->mCalendar, "day_selected_double_click",
		    G_CALLBACK (mCalendar_day_selected_double_click_cb),
		    NULL);

  g_signal_connect ((gpointer) xfcal->mCalendar, "month-changed",
		    G_CALLBACK (mCalendar_month_changed_cb),
		    (gpointer) xfcal);

  gtk_window_add_accel_group (GTK_WINDOW (xfcal->mWindow), xfcal->mAccel_group);


  xfcalendar_init_settings (xfcal);

  cal = GTK_CALENDAR(xfcal->mCalendar); //horrible hack :(
  xfcalendar_mark_appointments (xfcal);

  g_timeout_add_full(0, 
		     5000, 
		     (GtkFunction) xfcalendar_alarm_clock, 
		     (gpointer) xfcal, 
		     //(gpointer) xfcal->mWindow, 
		     NULL);

  /* Remember which day we are */
  tt=time(NULL);
  t=localtime(&tt);
  g_snprintf(today, 8, "%03d%02d%02d", t->tm_year, t->tm_mon, t->tm_mday);

}
