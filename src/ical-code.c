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
#include "ical-code.h"

#define MAX_APP_LENGTH 4096
#define LEN_BUFFER 1024
#define CHANNEL  "xfcalendar"
#define RCDIR    "xfce4" G_DIR_SEPARATOR_S "xfcalendar"
#define APPOINTMENT_FILE "appointments.ics"


static icalcomponent *ical;
static icalset* fical;


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

 /* allocates memory and initializes it for new ical_type structure
 *  returns: NULL if failed and pointer to appt_type if successfull.
 *                You must free it after not being used anymore. (g_free())
 */
appt_type *xf_alloc_ical_app()
{
    appt_type *temp;

    temp = g_new0(appt_type, 1);
    return(temp);
}


 /* add EVENT type ical appointment to ical file
 * app: pointer to filled appt_type structure, which is stored
 *      You are responsible for filling and allocating and freeing it.
 *  returns: NULL if failed and new ical id if successfully added. 
 *           This ical id is owned by the routine. Do not deallocate it.
 *           It will be overdriven by next invocation of this function.
 */
char *xf_add_ical_app(appt_type *app)
{
    icalcomponent *ievent;
    struct icaltimetype ctime = icaltime_from_timet(time(0), 0);
    struct icaltimetype sdate, edate;
    static gchar xf_uid[1000];
    gchar xf_host[501];

    sdate = icaltime_from_string(app->starttime);
    edate = icaltime_from_string(app->endtime);
    gethostname(xf_host, 500);
    sprintf(xf_uid, "Xfcalendar-%s-%lu@%s", icaltime_as_ical_string(ctime), 
                (long) getuid(), xf_host);
    ievent = icalcomponent_vanew(ICAL_VEVENT_COMPONENT
           ,icalproperty_new_uid(xf_uid)
           ,icalproperty_new_categories("XFCALNOTE")
           ,icalproperty_new_class(ICAL_CLASS_PUBLIC)
           ,icalproperty_new_dtstamp(ctime)
           ,icalproperty_new_created(ctime)
           ,icalproperty_new_summary(app->title)
           ,icalproperty_new_description(app->note)
           ,icalproperty_new_location(app->location)
           ,icalproperty_new_dtstart(sdate)
           ,icalproperty_new_dtend(edate)
           ,0);
    icalcomponent_add_component(ical, ievent);
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
appt_type *xf_get_ical_app(char *ical_uid)
{
    struct icaltimetype itime;
    icalcomponent *c;
    gboolean key_found=FALSE;
    const char *text;
    static appt_type app;

    for (c = icalcomponent_get_first_component(ical, ICAL_VEVENT_COMPONENT); 
         (c != 0) && (!key_found);
         c = icalcomponent_get_next_component(ical, ICAL_VEVENT_COMPONENT)) {
        text = icalcomponent_get_uid(c);
        if (strcmp(text, ical_uid) == 0) {
            key_found = TRUE;
            app.title    = (char *)icalcomponent_get_summary(c);
            app.location = (char *)icalcomponent_get_location(c);
            app.note     = (char *)icalcomponent_get_description(c);
            itime = icalcomponent_get_dtstart(c);
            text  = icaltime_as_ical_string(itime);
            strcpy(app.starttime, text);
            itime = icalcomponent_get_dtend(c);
            text  = icaltime_as_ical_string(itime);
            strcpy(app.endtime, text);
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
gboolean xf_del_ical_app(char *ical_uid)
{
    icalcomponent *c;
    char *uid;

    for (c = icalcomponent_get_first_component(ical, ICAL_VEVENT_COMPONENT); 
         c != 0;
         c = icalcomponent_get_next_component(ical, ICAL_VEVENT_COMPONENT)) {
        uid = (char *)icalcomponent_get_uid(c);
        if (strcmp(uid, ical_uid) == 0) {
            icalcomponent_remove_component(ical, c);
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
            app.location = (char *)icalcomponent_get_location(c);
            app.note     = (char *)icalcomponent_get_description(c);
            text  = (char *)icaltime_as_ical_string(sdate);
            strcpy(app.starttime, text);
            edate = icalcomponent_get_dtend(c);
            text  = (char *)icaltime_as_ical_string(edate);
            strcpy(app.endtime, text);

            if (icaltime_is_date(sdate))
                strcpy(hh_mm, "xx:xx-xx:xx");
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
    icalset_mark(fical);
}

