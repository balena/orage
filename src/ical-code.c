/* ical-code.c
 *
 * Copyright (C) 2005 Juha Kautto <juha@xfce.org>
 *                    Mickaël Graf <korbinus@lunar-linux.org>
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

#include "appointment.h"
#include "reminder.h"
#include "ical-code.h"

#define MAX_APP_LENGTH 4096
#define LEN_BUFFER 1024
#define CHANNEL  "xfcalendar"
#define RCDIR    "xfce4" G_DIR_SEPARATOR_S "xfcalendar"
#define APPOINTMENT_FILE "appointments.ics"

#define XFICAL_STR_EXISTS(str) ((str != NULL) && (str[0] != 0))

static icalcomponent *ical;
static icalset* fical;
static gboolean fical_modified=TRUE;

extern GList *alarm_list;

gboolean xfical_file_open(void)
{
	gchar *ficalpath; 
    icalcomponent *iter;
    gint cnt=0;

	ficalpath = xfce_resource_save_location(XFCE_RESOURCE_CONFIG,
                        RCDIR G_DIR_SEPARATOR_S APPOINTMENT_FILE, FALSE);
    if ((fical = icalset_new_file(ficalpath)) == NULL){
        if(icalerrno != ICAL_NO_ERROR){
            g_warning("xfical_file_open, ical-Error: %s\n"
                        , icalerror_strerror(icalerrno));
        g_error("xfical_file_open, Error: Could not open ical file \"%s\" \n"
                        , ficalpath);
        }
    }
    else{
        /* let's find last VCALENDAR entry */
        for (iter = icalset_get_first_component(fical); 
             iter != 0;
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
                   ,icalproperty_new_version("2.0")
                   ,icalproperty_new_prodid("-//Xfce//Xfcalendar//EN")
                   ,0);
            icalset_add_component(fical, icalcomponent_new_clone(ical));
            icalset_commit(fical);
        }
        else if (cnt > 1){
            g_warning("xfical_file_open, Too many top level components in calendar file\n");
        }
    }
	g_free(ficalpath);
    return(TRUE);
}

void xfical_file_close(void)
{
    icalset_free(fical);
    if (fical_modified) {
        fical_modified = FALSE;
        build_ical_alarm_list();
    }
}

struct icaltimetype ical_get_current_local_time()
{
    time_t t;
    struct tm *tm;
    struct icaltimetype ctime;
    char koe[200];

    t=time(NULL);
    tm=localtime(&t);
    
    ctime.year        = tm->tm_year+1900;
    ctime.month       = tm->tm_mon+1;
    ctime.day         = tm->tm_mday;
    ctime.hour        = tm->tm_hour;
    ctime.minute      = tm->tm_min;
    ctime.second      = tm->tm_sec;
    ctime.is_utc      = 0;
    ctime.is_date     = 0;
    ctime.is_daylight = 0;
    ctime.zone        = NULL;
    /*
    note: this gives UTC time:
    ctime = icaltime_current_time_with_zone(NULL);

    strftime(koe, 200, "%Z", tm);
    g_print("timezone: %s\n", koe);
    */
    return (ctime);
}

 /* allocates memory and initializes it for new ical_type structure
 *  returns: NULL if failed and pointer to appt_type if successfull.
 *          You must free it after not being used anymore. (g_free())
 */
appt_type *xfical_app_alloc()
{
    appt_type *temp;

    temp = g_new0(appt_type, 1);
    temp->alarm = -1;
    temp->alarmTimeType = 0;
    temp->availability = 1;
    return(temp);
}

 /* add EVENT type ical appointment to ical file
 * app: pointer to filled appt_type structure, which is stored
 *      Caller is responsible for filling and allocating and freeing it.
 *  returns: NULL if failed and new ical id if successfully added. 
 *           This ical id is owned by the routine. Do not deallocate it.
 *           It will be overdriven by next invocation of this function.
 */
char *xfical_app_add(appt_type *app)
{
    icalcomponent *ievent, *ialarm;
    struct icaltimetype ctime;
    static gchar xf_uid[1000];
    gchar xf_host[501];
    icalproperty_transp xf_transp;
    struct icaltriggertype trg;
    gint duration=0;
    icalattach *attach;

    ctime = ical_get_current_local_time();
    gethostname(xf_host, 500);
    sprintf(xf_uid, "Xfcalendar-%s-%lu@%s", icaltime_as_ical_string(ctime), 
                (long) getuid(), xf_host);

    ievent = icalcomponent_vanew(ICAL_VEVENT_COMPONENT
           ,icalproperty_new_uid(xf_uid)
           ,icalproperty_new_categories("XFCALNOTE")
           ,icalproperty_new_class(ICAL_CLASS_PUBLIC)
           ,icalproperty_new_dtstamp(ctime)
           ,icalproperty_new_created(ctime)
           ,0);

    if XFICAL_STR_EXISTS(app->title)
        icalcomponent_add_property(ievent
                , icalproperty_new_summary(app->title));
    if XFICAL_STR_EXISTS(app->note)
        icalcomponent_add_property(ievent
                , icalproperty_new_description(app->note));
    if XFICAL_STR_EXISTS(app->location)
        icalcomponent_add_property(ievent
                , icalproperty_new_location(app->location));
    if (app->availability == 0)
        icalcomponent_add_property(ievent
           , icalproperty_new_transp(ICAL_TRANSP_TRANSPARENT));
    else if (app->availability == 1)
        icalcomponent_add_property(ievent
           , icalproperty_new_transp(ICAL_TRANSP_OPAQUE));
    if (app->allDay) {
        app->starttime[8] = 0;
        app->endtime[8] = 0;
    }
    if XFICAL_STR_EXISTS(app->starttime)
        icalcomponent_add_property(ievent
           , icalproperty_new_dtstart(icaltime_from_string(app->starttime)));
    if XFICAL_STR_EXISTS(app->endtime)
        icalcomponent_add_property(ievent
           , icalproperty_new_dtend(icaltime_from_string(app->endtime)));
    if (!app->allDay  && app->alarm != 0)  {
        if (app->alarmTimeType == 0) 
            duration = app->alarm * 60;
        else if (app->alarmTimeType == 1) 
            duration = app->alarm * 60 * 60;
        else if (app->alarmTimeType == 2) 
            duration = app->alarm * 60 * 60 * 24;
        else 
            duration = app->alarm; /* secs */
        trg.time = icaltime_null_time();
        trg.duration =  icaldurationtype_from_int(-duration);
    /********** DISPLAY **********/
        ialarm = icalcomponent_vanew(ICAL_VALARM_COMPONENT
           ,icalproperty_new_action(ICAL_ACTION_DISPLAY)
           ,0);
        icalcomponent_add_property(ialarm
             , icalproperty_new_trigger(trg));
        if XFICAL_STR_EXISTS(app->note)
            icalcomponent_add_property(ialarm
                , icalproperty_new_description(app->note));
        else if XFICAL_STR_EXISTS(app->title)
            icalcomponent_add_property(ialarm
                , icalproperty_new_description(app->title));
        else
            icalcomponent_add_property(ialarm
                , icalproperty_new_description(_("Xfcalendar default alarm")));
        icalcomponent_add_component(ievent, ialarm);

    /********** AUDIO **********/
        if XFICAL_STR_EXISTS(app->sound) {
            ialarm = icalcomponent_vanew(ICAL_VALARM_COMPONENT
                ,icalproperty_new_action(ICAL_ACTION_AUDIO)
                ,0);
            icalcomponent_add_property(ialarm
                , icalproperty_new_trigger(trg));
            attach = icalattach_new_from_url(app->sound);
            icalcomponent_add_property(ialarm
                , icalproperty_new_attach(attach));
            icalcomponent_add_component(ievent, ialarm);
        }
    }

    icalcomponent_add_component(ical, ievent);
    fical_modified = TRUE;
    icalset_mark(fical);
    return(xf_uid);
}

 /* Read EVENT from ical datafile.
  *ical_uid:  key of ical EVENT appointment which is to be read
  * returns: NULL if failed and appt_type pointer to appt_type struct 
  *          filled with data if successfull.
  *          This appt_type struct is owned by the routine. 
  *          Do not deallocate it.
  *          It will be overdriven by next invocation of this function.
  */
appt_type *xfical_app_get(char *ical_uid)
{
    icalcomponent *c = NULL, *ca = NULL;
    icalproperty *p = NULL;
    gboolean key_found=FALSE;
    const char *text;
    static appt_type app;
    struct icaltimetype itime;
    icalproperty_transp xf_transp;
    struct icaltriggertype trg;
    gint mins;
    icalattach *attach;

    for (c = icalcomponent_get_first_component(ical, ICAL_VEVENT_COMPONENT); 
         (c != 0) && (!key_found);
         c = icalcomponent_get_next_component(ical, ICAL_VEVENT_COMPONENT)) {
        text = icalcomponent_get_uid(c);
        if (strcmp(text, ical_uid) == 0) {
        /*********** Defaults ***********/
            key_found = TRUE;
            app.title = NULL;
            app.location = NULL;
            app.note = NULL;
            app.sound = NULL;
            app.uid = NULL;
            app.allDay = FALSE;
            app.alarm = -1;
            app.alarmTimeType = -1;
            app.availability = -1;
            strcpy(app.starttime, "");
            strcpy(app.endtime, "");
        /*********** Properties ***********/
            for (p = icalcomponent_get_first_property(c, ICAL_ANY_PROPERTY);
                 p != 0;
                 p = icalcomponent_get_next_property(c, ICAL_ANY_PROPERTY)) {
                text = icalproperty_get_property_name(p);
                /* these are in icalderivedproperty.h */
                if (strcmp(text, "SUMMARY") == 0)
                    app.title = (char *) icalproperty_get_summary(p);
                else if (strcmp(text, "LOCATION") == 0)
                    app.location = (char *) icalproperty_get_location(p);
                else if (strcmp(text, "DESCRIPTION") == 0)
                    app.note = (char *) icalproperty_get_description(p);
                else if (strcmp(text, "UID") == 0)
                    app.uid = (char *) icalproperty_get_uid(p);
                else if (strcmp(text, "TRANSP") == 0) {
                    xf_transp = icalproperty_get_transp(p);
                    if (xf_transp == ICAL_TRANSP_TRANSPARENT)
                        app.availability = 0;
                    else if (xf_transp == ICAL_TRANSP_OPAQUE)
                        app.availability = 1;
                    else 
                        app.availability = -1;
                }
                else if (strcmp(text, "DTSTART") == 0) {
                    itime = icalproperty_get_dtstart(p);
                    text  = icaltime_as_ical_string(itime);
                    strcpy(app.starttime, text);
                    if (icaltime_is_date(itime))
                        app.allDay = TRUE;
                    if (strlen(app.endtime))
                        strcpy(app.endtime, text);
                }
                else if (strcmp(text, "DTEND") == 0) {
                    itime = icalproperty_get_dtend(p);
                    text  = icaltime_as_ical_string(itime);
                    strcpy(app.endtime, text);
                }
            }
        /*********** Alarms ***********/
            for (ca = icalcomponent_get_first_component(c
                        , ICAL_VALARM_COMPONENT); 
                 ca != 0;
                 ca = icalcomponent_get_next_component(c
                        , ICAL_VALARM_COMPONENT)) {
                for (p = icalcomponent_get_first_property(ca
                        , ICAL_ANY_PROPERTY);
                     p != 0;
                     p = icalcomponent_get_next_property(ca
                        , ICAL_ANY_PROPERTY)) {
                    text = icalproperty_get_property_name(p);
                    if (strcmp(text, "TRIGGER") == 0) {
                        trg = icalproperty_get_trigger(p);
                        if (icaltime_is_null_time(trg.time)) {
                            mins = icaldurationtype_as_int(trg.duration)/-60;
                            if (trg.duration.minutes == mins) {
                                app.alarm = mins;
                                app.alarmTimeType = 0;
                            }
                            else if (trg.duration.hours * 60 == mins) {
                                app.alarm = mins/60;
                                app.alarmTimeType = 1;
                            }
                            else if (trg.duration.days * 60 * 24 == mins) {
                                app.alarm = mins/60/24;
                                app.alarmTimeType = 2;
                            }
                            else {
                                app.alarm = mins;
                                app.alarmTimeType = 0;
                            }
                        }
                        else
                            g_warning("ical_code: Can not process time type triggers\n");
                    }
                    else if (strcmp(text, "ATTACH") == 0) {
                        attach = icalproperty_get_attach(p);
                        app.sound = (char *)icalattach_get_url(attach);
                    }
                }
            }
        }
    } 
    if (key_found)
        return(&app);
    else
        return(NULL);
}

 /* removes EVENT with ical_uid from ical file
  * ical_uid: pointer to ical_uid to be deleted
  * returns: TRUE is successfull, FALSE if failed
  */
gboolean xfical_app_del(char *ical_uid)
{
    icalcomponent *c;
    char *uid;

    if (ical_uid == NULL)
        return(FALSE);
    for (c = icalcomponent_get_first_component(ical, ICAL_VEVENT_COMPONENT); 
         c != 0;
         c = icalcomponent_get_next_component(ical, ICAL_VEVENT_COMPONENT)) {
        uid = (char *)icalcomponent_get_uid(c);
        if (strcmp(uid, ical_uid) == 0) {
            icalcomponent_remove_component(ical, c);
            fical_modified = TRUE;
            icalset_mark(fical);
            return(TRUE);
        }
    } 
    return(FALSE);
}

 /* Read next EVENT on the specified date from ical datafile.
  * a_day:  start date of ical EVENT appointment which is to be read
  * hh_mm:  return the start and end time of EVENT. NULL=start from 00
  * returns: NULL if failed and appt_type pointer to appt_type struct 
  *          filled with data if successfull.
  *          This appt_type struct is owned by the routine. 
  *          Do not deallocate it.
  *          It will be overdriven by next invocation of this function.
  */
appt_type *getnext_ical_app_on_day(char *a_day, char *hh_mm)
{
    struct icaltimetype adate, sdate, edate;
    static icalcomponent *c;
    gboolean date_found=FALSE;
    char *text;
    static appt_type app;

/* FIXME: does not find events which start on earlier dates and continues
          to this date */
    adate = icaltime_from_string(a_day);
    if (strlen(hh_mm) == 0){ /* start */
        c = icalcomponent_get_first_component(ical, ICAL_VEVENT_COMPONENT); 
    }
    for ( ; 
         (c != 0) && (!date_found);
         c = icalcomponent_get_next_component(ical, ICAL_VEVENT_COMPONENT)){
        sdate = icalcomponent_get_dtstart(c);
        if (icaltime_compare_date_only(adate, sdate) == 0){
            date_found = TRUE;
            app.title    = (char *)icalcomponent_get_summary(c);
            app.note     = (char *)icalcomponent_get_description(c);
            app.uid      = (char *)icalcomponent_get_uid(c);
            app.allDay   = FALSE;
            app.alarm    = -1;
            text  = (char *)icaltime_as_ical_string(sdate);
            strcpy(app.starttime, text);
            edate = icalcomponent_get_dtend(c);
            text  = (char *)icaltime_as_ical_string(edate);
            strcpy(app.endtime, text);

            if (icaltime_is_date(sdate)) {
                strcpy(hh_mm, "xx:xx-xx:xx");
                app.allDay = TRUE;
            }
            else {
                if (icaltime_is_null_time(edate)) {
                    edate.hour = sdate.hour;
                    edate.minute = sdate.minute;
                }
                sprintf(hh_mm, "%02d:%02d-%02d:%02d", sdate.hour, sdate.minute
                        , edate.hour, edate.minute);
            }
        }
    } 
    if (date_found)
        return(&app);
    else
        return(0);
}

 /* find next day in this year and month where any EVENTs start
  * year: Year to be searched
  * month: Month to be searched
  * day: -1 means start search from day 1. other values continue search
  * returns: day number of next day having EVENT. 0=no more EVENTs found
  */
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

 /* remove all EVENTs starting this day
  * a_day: date to clear (yyyymmdd format)
  */
void rmday_ical_app(char *a_day)
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
    fical_modified = TRUE;
    icalset_mark(fical);
}

void free_alarm(gpointer galarm, gpointer dummy)
{
    alarm_struct *alarm;

    alarm = (alarm_struct *)galarm;

    if (strcmp(alarm->action->str, "DISPLAY") == 0) {
        g_string_free(alarm->title, TRUE);
        g_string_free(alarm->description, TRUE);
    }
    else if (strcmp(alarm->action->str, "AUDIO") == 0) {
        g_string_free(alarm->sound, TRUE);
    }
    /*
    else if (strcmp(alarm->action->str, "EMAIL") == 0) {
    }
    else if (strcmp(alarm->action->str, "PROCEDURE") == 0) {
    }
    */
    g_string_free(alarm->uid, TRUE);
    g_string_free(alarm->action, TRUE);
    g_string_free(alarm->alarm_time, TRUE);
    g_string_free(alarm->event_time, TRUE);
    g_free(alarm);
}

gint alarm_order(gconstpointer a, gconstpointer b)
{
    struct icaltimetype t1, t2;

    t1=icaltime_from_string(((alarm_struct *)a)->alarm_time->str);
    t2=icaltime_from_string(((alarm_struct *)b)->alarm_time->str);

    return(icaltime_compare(t1, t2));
}

void build_ical_alarm_list()
{
    struct icaltimetype event_dtstart, alarm_time, cur_time;
    icalcomponent *c, *ca;
    icalproperty *p;
    icalproperty_status stat;
    struct icaltriggertype trg;
    char *s, *suid, *ssummary, *sdescription, *saction;
    gboolean trg_found;
    alarm_struct *new_alarm;
    icalattach *attach;

    cur_time = ical_get_current_local_time();
    /* read all alarms and build a list */
    g_list_foreach(alarm_list, free_alarm, NULL);
    g_list_free(alarm_list);
    alarm_list = NULL;
    xfical_file_open();
    for (c = icalcomponent_get_first_component(ical, ICAL_VEVENT_COMPONENT);
         c != 0;
         c = icalcomponent_get_next_component(ical, ICAL_VEVENT_COMPONENT)){
        suid = (char*)icalcomponent_get_uid(c);
        ssummary = (char*)icalcomponent_get_summary(c);
        sdescription = (char*)icalcomponent_get_description(c);
        event_dtstart = icalcomponent_get_dtstart(c);
        for (ca = icalcomponent_get_first_component(c, ICAL_VALARM_COMPONENT);
             ca != 0;
             ca = icalcomponent_get_next_component(c, ICAL_VALARM_COMPONENT)) {
            trg_found=FALSE;
            for (p = icalcomponent_get_first_property(ca, ICAL_ANY_PROPERTY);
                 p != 0;
                 p = icalcomponent_get_next_property(ca, ICAL_ANY_PROPERTY)) {
                s = (char *)icalproperty_get_property_name(p);
                if (strcmp(s, "ACTION") == 0)
                    stat = icalproperty_get_action(p);
                else if (strcmp(s, "DESCRIPTION") == 0)
                    sdescription = (char*)icalproperty_get_description(p);
                else if (strcmp(s, "ATTACH") == 0) {
                    attach = icalproperty_get_attach(p);
                }
                else if (strcmp(s, "TRIGGER") == 0) {
                    trg = icalproperty_get_trigger(p);
                    trg_found = TRUE;
                }
                else
                    g_warning("Unknown property in Alarm\n");
 
            } 
            if (trg_found) {
            /* all data available. let's pack it if alarm is still active */
                alarm_time = icaltime_add(event_dtstart, trg.duration);
                if (icaltime_compare(cur_time, alarm_time) <= 0) { /* active */
                    new_alarm = g_new(alarm_struct, 1);
                    new_alarm->uid = g_string_new(suid);
                    if (stat == ICAL_ACTION_DISPLAY) {
                        new_alarm->action = g_string_new("DISPLAY");
                        new_alarm->title = g_string_new(ssummary);
                        new_alarm->description = g_string_new(sdescription);
                    }
                    else if (stat == ICAL_ACTION_AUDIO) {
                        new_alarm->action = g_string_new("AUDIO");
                        new_alarm->sound = g_string_new(
                                    /*
                                    ical has not implemeneted data type attach
                                    icalattach_get_data(attach));
                                    */
                                    icalattach_get_url(attach));
                    }
                    else if (stat == ICAL_ACTION_EMAIL) {
                        new_alarm->action = g_string_new("EMAIL");
                    }
                    else if (stat == ICAL_ACTION_PROCEDURE) {
                        new_alarm->action = g_string_new("PROCEDURE");
                    }
                    else
                        g_warning("Unknown VALARM ACTION %d\n", stat);
                    new_alarm->alarm_time = g_string_new(
                        icaltime_as_ical_string(alarm_time));
                    new_alarm->event_time = g_string_new(
                        icaltime_as_ical_string(event_dtstart));
                    alarm_list = g_list_append(alarm_list, new_alarm);
                } /* active alarm */
            } /* trg_found */
        }  /* ALARMS */
    }  /* EVENTS */
    alarm_list = g_list_sort(alarm_list, alarm_order);
    xfical_file_close();
}

gboolean ical_alarm_passed(char *alarm_stime)
{
    struct icaltimetype alarm_time, cur_time;

    cur_time = ical_get_current_local_time();
    alarm_time = icaltime_from_string(alarm_stime);
    if (icaltime_compare(cur_time, alarm_time) > 0)
        return(TRUE);
    return(FALSE);
}
