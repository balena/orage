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

#include <libxfce4util/util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4util/i18n.h>
#include <libxfcegui4/netk-trayicon.h>
#include <libxfce4mcs/mcs-client.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <dbh.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#define MAX_APP_LENGTH 4096
#define LEN_BUFFER 1024
#define CHANNEL  "xfcalendar"

#define SUNDAY TRUE
#define MONDAY FALSE

settings calsets;
gboolean normalmode = TRUE;

static gboolean startday = SUNDAY;
/*
static gboolean showtaskbar = TRUE;
static gboolean showpager = TRUE;
*/

/* MCS client */
McsClient        *client = NULL;

static GtkWidget *info;
static GtkWidget *clearwarn;
static GtkCalendar *cal;
static GtkWidget *mainWindow;

/* Direction for changing day to look at */
enum{
  PREVIOUS,
    NEXT
    };

void init_settings(GtkWidget *w)
{
  gchar *fpath;
  FILE *fp;
  char buf[LEN_BUFFER];

  xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  /* default */
  calsets.showCal = TRUE;
  calsets.showTaskbar = TRUE;
  calsets.showPager = TRUE;
  calsets.startMonday = FALSE;
  calsets.dispOptions = GTK_CALENDAR_SHOW_HEADING | GTK_CALENDAR_SHOW_DAY_NAMES | GTK_CALENDAR_SHOW_WEEK_NUMBERS;

  fpath = xfce_get_userfile("xfcalendar", "xfcalendarrc", NULL);
  if ((fp = fopen(fpath, "r")) == NULL){
    g_warning("Unable to open RC file.");
  }else{
    /* *very* limited set of options */
    fgets(buf, LEN_BUFFER, fp); /* [Session Visibility] */
    fgets(buf, LEN_BUFFER, fp);
    if(strstr(buf, "hide")) 
      calsets.showCal = FALSE; 
    else
      {
	calsets.showCal = TRUE; /* default */
	gtk_widget_show(w);
      }
    /* The code below is deprecated
     *    fgets(buf, LEN_BUFFER, fp); // [Start Monday]
     *    fgets(buf, LEN_BUFFER, fp);
     *    if(strstr(buf, "false"))
     *      { 
     *	calsets.startMonday = FALSE; 
     *	gtk_calendar_display_options (GTK_CALENDAR (cal), calsets.dispOptions);
     *      }
     *    else 
     *      {
     *	calsets.startMonday = TRUE;
     *	gtk_calendar_display_options (GTK_CALENDAR (cal), calsets.dispOptions|GTK_CALENDAR_WEEK_START_MONDAY);
     *      }
     */
  }
}

void apply_settings()
{
  gchar *fpath;
  FILE *fp;

  xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  if(calsets.startMonday==TRUE)
    {
    gtk_calendar_display_options (GTK_CALENDAR (cal), calsets.dispOptions|GTK_CALENDAR_WEEK_START_MONDAY);
#ifdef debug
    g_message("apply_settings(): Monday true");
#endif
    }
  else
    {
    gtk_calendar_display_options (GTK_CALENDAR (cal), calsets.dispOptions);
#ifdef debug
    g_message("apply_settings(): Monday false");
#endif
    }

  /* Save settings here */
  /* I know, it's bad(tm) */
  fpath = xfce_get_userfile("xfcalendar", "xfcalendarrc", NULL);
  if ((fp = fopen(fpath, "w")) == NULL){
    g_warning("Unable to open RC file.");
  }else {
    fprintf(fp, "[Session Visibility]\n");
    if(calsets.showCal) fprintf(fp, "show\n"); else fprintf(fp, "hide\n");

    /* The code below is deprecated 
     *    fprintf(fp, "[Start Monday]\n");
     *    if(calsets.startMonday) fprintf(fp, "true\n"); else fprintf(fp, "false\n");
     */

    fclose(fp);
  }
  g_free(fpath);
}

void settings_set_showCal(GtkWidget *w)
{
  calsets.showCal = GTK_WIDGET_VISIBLE(w);
}

void set_mainWin(GtkWidget *w)
{
  mainWindow=w;
}

void set_cal(GtkWidget *w)
{
  //We define it once, there will be only one calendar
  cal=(GtkCalendar *)lookup_widget(w,"calendar1");
}

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
	gtk_calendar_get_date(cal, &year, &month, &day);
	g_snprintf(key, 8, "%03d%02d%02d", year-1900, month, day);
	DBH_sweep(fapp,markit,key,NULL,5);
	DBH_close(fapp);
	return TRUE;	
}

void pretty_window(char *text){
	GtkWidget *reminder;
	reminder = create_wReminder(text);
	gtk_widget_show(reminder);
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
on_close1_activate(GtkMenuItem *menuitem, gpointer user_data)
{
  GtkWidget *window = GTK_WIDGET(user_data);
  
  gtk_widget_hide(window);
}

void
on_quit1_activate(GtkMenuItem *menuitem, gpointer user_data)
{
  gtk_main_quit();
}

void 
on_preferences_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  mcs_client_show (GDK_DISPLAY (), DefaultScreen (GDK_DISPLAY ()),
		   CHANNEL);
}

void
on_selectToday_activate                (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
  struct tm *t;
  time_t tt;
  tt=time(NULL);
  t=localtime(&tt);
  gtk_calendar_select_month(cal, t->tm_mon, t->tm_year+1900);
  gtk_calendar_select_day(cal, t->tm_mday);
}


void
on_weekMonday_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  /* Deprecated */
  if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem))) 
    calsets.startMonday = TRUE; 
  else
    calsets.startMonday = FALSE;
  apply_settings();
  
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
  GtkWidget *window = GTK_WIDGET(user_data);
  
  gtk_widget_hide(window);
  return(TRUE);
}

void
on_calendar1_day_selected_double_click (GtkCalendar *calendar,
                                        gpointer user_data)
{
  GtkWidget *appointment;
  appointment = create_wAppointment();
  manageAppointment(calendar, appointment);
  gtk_widget_show(appointment);
}

void
on_Today_activate                      (GtkMenuItem *menuitem,
                                        gpointer user_data)
{
  struct tm *t;
  time_t tt;
  GtkWidget *appointment;

  appointment = create_wAppointment();
  tt=time(NULL);
  t=localtime(&tt);
  gtk_calendar_select_month(cal, t->tm_mon, t->tm_year+1900);
  gtk_calendar_select_day(cal, t->tm_mday);
  manageAppointment(cal, appointment);
  gtk_widget_show(appointment);
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
	GtkTextIter ctl_start, ctl_end;
	char *ctl_text;
  
	gtk_calendar_get_date(calendar, &year, &month, &day);
	g_snprintf(title, 12, "%d-%02d-%02d", year, month+1, day);
	fpath = xfce_get_userfile("xfcalendar", "appointments.dbh", NULL);

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
	gtk_window_set_title (GTK_WINDOW (appointment), _(title));

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
	
  appointment = lookup_widget(GTK_WIDGET(button),"wAppointment");

  tt=time(NULL);
  t=localtime(&tt);
  gtk_calendar_select_month(cal, t->tm_mon, t->tm_year+1900);
  gtk_calendar_select_day(cal, t->tm_mday);
  manageAppointment(cal, appointment);
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
	
  appointment = lookup_widget(GTK_WIDGET(button),"wAppointment");

  gtk_calendar_get_date(cal, &year, &month, &day);

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
      gtk_calendar_select_month(cal, month, year);
      day = monthdays[month];
    }
    break;
  case NEXT:
    if(++day == (monthdays[month]+1)){
      if(++month == 12){
	++year;
	month = 0;
      }
      gtk_calendar_select_month(cal, month, year);
      day = 1;
    }
    break;
  default:
    break;
  }
  gtk_calendar_select_day(cal, day);
  manageAppointment(cal, appointment);
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
	
	fpath = xfce_get_userfile("xfcalendar","appointments.dbh", NULL);

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
on_btOkInfo_clicked(GtkButton *button, gpointer user_data)
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

void
on_btOkReminder_clicked(GtkButton *button, gpointer user_data)
{
  GtkWidget *a=lookup_widget((GtkWidget *)button,"wReminder");
  gtk_widget_destroy(a); /* destroy the specific appointment window */
}

void
on_calendar1_scroll                    (GtkCalendar     *calendar,
					GdkEventScroll *event)
{
  guint year, month, day;
  gtk_calendar_get_date(cal, &year, &month, &day);
#ifdef DEBUG
  g_print("Year: %d, month: %d, day: %d\n", year, month, day);
#endif
  switch(event->direction)
    {
	case GDK_SCROLL_UP:
	    if(--month == -1){
#ifdef DEBUG
	      g_print("Up!! Year: %d, month: %d, day: %d\n", year, month, day);
#endif
	      month = 11;
	      --year;
	    }
	    gtk_calendar_select_month(cal, month, year);
	    break;
	case GDK_SCROLL_DOWN:
	    if(++month == 12){
#ifdef DEBUG
	      g_print("Down!! Year: %d, month: %d, day: %d\n", year, month, day);
#endif	      
	      month = 0;
	      ++year;
	    }
	    gtk_calendar_select_month(cal, month, year);
	    break;
	default:
	  g_print("get scroll event!!!");
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

void
watch_cb(Window window, Bool is_start, long mask, void *cb_data)
{
    GdkWindow *gdkwin;

    gdkwin = gdk_window_lookup(window);

    if(is_start)
    {
        if(!gdkwin)
        {
            gdkwin = gdk_window_foreign_new(window);
        }
        else
        {
            g_object_ref(gdkwin);
        }
        gdk_window_add_filter(gdkwin, client_event_filter, cb_data);
    }
    else
    {
        g_assert(gdkwin);
        gdk_window_remove_filter(gdkwin, client_event_filter, cb_data);
        g_object_unref(gdkwin);
    }
}

void 
notify_cb(const char *name, const char *channel_name, McsAction action, McsSetting * setting, void *data)
{
  if(g_ascii_strcasecmp(CHANNEL, channel_name))
    {
        g_message(_("This should not happen"));
        return;
    }

    switch (action)
    {
        case MCS_ACTION_NEW:
        case MCS_ACTION_CHANGED:
            if(setting->type == MCS_TYPE_INT)
            {
                if (!strcmp(name, "XFCalendar/StartDay"))
                {
		  startday = setting->data.v_int ? SUNDAY: MONDAY;
		  //I'll look later if I can clean that...
		  if(startday == SUNDAY) 
		    {
		      calsets.startMonday = FALSE;
#ifdef debug
		      g_message("Monday false");
#endif
		    }
		  else
		    {
		      calsets.startMonday = TRUE;
#ifdef debug
		      g_message("Monday true");
#endif
		    }
		  apply_settings();
                }
		if(!strcmp(name, "XFCalendar/NormalMode"))
		{
		  normalmode = setting->data.v_int ? TRUE: FALSE;
	          gtk_window_set_decorated(GTK_WINDOW(mainWindow), normalmode);
		  if(!normalmode)
		    gtk_widget_hide((GtkWidget *)lookup_widget(mainWindow, "menubar1"));
		  else
  	            gtk_widget_show((GtkWidget *)lookup_widget(mainWindow, "menubar1"));


		}

	        /* Commented until the bug is fixed :(
		if(!strcmp(name, "XFCalendar/TaskBar"))
		{
		  showtaskbar = setting->data.v_int ? TRUE: FALSE;
		   * Reminder: if we want to show the calendar in the taskbar (i.e. showtaskbar is TRUE)
		   * then gtk_window_set_skip_taskbar_hint must get a FALSE value, and if we don't want
		   * to be seen in the taskbar, then the function must eat a TRUE.
		   *
		  gtk_window_set_skip_taskbar_hint((GtkWindow*)mainWindow, !showtaskbar);
		  calsets.showTaskbar = showtaskbar;
		}
		if(!strcmp(name, "XFCalendar/Pager"))
		{
		  showpager = setting->data.v_int ? TRUE: FALSE;
		   * Reminder: if we want to show the calendar in the pager (i.e. showpager is TRUE)
		   * then gtk_window_set_skip_pager_hint must get a FALSE value, and if we don't want
		   * to be seen in the pager, then the function must eat a TRUE.
		   *
		  gtk_window_set_skip_pager_hint((GtkWindow*)mainWindow, !showpager);
		  calsets.showPager = showpager;
		}
		*/
            }
            break;
        case MCS_ACTION_DELETED:
        default:
            break;
    }
}

/*
 */
void
toggle_visible_cb(GtkWidget *window)
{
  if (GTK_WIDGET_VISIBLE(window))
    {
      gtk_widget_hide(window);
    }
  else
    {
      gtk_window_set_decorated(GTK_WINDOW(window), normalmode);
      if(!normalmode)
        gtk_widget_hide((GtkWidget *)lookup_widget(window, "menubar1"));
      else
        gtk_widget_show((GtkWidget *)lookup_widget(window, "menubar1"));
      gtk_widget_show(window);
      gtk_window_stick(GTK_WINDOW(window));
      /* Commented until the bug is fixed :(
      gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), !calsets.showTaskbar);
      gtk_window_set_skip_pager_hint(GTK_WINDOW(window), !calsets.showPager);
      */
    }
}

/*
 * Die callback
 *
 * This is called when the session manager requests the client to go down.
 */

void
die_cb(gpointer data)
{
  gtk_main_quit();
}

/*
 * SaveYourself callback
 *
 * This is called when the session manager requests the client to save its
 * state.
 */
/* ARGUSED */
void
save_yourself_cb(gpointer data, int save_style, gboolean shutdown,
                 int interact_style, gboolean fast)
{
  settings_set_showCal(mainWindow);
  apply_settings();
}
