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

void add_ical_app(char *text, char *a_day)
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

void rm_ical_app(char *a_day)
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

