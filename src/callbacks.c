/* xfcalendar
 *
 * Copyright (C) 2002 Mickael Graf (korbinus@linux.se)
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

#include <libxfce4util/util.h>
#include <libxfce4util/i18n.h>
#include <gtk/gtk.h>
#include <dbh.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

static GtkWidget *info;
static GtkWidget *clearwarn;
static GtkCalendar *cal;


void markit(DBHashTable *f){
	char *text=(char *)DBH_DATA(f);
	if (strlen(text)){
		guint day=atoi((char *)(f->key+5));
		if (day > 0 && day < 32){
#ifdef DEBUG
			printf("marking %u\n",day);
#endif
			gtk_calendar_mark_day(cal,day);
		}
	}
}

int mark_appointments(GtkWidget *w){
	guint year, month, day;
	char key[8];
	DBHashTable *fapp;
	char *fpath = xfce_get_userfile("xfcalendar", "appointments.dbh", NULL);
	if ((fapp = DBH_open(fpath)) == NULL) return FALSE;
	cal=(GtkCalendar *)lookup_widget(w,"calendar1");
	gtk_calendar_get_date(cal, &year, &month, &day);
	g_snprintf(key, 8, "%03d%02d%02d", year-1900, month, day);
	DBH_sweep(fapp,markit,key,NULL,5);
	DBH_close(fapp);
	return TRUE;	
}

void pretty_window(char *text){
	printf("FIXME: alarmclock!! (function pretty_window())\n"); 
	printf("FIXME: Draw a pretty window (noneditable) with the text:\n%s\n",text); 
}

gint alarm_clock(gpointer p){
	struct tm *t;
	char key[8];
	time_t tt;
	static char start_key[8]={0,0,0,0,0,0,0,0};
	tt=time(NULL);
	t=localtime(&tt);
	g_snprintf(key, 8, "%03d%02d%02d", t->tm_year, t->tm_mon, t->tm_mday);
#ifdef DEBUG
	printf("at alarm %s==%s\n",key,start_key);
#endif
	if (!strlen(start_key) || strcmp(start_key,key)!=0){
		/* buzzz */
		DBHashTable *fapp;
		char *fpath = xfce_get_userfile("xfcalendar", "appointments.dbh", NULL);
		if ((fapp = DBH_open(fpath)) == NULL) return FALSE;
		strcpy(start_key,key);
		strcpy((char *)(fapp->key),key);
		if (DBH_load(fapp)){
			char *text=(char *)DBH_DATA(fapp);
			if (strlen(text)) pretty_window(text);
		}
		DBH_close(fapp);
		g_free(fpath);
	}
	return TRUE;	
}

void remark_appointments (GtkCalendar *calendar,gpointer user_data){
#ifdef DEBUG
	printf("remark_appointments...\n");
#endif
	gtk_calendar_clear_marks(calendar);
	mark_appointments((GtkWidget *)calendar);
}



void setup_signals(GtkWidget *w){
	cal=(GtkCalendar *)lookup_widget(w,"calendar1");
	g_signal_connect ((gpointer) cal, "month-changed",
                    G_CALLBACK (remark_appointments),
                   NULL);
	/* the alarm callback: */		    
 	g_timeout_add_full(0, 5000, (GtkFunction) alarm_clock, (gpointer) w, NULL);
}


int keep_tidy(void){
	/* keep a tidy DBHashTable */
	DBHashTable *fapp;
	char *fpath = xfce_get_userfile("xfcalendar", "appointments.dbh", NULL);
	if ((fapp = DBH_open(fpath)) != NULL){
		char *wd = xfce_get_userfile("xfcalendar", NULL);
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

void
on_quit1_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	gtk_main_quit();
}

void
on_about1_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	info = create_wInfo();
	gtk_widget_show(info);
}

gboolean
on_XFCalendar_delete_event(GtkWidget *widget, GdkEvent *event,
                           gpointer user_data)
{
	gtk_main_quit();
	return(FALSE);
}

#define MAX_APP_LENGTH 4096
void
on_calendar1_day_selected_double_click (GtkCalendar *calendar,
                                        gpointer user_data)
{
	guint year, month, day;
	char title[12], *text;
	gchar *fpath;
	DBHashTable *fapp;
	GtkTextView *tv;
	GtkTextBuffer *tb = gtk_text_buffer_new(NULL);
	GtkTextIter *end;
	GtkTextIter ctl_start, ctl_end;
	char *ctl_text;
	GtkWidget *appointment;
  
	gtk_calendar_get_date(calendar, &year, &month, &day);
	g_snprintf(title, 12, "%d-%02d-%02d", year, month+1, day);
	fpath = xfce_get_userfile("xfcalendar", "appointments.dbh", NULL);
	appointment = create_wAppointment();

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
		gtk_text_buffer_get_bounds(tb, &ctl_start, &ctl_end);
		ctl_text = gtk_text_iter_get_text(&ctl_start, &ctl_end);
#ifdef DEBUG
		g_print("Content from GtkTextBuffer: %s\n", ctl_text);
#endif
	}
	else {
		g_warning("Cannot open file %s.\n", fpath);
	}
	
	gtk_text_view_set_buffer(tv, tb);
	gtk_window_set_title (GTK_WINDOW (appointment), _(title));
	gtk_widget_show(appointment);

	g_free(fpath);
}

void
on_btClose_clicked(GtkButton *button, gpointer user_data)
{
	GtkWidget *a=lookup_widget((GtkWidget *)button,"wAppointment");
	gtk_widget_destroy(a); /* destroy the specific appointment window */
}

gboolean 
on_wAppointment_delete_event(GtkWidget *widget, GdkEvent *event,
                             gpointer user_data)
{
	gtk_widget_destroy(widget); /* destroy the appointment window */
	return(FALSE);
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
	
	fpath = xfce_get_userfile("xfcalendar","appointments.dbh", NULL);
	

	if (gtk_text_buffer_get_modified(tb)) {
		DBH_Size(NULL,MAX_APP_LENGTH);
		if ((fapp = DBH_open(fpath)) == NULL){
			fapp = DBH_create(fpath,8) ;
		}
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
			if (strlen(save_text)) gtk_calendar_mark_day(cal,day);
			else gtk_calendar_unmark_day(cal,day);

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
on_okbutton1_clicked(GtkButton *button, gpointer user_data)
{
	gtk_widget_destroy(info);
}

gboolean
on_wInfo_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	gtk_widget_destroy(widget);
	return(FALSE);
}


void
on_btDelete_clicked(GtkButton *button, gpointer user_data)
{
	GtkWidget *w;
	clearwarn = create_wClearWarn();
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

	fpath = xfce_get_userfile("xfcalendar", "appointments.dbh", NULL);
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
		gtk_calendar_unmark_day(cal,day);
		
	}
#ifdef DEBUG
	else {
		g_print("error: cannot open %s\n", fpath);
	}
#endif
	g_free(fpath);
}
