/* reminder.c
 *
 * (C) 2003-2005 Mickaël Graf
 * (C) 2005      Juha Kautto 
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
#include <string.h>
#include <stdio.h>
#include <time.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfcegui4/dialogs.h>

#include "mainbox.h"
#include "appointment.h"
#include "reminder.h"
#include "ical-code.h"

extern GList *alarm_list;

void
create_soundReminder(char *sound)
{
    GError *error;
    gboolean status;
    char play_cmd[1000]="play ";

    strcat(play_cmd, sound);
    status=xfce_exec(play_cmd, FALSE, FALSE, &error);
    if (!status)
        g_warning("play failed\n");
}

void
on_btOkReminder_clicked(GtkButton *button, gpointer user_data)
{
  GtkWidget *wReminder = (GtkWidget *)user_data;
  gtk_widget_destroy(wReminder); /* destroy the specific appointment window */
}

void
on_btOpenReminder_clicked(GtkButton *button, gpointer user_data)
{
  GtkWidget *wReminder = (GtkWidget *)user_data;
  gtk_widget_destroy(wReminder); /* destroy the specific appointment window */
}

void
create_wReminder(char *title, char *text)
{
  GtkWidget *wReminder;
  GtkWidget *vbReminder;
  GtkWidget *lbReminder;
  GtkWidget *daaReminder;
  GtkWidget *btOpenReminder;
  GtkWidget *btOkReminder;
  GtkWidget *swReminder;
  GtkWidget *hdReminder;
  char heading[200];

  wReminder = gtk_dialog_new ();
  gtk_widget_set_size_request (wReminder, 300, 250);
  strcpy(heading,  _("Reminder "));
  gtk_window_set_title (GTK_WINDOW (wReminder),  heading);
  gtk_window_set_position (GTK_WINDOW (wReminder), GTK_WIN_POS_CENTER);
  gtk_window_set_modal (GTK_WINDOW (wReminder), FALSE);
  gtk_window_set_resizable (GTK_WINDOW (wReminder), TRUE);

  vbReminder = GTK_DIALOG (wReminder)->vbox;
  gtk_widget_show (vbReminder);

  strncat(heading, title, 50);
  hdReminder = xfce_create_header(NULL, heading);
  gtk_widget_show(hdReminder);
  gtk_box_pack_start (GTK_BOX (vbReminder), hdReminder, FALSE, TRUE, 0);

  swReminder = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(swReminder), GTK_SHADOW_NONE);
  gtk_widget_show (swReminder);
  gtk_box_pack_start (GTK_BOX (vbReminder), swReminder, TRUE, TRUE, 5);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(swReminder), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  lbReminder = gtk_label_new (_(text));
  gtk_label_set_line_wrap(GTK_LABEL(lbReminder), TRUE);
  gtk_widget_show (lbReminder);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(swReminder), lbReminder);

  daaReminder = GTK_DIALOG (wReminder)->action_area;
  gtk_dialog_set_has_separator(GTK_DIALOG(wReminder), FALSE);
  gtk_widget_show (daaReminder);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (daaReminder), GTK_BUTTONBOX_END);

/*
  btOpenReminder = gtk_button_new_from_stock ("gtk-open");
  gtk_widget_show (btOpenReminder);
  gtk_dialog_add_action_widget (GTK_DIALOG (wReminder), btOpenReminder, GTK_RESPONSE_OK);
*/

  btOkReminder = gtk_button_new_from_stock ("gtk-close");
  gtk_widget_show (btOkReminder);
  gtk_dialog_add_action_widget (GTK_DIALOG (wReminder), btOkReminder, GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS (btOkReminder, GTK_CAN_DEFAULT);

  g_signal_connect((gpointer) btOpenReminder, "clicked",
            G_CALLBACK (on_btOpenReminder_clicked),
            wReminder);

  g_signal_connect ((gpointer) btOkReminder, "clicked",
		    G_CALLBACK (on_btOkReminder_clicked),
		    wReminder);

  gtk_widget_show (wReminder);
}

gboolean
xfcalendar_alarm_clock(gpointer user_data)
{
    CalWin *xfcal = (CalWin *)user_data;
    time_t tt;
    struct tm *t;
    static guint previous_year=0, previous_month=0, previous_day=0;
    guint selected_year=0, selected_month=0, selected_day=0;
    guint current_year=0, current_month=0, current_day=0;
    GList *alarm_l;
    alarm_struct *cur_alarm;
    gboolean alarm_raised=FALSE;
                                                                                
    tt=time(NULL);
    t=localtime(&tt);
  /* See if the day just changed and the former current date was selected */
    if (previous_day != t->tm_mday) {
        current_year  = t->tm_year + 1900;
        current_month = t->tm_mon;
        current_day   = t->tm_mday;
      /* Get the selected data from calendar */
        gtk_calendar_get_date(GTK_CALENDAR (xfcal->mCalendar),
                 &selected_year, &selected_month, &selected_day);
        if (selected_year == previous_year 
        && selected_month == previous_month 
        && selected_day == previous_day) {
            /* previous day was indeed selected, 
               keep it current automatically */
            gtk_calendar_select_month(GTK_CALENDAR(xfcal->mCalendar),
                    current_month, current_year);
            gtk_calendar_select_day(GTK_CALENDAR(xfcal->mCalendar),
                    current_day);
        }
        previous_year  = current_year;
        previous_month = current_month;
        previous_day   = current_day;
        xfical_alarm_build_list(TRUE);  /* new alarm list when date changed */
    }

  /* Check if there are any alarms to show */
    alarm_l=alarm_list;
    for (alarm_l = g_list_first(alarm_l);
         alarm_l != NULL;
         alarm_l = g_list_next(alarm_l)) {
        cur_alarm = (alarm_struct *)alarm_l->data;
        if (ical_alarm_passed(cur_alarm->alarm_time->str)) {
            if (strcmp(cur_alarm->action->str, "DISPLAY") == 0)
                create_wReminder(cur_alarm->title->str
                                ,cur_alarm->description->str);
            else if (strcmp(cur_alarm->action->str, "AUDIO") == 0)
                create_soundReminder(cur_alarm->sound->str);
            else
                g_warning("Unsupported VALARM ACTION: %s in: %s\n"
                        , cur_alarm->action->str, cur_alarm->uid->str);
            alarm_raised = TRUE;
        }
    }
    if (alarm_raised) /* at least one alarm processed, need new list */
        xfical_alarm_build_list(FALSE); 

    return TRUE;
}

