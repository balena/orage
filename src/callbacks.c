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
#include <dbh.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "reminder.h"
#include "about-xfcalendar.h"
#include "mainbox.h"

#define MAX_APP_LENGTH 4096
#define LEN_BUFFER 1024
/* FIXME: check if we can remove this */
#define CHANNEL  "xfcalendar"
#define RCDIR    "xfce4" G_DIR_SEPARATOR_S "xfcalendar"

#define SUNDAY TRUE
#define MONDAY FALSE

extern gboolean normalmode;

/*
static gboolean showtaskbar = TRUE;
static gboolean showpager = TRUE;
*/

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
    fprintf(fp, "[Session Visibility]\n");
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
	/* keep a tidy DBHashTable */
	DBHashTable *fapp;
	char *fpath = xfce_resource_save_location(XFCE_RESOURCE_CONFIG,
                RCDIR G_DIR_SEPARATOR_S "appointments.dbh", FALSE);
	if ((fapp = DBH_open(fpath)) != NULL){
		char *wd = g_path_get_dirname (fpath);
#ifdef DEBUG
		printf("wd=%s\n",wd);
#endif
		chdir(wd);
		fapp=DBH_regen(fapp);
		DBH_close(fapp);
		g_free(wd);
	}
	g_free(fpath);
	return TRUE;	
}

void manageAppointment(GtkCalendar *calendar, GtkWidget *appointment)
{
	guint year, month, day;
	char title[12], *text;
	gchar *fpath;
	DBHashTable *fapp;
	GtkTextView *tv;
	GtkTextBuffer *tb = gtk_text_buffer_new(NULL);
	GtkTextIter *end;
  
	gtk_calendar_get_date(calendar, &year, &month, &day);
	g_snprintf(title, 12, "%d-%02d-%02d", year, month+1, day);
	fpath = xfce_resource_save_location(XFCE_RESOURCE_CONFIG,
                        RCDIR G_DIR_SEPARATOR_S "appointments.dbh", FALSE);
	tv = GTK_TEXT_VIEW(lookup_widget(GTK_WIDGET(appointment),"textview1"));

	/* Tell DBH that we will work with records of MAX_APP_LENGTH maximum
	 * size (the DBH default is 1024) */
	DBH_Size(NULL,MAX_APP_LENGTH);
	
	/* DBH key should have year,month,day format to permit sweep of
	 * year,month branch to get list of marked days:
	 * YYYMMDD keylength=7, but let's throw in a \0 so that we
	 * can use string functions on the key */
	if ((fapp = DBH_open(fpath)) == NULL){
		fapp = DBH_create(fpath,8) ;
	}
	if (fapp) {
		gchar *key=g_strdup("YYYMMDD");
		end = g_new0(GtkTextIter, 1);
		/* create the proper key: */
		g_snprintf(key, 8, "%03d%02d%02d", year-1900, month, day);
#ifdef DEBUG
		printf("DBG:key=%s\n",key);
#endif
		strcpy((char *)fapp->key,key);
		/* associate the key to the widget */
		g_object_set_data_full (G_OBJECT (appointment),"key",key,g_free); 
	        /* load the record, if any */
		if (DBH_load(fapp)) {	
			/* use the DBH data buffer directly: */
			text =  (char *)DBH_DATA (fapp);
#ifdef DEBUG
			g_print("Appointment content: %s\n", text);
#endif
			gtk_text_buffer_get_end_iter(tb, end);
			gtk_text_buffer_insert(tb, end, text, strlen(text));
		} 
		DBH_close(fapp);	
		gtk_window_set_title (GTK_WINDOW (appointment), _(title));
#ifdef DEBUG
		gtk_text_buffer_get_bounds(tb, &ctl_start, &ctl_end);
		ctl_text = gtk_text_iter_get_text(&ctl_start, &ctl_end);
		g_print("Content from GtkTextBuffer: %s\n", ctl_text);
#endif
	}
	else {
		g_warning("Cannot open file %s.\n", fpath);
	}
	
	gtk_text_view_set_buffer(tv, tb);
	gtk_text_buffer_set_modified(tb, FALSE);
	gtk_window_set_title (GTK_WINDOW (appointment), _(title));

	g_free(fpath);

}

void
on_btClose_clicked(GtkButton *button, gpointer user_data)
{
  GtkTextView *tv;
  GtkTextBuffer *tb;

  GtkWidget *a=lookup_widget((GtkWidget *)button,"wAppointment");

  tv = GTK_TEXT_VIEW(lookup_widget(GTK_WIDGET(button),"textview1"));
  tb = gtk_text_view_get_buffer(tv);

  if(gtk_text_buffer_get_modified(tb))
    {
      gint result =
	dialogWin(a);
      if(result == GTK_RESPONSE_ACCEPT)
	{
	  gtk_widget_destroy(a); /* destroy the specific appointment window */
	}
    }
  else
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
  GtkTextView *tv;
  GtkTextBuffer *tb;

  GtkWidget *a=lookup_widget((GtkWidget *)button,"wAppointment");

  tv = GTK_TEXT_VIEW(lookup_widget(GTK_WIDGET(button),"textview1"));
  tb = gtk_text_view_get_buffer(tv);

  if(gtk_text_buffer_get_modified(tb))
    {
      gint result =
	dialogWin(a);
      if(result == GTK_RESPONSE_ACCEPT)
	{
	  changeSelectedDate(button, PREVIOUS);
	}
    }
  else
    changeSelectedDate(button, PREVIOUS);

}

void
on_btToday_clicked(GtkButton *button, gpointer user_data)
{
  struct tm *t;
  time_t tt;
  GtkWidget *appointment; 
  GtkTextView *tv;
  GtkTextBuffer *tb;
	
  appointment = lookup_widget(GTK_WIDGET(button),"wAppointment");

  tt=time(NULL);
  t=localtime(&tt);

  tv = GTK_TEXT_VIEW(lookup_widget(GTK_WIDGET(button),"textview1"));
  tb = gtk_text_view_get_buffer(tv);

  if(gtk_text_buffer_get_modified(tb))
    {
      gint result =
	dialogWin(appointment);
      if(result == GTK_RESPONSE_ACCEPT)
	{
	  gtk_calendar_select_month(GTK_CALENDAR(xfcal->mCalendar), t->tm_mon, t->tm_year+1900);
	  gtk_calendar_select_day(GTK_CALENDAR(xfcal->mCalendar), t->tm_mday);
	  manageAppointment(GTK_CALENDAR(xfcal->mCalendar), appointment);
	}
    }
  else
    {
      gtk_calendar_select_month(GTK_CALENDAR(xfcal->mCalendar), t->tm_mon, t->tm_year+1900);
      gtk_calendar_select_day(GTK_CALENDAR(xfcal->mCalendar), t->tm_mday);
      manageAppointment(GTK_CALENDAR(xfcal->mCalendar), appointment);
    }
}

void
on_btNext_clicked(GtkButton *button, gpointer user_data)
{
  GtkTextView *tv;
  GtkTextBuffer *tb;

  GtkWidget *a=lookup_widget((GtkWidget *)button,"wAppointment");

  tv = GTK_TEXT_VIEW(lookup_widget(GTK_WIDGET(button),"textview1"));
  tb = gtk_text_view_get_buffer(tv);

  if(gtk_text_buffer_get_modified(tb))
    {
      gint result =
	dialogWin(a);
      if(result == GTK_RESPONSE_ACCEPT)
	{
	  changeSelectedDate(button, NEXT);
	}
    }
  else
    changeSelectedDate(button, NEXT);
}

void
changeSelectedDate(GtkButton *button, gint direction){
  guint year, month, day;
  guint monthdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  GtkWidget *appointment; 
	
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
on_btSave_clicked(GtkButton *button, gpointer user_data)
{
	gchar *fpath;
	DBHashTable *fapp;
	GtkTextView *tv;
	GtkTextBuffer *tb;
	GtkTextIter start, end;
	char *text;
	G_CONST_RETURN gchar *title;
	GtkWidget *appointment; 
	
	tv = GTK_TEXT_VIEW(lookup_widget(GTK_WIDGET(button),"textview1"));
	appointment = lookup_widget(GTK_WIDGET(button),"wAppointment");
	tb = gtk_text_view_get_buffer(tv);
	gtk_text_buffer_get_bounds(tb, &start, &end);
	text = gtk_text_iter_get_text(&start, &end);
	title = gtk_window_get_title(GTK_WINDOW(appointment));
#ifdef DEBUG
	g_print("Appointment content: %s\n", text);
	g_print("Date created: %s\n", title);
#endif
	
	fpath = xfce_resource_save_location(XFCE_RESOURCE_CONFIG,
                        RCDIR G_DIR_SEPARATOR_S "appointments.dbh", FALSE);

	if (gtk_text_buffer_get_modified(tb)) {
		if ((fapp = DBH_open(fpath)) == NULL){
			fapp = DBH_create(fpath,8) ;
		}
		DBH_Size(fapp,MAX_APP_LENGTH);
		if (!fapp) {
			g_warning("Cannot open file %s\n", fpath);
		}
		else {
			char *save_text=(char *)DBH_DATA(fapp);
			guint day;
			GtkWidget *a=lookup_widget((GtkWidget *)button,"wAppointment");
			char *key=g_object_get_data(G_OBJECT (a),"key");
#ifdef DEBUG
			printf("DBG:key=%s\n",key);
#endif
			g_strlcpy(save_text,text,MAX_APP_LENGTH-1);
			save_text[MAX_APP_LENGTH-1]=0;
			/* since record length is variable,this is crucial: */
			DBH_set_recordsize(fapp,strlen(save_text)+1);
			/* set the key */
			strncpy((char *)fapp->key,key,8); fapp->key[7]=0;
			/* update the DBHashtable: */
			DBH_update(fapp);
			day=atoi((char *)(fapp->key+5));
			/*
			if (strlen(save_text)) gtk_calendar_mark_day(cal,day);
			else gtk_calendar_unmark_day(cal,day);
			*/

			if (strlen(save_text)) 
			  gtk_calendar_mark_day (GTK_CALENDAR (xfcal->mCalendar), day);
			else 
			  gtk_calendar_unmark_day (GTK_CALENDAR (xfcal->mCalendar), day);

			DBH_close(fapp);	
			gtk_text_buffer_set_modified(tb, FALSE);
		}
	}

	g_free(fpath);

#ifdef DEBUG 
	g_print("Procedure on_btSave_clicked finished\n");
#endif
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
	gchar *fpath;
	DBHashTable *fapp;
	GtkTextView *tv;
	GtkTextBuffer *tb;
	GtkTextIter start, end;
	
	gtk_widget_destroy(clearwarn);
	
#ifdef DEBUG
	g_print("Clear textbuffer chosen (oops!)\n");
#endif

	fpath = xfce_resource_save_location(XFCE_RESOURCE_CONFIG,
                        RCDIR G_DIR_SEPARATOR_S "appointments.dbh", FALSE);
	if ((fapp = DBH_open(fpath)) != NULL){
		char *save_text=(char *)DBH_DATA(fapp);
		GtkWidget *a=lookup_widget((GtkWidget *)user_data,"wAppointment");
		char *key=g_object_get_data(G_OBJECT (a),"key");
		guint day=atoi(key+5);
		save_text[0]=0;
		DBH_set_recordsize(fapp,1);
		strncpy((char *)fapp->key,key,8); fapp->key[7]=0;
		DBH_update(fapp);
		DBH_close(fapp);	
		tv = GTK_TEXT_VIEW(lookup_widget(a,"textview1"));
		tb = gtk_text_view_get_buffer(tv);
		gtk_text_buffer_get_bounds(tb, &start, &end);

		gtk_text_buffer_delete(tb, &start, &end);
		gtk_text_buffer_set_modified(tb, FALSE);
		gtk_calendar_unmark_day(GTK_CALENDAR(xfcal->mCalendar), day);
		
	}
#ifdef DEBUG
	else {
		g_print("error: cannot open %s\n", fpath);
	}
#endif
	g_free(fpath);
}

GdkFilterReturn
client_event_filter(GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
    if(mcs_client_process_event(client, (XEvent *) xevent))
        return GDK_FILTER_REMOVE;
    else
        return GDK_FILTER_CONTINUE;
}

