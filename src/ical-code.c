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
#define RCDIR    "xfce4" G_DIR_SEPARATOR_S "xfcalendar"
#define APPOINTMENT_FILE "appointments.ics"
#define ARCHIVE_FILE "archdev.ics"

#define XFICAL_STR_EXISTS(str) ((str != NULL) && (str[0] != 0))

static icalcomponent *ical,
                     *aical;
static icalset *fical = NULL,
               *afical = NULL;
static gboolean fical_modified = TRUE,
                afical_modified = TRUE;
static gchar *ical_path,
             *aical_path;

static icaltimezone *local_icaltimezone = NULL;
static char *local_icaltimezone_location = NULL;

static int lookback;

extern GList *alarm_list;

void set_default_ical_path (void)
{
    if (ical_path)
        g_free (ical_path);

    ical_path = xfce_resource_save_location(XFCE_RESOURCE_CONFIG,
                    RCDIR G_DIR_SEPARATOR_S APPOINTMENT_FILE, FALSE);
}

void set_ical_path (gchar *path)
{
    if (ical_path)
        g_free (ical_path);

    ical_path = path;
}

void set_aical_path (gchar *path)
{
    if (aical_path)
        g_free (aical_path);

    aical_path = path;
}

void set_lookback (int i) 
{
    lookback = i;
}

gboolean xfical_set_local_timezone(char *location)
{
    if (!location)
    {
        g_warning("xfical_set_local_timezone: no timezone defined\n");
        return (FALSE);
    }
    local_icaltimezone_location=strdup(location);
    if (!local_icaltimezone_location)
    {
        g_warning("xfical_set_local_timezone: Memory exhausted\n");
        return (FALSE);
    }
    local_icaltimezone=icaltimezone_get_builtin_timezone(location);
    if (!local_icaltimezone)
    {
        g_warning("xfical_set_local_timezone: timezone not found\n");
        return (FALSE);
    }
    return (TRUE); 
}

static char*
icaltimezone_get_location_from_vtimezone(icalcomponent *component)
{
/* * basically taken from libical timezone.c
 * * Gets the LOCATION or X-LIC-LOCATION property from a VTIMEZONE. */
    icalproperty *prop;
    const char *location;
    const char *name;
                                                                                
    prop=icalcomponent_get_first_property(component, ICAL_X_PROPERTY);
    while (prop) {
        name = icalproperty_get_x_name(prop);
        if (name && !strcmp (name, "X-LIC-LOCATION")) {
            location=icalproperty_get_x(prop);
            if (location)
                return strdup(location);
        }
        prop=icalcomponent_get_next_property(component, ICAL_X_PROPERTY);
    }
                                                                                
    prop=icalcomponent_get_first_property(component, ICAL_LOCATION_PROPERTY);
    if (prop) {
        location=icalproperty_get_location(prop);
        if (location)
            return strdup(location);
    }
    return NULL;
}

void xfical_add_timezone(icalcomponent *p_ical
        , icalset *p_fical
        , char *location)
{
    icaltimezone *icaltz=NULL;
    icalcomponent *itimezone=NULL;
                                                                                
    if (!location) {
        g_warning("xfical_add_timezone: no location defined \n");
        return;
/*        g_strlcpy(tz, "Europe/Helsinki", 201); */
    }
    g_print("xfical_add_timezone: %s\n", location);
                                                                                
    icaltz=icaltimezone_get_builtin_timezone(location);
    if (icaltz==NULL) {
        g_warning("xfical_add_timezone: timezone not found %s\n", location);
        return;
    }
    itimezone=icaltimezone_get_component(icaltz);
    if (itimezone != NULL) {
        icalcomponent_add_component(p_ical
            , icalcomponent_new_clone(itimezone));
        fical_modified = TRUE;
        icalset_mark(p_fical);
    }
    else
        g_warning("xfical_add_timezone: timezone add failed %s\na", location);
}

void xfical_internal_file_open_timezone(icalcomponent *p_ical
        , icalset *p_fical)
{
    icalcomponent *iter, *iter2;
    gint cnt=0;

    for (iter = icalcomponent_get_first_component(p_ical
                , ICAL_VTIMEZONE_COMPONENT), cnt=0, iter2=NULL;
         iter != 0;
         iter = icalcomponent_get_next_component(p_ical
                , ICAL_VTIMEZONE_COMPONENT)) {
        cnt++;
        iter2=iter;
    }
    if (cnt == 0) { /* timezone missing, need to add one? */
        xfical_add_timezone(p_ical, p_fical, local_icaltimezone_location);
    }
}

void xfical_internal_file_open(icalcomponent **p_ical
        , icalset **p_fical
        , gchar *file_icalpath)
{
    icalcomponent *iter;
    gint cnt=0;
    char *loc;

    if (*p_fical != NULL) {
        g_warning("xfical_internal_file_open: file already open\n");
    }
    if ((*p_fical = icalset_new_file(file_icalpath)) == NULL) {
        g_error("xfical_internal_file_open: Could not open ical file (%s) %s\n"
                , file_icalpath, icalerror_strerror(icalerrno));
    }
    else { /* file open, let's find last VCALENDAR entry */
        for (iter = icalset_get_first_component(*p_fical); 
             iter != 0;
             iter = icalset_get_next_component(*p_fical)) {
            cnt++;
            *p_ical = iter; /* last valid component */
        }
        if (cnt == 0) {
        /* calendar missing, need to add one.  
         * Note: According to standard rfc2445 calendar always needs to
         *       contain at least one other component. So strictly speaking
         *       this is not valid entry before adding an event 
         */
            *p_ical = icalcomponent_vanew(ICAL_VCALENDAR_COMPONENT
                   , icalproperty_new_version("2.0")
                   , icalproperty_new_prodid("-//Xfce//Orage//EN")
                   , 0);
            xfical_add_timezone(*p_ical, *p_fical, local_icaltimezone_location);
            icalset_add_component(*p_fical
                   , icalcomponent_new_clone(*p_ical));
            icalset_commit(*p_fical);
        }
        else { /* VCALENDAR found, let's find VTIMEZONE */
            if (cnt > 1) {
                g_warning("xfical_internal_file_open: Too many top level components in calendar file %s\n", file_icalpath);
            }
            xfical_internal_file_open_timezone(*p_ical, *p_fical);
        }
    }
}

gboolean xfical_file_open (void)
{ 

    xfical_internal_file_open (&ical, &fical, ical_path);
    return (TRUE);
}

gboolean xfical_archive_open (void)
{

    if (!aical_path)
        return (FALSE);

    xfical_internal_file_open (&aical, &afical, aical_path);
    return (TRUE);
}

void xfical_file_close(void)
{
    if(fical == NULL)
        g_warning("xfical_file_close: fical is NULL\n");
    icalset_free(fical);
    fical = NULL;
    if (fical_modified) {
        fical_modified = FALSE;
        xfical_alarm_build_list(FALSE);
        xfcalendar_mark_appointments();
    }
}

void xfical_archive_close(void)
{

    if (!aical_path)
        return;

    if(afical == NULL)
        g_warning("xfical_file_close: afical is NULL\n");
    icalset_free(afical);
    afical = NULL;
    if (afical_modified) {
        afical_modified = FALSE;
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

    ctime = icaltime_current_time_with_zone(local_icaltimezone);
    */
    return (ctime);
}

 /* allocates memory and initializes it for new ical_type structure
  * returns: NULL if failed and pointer to appt_type if successfull.
  *         You must free it after not being used anymore. (g_free())
  */
appt_type *xfical_app_alloc()
{
    appt_type *temp;

    temp = g_new0(appt_type, 1);
    temp->availability = 1;
    return(temp);
}

void app_add_alarm_internal(appt_type *app, icalcomponent *ievent)
{
    icalcomponent *ialarm;
    gint duration=0;
    struct icaltriggertype trg;
    icalattach *attach;

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
    trg.duration = icaldurationtype_from_int(-duration);
    /********** DISPLAY **********/
    ialarm = icalcomponent_vanew(ICAL_VALARM_COMPONENT
        , icalproperty_new_action(ICAL_ACTION_DISPLAY)
        , icalproperty_new_trigger(trg)
        , 0);
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
            , icalproperty_new_action(ICAL_ACTION_AUDIO)
            , icalproperty_new_trigger(trg)
            , 0);
        attach = icalattach_new_from_url(app->sound);
        icalcomponent_add_property(ialarm
            , icalproperty_new_attach(attach));
        if (app->alarmrepeat) {
           /* loop 500 times with 2 secs interval */
            icalcomponent_add_property(ialarm
                , icalproperty_new_repeat(500));
            icalcomponent_add_property(ialarm
                , icalproperty_new_duration(icaldurationtype_from_int(2)));
        }
        icalcomponent_add_component(ievent, ialarm);
    }
}

char *app_add_internal(appt_type *app, gboolean add, char *uid
        , struct icaltimetype cre_time)
{
    icalcomponent *ievent;
    struct icaltimetype ctime, create_time, wtime;
    static gchar xf_uid[1001];
    gchar xf_host[501];
    struct icalrecurrencetype rrule;

    ctime = ical_get_current_local_time();
    if (add) {
        gethostname(xf_host, 500);
        xf_host[500] = '\0';
        g_snprintf(xf_uid, 1000, "Orage-%s-%lu@%s"
                , icaltime_as_ical_string(ctime), (long) getuid(), xf_host);
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
           , icalproperty_new_uid(xf_uid)
           , icalproperty_new_categories("XFCALNOTE")
           , icalproperty_new_class(ICAL_CLASS_PUBLIC)
           , icalproperty_new_dtstamp(ctime)
           , icalproperty_new_created(create_time)
           , 0);

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
    if (app->allDay) { /* cut the string after Date: yyyymmdd */
        app->starttime[8] = '\0';
        app->endtime[8] = '\0';
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
        case XFICAL_FREQ_YEARLY:
            rrule = icalrecurrencetype_from_string("FREQ=YEARLY");
            break;
        default:
            g_warning("ical-code: app_add_internal: Unsupported freq\n");
            icalrecurrencetype_clear(&rrule);
        }
        icalcomponent_add_property (ievent
                                    , icalproperty_new_rrule(rrule));
    }
    if (!app->allDay && app->alarmtime != 0) {
        app_add_alarm_internal(app, ievent);
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
  *           It will be overwrittewritten by next invocation of this function.
  */
char *xfical_app_add(appt_type *app)
{
    return(app_add_internal(app, TRUE, NULL, icaltime_null_time()));
}

void ical_app_get_alarm_internal(icalcomponent *c,  appt_type *app)
{
    icalcomponent *ca = NULL;
    icalproperty *p = NULL;
    gint seconds;
    struct icaltriggertype trg;
    icalattach *attach;

    for (ca = icalcomponent_get_first_component(c, ICAL_VALARM_COMPONENT); 
         ca != 0;
         ca = icalcomponent_get_next_component(c, ICAL_VALARM_COMPONENT)) {
        for (p = icalcomponent_get_first_property(ca, ICAL_ANY_PROPERTY);
             p != 0;
             p = icalcomponent_get_next_property(ca, ICAL_ANY_PROPERTY)) {
            switch (icalproperty_isa(p)) {
                case ICAL_TRIGGER_PROPERTY:
                    trg = icalproperty_get_trigger(p);
                    if (icaltime_is_null_time(trg.time)) {
                        seconds = icaldurationtype_as_int(trg.duration) * -1;
                        switch (seconds) {
                            case 0:
                                app->alarmtime = 0;
                                break;
                            case 5 * 60:
                                app->alarmtime = 1;
                                break;
                            case 15 * 60:
                                app->alarmtime = 2;
                                break;
                            case 30 * 60:
                                app->alarmtime = 3;
                                break;
                            case 45 * 60:
                                app->alarmtime = 4;
                                break;
                            case 1 * 3600:
                                app->alarmtime = 5;
                                break;
                            case 2 * 3600:
                                app->alarmtime = 6;
                                break;
                            case 4 * 3600:
                                 app->alarmtime = 7;
                                 break;
                            case 8 * 3600:
                                app->alarmtime = 8;
                                break;
                            case 24 * 3600:
                                app->alarmtime = 9;
                                break;
                            case 48 * 3600:
                                app->alarmtime = 10;
                                break;
                            default:
                                app->alarmtime = 1;
                                break;
                        }
                    }
                    else
                        g_warning("ical_code: Can not process time triggers\n");
                    break;
                case ICAL_ATTACH_PROPERTY:
                    attach = icalproperty_get_attach(p);
                    app->sound = (char *)icalattach_get_url(attach);
                    break;
                case ICAL_REPEAT_PROPERTY:
                    app->alarmrepeat = TRUE;
                    break;
                case ICAL_DURATION_PROPERTY:
                /* no actions defined */
                    break;
                case ICAL_DESCRIPTION_PROPERTY:
                    if (app->note == NULL)
                        app->note = (char *) icalproperty_get_description(p);
                    break;
                default:
                    g_warning("ical_code: unknown property\n");
                    break;
            }
        }
    }
}

 /* Read EVENT from ical datafile.
  * ical_uid:  key of ical EVENT app-> is to be read
  * returns: NULL if failed and appt_type pointer to appt_type struct
  *          filled with data if successfull.
  *          NOTE: This routine does not fill starttimecur nor endtimecur,
  *          Those are alwasy initialized to null string
  *          This appt_type struct is owned by the routine.
  *          Do not deallocate it.
  *          It will be overdriven by next invocation of this function.
  */
appt_type *xfical_app_get(char *ical_uid)
{
    static appt_type app;
    icalcomponent *c = NULL;
    icalproperty *p = NULL;
    gboolean key_found = FALSE;
    const char *text;
    struct icaltimetype itime;
    icalproperty_transp xf_transp;
    struct icalrecurrencetype rrule;

    for (c = icalcomponent_get_first_component(ical, ICAL_VEVENT_COMPONENT); 
         c != 0 && !key_found;
         c = icalcomponent_get_next_component(ical, ICAL_VEVENT_COMPONENT)) {
        text = icalcomponent_get_uid(c);
        if (strcmp(text, ical_uid) == 0) {
        /*********** Defaults ***********/
            key_found = TRUE;
            app.title = NULL;
            app.location = NULL;
            app.alarmtime = 0;
            app.availability = -1;
            app.allDay = FALSE;
            app.alarmrepeat = FALSE;
            app.note = NULL;
            app.sound = NULL;
            app.uid = NULL;
            app.starttime[0] = '\0';
            app.endtime[0] = '\0';
            app.starttimecur[0] = '\0';
            app.endtimecur[0] = '\0';
            app.freq = XFICAL_FREQ_NONE;
        /*********** Properties ***********/
            for (p = icalcomponent_get_first_property(c, ICAL_ANY_PROPERTY);
                 p != 0;
                 p = icalcomponent_get_next_property(c, ICAL_ANY_PROPERTY)) {
                /* these are in icalderivedproperty.h */
                switch (icalproperty_isa(p)) {
                    case ICAL_SUMMARY_PROPERTY:
                        app.title = (char *) icalproperty_get_summary(p);
                        break;
                    case ICAL_LOCATION_PROPERTY:
                        app.location = (char *) icalproperty_get_location(p);
                        break;
                    case ICAL_DESCRIPTION_PROPERTY:
                        app.note = (char *) icalproperty_get_description(p);
                        break;
                    case ICAL_UID_PROPERTY:
                        app.uid = (char *) icalproperty_get_uid(p);
                        break;
                    case ICAL_TRANSP_PROPERTY:
                        xf_transp = icalproperty_get_transp(p);
                        if (xf_transp == ICAL_TRANSP_TRANSPARENT)
                            app.availability = 0;
                        else if (xf_transp == ICAL_TRANSP_OPAQUE)
                            app.availability = 1;
                        else 
                            app.availability = -1;
                        break;
                    case ICAL_DTSTART_PROPERTY:
                        itime = icalproperty_get_dtstart(p);
                        text  = icaltime_as_ical_string(itime);
                        g_strlcpy(app.starttime, text, 17);
                        if (icaltime_is_date(itime))
                            app.allDay = TRUE;
                        if (app.endtime[0] == '\0')
                            g_strlcpy(app.endtime, text, 17);
                        break;
                    case ICAL_DTEND_PROPERTY:
                        itime = icalproperty_get_dtend(p);
                        text  = icaltime_as_ical_string(itime);
                        g_strlcpy(app.endtime, text, 17);
                        break;
                    case ICAL_RRULE_PROPERTY:
                        rrule = icalproperty_get_rrule(p);
                        switch (rrule.freq) {
                        case ICAL_DAILY_RECURRENCE:
                            app.freq = XFICAL_FREQ_DAILY;
                            break;
                        case ICAL_WEEKLY_RECURRENCE:
                            app.freq = XFICAL_FREQ_WEEKLY;
                            break;
                        case ICAL_MONTHLY_RECURRENCE:
                            app.freq = XFICAL_FREQ_MONTHLY;
                            break;
                        case ICAL_YEARLY_RECURRENCE:
                            app.freq = XFICAL_FREQ_YEARLY;
                            break;
                        default:
                            app.freq = XFICAL_FREQ_NONE;
                            break;
                        }
                        break;
                    default:
                        g_warning("ical-code: unknown property\n");
                        break;
                }
            }
        ical_app_get_alarm_internal(c, &app);
        }
    } 
    if (key_found)
        return(&app);
    else
        return(NULL);
}

 /* modify EVENT type ical appointment in ical file
  * ical_uid: char pointer to ical ical_uid to modify
  * app: pointer to filled appt_type structure, which is stored
  *      Caller is responsible for filling and allocating and freeing it.
  * returns: TRUE is successfull, FALSE if failed
  */
gboolean xfical_app_mod(char *ical_uid, appt_type *app)
{
    icalcomponent *c;
    char *uid;
    icalproperty *p = NULL;
    gboolean key_found = FALSE;
    struct icaltimetype create_time = icaltime_null_time();

    if (ical_uid == NULL) {
        g_warning("xfical_app_mod: Got NULL uid. doing nothing\n");
        return(FALSE);
    }
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
    if (!key_found) {
        g_warning("xfical_app_mod: uid not found. doing nothing\n");
        return(FALSE);
    }

    app_add_internal(app, FALSE, ical_uid, create_time);
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

    if (ical_uid == NULL) {
        g_warning("xfical_app_del: Got NULL uid. doing nothing\n");
        return(FALSE);
     }
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
    g_warning("xfical_app_del: uid not found. doing nothing\n");
    return(FALSE);
}

 /* Read next EVENT on the specified date from ical datafile.
  * a_day:  start date of ical EVENT appointment which is to be read
  * first:  get first appointment is TRUE, if not get next.
  * days:   how many more days to check forward. 0 = only one day
  * returns: NULL if failed and appt_type pointer to appt_type struct 
  *          filled with data if successfull.
  *          This appt_type struct is owned by the routine. 
  *          Do not deallocate it.
  *          It will be overdriven by next invocation of this function.
  */
appt_type *xfical_app_get_next_on_day(char *a_day, gboolean first, gint days)
{
    struct icaltimetype 
              asdate, aedate    /* period to check */
            , sdate, edate      /* event start and end times */
            , nsdate, nedate;   /* repeating event occurrency start and end */
    icalcomponent *c=NULL;
    static icalcompiter ci;
    gboolean date_found=FALSE;
    gboolean date_rec_found=FALSE;
    char *uid;
    appt_type *app;
    struct icalrecurrencetype rrule;
    icalrecur_iterator* ri;
    icalproperty *p = NULL;
    struct icaldurationtype duration;

    asdate = icaltime_from_string(a_day);
    if (days) { /* more that one day to check */
        duration = icaldurationtype_null_duration();
        duration.days = days;
        aedate = icaltime_add(asdate, duration);
    }
    else /* only one day */
        aedate = asdate;
    if (first)
        ci = icalcomponent_begin_component(ical, ICAL_VEVENT_COMPONENT);
    for ( ; 
         icalcompiter_deref(&ci) != 0 && !date_found;
         icalcompiter_next(&ci)) {
        c = icalcompiter_deref(&ci);
        sdate = icalcomponent_get_dtstart(c);
        edate = icalcomponent_get_dtend(c);
        if (icaltime_is_null_time(edate))
            edate = sdate;
        if (icaltime_compare_date_only(sdate, aedate) <= 0
            && icaltime_compare_date_only(asdate, edate) <= 0) {
            date_found = TRUE;
        }
        else if ((p = icalcomponent_get_first_property(c
                , ICAL_RRULE_PROPERTY)) != 0) { /* check recurring EVENTs */
            duration = icaltime_subtract(edate, sdate);
            rrule = icalproperty_get_rrule(p);
            ri = icalrecur_iterator_new(rrule, sdate);
            nsdate = icaltime_null_time();
            for (nsdate = icalrecur_iterator_next(ri),
                    nedate = icaltime_add(nsdate, duration);
                 !icaltime_is_null_time(nsdate)
                    && icaltime_compare_date_only(nedate, asdate) < 0;
                 nsdate = icalrecur_iterator_next(ri),
                    nedate = icaltime_add(nsdate, duration)) {
            }
            if (icaltime_compare_date_only(nsdate, aedate) <= 0
                && icaltime_compare_date_only(asdate, nedate) <= 0) {
                date_found = TRUE;
                date_rec_found = TRUE;
            }
        }
    }
    if (date_found) {
        uid = (char *)icalcomponent_get_uid(c);
        app = xfical_app_get(uid);
        if (date_rec_found) {
            g_strlcpy(app->starttimecur, icaltime_as_ical_string(nsdate), 17);
            g_strlcpy(app->endtimecur, icaltime_as_ical_string(nedate), 17);
        }
        else {
            g_strlcpy(app->starttimecur, app->starttime, 17);
            g_strlcpy(app->endtimecur, app->endtime, 17);
        }
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
    struct icaltimetype sdate, edate, nsdate, nedate;
    static icalcomponent *c;
    struct icalrecurrencetype rrule;
    icalrecur_iterator* ri;
    icalproperty *p = NULL;
    struct icaldurationtype duration;
    gint start_day, day_cnt, end_day;

    gtk_calendar_freeze(gtkcal);
    gtk_calendar_clear_marks(gtkcal);
    for (c = icalcomponent_get_first_component(ical, ICAL_VEVENT_COMPONENT);
         c != 0;
         c = icalcomponent_get_next_component(ical, ICAL_VEVENT_COMPONENT)) {
        sdate = icalcomponent_get_dtstart(c);
        edate = icalcomponent_get_dtend(c);
        if (icaltime_is_null_time(edate))
            edate = sdate;
        if ((sdate.year*12+sdate.month) <= (year*12+month)
                && (year*12+month) <= (edate.year*12+edate.month)) {
                /* event is in our year+month = visible in calendar */
            if (sdate.year == year && sdate.month == month)
                start_day = sdate.day;
            else
                start_day = 1;
            if (edate.year == year && edate.month == month)
                end_day = edate.day;
            else
                end_day = 31;
            for (day_cnt = start_day; day_cnt <= end_day; day_cnt++)
                gtk_calendar_mark_day(gtkcal, day_cnt);
        }
        if ((p = icalcomponent_get_first_property(c
                , ICAL_RRULE_PROPERTY)) != 0) { /* check recurring EVENTs */
            duration = icaltime_subtract(edate, sdate);
            rrule = icalproperty_get_rrule(p);
            ri = icalrecur_iterator_new(rrule, sdate);
            nsdate = icaltime_null_time();
            for (nsdate = icalrecur_iterator_next(ri),
                    nedate = icaltime_add(nsdate, duration);
                 !icaltime_is_null_time(nsdate)
                    && (nsdate.year*12+nsdate.month) <= (year*12+month);
                 nsdate = icalrecur_iterator_next(ri),
                    nedate = icaltime_add(nsdate, duration)) {
                if ((nsdate.year*12+nsdate.month) <= (year*12+month)
                        && (year*12+month) <= (nedate.year*12+nedate.month)) {
                        /* event is in our year+month = visible in calendar */
                    if (nsdate.year == year && nsdate.month == month)
                        start_day = nsdate.day;
                    else
                        start_day = 1;
                    if (nedate.year == year && nedate.month == month)
                        end_day = nedate.day;
                    else
                        end_day = 31;
                    for (day_cnt = start_day; day_cnt <= end_day; day_cnt++)
                        gtk_calendar_mark_day(gtkcal, day_cnt);
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
        if (icaltime_compare_date_only(t, adate) == 0) {
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

    g_string_free(alarm->uid, TRUE);
    g_string_free(alarm->alarm_time, TRUE);
    g_string_free(alarm->event_time, TRUE);
    if (alarm->title != NULL)
        g_string_free(alarm->title, TRUE);
    if (alarm->description != NULL)
        g_string_free(alarm->description, TRUE);
    if (alarm->sound != NULL)
        g_string_free(alarm->sound, TRUE);
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
             , char *uid, char *title, char *description
             , char *sound, gint repeat_cnt, gint repeat_delay
             , struct icaltimetype alarm_time, struct icaltimetype event_time)
{
    alarm_struct *new_alarm;
                                                                                
    new_alarm = g_new(alarm_struct, 1);
    new_alarm->uid = g_string_new(uid);
    new_alarm->title = g_string_new(title);
    new_alarm->alarm_time = g_string_new(
        icaltime_as_ical_string(alarm_time));
    new_alarm->event_time = g_string_new(
        icaltime_as_ical_string(event_time));
    new_alarm->description = g_string_new(description);
    new_alarm->sound = g_string_new(sound);
    new_alarm->repeat_cnt = repeat_cnt;
    new_alarm->repeat_delay = repeat_delay;
    if (description != NULL)
        new_alarm->display = TRUE;
    else
        new_alarm->display = FALSE;
    if (sound != NULL)
        new_alarm->audio = TRUE;
    else
        new_alarm->audio = FALSE;
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
    gint repeat_cnt, repeat_delay;
    struct icaldurationtype duration;

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
                        , suid, ssummary, sdescription, NULL, 0, 0
                        , event_dtstart, event_dtstart);
            }
            else if ((p = icalcomponent_get_first_property(c
                , ICAL_RRULE_PROPERTY)) != 0) { /* check recurring EVENTs */
                rrule = icalproperty_get_rrule(p);
                ri = icalrecur_iterator_new(rrule, event_dtstart);
                next_date = icaltime_null_time();
                for (next_date = icalrecur_iterator_next(ri);
                    !icaltime_is_null_time(next_date)
                        && icaltime_compare_date_only(cur_time, next_date) > 0;
                    next_date = icalrecur_iterator_next(ri)) {
                }
                if (icaltime_compare_date_only(next_date, cur_time) == 0) {
                    alarm_add(ICAL_ACTION_DISPLAY
                            , suid, ssummary, sdescription, NULL, 0, 0
                            , next_date, event_dtstart);
                }
            }
        }
        trg_found = FALSE;
        ssound = NULL;
        repeat_cnt = 0;
        repeat_delay = 0;
        for (ca = icalcomponent_get_first_component(c, ICAL_VALARM_COMPONENT);
             ca != 0;
             ca = icalcomponent_get_next_component(c, ICAL_VALARM_COMPONENT)) {
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
                else if (strcmp(s, "REPEAT") == 0)
                    repeat_cnt = icalproperty_get_repeat(p);
                else if (strcmp(s, "DURATION") == 0) {
                    duration = icalproperty_get_duration(p);
                    repeat_delay = icaldurationtype_as_int(duration);
                }
                else
                    g_warning("ical-code: Unknown property (%s) in Alarm\n",s);
            } 
        }  /* ALARMS */
        if (trg_found) {
        /* all data available. let's pack it if alarm is still active */
            alarm_time = icaltime_add(event_dtstart, trg.duration);
            if (icaltime_compare(cur_time, alarm_time) <= 0) /* active */
                alarm_add(stat, suid, ssummary, sdescription
                        , ssound, repeat_cnt, repeat_delay
                        , alarm_time, event_dtstart);
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
                if (icaltime_compare(cur_time, alarm_time) <= 0)
                    alarm_add(stat, suid, ssummary, sdescription
                        , ssound, repeat_cnt, repeat_delay
                        , alarm_time, event_dtstart);
            }
        } /* trg_found */
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

void xfical_icalcomponent_get_first_occurence_after_threshold (struct tm *threshold, icalcomponent *c)
{
    struct icaltimetype sdate, edate, nsdate, nedate;
    struct icalrecurrencetype rrule;
    struct icaldurationtype duration;
    icalrecur_iterator* ri;
    icalproperty *p = NULL;

    if ((p = icalcomponent_get_first_property(c
           , ICAL_RRULE_PROPERTY)) != 0) { /* check recurring EVENTs */

        sdate = icalcomponent_get_dtstart (c);
        edate = icalcomponent_get_dtend (c);

        duration = icaltime_subtract(edate, sdate);
        rrule = icalproperty_get_rrule(p);
        ri = icalrecur_iterator_new(rrule, sdate);
        nsdate = icaltime_null_time();

        /* We must do a loop for finding the first occurence after the threshold */
        for (nsdate = icalrecur_iterator_next (ri),
                nedate = icaltime_add (nsdate,  duration);
             !icaltime_is_null_time (nsdate)
                && (nedate.year*12 + nedate.month) <= ((threshold->tm_year + 1900)*12 + threshold->tm_mon);
             nsdate = icalrecur_iterator_next(ri),
                nedate = icaltime_add (nsdate, duration)){
        }
    } 

    /* Here I should change the values of the icalcomponent we received */
    icalcomponent_set_dtstart (c, nsdate);
    icalcomponent_set_dtend (c, nedate);
}

void xfical_icalcomponent_archive (icalcomponent *e, struct tm *threshold) 
{
    icalcomponent *d;
    icalproperty *p = NULL;
    gboolean recurrence;

    recurrence = FALSE;
    /* Add to the archive file */
    d = icalcomponent_new_clone (e);
    icalcomponent_add_component(aical, d);
    /* Look for recurrence */
    for (p = icalcomponent_get_first_property(e, ICAL_ANY_PROPERTY);
        p != 0;
        p = icalcomponent_get_next_property(e, ICAL_ANY_PROPERTY)) {
        /* these are in icalderivedproperty.h */
        if (icalproperty_isa (p) == ICAL_RRULE_PROPERTY) {
            recurrence = TRUE;
            break; /* Leave the boat here */
        }
    }
    /* If it's not a recurring appointment we remove it purely and simply */
    if (!recurrence)
        icalcomponent_remove_component(ical, e);
    else
    /* Otherwise we update its next occurence */
        xfical_icalcomponent_get_first_occurence_after_threshold (threshold, e);
}

gboolean xfical_keep_tidy(void)
{

    struct icaltimetype sdate, edate;
    static icalcomponent *c, *e;
    struct tm *threshold;
    time_t t;

    gboolean recurrence;

    xfical_archive_open(); /* Create the file ?*/
    xfical_archive_close(); /* Close it, so we may have the file ready to use. Ugly isn't it? FIXME */

    e = NULL;

    if (   xfical_file_open ()
        && xfical_archive_open ()){

        t = time (NULL);
        threshold = localtime (&t);

        g_message("Current date: %04d-%02d-%02d", threshold->tm_year+1900, threshold->tm_mon+1, threshold->tm_mday);

        threshold->tm_mday = 1;
        if (threshold->tm_mon > lookback) {
            threshold->tm_mon -= lookback;
        }
        else {
            threshold->tm_mon += (12 - lookback);
            threshold->tm_year--;
        }

        g_message("Threshold: %d month(s)", lookback);
        g_message("Archiving before: %04d-%02d-%02d", threshold->tm_year+1900, threshold->tm_mon+1, threshold->tm_mday);

        /* Parse appointment file for looking for items older than the threshold */
        for (c = icalcomponent_get_first_component(ical, ICAL_VEVENT_COMPONENT);
             c != 0;
             c = icalcomponent_get_next_component(ical, ICAL_VEVENT_COMPONENT)) {

            if (e) {
                xfical_icalcomponent_archive (e, threshold);
            }
            recurrence = FALSE;
            sdate = icalcomponent_get_dtstart (c);
            edate = icalcomponent_get_dtend (c);
            if (icaltime_is_null_time (edate)) {
                edate = sdate;
            }
            /* Items with startdate and endate before theshold => archived.
             * Recurring items are duplicated in the archive file, and they are 
             * also updated in the current appointment file with a new startdate and enddate.
             * Items sitting on the threshold are kept as they are.
             */
            if ((edate.year*12 + edate.month) < ((threshold->tm_year+1900)*12 + (threshold->tm_mon+1))) {
                /* Read from appointment files and copy into archive file */
                g_message("End year: %04d", edate.year);
                g_message("End month: %02d", edate.month);
                g_message("End day: %02d", edate.day);
                g_message("We found something dude!");
                e = c;
            }
        }

        if (e) {
            xfical_icalcomponent_archive (e, threshold);
        }
        icalset_mark (fical);
        icalset_commit (fical);
        icalset_mark (afical);
        icalset_commit (afical);
    }
    return(TRUE);
}
