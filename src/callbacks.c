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
#include <ical.h>
#include <icalss.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "reminder.h"
#include "about-xfcalendar.h"
#include "mainbox.h"

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

static icalcomponent *ical;
static icalset* fical;

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

gboolean open_ical_file(void)
{
	gchar *ficalpath; 
    icalcomponent *iter;
    gint cnt=0;

	ficalpath = xfce_resource_save_location(XFCE_RESOURCE_CONFIG,
                        RCDIR G_DIR_SEPARATOR_S APPOINTMENT_FILE, FALSE);
    if ((fical = icalset_new_file(ficalpath)) == NULL){
        if(icalerrno != ICAL_NO_ERROR){
            g_warning("open_ical_file, ical-Error: %s\n",icalerror_strerror(icalerrno));
        g_error("open_ical_file, Error: Could not open ical file \"%s\" \n", ficalpath);
        }
    }
    else{
        /* let's find last VCALENDAR entry */
        for (iter = icalset_get_first_component(fical); iter != 0;
             iter = icalset_get_next_component(fical)){
            cnt++;
            ical = iter; /* last valid component */
        }
        if (cnt == 0){
        /* calendar missing, need to add one. 
           Note: According to standard rfc2445 calendar always needs to
                 contain at least one other component. So strictly speaking
                 this is not valid entry before adding an event */
            ical = icalcomponent_vanew(ICAL_VCALENDAR_COMPONENT
                   ,icalproperty_new_version("1.0")
                   ,icalproperty_new_prodid("-//Xfce//Xfcalendar//EN")
                   ,0);
            icalset_add_component(fical, icalcomponent_new_clone(ical));
            icalset_commit(fical);
        }
        else if (cnt > 1){
            g_warning("open_ical_file, Too many top level components in calendar file\n");
        }
    }
	g_free(ficalpath);
    return(TRUE);
}

void close_ical_file(void)
{
    icalset_free(fical);
}

static void add_ical_app(char *text, char *a_day)
{
    icalcomponent *ievent;
    struct icaltimetype atime = icaltime_from_timet(time(0), 0);
    struct icaltimetype adate;
    gchar xf_uid[1000];
    gchar xf_host[501];

    adate = icaltime_from_string(a_day);
    gethostname(xf_host, 500);
    sprintf(xf_uid, "Xfcalendar-%s-%lu@%s", icaltime_as_ical_string(atime), 
                (long) getuid(), xf_host);
    ievent = icalcomponent_vanew(ICAL_VEVENT_COMPONENT
           ,icalproperty_new_uid(xf_uid)
           ,icalproperty_new_categories("XFCALNOTE")
           ,icalproperty_new_class(ICAL_CLASS_PUBLIC)
           ,icalproperty_new_dtstamp(atime)
           ,icalproperty_new_created(atime)
           ,icalproperty_new_description(text)
           ,icalproperty_new_dtstart(adate)
           ,0);
    icalcomponent_add_component(ical, ievent);
}

gboolean get_ical_app(char **desc, char **sum, char *a_day, char *hh_mm)
{
    struct icaltimetype t, adate, edate;
    static icalcomponent *c;
    gboolean date_found=FALSE;

    adate = icaltime_from_string(a_day);
    if (strlen(hh_mm) == 0){ /* start */
        c = icalcomponent_get_first_component(ical, ICAL_VEVENT_COMPONENT); 
    }
    for ( ; 
         (c != 0) && (!date_found);
         c = icalcomponent_get_next_component(ical, ICAL_VEVENT_COMPONENT)){
        t = icalcomponent_get_dtstart(c);
        if (icaltime_compare_date_only(t, adate) == 0){
            date_found = TRUE;
            *desc = (char *)icalcomponent_get_description(c);
            *sum = (char *)icalcomponent_get_summary(c);
            edate = icalcomponent_get_dtend(c);
            if (icaltime_is_date(t))
                strcpy(hh_mm, "xx:xx-xx:xx");
            else {
                if (icaltime_is_null_time(edate)) {
                    edate.hour = t.hour;
                    edate.minute = t.minute;
                }
                sprintf(hh_mm, "%02d:%02d-%02d:%02d", t.hour, t.minute
                        , edate.hour, edate.minute);
            }
        }
    } 
    return(date_found);
}

int getnextday_ical_app(int year, int month, int day)
{
    struct icaltimetype t;
    static icalcomponent *c;
    int next_day = 0;

    if (day == -1){ /* start */
        c = icalcomponent_get_first_component(ical, ICAL_VEVENT_COMPONENT); 
    }
    for ( ;
        (c != 0) && (next_day == 0);
         c = icalcomponent_get_next_component(ical, ICAL_VEVENT_COMPONENT)){
        t = icalcomponent_get_dtstart(c);
        if ((t.year == year) && (t.month == month)){
            next_day = t.day;
        }
    } 
    return(next_day);
}

static void rm_ical_app(char *a_day)
{
    struct icaltimetype t, adate;
    icalcomponent *c, *c2;

/* Note: remove moves the "c" pointer to next item, so we need to store it 
         first to process all of them or we end up skipping entries */
    adate = icaltime_from_string(a_day);
    for (c = icalcomponent_get_first_component(ical, ICAL_VEVENT_COMPONENT); 
         c != 0;
         c = c2){
        c2 = icalcomponent_get_next_component(ical, ICAL_VEVENT_COMPONENT);
        t = icalcomponent_get_dtstart(c);
        if (icaltime_compare_date_only(t, adate) == 0){
            icalcomponent_remove_component(ical, c);
        }
    } 
}

void addAppointment(GtkWidget *vbox, gchar *xftime, gchar *xftext, gchar *xfsum)
{
    GtkWidget *hseparator;
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *entry;
    GtkTooltips *event_tooltips;
    int len, max_len=40;
    gchar *eolstr, *text;

    hseparator = gtk_hseparator_new();
    gtk_widget_show(hseparator);
    gtk_box_pack_start(GTK_BOX(vbox), hseparator, FALSE, FALSE, 0);
                                                                                
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
                                                                                
    label = gtk_label_new(xftime);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_widget_show(label);

    entry = gtk_entry_new();
    if (xfsum == NULL) { /* let's take first line from the text */
        if ((eolstr = g_strstr_len(xftext, strlen(xftext), "\n")) == NULL) {
            len=max_len;
        }
        else {
            len=strlen(xftext)-strlen(eolstr);
            gtk_entry_set_max_length(GTK_ENTRY(entry), len);
        }
        text=xftext;
    }
    else
        text=xfsum;
    gtk_entry_set_text(GTK_ENTRY(entry), text);
    gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
    gtk_widget_show(entry);

    event_tooltips = gtk_tooltips_new();
    gtk_tooltips_set_tip(event_tooltips, entry, xftext, NULL);
                                                                                
}

void manageAppointment(GtkCalendar *calendar, GtkWidget *appointment)
{
	guint year, month, day;
	char title[12], *description, *summary;
	char a_day[9];  /* yyyymmdd */
	char a_time[12]=""; /* hh:mm-hh:mm */
    GtkWidget *vbox;
    int i;
    char kello[20];

  
	gtk_calendar_get_date(calendar, &year, &month, &day);
	g_snprintf(title, 12, "%d-%02d-%02d", year, month+1, day);
	gtk_window_set_title(GTK_WINDOW(appointment), _(title));
	vbox = lookup_widget(GTK_WIDGET(appointment), "vbox3");

    if (open_ical_file()){
        g_snprintf(a_day, 9, "%04d%02d%02d", year, month+1, day);
        while (get_ical_app(&description, &summary, a_day, a_time)){ 
            /* data found */
            addAppointment(vbox, a_time, description, summary);
        }
        close_ical_file();
    }
    /*
    for (i=1; i<5; i++){
        sprintf(kello, "%02d:00-%02d:%02d", 7+i, 11+i, 2*i);
        addAppointment(vbox, kello, "koe\n eli pitkä testi.", "summary");
    }
    addAppointment(vbox, _("<New>"), _("This is just a place holder.\nClick it to create a real appointmen"), _("<click to modify>"));
    */
}

void
on_btClose_clicked(GtkButton *button, gpointer user_data)
{
  GtkTextView *tv;
  GtkTextBuffer *tb;
  gint result;
  GtkWidget *a=lookup_widget((GtkWidget *)button,"wAppointment");

  recreate_wAppointment(button);

  /* FIXME: need to decide if it is possible to do updates....
  tv = GTK_TEXT_VIEW(lookup_widget(GTK_WIDGET(button),"textview1"));
  tb = gtk_text_view_get_buffer(tv);

  if(gtk_text_buffer_get_modified(tb)) {
      result = dialogWin(a);
      if(result == GTK_RESPONSE_ACCEPT) {
	  gtk_widget_destroy(a); 
      }
  }
  else
  */
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
  gint result;

  GtkWidget *a=lookup_widget((GtkWidget *)button,"wAppointment");

  recreate_wAppointment(button);

  /* FIXME: need to decide if it is possible to do updates....
  tv = GTK_TEXT_VIEW(lookup_widget(GTK_WIDGET(button),"textview1"));
  tb = gtk_text_view_get_buffer(tv);

  if(gtk_text_buffer_get_modified(tb))
    {
      result = dialogWin(a);
      if(result == GTK_RESPONSE_ACCEPT)
	{
	  changeSelectedDate(button, PREVIOUS);
	}
    }
  else
  */
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
  gint result;
	
  appointment = lookup_widget(GTK_WIDGET(button),"wAppointment");

  recreate_wAppointment(button);

  tt=time(NULL);
  t=localtime(&tt);

  /* FIXME: need to decide if it is possible to do updates....
  tv = GTK_TEXT_VIEW(lookup_widget(GTK_WIDGET(button),"textview1"));
  tb = gtk_text_view_get_buffer(tv);

  if(gtk_text_buffer_get_modified(tb))
    {
      result = dialogWin(appointment);
      if(result == GTK_RESPONSE_ACCEPT)
	{
	  gtk_calendar_select_month(GTK_CALENDAR(xfcal->mCalendar), t->tm_mon, t->tm_year+1900);
	  gtk_calendar_select_day(GTK_CALENDAR(xfcal->mCalendar), t->tm_mday);
	  manageAppointment(GTK_CALENDAR(xfcal->mCalendar), appointment);
	}
    }
  else
  */
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
  gint result;
  GtkWidget *a=lookup_widget((GtkWidget *)button,"wAppointment");

  recreate_wAppointment(button);

  /* FIXME: need to decide if it is possible to do updates....
  tv = GTK_TEXT_VIEW(lookup_widget(GTK_WIDGET(button),"textview1"));
  tb = gtk_text_view_get_buffer(tv);

  if(gtk_text_buffer_get_modified(tb))
    {
      result = dialogWin(a);
      if(result == GTK_RESPONSE_ACCEPT)
	{
	  changeSelectedDate(button, NEXT);
	}
    }
  else
  */
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
	GtkTextView *tv;
	GtkTextBuffer *tb;
	GtkTextIter start, end;
	char *text;
	G_CONST_RETURN gchar *title;
	GtkWidget *appointment; 
	GtkWidget *vbox; 
    char a_day[10];
    guint day;

	vbox = lookup_widget(GTK_WIDGET(button),"vbox3");
    addAppointment(vbox, "<??:??>", "not decided how this is supposed to work.\nBe patient.", "<Not Yet Implemented>");
    /*
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
	if (gtk_text_buffer_get_modified(tb)) {
        if (open_ical_file()){
            a_day[0]=title[0]; a_day[1]=title[1];     
                    a_day[2]=title[2]; a_day[3]=title[3];
            a_day[4]=title[5]; a_day[5]=title[6];       
            a_day[6]=title[8]; a_day[7]=title[9];      
            a_day[8]=title[10];                       
            rm_ical_app(a_day);
            add_ical_app(text, a_day);
            icalset_mark(fical);
            close_ical_file();
			day = atoi(a_day+6); 
			if (strlen(text)) 
                gtk_calendar_mark_day (GTK_CALENDAR (xfcal->mCalendar), day);
			else 
                gtk_calendar_unmark_day (GTK_CALENDAR (xfcal->mCalendar), day);
			gtk_text_buffer_set_modified(tb, FALSE);
        }
	}
    */
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
on_btCreate_clicked(GtkButton *button, gpointer user_data)
{
	GtkWidget *w;
	GtkWidget *vbox; 
	
	vbox = lookup_widget(GTK_WIDGET(button),"vbox3");

    addAppointment(vbox, _("<New>"), _("This is just a place holder.\nClick it to create a real appointmen"), _("<click to modify>"));
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
        icalset_mark(fical);
        close_ical_file();

        /*
		tv = GTK_TEXT_VIEW(lookup_widget(a,"textview1"));
		tb = gtk_text_view_get_buffer(tv);
		gtk_text_buffer_get_bounds(tb, &start, &end);
		gtk_text_buffer_delete(tb, &start, &end);
		gtk_text_buffer_set_modified(tb, FALSE);
        day = atoi(a_day+6);
        */
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

