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
#include "mainbox.h"
#include "ical-code.h"

#define MAX_APP_LENGTH 4096
#define LEN_BUFFER 1024
#define CHANNEL  "xfcalendar"
#define RCDIR    "xfce4" G_DIR_SEPARATOR_S "xfcalendar"
#define APPOINTMENT_FILE "appointments.ics"

#define XFICAL_STR_EXISTS(str) ((str != NULL) && (str[0] != 0))

static icalcomponent *ical;
static icalset* fical = NULL;
static gboolean fical_modified=TRUE;

extern GList *alarm_list;

gboolean xfical_file_open(void)
{
	gchar *ficalpath; 
    icalcomponent *iter;
    gint cnt=0;

    if (fical != NULL)
        g_warning("ical-code:xfical_file_open: fical already open\n");
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
    fical = NULL;
    if (fical_modified) {
        fical_modified = FALSE;
        xfical_alarm_build_list(FALSE);
        xfcalendar_mark_appointments();
    }
}

struct icaltimetype ical_get_current_local_time()
{
    time_t t;
    struct tm *tm;
    struct icaltimetype ctime;
    /*
    char koe[200];
    */

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
    temp->availability = 1;
    return(temp);
}

char *app_add_int(appt_type *app, gboolean add, char *uid
        , struct icaltimetype cre_time)
{
    icalcomponent *ievent, *ialarm;
    struct icaltimetype ctime, create_time;
    static gchar xf_uid[1000];
    gchar xf_host[501];
    struct icaltriggertype trg;
    gint duration=0;
    icalattach *attach;
    struct icalrecurrencetype rrule;

    ctime = ical_get_current_local_time();
    if (add) {
        gethostname(xf_host, 500);
        sprintf(xf_uid, "Xfcalendar-%s-%lu@%s", icaltime_as_ical_string(ctime), 
                (long) getuid(), xf_host);
        create_time = ctime;
    }
    else { /* mod */
        strcpy(xf_uid, uid);
        if (icaltime_is_null_time(cre_time))
            create_time = ctime;
        else
            create_time = cre_time;
    }

    ievent = icalcomponent_vanew(ICAL_VEVENT_COMPONENT
           ,icalproperty_new_uid(xf_uid)
           ,icalproperty_new_categories("XFCALNOTE")
           ,icalproperty_new_class(ICAL_CLASS_PUBLIC)
           ,icalproperty_new_dtstamp(ctime)
           ,icalproperty_new_created(create_time)
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
    if (app->freq != XFICAL_FREQ_NONE) {
        switch(app->freq) {
            case XFICAL_FREQ_DAILY:
                rrule = icalrecurrencetype_from_string("FREQ=DAILY");
                break;
            case XFICAL_FREQ_WEEKLY:
                rrule = icalrecurrencetype_from_string("FREQ=WEEKLY");
                break;
            case XFICAL_FREQ_MONTHLY:
                rrule = icalrecurrencetype_from_string("FREQ=MONTHLY");
                break;
            default:
                g_warning("ical-code: Unsupported freq\n");
                icalrecurrencetype_clear(&rrule);
        }
        icalcomponent_add_property(ievent
           , icalproperty_new_rrule(rrule));
    }
    if (!app->allDay  && app->alarmtime != 0)  {
        switch (app->alarmtime) {
            case 0:
                duration = 0;
                break;
            case 1:
                duration = 5 * 60;
                break;
            case 2:
                duration = 15 * 60;
                break;
            case 3:
                duration = 30 * 60;
                break;
            case 4:
                duration = 45 * 60;
                break;
            case 5:
                duration = 1 * 3600;
                break;
            case 6:
                duration = 2 * 3600;
                break;
            case 7:
                duration = 4 * 3600;
                break;
            case 8:
                duration = 8 * 3600;
                break;
            case 9:
                duration = 24 * 3600;
                break;
            case 10:
                duration = 48 * 3600;
                break;
        }
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

 /* add EVENT type ical appointment to ical file
 * app: pointer to filled appt_type structure, which is stored
 *      Caller is responsible for filling and allocating and freeing it.
 *  returns: NULL if failed and new ical id if successfully added. 
 *           This ical id is owned by the routine. Do not deallocate it.
 *           It will be overdriven by next invocation of this function.
 */
char *xfical_app_add(appt_type *app)
{
    return(app_add_int(app, TRUE, NULL, icaltime_null_time()));
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
    struct icalrecurrencetype rrule;

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
            app.alarmtime = 0;
            app.availability = -1;
            app.freq = XFICAL_FREQ_NONE;
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
                else if (strcmp(text, "RRULE") == 0) {
                    rrule = icalproperty_get_rrule(p);
                    switch ( rrule.freq) {
                        case ICAL_DAILY_RECURRENCE:
                            app.freq = XFICAL_FREQ_DAILY;
                            break;
                        case ICAL_WEEKLY_RECURRENCE:
                            app.freq = XFICAL_FREQ_WEEKLY;
                            break;
                        case ICAL_MONTHLY_RECURRENCE:
                            app.freq = XFICAL_FREQ_MONTHLY;
                            break;
                        default:
                            app.freq = XFICAL_FREQ_NONE;
                    }
                }
                /*
                else
                    g_warning("ical-code: Unprocessed property (%s)\n", text);
                    */
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
                            mins = icaldurationtype_as_int(trg.duration) * -1;
                            switch (mins) {
                                case 0:
                                    app.alarmtime = 0;
                                    break;
                                case 5 * 60:
                                    app.alarmtime = 1;
                                    break;
                                case 15 * 60:
                                    app.alarmtime = 2;
                                    break;
                                case 30 * 60:
                                    app.alarmtime = 3;
                                    break;
                                case 45 * 60:
                                    app.alarmtime = 4;
                                    break;
                                case 1 * 3600:
                                    app.alarmtime = 5;
                                    break;
                                case 2 * 3600:
                                    app.alarmtime = 6;
                                    break;
                                case 4 * 3600:
                                     app.alarmtime = 7;
                                     break;
                                case 8 * 3600:
                                    app.alarmtime = 8;
                                    break;
                                case 24 * 3600:
                                    app.alarmtime = 9;
                                    break;
                                case 48 * 3600:
                                    app.alarmtime = 10;
                                    break;
                                default:
                                    app.alarmtime = 1;
                            }
                        }
                        else
                            g_warning("ical_code: Can not process time type triggers\n");
                    }
                    else if (strcmp(text, "ATTACH") == 0) {
                        attach = icalproperty_get_attach(p);
                        app.sound = (char *)icalattach_get_url(attach);
                    }
                    else if (app.note == NULL
                            && strcmp(text, "DESCRIPTION") == 0) {
                        app.note = (char *) icalproperty_get_description(p);
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

 /* modify EVENT type ical appointment in ical file
 * key: char pointer to ical key to modify
 * app: pointer to filled appt_type structure, which is stored
 *      Caller is responsible for filling and allocating and freeing it.
  * returns: TRUE is successfull, FALSE if failed
 */
gboolean xfical_app_mod(char *ical_uid, appt_type *app)
{
    icalcomponent *c;
    char *uid;
    const char *text;
    icalproperty *p = NULL;
    gboolean key_found = FALSE;
    struct icaltimetype create_time = icaltime_null_time();

    if (ical_uid == NULL)
        return(FALSE);
    for (c = icalcomponent_get_first_component(ical, ICAL_VEVENT_COMPONENT); 
         c != 0 && !key_found;
         c = icalcomponent_get_next_component(ical, ICAL_VEVENT_COMPONENT)) {
        uid = (char *)icalcomponent_get_uid(c);
        if (strcmp(uid, ical_uid) == 0) {
            if ((p = icalcomponent_get_first_property(c,ICAL_CREATED_PROPERTY)))
                    create_time = icalproperty_get_created(p);
            icalcomponent_remove_component(ical, c);
            key_found = TRUE;
        }
    } 
    if (!key_found)
        return(FALSE);

    app_add_int(app, FALSE, ical_uid, create_time);
    return(TRUE);
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
appt_type *xfical_app_get_next_on_day(char *a_day, gboolean first)
{
    struct icaltimetype adate, sdate, edate, ndate;
    icalcomponent *c;
    static icalcompiter ci;
    gboolean date_found=FALSE;
    char *uid;
    appt_type *app;
    struct icalrecurrencetype rrule;
    icalrecur_iterator* ri;
    icalproperty *p = NULL;

/* FIXME: does not find events which start on earlier dates and continues
          to this date */
    adate = icaltime_from_string(a_day);
    if (first)
        ci = icalcomponent_begin_component(ical, ICAL_VEVENT_COMPONENT);
    for ( ; 
         (icalcompiter_deref(&ci) != 0) && (!date_found);
          icalcompiter_next(&ci)) {
        c = icalcompiter_deref(&ci);
        sdate = icalcomponent_get_dtstart(c);
        edate = icalcomponent_get_dtend(c);
        if (icaltime_compare_date_only(adate, sdate) == 0) {
            date_found = TRUE;
        }
        else if ((p = icalcomponent_get_first_property(c, ICAL_RRULE_PROPERTY))
                 != 0) { /* need to check recurring EVENTs */
            rrule = icalproperty_get_rrule(p);
            ri = icalrecur_iterator_new(rrule, sdate);
            ndate = icaltime_null_time();
            for (ndate = icalrecur_iterator_next(ri);
                 (!icaltime_is_null_time(ndate))
                 && (icaltime_compare_date_only(adate, ndate) > 0) ;
                 ndate = icalrecur_iterator_next(ri)) {
            }
            if (icaltime_compare_date_only(adate, ndate) == 0) {
                date_found = TRUE;
            } 
        }
    } 
    if (date_found) {
        uid = (char *)icalcomponent_get_uid(c);
        app = xfical_app_get(uid);
        return(app);
    }
    else
        return(0);
}

 /* Mark days with appointments into calendar
  * year: Year to be searched
  * month: Month to be searched
  */
void xfical_mark_calendar(GtkCalendar *gtkcal, int year, int month)
{
    struct icaltimetype sdate, ndate;
    static icalcomponent *c;
    struct icalrecurrencetype rrule;
    icalrecur_iterator* ri;
    icalproperty *p = NULL;

    gtk_calendar_freeze(gtkcal);
    gtk_calendar_clear_marks(gtkcal);
    for (c = icalcomponent_get_first_component(ical, ICAL_VEVENT_COMPONENT);
         c != 0; 
         c = icalcomponent_get_next_component(ical, ICAL_VEVENT_COMPONENT)) {
        sdate = icalcomponent_get_dtstart(c);
        if ((sdate.year == year) && (sdate.month == month)) {
            gtk_calendar_mark_day(gtkcal, sdate.day);
        }
        if ((p = icalcomponent_get_first_property(c, ICAL_RRULE_PROPERTY))
                 != 0) { /* need to check recurring EVENTs */
            rrule = icalproperty_get_rrule(p);
            ri = icalrecur_iterator_new(rrule, sdate);
            ndate = icaltime_null_time();
            for (ndate = icalrecur_iterator_next(ri);
                 (!icaltime_is_null_time(ndate))
                 && (ndate.year <= year) && (ndate.month <= month);
                 ndate = icalrecur_iterator_next(ri)) {
                if ((ndate.year == year) && (ndate.month == month)) {
                    gtk_calendar_mark_day(gtkcal, ndate.day);
                }
            }
        } 
    } 
    gtk_calendar_thaw(gtkcal);
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

void alarm_free(gpointer galarm, gpointer dummy)
{
    alarm_struct *alarm;

    alarm = (alarm_struct *)galarm;

    if (strcmp(alarm->action->str, "DISPLAY") == 0) {
        if (alarm->title != NULL)
            g_string_free(alarm->title, TRUE);
        if (alarm->description != NULL)
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

void alarm_add(icalproperty_status action
             , char *uid, char *title, char *description, char *sound
             , struct icaltimetype alarm_time, struct icaltimetype event_time)
{
    alarm_struct *new_alarm;
                                                                                
    new_alarm = g_new(alarm_struct, 1);
    if (action == ICAL_ACTION_DISPLAY) {
        new_alarm->action = g_string_new("DISPLAY");
        new_alarm->title = g_string_new(title);
        new_alarm->description = g_string_new(description);
        new_alarm->sound = NULL;
    }
    else if (action == ICAL_ACTION_AUDIO) {
        new_alarm->action = g_string_new("AUDIO");
        new_alarm->title = NULL;
        new_alarm->description = NULL;
        new_alarm->sound = g_string_new(sound);
    }
    else {
        g_warning("Unsupported ALARM action type (%d)\n",  new_alarm->action);
        return;
    }
    new_alarm->uid = g_string_new(uid);
    new_alarm->alarm_time = g_string_new(
        icaltime_as_ical_string(alarm_time));
    new_alarm->event_time = g_string_new(
        icaltime_as_ical_string(event_time));
    alarm_list = g_list_append(alarm_list, new_alarm);
}

void xfical_alarm_build_list(gboolean first_list_today)
{
    struct icaltimetype event_dtstart, alarm_time, cur_time, next_date;
    icalcomponent *c, *ca;
    icalproperty *p;
    icalproperty_status stat=ICAL_ACTION_DISPLAY;
    struct icaltriggertype trg;
    char *s, *suid, *ssummary, *sdescription, *ssound;
    gboolean trg_found;
    icalattach *attach = NULL;
    struct icalrecurrencetype rrule;
    icalrecur_iterator* ri;

    cur_time = ical_get_current_local_time();
    g_list_foreach(alarm_list, alarm_free, NULL);
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
        if (first_list_today && icaltime_is_date(event_dtstart)) {
        /* this is special pre 4.3 xfcalendar compatibility alarm:
           Send alarm window always for date type events. */
            if (icaltime_compare_date_only(event_dtstart, cur_time) == 0) {
                alarm_add(ICAL_ACTION_DISPLAY
                        , suid, ssummary, sdescription, NULL
                        , event_dtstart, event_dtstart);
            }
            else if ((p = icalcomponent_get_first_property(c
                , ICAL_RRULE_PROPERTY)) != 0) { /* check recurring EVENTs */
                rrule = icalproperty_get_rrule(p);
                ri = icalrecur_iterator_new(rrule, event_dtstart);
                next_date = icaltime_null_time();
                for (next_date = icalrecur_iterator_next(ri);
                    (!icaltime_is_null_time(next_date))
                    && (icaltime_compare_date_only(cur_time, next_date) > 0) ;
                    next_date = icalrecur_iterator_next(ri)) {
                }
                if (icaltime_compare_date_only(next_date, cur_time) == 0) {
                    alarm_add(ICAL_ACTION_DISPLAY
                            , suid, ssummary, sdescription,NULL
                            , next_date, event_dtstart);
                }
            }
        }
        for (ca = icalcomponent_get_first_component(c, ICAL_VALARM_COMPONENT);
             ca != 0;
             ca = icalcomponent_get_next_component(c, ICAL_VALARM_COMPONENT)) {
            trg_found = FALSE;
            sdescription = NULL;
            ssound = NULL;
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
                    ssound = (char *)icalattach_get_url(attach);
                }
                else if (strcmp(s, "TRIGGER") == 0) {
                    trg = icalproperty_get_trigger(p);
                    trg_found = TRUE;
                }
                else
                    g_warning("ical-code: Unknown property (%s) in Alarm\n",s);
            } 
            if (trg_found) {
            /* all data available. let's pack it if alarm is still active */
                alarm_time = icaltime_add(event_dtstart, trg.duration);
                if (icaltime_compare(cur_time, alarm_time) <= 0) { /* active */
                    alarm_add(stat, suid, ssummary, sdescription, ssound
                            , alarm_time, event_dtstart);
                } /* active alarm */
                else if ((p = icalcomponent_get_first_property(c
                    , ICAL_RRULE_PROPERTY)) != 0) { /* check recurring EVENTs */                    rrule = icalproperty_get_rrule(p);
                    ri = icalrecur_iterator_new(rrule, event_dtstart);
                    next_date = icaltime_null_time();
                    for (next_date = icalrecur_iterator_next(ri);
                        (!icaltime_is_null_time(next_date))
                        && (icaltime_compare_date_only(cur_time, next_date) > 0) ;
                        next_date = icalrecur_iterator_next(ri)) {
                    }
                    alarm_time = icaltime_add(next_date, trg.duration);
                    if (icaltime_compare(cur_time, alarm_time) <= 0) {
                        alarm_add(stat, suid, ssummary, sdescription, ssound
                            , alarm_time, event_dtstart);
                    }
                }
            } /* trg_found */
        }  /* ALARMS */
    }  /* EVENTS */
    alarm_list = g_list_sort(alarm_list, alarm_order);
    xfical_file_close();
}

gboolean xfical_alarm_passed(char *alarm_stime)
{
    struct icaltimetype alarm_time, cur_time;

    cur_time = ical_get_current_local_time();
    alarm_time = icaltime_from_string(alarm_stime);
    if (icaltime_compare(cur_time, alarm_time) > 0)
        return(TRUE);
    return(FALSE);
}
