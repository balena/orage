/* xfcalendar
 *
 * Copyright (C) 2003 Mickael Graf (korbinus@linux.se)
 * Copyright (C) 2003 edscott wilson garcia <edscott@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  This program is distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.  You
 * should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <unistd.h>
#include <time.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfcegui4/netk-trayicon.h>
#include <libxfce4mcs/mcs-client.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "reminder.h"
#include "about-xfcalendar.h"
#include "mainbox.h"
#include "appointment.h"
#include "ical-code.h"


#define MAX_APP_LENGTH 4096
#define LEN_BUFFER 1024
#define CHANNEL  "xfcalendar"
#define RCDIR    "xfce4" G_DIR_SEPARATOR_S "xfcalendar"
#define APPOINTMENT_FILE "appointments.ics"

extern gint pos_x, pos_y;

/* MCS client */
McsClient        *client = NULL;

static GtkWidget *clearwarn;

extern CalWin *xfcal;


/* Direction for changing day to look at */
enum{
  PREVIOUS,
    NEXT
    };

void apply_settings()
{
  gchar *fpath;
  FILE *fp;

  xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  gtk_calendar_display_options (GTK_CALENDAR (xfcal->mCalendar), xfcal->display_Options);

  /* Save settings here */
  /* I know, it's bad(tm) */
  fpath = xfce_resource_save_location(XFCE_RESOURCE_CONFIG,
                RCDIR G_DIR_SEPARATOR_S "xfcalendarrc", FALSE);
  if ((fp = fopen(fpath, "w")) == NULL){
    g_warning("Unable to open RC file.");
  }else {
    fprintf(fp, "[Window Position]\n");
    gtk_window_get_position(GTK_WINDOW(xfcal->mWindow), &pos_x, &pos_y);
    fprintf(fp, "X=%i, Y=%i\n", pos_x, pos_y);
    fclose(fp);
  }
  g_free(fpath);
}

void pretty_window(char *text){
	GtkWidget *reminder;
	reminder = create_wReminder(text);
	gtk_widget_show(reminder);
}

int keep_tidy(void){
    /* we could move old appointment to other file to keep the active
       calendar file smaller and faster */
	return TRUE;	
}


void editAppointment(GtkWidget *widget, GdkEventButton *event
            , GtkWidget *control)
{
    AppWin *app;

    /*
    if (event->button == 1)
    */
    app = create_appWin("2005", "03", "10");
    gtk_widget_show(app->appWindow);
}

void addAppointment(GtkWidget *vbox, gchar *xftime, gchar *xftext
        , gchar *xfsum)
{
    GtkWidget *hseparator;
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *ebox;
    GtkTooltips *event_tooltips;
    int len=-1;
    gchar *text;

    hseparator = gtk_hseparator_new();
    gtk_widget_show(hseparator);
    gtk_box_pack_start(GTK_BOX(vbox), hseparator, FALSE, FALSE, 0);
                                                                                
    ebox = gtk_event_box_new();
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(ebox), FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), ebox, FALSE, FALSE, 0);
    gtk_widget_show(ebox);
                                                                                
    hbox = gtk_hbox_new(FALSE, 5);
    gtk_widget_show(hbox);
    gtk_container_add(GTK_CONTAINER(ebox), hbox);

    text = g_strconcat("<b>",xftime,"</b>",NULL);
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), text);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_widget_show(label);
    g_free(text);

    if (xfsum != NULL)
        text = xfsum;
    else { /* let's take first line from the text */
        if ((text = g_strstr_len(xftext, strlen(xftext), "\n")) != NULL) {
            len=strlen(xftext)-strlen(text);
            text = g_strndup(xftext, len);
        }
        else
            text=xftext;
    }
    label = gtk_label_new(text);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_widget_show(label);
    if (len != -1) 
        g_free(text);

    event_tooltips = gtk_tooltips_new();
    gtk_tooltips_set_tip(event_tooltips, ebox, xftext, NULL);

    g_signal_connect (ebox, "button-press-event",
            G_CALLBACK(editAppointment), NULL);
}

void manageAppointment(GtkCalendar *calendar, GtkWidget *appwin)
{
	guint year, month, day;
	char title[12];
	char a_day[9];  /* yyyymmdd */
	char a_time[12]=""; /* hh:mm-hh:mm */
    GtkWidget *vbox;
    appointment app;
  
	gtk_calendar_get_date(calendar, &year, &month, &day);
	g_snprintf(title, 12, "%d-%02d-%02d", year, month+1, day);
	gtk_window_set_title(GTK_WINDOW(appwin), _(title));
	vbox = lookup_widget(GTK_WIDGET(appwin), "vbox3");

    if (open_ical_file()){
        g_snprintf(a_day, 9, "%04d%02d%02d", year, month+1, day);
        app.note=NULL; app.title=NULL;
        while (get_ical_app(&app, a_day, a_time)){ 
            /* data found */
            addAppointment(vbox, a_time, app.note, (gchar*) app.title);
        }
        close_ical_file();
    }
}

void
on_btClose_clicked(GtkButton *button, gpointer user_data)
{
  GtkWidget *a=lookup_widget((GtkWidget *)button,"wAppointment");

  gtk_widget_destroy(a); /* destroy the specific appointment window */
}

gint 
dialogWin(gpointer user_data)
{
  GtkWidget *dialog, *message;
  dialog = gtk_dialog_new_with_buttons (_("Question"),
					GTK_WINDOW(user_data),
					GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_STOCK_NO,
					GTK_RESPONSE_REJECT,
					GTK_STOCK_YES,
					GTK_RESPONSE_ACCEPT,
					NULL);

  message = gtk_label_new(_("\nThe information has been modified.\n Do you want to continue ?\n"));

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), message);
  gtk_widget_show_all(dialog);
  gint result = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  return result;
}

gboolean 
on_wAppointment_delete_event(GtkWidget *widget, GdkEvent *event,
                             gpointer user_data)
{
  gtk_widget_destroy(widget); /* destroy the appointment window */
  return(FALSE);
}

void
on_btPrevious_clicked(GtkButton *button, gpointer user_data)
{
  changeSelectedDate(button, PREVIOUS);
}

void
on_btToday_clicked(GtkButton *button, gpointer user_data)
{
  struct tm *t;
  time_t tt;
  GtkWidget *appointment; 
	
  recreate_wAppointment(GTK_WIDGET(button));
  appointment = lookup_widget(GTK_WIDGET(button),"wAppointment");

  tt=time(NULL);
  t=localtime(&tt);

  gtk_calendar_select_month(GTK_CALENDAR(xfcal->mCalendar), t->tm_mon, t->tm_year+1900);
  gtk_calendar_select_day(GTK_CALENDAR(xfcal->mCalendar), t->tm_mday);
  manageAppointment(GTK_CALENDAR(xfcal->mCalendar), appointment);
}

void
on_btNext_clicked(GtkButton *button, gpointer user_data)
{
  changeSelectedDate(button, NEXT);
}

void
changeSelectedDate(GtkButton *button, gint direction){
  guint year, month, day;
  guint monthdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  GtkWidget *appointment; 
	
  recreate_wAppointment(GTK_WIDGET(button));
  appointment = lookup_widget(GTK_WIDGET(button),"wAppointment");

  gtk_calendar_get_date(GTK_CALENDAR(xfcal->mCalendar), &year, &month, &day);

  if(bisextile(year)){
    ++monthdays[1];
  }
  switch(direction){
  case PREVIOUS:
    if(--day == 0){
      if(--month == -1){
	--year;
	month = 11;
      }
      gtk_calendar_select_month(GTK_CALENDAR(xfcal->mCalendar), month, year);
      day = monthdays[month];
    }
    break;
  case NEXT:
    if(++day == (monthdays[month]+1)){
      if(++month == 12){
	++year;
	month = 0;
      }
      gtk_calendar_select_month(GTK_CALENDAR(xfcal->mCalendar), month, year);
      day = 1;
    }
    break;
  default:
    break;
  }
  gtk_calendar_select_day(GTK_CALENDAR(xfcal->mCalendar), day);
  manageAppointment(GTK_CALENDAR(xfcal->mCalendar), appointment);
}

gboolean
bisextile(guint year)
{
  return ((year%4)==0)&&(((year%100)!=0)||((year%400)==0));
}

void
on_btDelete_clicked(GtkButton *button, gpointer user_data)
{
	GtkWidget *w;
	GtkWidget *appointment; 
	
	appointment = lookup_widget(GTK_WIDGET(button),"wAppointment");

	clearwarn = create_wClearWarn(appointment);
	w=lookup_widget(clearwarn,"okbutton2");
	/* we connect here instead of in glade to pass the data field */
	g_signal_connect ((gpointer) w, "clicked",
                    G_CALLBACK (on_okbutton2_clicked),
                    (gpointer)button);
	gtk_widget_show(clearwarn);
}

void
on_btCreate_clicked(GtkButton *button, gpointer user_data)
{
    AppWin *app;

    app = create_appWin("2005", "03", "10");
    gtk_widget_show(app->appWindow);
}

void
on_cancelbutton1_clicked(GtkButton *button, gpointer user_data)
{
#ifdef DEBUG
	g_print("Clear textbuffer not chosen (pffiou!)\n");
#endif
	gtk_widget_destroy(clearwarn);
}


void
on_okbutton2_clicked(GtkButton *button, gpointer user_data)
{
	GtkTextView *tv;
	GtkTextBuffer *tb;
	GtkTextIter start, end;
    char a_day[10];
    GtkWidget *a;
    char *key;
    guint day;
	
	gtk_widget_destroy(clearwarn);
	
#ifdef DEBUG
	g_print("Clear textbuffer chosen (oops!)\n");
#endif

    if (open_ical_file()){
		a=lookup_widget((GtkWidget *)user_data,"wAppointment");
		key = (char*)gtk_window_get_title(GTK_WINDOW (a));
        a_day[0]=key[0]; a_day[1]=key[1];           /* yy   */
                a_day[2]=key[2]; a_day[3]=key[3];   /*   yy */
        a_day[4]=key[5]; a_day[5]=key[6];           /* mm */
        a_day[6]=key[8]; a_day[7]=key[9];           /* dd */
        a_day[8]=key[10];                           /* \0 */
        rm_ical_app(a_day);
        close_ical_file();

        day = atoi(a_day+6);
		gtk_calendar_unmark_day(GTK_CALENDAR(xfcal->mCalendar), day);
        recreate_wAppointment(user_data);
    }
}

GdkFilterReturn
client_event_filter(GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
    if(mcs_client_process_event(client, (XEvent *) xevent))
        return GDK_FILTER_REMOVE;
    else
        return GDK_FILTER_CONTINUE;
}

