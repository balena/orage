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
#include <glib/gprintf.h>
#include <ical.h>
#include <icalss.h>

#include "event-list.h"
#include "appointment.h"
#include "reminder.h"
#include "mainbox.h"
#include "ical-code.h"
#include "functions.h"

#define MAX_APPT_LENGTH 4096
#define LEN_BUFFER 1024
#define RCDIR    "xfce4" G_DIR_SEPARATOR_S "orage"
#define APPOINTMENT_FILE "orage.ics"

#define XFICAL_STR_EXISTS(str) ((str != NULL) && (str[0] != 0))

typedef struct
{
    struct icaltimetype stime;
    struct icaltimetype etime;
    struct icaldurationtype duration;
} xfical_period;

static icalcomponent *ical = NULL,
                     *aical = NULL;
static icalset *fical = NULL,
               *afical = NULL;
static gboolean fical_modified = FALSE,
                afical_modified = FALSE;
static gchar *ical_path = NULL,
             *aical_path = NULL;

/* timezone handling */
static icaltimezone *utc_icaltimezone = NULL;
static icaltimezone *local_icaltimezone = NULL;
extern char *local_icaltimezone_location;
extern gboolean local_icaltimezone_utc;

static int lookback = 0;

extern GList *alarm_list;
                                                                                

/* Remember to keep this string table in sync with zones.tab
 * This is used only for translations purposes. It makes it
 * possible to translate these timezones.
 */
gchar *trans_timezone[] = {
    N_("Africa"),
    N_("Africa/Abidjan"),
    N_("Africa/Accra"),
    N_("Africa/Addis_Ababa"),
    N_("Africa/Algiers"),
    N_("Africa/Asmera"),
    N_("Africa/Bamako"),
    N_("Africa/Bangui"),
    N_("Africa/Banjul"),
    N_("Africa/Bissau"),
    N_("Africa/Blantyre"),
    N_("Africa/Brazzaville"),
    N_("Africa/Bujumbura"),
    N_("Africa/Cairo"),
    N_("Africa/Casablanca"),
    N_("Africa/Ceuta"),
    N_("Africa/Conakry"),
    N_("Africa/Dakar"),
    N_("Africa/Dar_es_Salaam"),
    N_("Africa/Djibouti"),
    N_("Africa/Douala"),
    N_("Africa/El_Aaiun"),
    N_("Africa/Freetown"),
    N_("Africa/Gaborone"),
    N_("Africa/Harare"),
    N_("Africa/Johannesburg"),
    N_("Africa/Kampala"),
    N_("Africa/Khartoum"),
    N_("Africa/Kigali"),
    N_("Africa/Kinshasa"),
    N_("Africa/Lagos"),
    N_("Africa/Libreville"),
    N_("Africa/Lome"),
    N_("Africa/Luanda"),
    N_("Africa/Lubumbashi"),
    N_("Africa/Lusaka"),
    N_("Africa/Malabo"),
    N_("Africa/Maputo"),
    N_("Africa/Maseru"),
    N_("Africa/Mbabane"),
    N_("Africa/Mogadishu"),
    N_("Africa/Monrovia"),
    N_("Africa/Nairobi"),
    N_("Africa/Ndjamena"),
    N_("Africa/Niamey"),
    N_("Africa/Nouakchott"),
    N_("Africa/Ouagadougou"),
    N_("Africa/Porto-Novo"),
    N_("Africa/Sao_Tome"),
    N_("Africa/Timbuktu"),
    N_("Africa/Tripoli"),
    N_("Africa/Tunis"),
    N_("Africa/Windhoek"),
    N_("America"),
    N_("America/Adak"),
    N_("America/Anchorage"),
    N_("America/Anguilla"),
    N_("America/Antigua"),
    N_("America/Araguaina"),
    N_("America/Aruba"),
    N_("America/Asuncion"),
    N_("America/Barbados"),
    N_("America/Belem"),
    N_("America/Belize"),
    N_("America/Boa_Vista"),
    N_("America/Bogota"),
    N_("America/Boise"),
    N_("America/Buenos_Aires"),
    N_("America/Cambridge_Bay"),
    N_("America/Cancun"),
    N_("America/Caracas"),
    N_("America/Catamarca"),
    N_("America/Cayenne"),
    N_("America/Cayman"),
    N_("America/Chicago"),
    N_("America/Chihuahua"),
    N_("America/Cordoba"),
    N_("America/Costa_Rica"),
    N_("America/Cuiaba"),
    N_("America/Curacao"),
    N_("America/Dawson"),
    N_("America/Dawson_Creek"),
    N_("America/Denver"),
    N_("America/Detroit"),
    N_("America/Dominica"),
    N_("America/Edmonton"),
    N_("America/Eirunepe"),
    N_("America/El_Salvador"),
    N_("America/Fortaleza"),
    N_("America/Glace_Bay"),
    N_("America/Godthab"),
    N_("America/Goose_Bay"),
    N_("America/Grand_Turk"),
    N_("America/Grenada"),
    N_("America/Guadeloupe"),
    N_("America/Guatemala"),
    N_("America/Guayaquil"),
    N_("America/Guyana"),
    N_("America/Halifax"),
    N_("America/Havana"),
    N_("America/Hermosillo"),
    N_("America/Indiana/Indianapolis"),
    N_("America/Indiana/Knox"),
    N_("America/Indiana/Marengo"),
    N_("America/Indiana/Vevay"),
    N_("America/Indianapolis"),
    N_("America/Inuvik"),
    N_("America/Iqaluit"),
    N_("America/Jamaica"),
    N_("America/Jujuy"),
    N_("America/Juneau"),
    N_("America/Kentucky/Louisville"),
    N_("America/Kentucky/Monticello"),
    N_("America/La_Paz"),
    N_("America/Lima"),
    N_("America/Los_Angeles"),
    N_("America/Louisville"),
    N_("America/Maceio"),
    N_("America/Managua"),
    N_("America/Manaus"),
    N_("America/Martinique"),
    N_("America/Mazatlan"),
    N_("America/Mendoza"),
    N_("America/Menominee"),
    N_("America/Merida"),
    N_("America/Mexico_City"),
    N_("America/Miquelon"),
    N_("America/Monterrey"),
    N_("America/Montevideo"),
    N_("America/Montreal"),
    N_("America/Montserrat"),
    N_("America/Nassau"),
    N_("America/New_York"),
    N_("America/Nipigon"),
    N_("America/Nome"),
    N_("America/Noronha"),
    N_("America/Panama"),
    N_("America/Pangnirtung"),
    N_("America/Paramaribo"),
    N_("America/Phoenix"),
    N_("America/Port-au-Prince"),
    N_("America/Port_of_Spain"),
    N_("America/Porto_Velho"),
    N_("America/Puerto_Rico"),
    N_("America/Rainy_River"),
    N_("America/Rankin_Inlet"),
    N_("America/Recife"),
    N_("America/Regina"),
    N_("America/Rio_Branco"),
    N_("America/Rosario"),
    N_("America/Santiago"),
    N_("America/Santo_Domingo"),
    N_("America/Sao_Paulo"),
    N_("America/Scoresbysund"),
    N_("America/Shiprock"),
    N_("America/St_Johns"),
    N_("America/St_Kitts"),
    N_("America/St_Lucia"),
    N_("America/St_Thomas"),
    N_("America/St_Vincent"),
    N_("America/Swift_Current"),
    N_("America/Tegucigalpa"),
    N_("America/Thule"),
    N_("America/Thunder_Bay"),
    N_("America/Tijuana"),
    N_("America/Tortola"),
    N_("America/Vancouver"),
    N_("America/Whitehorse"),
    N_("America/Winnipeg"),
    N_("America/Yakutat"),
    N_("America/Yellowknife"),
    N_("Antarctica"),
    N_("Antarctica/Casey"),
    N_("Antarctica/Davis"),
    N_("Antarctica/DumontDUrville"),
    N_("Antarctica/Mawson"),
    N_("Antarctica/McMurdo"),
    N_("Antarctica/Palmer"),
    N_("Antarctica/South_Pole"),
    N_("Antarctica/Syowa"),
    N_("Antarctica/Vostok"),
    N_("Arctic"),
    N_("Arctic/Longyearbyen"),
    N_("Asia"),
    N_("Asia/Aden"),
    N_("Asia/Almaty"),
    N_("Asia/Amman"),
    N_("Asia/Anadyr"),
    N_("Asia/Aqtau"),
    N_("Asia/Aqtobe"),
    N_("Asia/Ashgabat"),
    N_("Asia/Baghdad"),
    N_("Asia/Bahrain"),
    N_("Asia/Baku"),
    N_("Asia/Bangkok"),
    N_("Asia/Beirut"),
    N_("Asia/Bishkek"),
    N_("Asia/Brunei"),
    N_("Asia/Calcutta"),
    N_("Asia/Chungking"),
    N_("Asia/Colombo"),
    N_("Asia/Damascus"),
    N_("Asia/Dhaka"),
    N_("Asia/Dili"),
    N_("Asia/Dubai"),
    N_("Asia/Dushanbe"),
    N_("Asia/Gaza"),
    N_("Asia/Harbin"),
    N_("Asia/Hong_Kong"),
    N_("Asia/Hovd"),
    N_("Asia/Irkutsk"),
    N_("Asia/Istanbul"),
    N_("Asia/Jakarta"),
    N_("Asia/Jayapura"),
    N_("Asia/Jerusalem"),
    N_("Asia/Kabul"),
    N_("Asia/Kamchatka"),
    N_("Asia/Karachi"),
    N_("Asia/Kashgar"),
    N_("Asia/Katmandu"),
    N_("Asia/Krasnoyarsk"),
    N_("Asia/Kuala_Lumpur"),
    N_("Asia/Kuching"),
    N_("Asia/Kuwait"),
    N_("Asia/Macao"),
    N_("Asia/Magadan"),
    N_("Asia/Manila"),
    N_("Asia/Muscat"),
    N_("Asia/Nicosia"),
    N_("Asia/Novosibirsk"),
    N_("Asia/Omsk"),
    N_("Asia/Phnom_Penh"),
    N_("Asia/Pontianak"),
    N_("Asia/Pyongyang"),
    N_("Asia/Qatar"),
    N_("Asia/Rangoon"),
    N_("Asia/Riyadh"),
    N_("Asia/Saigon"),
    N_("Asia/Samarkand"),
    N_("Asia/Seoul"),
    N_("Asia/Shanghai"),
    N_("Asia/Singapore"),
    N_("Asia/Taipei"),
    N_("Asia/Tashkent"),
    N_("Asia/Tbilisi"),
    N_("Asia/Tehran"),
    N_("Asia/Thimphu"),
    N_("Asia/Tokyo"),
    N_("Asia/Ujung_Pandang"),
    N_("Asia/Ulaanbaatar"),
    N_("Asia/Urumqi"),
    N_("Asia/Vientiane"),
    N_("Asia/Vladivostok"),
    N_("Asia/Yakutsk"),
    N_("Asia/Yekaterinburg"),
    N_("Asia/Yerevan"),
    N_("Atlantic"),
    N_("Atlantic/Azores"),
    N_("Atlantic/Bermuda"),
    N_("Atlantic/Canary"),
    N_("Atlantic/Cape_Verde"),
    N_("Atlantic/Faeroe"),
    N_("Atlantic/Jan_Mayen"),
    N_("Atlantic/Madeira"),
    N_("Atlantic/Reykjavik"),
    N_("Atlantic/South_Georgia"),
    N_("Atlantic/St_Helena"),
    N_("Atlantic/Stanley"),
    N_("Australia"),
    N_("Australia/Adelaide"),
    N_("Australia/Brisbane"),
    N_("Australia/Broken_Hill"),
    N_("Australia/Darwin"),
    N_("Australia/Hobart"),
    N_("Australia/Lindeman"),
    N_("Australia/Lord_Howe"),
    N_("Australia/Melbourne"),
    N_("Australia/Perth"),
    N_("Australia/Sydney"),
    N_("Europe"),
    N_("Europe/Amsterdam"),
    N_("Europe/Andorra"),
    N_("Europe/Athens"),
    N_("Europe/Belfast"),
    N_("Europe/Belgrade"),
    N_("Europe/Berlin"),
    N_("Europe/Bratislava"),
    N_("Europe/Brussels"),
    N_("Europe/Bucharest"),
    N_("Europe/Budapest"),
    N_("Europe/Chisinau"),
    N_("Europe/Copenhagen"),
    N_("Europe/Dublin"),
    N_("Europe/Gibraltar"),
    N_("Europe/Helsinki"),
    N_("Europe/Istanbul"),
    N_("Europe/Kaliningrad"),
    N_("Europe/Kiev"),
    N_("Europe/Lisbon"),
    N_("Europe/Ljubljana"),
    N_("Europe/London"),
    N_("Europe/Luxembourg"),
    N_("Europe/Madrid"),
    N_("Europe/Malta"),
    N_("Europe/Minsk"),
    N_("Europe/Monaco"),
    N_("Europe/Moscow"),
    N_("Europe/Nicosia"),
    N_("Europe/Oslo"),
    N_("Europe/Paris"),
    N_("Europe/Prague"),
    N_("Europe/Riga"),
    N_("Europe/Rome"),
    N_("Europe/Samara"),
    N_("Europe/San_Marino"),
    N_("Europe/Sarajevo"),
    N_("Europe/Simferopol"),
    N_("Europe/Skopje"),
    N_("Europe/Sofia"),
    N_("Europe/Stockholm"),
    N_("Europe/Tallinn"),
    N_("Europe/Tirane"),
    N_("Europe/Uzhgorod"),
    N_("Europe/Vaduz"),
    N_("Europe/Vatican"),
    N_("Europe/Vienna"),
    N_("Europe/Vilnius"),
    N_("Europe/Warsaw"),
    N_("Europe/Zagreb"),
    N_("Europe/Zaporozhye"),
    N_("Europe/Zurich"),
    N_("Indian"),
    N_("Indian/Antananarivo"),
    N_("Indian/Chagos"),
    N_("Indian/Christmas"),
    N_("Indian/Cocos"),
    N_("Indian/Comoro"),
    N_("Indian/Kerguelen"),
    N_("Indian/Mahe"),
    N_("Indian/Maldives"),
    N_("Indian/Mauritius"),
    N_("Indian/Mayotte"),
    N_("Indian/Reunion"),
    N_("Pacific"),
    N_("Pacific/Apia"),
    N_("Pacific/Auckland"),
    N_("Pacific/Chatham"),
    N_("Pacific/Easter"),
    N_("Pacific/Efate"),
    N_("Pacific/Enderbury"),
    N_("Pacific/Fakaofo"),
    N_("Pacific/Fiji"),
    N_("Pacific/Funafuti"),
    N_("Pacific/Galapagos"),
    N_("Pacific/Gambier"),
    N_("Pacific/Guadalcanal"),
    N_("Pacific/Guam"),
    N_("Pacific/Honolulu"),
    N_("Pacific/Johnston"),
    N_("Pacific/Kiritimati"),
    N_("Pacific/Kosrae"),
    N_("Pacific/Kwajalein"),
    N_("Pacific/Majuro"),
    N_("Pacific/Marquesas"),
    N_("Pacific/Midway"),
    N_("Pacific/Nauru"),
    N_("Pacific/Niue"),
    N_("Pacific/Norfolk"),
    N_("Pacific/Noumea"),
    N_("Pacific/Pago_Pago"),
    N_("Pacific/Palau"),
    N_("Pacific/Pitcairn"),
    N_("Pacific/Ponape"),
    N_("Pacific/Port_Moresby"),
    N_("Pacific/Rarotonga"),
    N_("Pacific/Saipan"),
    N_("Pacific/Tahiti"),
    N_("Pacific/Tarawa"),
    N_("Pacific/Tongatapu"),
    N_("Pacific/Truk"),
    N_("Pacific/Wake"),
    N_("Pacific/Wallis"),
    N_("Pacific/Yap"),
};
xfical_timezone_array xfical_get_timezones()
{
    static xfical_timezone_array tz={0, NULL};
    static char tz_utc[]="UTC";
    static char tz_floating[]="floating";
    icalarray *tz_array;
    icaltimezone *l_tz;

    if (tz.count == 0) {
        tz_array = icaltimezone_get_builtin_timezones();
        tz.city = (char **)g_malloc(sizeof(char *)*(2+tz_array->num_elements));
        for (tz.count = 0; tz.count <  tz_array->num_elements; tz.count++) {
            l_tz = (icaltimezone *)icalarray_element_at(tz_array, tz.count);
            /* ical timezones are static so this is safe although not
             * exactly pretty */
            tz.city[tz.count] = icaltimezone_get_location(l_tz);
        }
        tz.city[tz.count++] = tz_utc;
        tz.city[tz.count++] = tz_floating;
    }
    return (tz);
}

void set_default_ical_path (void)
{
    if (ical_path)
        g_free(ical_path);

    ical_path = xfce_resource_save_location(XFCE_RESOURCE_CONFIG,
                    RCDIR G_DIR_SEPARATOR_S APPOINTMENT_FILE, FALSE);
}

void set_ical_path(gchar *path)
{
    if (ical_path)
        g_free(ical_path);

    ical_path = path;
}

void set_aical_path(gchar *path)
{
    if (aical_path)
        g_free(aical_path);

    aical_path = g_strdup(path);
}

void set_lookback(int i) 
{
    lookback = i;
}

gboolean xfical_set_local_timezone(char *location)
{
    local_icaltimezone_utc = FALSE;
    if (!utc_icaltimezone)
            utc_icaltimezone=icaltimezone_get_utc_timezone();

    if (local_icaltimezone_location)
        g_free(local_icaltimezone_location);
    local_icaltimezone_location = NULL;
    local_icaltimezone = NULL;

    if XFICAL_STR_EXISTS(location) {
        local_icaltimezone_location = g_strdup(location);
        if (!local_icaltimezone_location) {
            g_warning("Orage xfical_set_local_timezone: strdup memory exhausted");
            return (FALSE);
        }
        if (strcmp(location,"UTC") == 0) {
            local_icaltimezone_utc = TRUE;
            local_icaltimezone = utc_icaltimezone;
        }
        else if (strcmp(location,"floating") == 0) {
            g_warning("Orage xfical_set_local_timezone: default timezone set to floating. Do not use timezones when setting appointments, it does not make sense without proper local timezone.");
            return(TRUE); /* local_icaltimezone_location = NULL */
        }
        else
            local_icaltimezone = icaltimezone_get_builtin_timezone(location);

        if (!local_icaltimezone) {
            g_warning("Orage xfical_set_local_timezone: builtin timezone %s not found"
                    , location);
            return (FALSE);
        }
    }
    return (TRUE); 
}

void xfical_add_timezone(icalcomponent *p_ical, icalset *p_fical, char *loc)
{
    icaltimezone *icaltz=NULL;
    icalcomponent *itimezone=NULL;
                                                                                
    if (!loc) {
        g_warning("Orage xfical_add_timezone: no location defined");
        return;
    }
    else
        g_message("adding timezone %s", loc);

    if (strcmp(loc,"UTC") == 0 
    ||  strcmp(loc,"floating") == 0) {
        return;
    }
                                                                                
    icaltz=icaltimezone_get_builtin_timezone(loc);
    if (icaltz==NULL) {
        g_warning("Orage xfical_add_timezone: timezone not found %s", loc);
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
        g_warning("Orage xfical_add_timezone: timezone add failed %s", loc);
}

gboolean xfical_internal_file_open(icalcomponent **p_ical
        , icalset **p_fical
        , gchar *file_icalpath)
{
    icalcomponent *iter;
    gint cnt=0;

    if (*p_fical != NULL)
        g_warning("Orage xfical_internal_file_open: file already open");
    if ((*p_fical = icalset_new_file(file_icalpath)) == NULL) {
        g_error("xfical_internal_file_open: Could not open ical file (%s) %s\n"
                , file_icalpath, icalerror_strerror(icalerrno));
        return(FALSE);
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
         *       this is not valid entry before adding an event or timezone
         */
            *p_ical = icalcomponent_vanew(ICAL_VCALENDAR_COMPONENT
                   , icalproperty_new_version("2.0")
                   , icalproperty_new_prodid("-//Xfce//Orage//EN")
                   , 0);
            xfical_add_timezone(*p_ical, *p_fical, local_icaltimezone_location);
            icalset_add_component(*p_fical, *p_ical);
            /*
            icalset_add_component(*p_fical
                   , icalcomponent_new_clone(*p_ical));

            *p_ical = icalset_get_first_component(*p_fical);
            */
            icalset_commit(*p_fical);
        }
        else { /* VCALENDAR found */
            if (cnt > 1) {
                g_warning("Orage xfical_internal_file_open: Too many top level components in calendar file %s", file_icalpath);
            }
        }
    }
    return(TRUE);
}

gboolean xfical_file_open (void)
{ 
    return(xfical_internal_file_open(&ical, &fical, ical_path));
}

gboolean xfical_archive_open (void)
{
    if (!lookback)
        return (FALSE);
    if (!aical_path)
        return (FALSE);

    return(xfical_internal_file_open(&aical, &afical, aical_path));
}

void xfical_file_close(void)
{
    if (fical == NULL)
        g_warning("Orage xfical_file_close: fical is NULL");
    icalset_free(fical);
    fical = NULL;
}

void xfical_archive_close(void)
{
    if (!aical_path)
        return;

    if (afical == NULL)
        g_warning("Orage xfical_file_close: afical is NULL");
    icalset_free(afical);
    afical = NULL;
}

struct icaltimetype ical_get_current_local_time()
{
    struct tm *tm;
    static struct icaltimetype ctime;

    if (local_icaltimezone_utc)
        ctime = icaltime_current_time_with_zone(utc_icaltimezone);
    else if ((local_icaltimezone_location)
        &&   (strcmp(local_icaltimezone_location, "floating") != 0))
        ctime = icaltime_current_time_with_zone(local_icaltimezone);
    else { /* use floating time */
        ctime.is_utc      = 0;
        ctime.is_date     = 0;
        ctime.is_daylight = 0;
        ctime.zone        = NULL;
    }
    /* and at the end we need to change the clock to be correct */
    tm = orage_localtime();
    ctime.year        = tm->tm_year+1900;
    ctime.month       = tm->tm_mon+1;
    ctime.day         = tm->tm_mday;
    ctime.hour        = tm->tm_hour;
    ctime.minute      = tm->tm_min;
    ctime.second      = tm->tm_sec;

    return (ctime);
}

struct icaltimetype convert_to_timezone(struct icaltimetype t, icalproperty *p)
{
    icalparameter *itime_tz = NULL;
    gchar *tz_loc = NULL;
    icaltimezone *l_icaltimezone = NULL;
    struct icaltimetype tz;

    itime_tz = icalproperty_get_first_parameter(p, ICAL_TZID_PARAMETER);
    if (itime_tz) {
         tz_loc = (char *) icalparameter_get_tzid(itime_tz);
         l_icaltimezone = icaltimezone_get_builtin_timezone(tz_loc);
         if (!l_icaltimezone) {
            g_warning("Orage convert_to_timezone: builtin timezone %s not found, conversion failed.", tz_loc);
        }
        tz = icaltime_convert_to_zone(t, l_icaltimezone);
    }
    else
        tz = t;

    return (tz);
}

struct icaltimetype convert_to_local_timezone(struct icaltimetype t
            , icalproperty *p)
{
    struct icaltimetype tl;

    tl = convert_to_timezone(t, p);
    tl = icaltime_convert_to_zone(tl, local_icaltimezone);

    return (tl);
}

xfical_period get_period(icalcomponent *c_event) 
{
    icalproperty *p;
    xfical_period per;

    /* Exactly one start time must be there */
    p = icalcomponent_get_first_property(c_event, ICAL_DTSTART_PROPERTY);
    if (p != NULL) {
        per.stime = icalproperty_get_dtstart(p);
        per.stime = convert_to_local_timezone(per.stime, p);
    }
    else {
        g_warning("Orage get_period: start time not found (%s)"
                , icalcomponent_get_uid(c_event));
        per.stime = icaltime_null_time();
    } 

    /* Either endtime or duration may be there. But neither is required */
    p = icalcomponent_get_first_property(c_event, ICAL_DTEND_PROPERTY);
    if (p != NULL) {
        per.etime = icalproperty_get_dtend(p);
        per.etime = convert_to_local_timezone(per.etime, p);
        per.duration = icaltime_subtract(per.etime, per.stime);
    }
    else {
        p = icalcomponent_get_first_property(c_event, ICAL_DURATION_PROPERTY);
        if (p != NULL) {
            per.duration = icalproperty_get_duration(p);
            per.etime = icaltime_add(per.stime, per.duration);
        }
        else {
            g_message("Orage get_period: end time/duration not found");
            per.etime = per.stime;
            per.duration = icaldurationtype_null_duration();
        } 
    } 

    return (per);
}

/* basically copied from icaltime_compare, which can't be used
 * because it uses utc
 */
int local_compare(struct icaltimetype a, struct icaltimetype b)
{
    int retval = 0;

    if (a.year > b.year)
        retval = 1;
    else if (a.year < b.year)
        retval = -1;

    else if (a.month > b.month)
        retval = 1;
    else if (a.month < b.month)
        retval = -1;

    else if (a.day > b.day)
        retval = 1;
    else if (a.day < b.day)
        retval = -1;

    /* if both are dates, we are done */
    if (a.is_date && b.is_date)
        return retval;

    /* else, if we already found a difference, we are done */
    else if (retval != 0)
        return retval;

    /* else, if only one is a date (and we already know the date part is equal),       then the other is greater */
    else if (b.is_date)
        retval = 1;
    else if (a.is_date)
        retval = -1;

    else if (a.hour > b.hour)
        retval = 1;
    else if (a.hour < b.hour)
        retval = -1;

    else if (a.minute > b.minute)
        retval = 1;
    else if (a.minute < b.minute)
        retval = -1;

    else if (a.second > b.second)
        retval = 1;
    else if (a.second < b.second)
        retval = -1;

    return retval;
}

/* basically copied from icaltime_compare_date_only, which can't be used
 * because it uses utc 
 */
int local_compare_date_only(struct icaltimetype a, struct icaltimetype b)
{
    int retval;
                                                                                
    if (a.year > b.year)
        retval = 1;
    else if (a.year < b.year)
        retval = -1;
                                                                                
    else if (a.month > b.month)
        retval = 1;
    else if (a.month < b.month)
        retval = -1;
                                                                                
    else if (a.day > b.day)
        retval = 1;
    else if (a.day < b.day)
        retval = -1;
                                                                                
    else
        retval = 0;
                                                                                
    return retval;
}

struct icaltimetype convert_to_zone(struct icaltimetype t, gchar *tz)
{
    struct icaltimetype wtime = t;
    icaltimezone *l_icaltimezone = NULL;

    if XFICAL_STR_EXISTS(tz) {
        if (strcmp(tz, "UTC") == 0) {
            wtime = icaltime_convert_to_zone(t, utc_icaltimezone);
        }
        else if (strcmp(tz, "floating") == 0) {
            if (local_icaltimezone)
                wtime = icaltime_convert_to_zone(t, local_icaltimezone);
        }
        else {
            l_icaltimezone = icaltimezone_get_builtin_timezone(tz);
            if (!l_icaltimezone)
                g_warning("Orage convert_to_zone: builtin timezone %s not found, conversion failed.", tz);
            else
                wtime = icaltime_convert_to_zone(t, l_icaltimezone);
        }
    }
    else  /* floating time */
        if (local_icaltimezone)
            wtime = icaltime_convert_to_zone(t, local_icaltimezone);

    return(wtime);
}

int xfical_compare_times(appt_data *appt)
{
    struct icaltimetype stime, etime;
    const char *text;
    struct icaldurationtype duration;

    if (appt->use_duration) {
        if (! XFICAL_STR_EXISTS(appt->starttime)) {
            g_warning("Orage xfical_compare_times: null start time");
            return(0); /* should be error ! */
        }
        stime = icaltime_from_string(appt->starttime);
        duration = icaldurationtype_from_int(appt->duration);
        etime = icaltime_add(stime, duration);
        text  = icaltime_as_ical_string(etime);
        g_strlcpy(appt->endtime, text, 17);
        g_free(appt->end_tz_loc);
        appt->end_tz_loc = g_strdup(appt->start_tz_loc);
        return(0); /* ok */

    }
    else {
        if (XFICAL_STR_EXISTS(appt->starttime) 
        &&  XFICAL_STR_EXISTS(appt->endtime)) {
            stime = icaltime_from_string(appt->starttime);
            etime = icaltime_from_string(appt->endtime);

            stime = convert_to_zone(stime, appt->start_tz_loc);
            stime = icaltime_convert_to_zone(stime, local_icaltimezone);
            etime = convert_to_zone(etime, appt->end_tz_loc);
            etime = icaltime_convert_to_zone(etime, local_icaltimezone);

            duration = icaltime_subtract(etime, stime);
            appt->duration = icaldurationtype_as_int(duration);
            return (icaltime_compare(stime, etime));
        }
        else {
            g_warning("Orage xfical_compare_times: null time %s %s"
                    , appt->starttime, appt->endtime);
            return(0); /* should be error ! */
        }
    }
}

 /* allocates memory and initializes it for new ical_type structure
  * returns: NULL if failed and pointer to appt_data if successfull.
  *         You must free it after not being used anymore. (g_free())
  */
appt_data *xfical_appt_alloc()
{
    appt_data *temp;

    temp = g_new0(appt_data, 1);
    temp->availability = 1;
    return(temp);
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
    new_alarm->alarm_time = g_string_new(icaltime_as_ical_string(alarm_time));
    new_alarm->event_time = g_string_new(icaltime_as_ical_string(event_time));
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

void xfical_alarm_build_list_internal(gboolean first_list_today)
{
    xfical_period per;
    struct icaltimetype alarm_time, cur_time, next_date;
    icalcomponent *c, *ca;
    icalproperty *p;
    icalproperty_status stat=ICAL_ACTION_DISPLAY;
    struct icaltriggertype trg;
    char *suid, *ssummary, *sdescription, *ssound;
    gboolean trg_found;
    icalattach *attach = NULL;
    struct icalrecurrencetype rrule;
    icalrecur_iterator* ri;
    gint repeat_cnt, repeat_delay;
    struct icaldurationtype duration;
    gint cnt_alarm=0, cnt_repeat=0, cnt_event=0, cnt_act_alarm=0;

    cur_time = ical_get_current_local_time();
    g_list_foreach(alarm_list, alarm_free, NULL);
    g_list_free(alarm_list);
    alarm_list = NULL;

    for (c = icalcomponent_get_first_component(ical, ICAL_VEVENT_COMPONENT);
         c != 0;
         c = icalcomponent_get_next_component(ical, ICAL_VEVENT_COMPONENT)){
        cnt_event++;
        suid = (char*)icalcomponent_get_uid(c);
        ssummary = (char*)icalcomponent_get_summary(c);
        sdescription = (char*)icalcomponent_get_description(c);
        per = get_period(c);
        if (first_list_today && icaltime_is_date(per.stime)) {
        /* this is special pre 4.3 xfcalendar compatibility alarm:
           Send alarm window always for date type events. */
            if (local_compare_date_only(per.stime, cur_time) == 0) {
                alarm_add(ICAL_ACTION_DISPLAY
                        , suid, ssummary, sdescription, NULL, 0, 0
                        , per.stime, per.stime);
                cnt_act_alarm++;
            }
            else if ((p = icalcomponent_get_first_property(c
                , ICAL_RRULE_PROPERTY)) != 0) { /* check recurring EVENTs */
                next_date = icaltime_null_time();
                rrule = icalproperty_get_rrule(p);
                ri = icalrecur_iterator_new(rrule, per.stime);
                for (next_date = icalrecur_iterator_next(ri);
                    !icaltime_is_null_time(next_date)
                        && local_compare_date_only(cur_time, next_date) > 0;
                    next_date = icalrecur_iterator_next(ri)) {
                    cnt_repeat++;
                }
                icalrecur_iterator_free(ri);
                if (local_compare_date_only(next_date, cur_time) == 0) {
                    alarm_add(ICAL_ACTION_DISPLAY
                            , suid, ssummary, sdescription, NULL, 0, 0
                            , next_date, per.stime);
                    cnt_act_alarm++;
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
                switch (icalproperty_isa(p)) {
                    case ICAL_ACTION_PROPERTY:
                        stat = icalproperty_get_action(p);
                        break;
                    case ICAL_DESCRIPTION_PROPERTY:
                        sdescription = (char *)icalproperty_get_description(p);
                        break;
                    case ICAL_ATTACH_PROPERTY:
                        attach = icalproperty_get_attach(p);
                        ssound = (char *)icalattach_get_url(attach);
                        break;
                    case ICAL_TRIGGER_PROPERTY:
                        trg = icalproperty_get_trigger(p);
                        trg_found = TRUE;
                        break;
                    case ICAL_REPEAT_PROPERTY:
                        repeat_cnt = icalproperty_get_repeat(p);
                        break;
                    case ICAL_DURATION_PROPERTY:
                        duration = icalproperty_get_duration(p);
                        repeat_delay = icaldurationtype_as_int(duration);
                        break;
                    default:
                        g_warning("Orage ical-code: Unknown property (%s) in Alarm",
                            (char *)icalproperty_get_property_name(p));
                        break;
                } 
            } 
        }  /* ALARMS */
        if (trg_found) {
            cnt_alarm++;
            if (icaltime_is_date(per.stime)) { 
    /* HACK: convert to local time so that we can use time arithmetic
     * when counting alarm time. */
                per.stime.is_date       = 0;
                per.stime.is_utc        = cur_time.is_utc;
                per.stime.is_daylight   = cur_time.is_daylight;
                per.stime.zone          = cur_time.zone;
                per.stime.hour          = 0;
                per.stime.minute        = 0;
                per.stime.second        = 0;
            }
        /* all data available. let's pack it if alarm is still active */
            alarm_time = icaltime_add(per.stime, trg.duration);
            if (icaltime_compare(cur_time, alarm_time) <= 0) { /* active */
                alarm_add(stat, suid, ssummary, sdescription
                        , ssound, repeat_cnt, repeat_delay
                        , alarm_time, per.stime);
                cnt_act_alarm++;
            }
            else if ((p = icalcomponent_get_first_property(c
                , ICAL_RRULE_PROPERTY)) != 0) { /* check recurring EVENTs */                    rrule = icalproperty_get_rrule(p);
                next_date = icaltime_null_time();
                alarm_time = icaltime_add(per.stime, trg.duration);
                ri = icalrecur_iterator_new(rrule, alarm_time);
                for (next_date = icalrecur_iterator_next(ri);
                    (!icaltime_is_null_time(next_date))
                    && (local_compare_date_only(cur_time, next_date) > 0) ;
                    next_date = icalrecur_iterator_next(ri)) {
                    cnt_repeat++;
                }
                icalrecur_iterator_free(ri);
                if (icaltime_compare(cur_time, next_date) <= 0) {
                    alarm_add(stat, suid, ssummary, sdescription
                        , ssound, repeat_cnt, repeat_delay
                        , next_date, per.stime);
                    cnt_act_alarm++;
                }
            }
        } /* trg_found */
    }  /* EVENTS */
    alarm_list = g_list_sort(alarm_list, alarm_order);
    g_message("Orage build alarm list: Processed %d events. Found %d alarms of which %d are active. (Searched %d recurring alarms.)"
            , cnt_event, cnt_alarm, cnt_act_alarm, cnt_repeat);
}

void xfical_alarm_build_list(gboolean first_list_today)
{
    xfical_file_open();
    xfical_alarm_build_list_internal(first_list_today);
    xfical_file_close();
}

gboolean xfical_alarm_passed(char *alarm_stime)
{
    struct icaltimetype alarm_time, cur_time;

    cur_time = ical_get_current_local_time();
    alarm_time = icaltime_from_string(alarm_stime);
    if (local_compare(cur_time, alarm_time) > 0)
        return(TRUE);
    return(FALSE);
 }

void appt_add_alarm_internal(appt_data *appt, icalcomponent *ievent)
{
    icalcomponent *ialarm;
    gint duration=0;
    struct icaltriggertype trg;
    icalattach *attach;

    duration = appt->alarmtime;
    trg.time = icaltime_null_time();
    trg.duration = icaldurationtype_from_int(-duration);
    /********** DISPLAY **********/
    ialarm = icalcomponent_vanew(ICAL_VALARM_COMPONENT
        , icalproperty_new_action(ICAL_ACTION_DISPLAY)
        , icalproperty_new_trigger(trg)
        , 0);
    if XFICAL_STR_EXISTS(appt->note)
        icalcomponent_add_property(ialarm
            , icalproperty_new_description(appt->note));
    else if XFICAL_STR_EXISTS(appt->title)
        icalcomponent_add_property(ialarm
            , icalproperty_new_description(appt->title));
    else
        icalcomponent_add_property(ialarm
            , icalproperty_new_description(_("Orage default alarm")));
    icalcomponent_add_component(ievent, ialarm);
    /********** AUDIO **********/
    if XFICAL_STR_EXISTS(appt->sound) {
        ialarm = icalcomponent_vanew(ICAL_VALARM_COMPONENT
            , icalproperty_new_action(ICAL_ACTION_AUDIO)
            , icalproperty_new_trigger(trg)
            , 0);
        attach = icalattach_new_from_url(appt->sound);
        icalcomponent_add_property(ialarm
            , icalproperty_new_attach(attach));
        if (appt->alarmrepeat) {
           /* loop 500 times with 2 secs interval */
            icalcomponent_add_property(ialarm
                , icalproperty_new_repeat(500));
            icalcomponent_add_property(ialarm
                , icalproperty_new_duration(icaldurationtype_from_int(2)));
        }
        icalcomponent_add_component(ievent, ialarm);
    }
}

char *appt_add_internal(appt_data *appt, gboolean add, char *uid
        , struct icaltimetype cre_time)
{
    icalcomponent *ievent;
    struct icaltimetype dtstamp, create_time, wtime;
    static gchar xf_uid[1001];
    gchar xf_host[501];
    gchar recur_str[101], *recur_p;
    struct icalrecurrencetype rrule;
    struct icaldurationtype duration;

    dtstamp = icaltime_current_time_with_zone(utc_icaltimezone);
    if (add) {
        gethostname(xf_host, 500);
        xf_host[500] = '\0';
        g_snprintf(xf_uid, 1000, "Orage-%s-%lu@%s"
                , icaltime_as_ical_string(dtstamp), (long) getuid(), xf_host);
        create_time = dtstamp;
    }
    else { /* mod */
        strcpy(xf_uid, uid);
        if (icaltime_is_null_time(cre_time))
            create_time = dtstamp;
        else
            create_time = cre_time;
    }

    ievent = icalcomponent_vanew(ICAL_VEVENT_COMPONENT
           , icalproperty_new_uid(xf_uid)
           , icalproperty_new_categories("ORAGENOTE")
           , icalproperty_new_class(ICAL_CLASS_PUBLIC)
           , icalproperty_new_dtstamp(dtstamp)
           , icalproperty_new_created(create_time)
           , 0);

    if XFICAL_STR_EXISTS(appt->title)
        icalcomponent_add_property(ievent
                , icalproperty_new_summary(appt->title));
    if XFICAL_STR_EXISTS(appt->note)
        icalcomponent_add_property(ievent
                , icalproperty_new_description(appt->note));
    if XFICAL_STR_EXISTS(appt->location)
        icalcomponent_add_property(ievent
                , icalproperty_new_location(appt->location));
    if (appt->availability == 0)
        icalcomponent_add_property(ievent
           , icalproperty_new_transp(ICAL_TRANSP_TRANSPARENT));
    else if (appt->availability == 1)
        icalcomponent_add_property(ievent
           , icalproperty_new_transp(ICAL_TRANSP_OPAQUE));
    if (appt->allDay) { /* cut the string after Date: yyyymmdd */
        appt->starttime[8] = '\0';
        appt->endtime[8] = '\0';
    }

    if XFICAL_STR_EXISTS(appt->starttime) {
        wtime=icaltime_from_string(appt->starttime);
        if XFICAL_STR_EXISTS(appt->start_tz_loc) {
            if (strcmp(appt->start_tz_loc, "UTC") == 0) {
                wtime=icaltime_convert_to_zone(wtime, utc_icaltimezone);
                icalcomponent_add_property(ievent
                    , icalproperty_new_dtstart(wtime));
            }
            else if (strcmp(appt->start_tz_loc, "floating") == 0) {
                icalcomponent_add_property(ievent
                    , icalproperty_new_dtstart(wtime));
            }
            else {
            /* FIXME: add this vtimezone to vcalendar if it is not there */
                icalcomponent_add_property(ievent
                    , icalproperty_vanew_dtstart(wtime
                    , icalparameter_new_tzid(appt->start_tz_loc)
                    , 0));
            }
        }
        else { /* floating time */
            icalcomponent_add_property(ievent, icalproperty_new_dtstart(wtime));
        }
    }
    if (appt->use_duration) {
        duration = icaldurationtype_from_int(appt->duration);
        icalcomponent_add_property(ievent
            , icalproperty_new_duration(duration));
    }
    else if XFICAL_STR_EXISTS(appt->endtime) {
        wtime=icaltime_from_string(appt->endtime);
        if XFICAL_STR_EXISTS(appt->end_tz_loc) {
            if (strcmp(appt->end_tz_loc, "UTC") == 0) {
                wtime=icaltime_convert_to_zone(wtime, utc_icaltimezone);
                icalcomponent_add_property(ievent
                    , icalproperty_new_dtend(wtime));
            }
            else if (strcmp(appt->end_tz_loc, "floating") == 0) {
                icalcomponent_add_property(ievent
                    , icalproperty_new_dtend(wtime));
            }
            else {
            /* FIXME: add this vtimezone to vcalendar if it is not there */
                icalcomponent_add_property(ievent
                    , icalproperty_vanew_dtend(wtime
                    , icalparameter_new_tzid(appt->end_tz_loc)
                    , 0));
            }
        }
        else {
            icalcomponent_add_property(ievent, icalproperty_new_dtend(wtime));
        }
    }
    if (appt->freq != XFICAL_FREQ_NONE) {
        recur_p = g_stpcpy(recur_str, "FREQ=");
        switch(appt->freq) {
            case XFICAL_FREQ_DAILY:
                recur_p = g_stpcpy(recur_p, "DAILY");
                break;
            case XFICAL_FREQ_WEEKLY:
                recur_p = g_stpcpy(recur_p, "WEEKLY");
                break;
            case XFICAL_FREQ_MONTHLY:
                recur_p = g_stpcpy(recur_p, "MONTHLY");
                break;
            case XFICAL_FREQ_YEARLY:
                recur_p = g_stpcpy(recur_p, "YEARLY");
                break;
            default:
                g_warning("Orage appt_add_internal: Unsupported freq");
                icalrecurrencetype_clear(&rrule);
        }
        if (appt->recur_limit == 1) {
            g_sprintf(recur_p, ";COUNT=%d", appt->recur_count);
        }
        else if (appt->recur_limit == 2) { /* needs to be in UTC */
            g_sprintf(recur_p, ";UNTIL=%sZ", appt->recur_until);
        }
        rrule = icalrecurrencetype_from_string(recur_str);
        icalcomponent_add_property(ievent, icalproperty_new_rrule(rrule));
    }
    if (appt->alarmtime != 0) {
        appt_add_alarm_internal(appt, ievent);
    }

    icalcomponent_add_component(ical, ievent);
    fical_modified = TRUE;
    icalset_mark(fical);
    xfical_alarm_build_list_internal(FALSE);
    return(xf_uid);
}

 /* add EVENT type ical appointment to ical file
  * app: pointer to filled appt_data structure, which is stored
  *      Caller is responsible for filling and allocating and freeing it.
  *  returns: NULL if failed and new ical id if successfully added. 
  *           This ical id is owned by the routine. Do not deallocate it.
  *           It will be overwrittewritten by next invocation of this function.
  */
char *xfical_appt_add(appt_data *appt)
{
    return(appt_add_internal(appt, TRUE, NULL, icaltime_null_time()));
}

void ical_appt_get_alarm_internal(icalcomponent *c,  appt_data *appt)
{
    icalcomponent *ca = NULL;
    icalproperty *p = NULL;
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
                        appt->alarmtime = icaldurationtype_as_int(trg.duration)
                                            * -1;
                    }
                    else
                        g_warning("Orage ical_appt_get_alarm_internal: Can not process time triggers");
                    break;
                case ICAL_ATTACH_PROPERTY:
                    attach = icalproperty_get_attach(p);
                    appt->sound = (char *)icalattach_get_url(attach);
                    break;
                case ICAL_REPEAT_PROPERTY:
                    appt->alarmrepeat = TRUE;
                    break;
                case ICAL_DESCRIPTION_PROPERTY:
                    if (appt->note == NULL)
                        appt->note = (char *) icalproperty_get_description(p);
                    break;
                case ICAL_DURATION_PROPERTY:
                case ICAL_ACTION_PROPERTY:
                /* no actions defined */
                    break;
                default:
                    g_warning("Orage ical_appt_get_alarm_internal: unknown property %s", (char *)icalproperty_get_property_name(p));
                    break;
            }
        }
    }
}

 /* Read EVENT from ical datafile.
  * ical_uid:  key of ical EVENT appt-> is to be read
  * returns: if failed: NULL
  *          if successfull: appt_data pointer to appt_data struct 
  *              filled with data.
  *              This appt_data struct is owned by the routine.
  *              Do not deallocate it.
  *             It will be overdriven by next invocation of this function.
  *          NOTE: This routine does not fill starttimecur nor endtimecur,
  *                Those are always initialized to null string
  */
appt_data *xfical_appt_get_internal(char *ical_uid)
{
    static appt_data appt;
    icalcomponent *c = NULL;
    icalproperty *p = NULL;
    gboolean key_found = FALSE;
    const char *text;
    struct icaltimetype itime, stime, etime, sltime, eltime;
    icalparameter *itime_tz;
    icalproperty_transp xf_transp;
    struct icalrecurrencetype rrule;
    struct icaldurationtype duration;

    for (c = icalcomponent_get_first_component(ical, ICAL_VEVENT_COMPONENT); 
         c != 0 && !key_found;
         c = icalcomponent_get_next_component(ical, ICAL_VEVENT_COMPONENT)) {
        text = icalcomponent_get_uid(c);
        if (strcmp(text, ical_uid) == 0) {
        /*********** Defaults ***********/
            stime = icaltime_null_time();
            sltime = icaltime_null_time();
            eltime = icaltime_null_time();
            duration = icaldurationtype_null_duration();
            key_found = TRUE;
            appt.title = NULL;
            appt.location = NULL;
            appt.alarmtime = 0;
            appt.availability = -1;
            appt.allDay = FALSE;
            appt.alarmrepeat = FALSE;
            appt.note = NULL;
            appt.sound = NULL;
            appt.uid = NULL;
            appt.starttime[0] = '\0';
            appt.start_tz_loc = NULL;
            appt.endtime[0] = '\0';
            appt.end_tz_loc = NULL;
            appt.use_duration = FALSE;
            appt.duration = 0;
            appt.starttimecur[0] = '\0';
            appt.endtimecur[0] = '\0';
            appt.freq = XFICAL_FREQ_NONE;
            appt.recur_limit = 0;
            appt.recur_count = 0;
            appt.recur_until[0] = '\0';
        /*********** Properties ***********/
            for (p = icalcomponent_get_first_property(c, ICAL_ANY_PROPERTY);
                 p != 0;
                 p = icalcomponent_get_next_property(c, ICAL_ANY_PROPERTY)) {
                /* these are in icalderivedproperty.h */
                switch (icalproperty_isa(p)) {
                    case ICAL_SUMMARY_PROPERTY:
                        appt.title = (char *) icalproperty_get_summary(p);
                        break;
                    case ICAL_LOCATION_PROPERTY:
                        appt.location = (char *) icalproperty_get_location(p);
                        break;
                    case ICAL_DESCRIPTION_PROPERTY:
                        appt.note = (char *) icalproperty_get_description(p);
                        break;
                    case ICAL_UID_PROPERTY:
                        appt.uid = (char *) icalproperty_get_uid(p);
                        break;
                    case ICAL_TRANSP_PROPERTY:
                        xf_transp = icalproperty_get_transp(p);
                        if (xf_transp == ICAL_TRANSP_TRANSPARENT)
                            appt.availability = 0;
                        else if (xf_transp == ICAL_TRANSP_OPAQUE)
                            appt.availability = 1;
                        else 
                            appt.availability = -1;
                        break;
                    case ICAL_DTSTART_PROPERTY:
                        itime = icalproperty_get_dtstart(p);
                        stime = convert_to_timezone(itime, p);
                        sltime = convert_to_local_timezone(itime, p);
                        text  = icaltime_as_ical_string(itime);
                        g_strlcpy(appt.starttime, text, 17);
                        if (icaltime_is_date(itime))
                            appt.allDay = TRUE;
                        else if (icaltime_is_utc(itime)) { 
                            appt.start_tz_loc = "UTC";
                        }
                        else { /* let's check timezone */
                            itime_tz = icalproperty_get_first_parameter(p
                                    , ICAL_TZID_PARAMETER);
                            if (itime_tz)
                                appt.start_tz_loc = 
                                    (char *) icalparameter_get_tzid(itime_tz);
                        }
                        if (appt.endtime[0] == '\0') {
                            g_strlcpy(appt.endtime,  appt.starttime, 17);
                            appt.end_tz_loc = appt.start_tz_loc;
                            etime = stime;
                        }
                        break;
                    case ICAL_DTEND_PROPERTY:
                        itime = icalproperty_get_dtend(p);
                        eltime = convert_to_local_timezone(itime, p);
                        text  = icaltime_as_ical_string(itime);
                        g_strlcpy(appt.endtime, text, 17);
                        if (icaltime_is_date(itime))
                            appt.allDay = TRUE;
                        else if (icaltime_is_utc(itime)) { 
                            appt.end_tz_loc = "UTC";
                        }
                        else { /* let's check timezone */
                            itime_tz = icalproperty_get_first_parameter(p
                                    , ICAL_TZID_PARAMETER);
                            if (itime_tz)
                                appt.end_tz_loc = 
                                    (char *) icalparameter_get_tzid(itime_tz);
                            else
                                appt.end_tz_loc = NULL;
                        }
                        break;
                    case ICAL_DURATION_PROPERTY:
                        appt.use_duration = TRUE;
                        duration = icalproperty_get_duration(p);
                        appt.duration = icaldurationtype_as_int(duration);
                        break;
                    case ICAL_RRULE_PROPERTY:
                        rrule = icalproperty_get_rrule(p);
                        switch (rrule.freq) {
                            case ICAL_DAILY_RECURRENCE:
                                appt.freq = XFICAL_FREQ_DAILY;
                                break;
                            case ICAL_WEEKLY_RECURRENCE:
                                appt.freq = XFICAL_FREQ_WEEKLY;
                                break;
                            case ICAL_MONTHLY_RECURRENCE:
                                appt.freq = XFICAL_FREQ_MONTHLY;
                                break;
                            case ICAL_YEARLY_RECURRENCE:
                                appt.freq = XFICAL_FREQ_YEARLY;
                                break;
                            default:
                                appt.freq = XFICAL_FREQ_NONE;
                                break;
                        }
                        if (appt.recur_count = rrule.count)
                            appt.recur_limit = 1;
                        else if(! icaltime_is_null_time(rrule.until)) {
                            appt.recur_limit = 2;
                            text  = icaltime_as_ical_string(rrule.until);
                            g_strlcpy(appt.recur_until, text, 17);
                        }
                        break;
                    case ICAL_CATEGORIES_PROPERTY:
                    case ICAL_CLASS_PROPERTY:
                    case ICAL_DTSTAMP_PROPERTY:
                    case ICAL_CREATED_PROPERTY:
                        break;
                    default:
                        g_warning("Orage xfical_appt_get: unknown property %s", (char *)icalproperty_get_property_name(p));
                        break;
                }
            }
        ical_appt_get_alarm_internal(c, &appt);
        }
    } 

    /* need to set missing endtime or duration */
    if (appt.use_duration) { 
        etime = icaltime_add(stime, duration);
        text  = icaltime_as_ical_string(etime);
        g_strlcpy(appt.endtime, text, 17);
        appt.end_tz_loc = appt.start_tz_loc;
    }
    else {
        duration = icaltime_subtract(eltime, sltime);
        appt.duration = icaldurationtype_as_int(duration);
    }
    if (key_found)
        return(&appt);
    else
        return(NULL);
}

 /* Read EVENT from ical datafile.
  * ical_uid:  key of ical EVENT appt-> is to be read
  * returns: if failed: NULL
  *          if successfull: appt_data pointer to appt_data struct 
  *             filled with data.
  *             You must free it after not being used anymore. (g_free())
  *          NOTE: This routine does not fill starttimecur nor endtimecur,
  *                Those are always initialized to null string
  */
appt_data *xfical_appt_get(char *ical_uid)
{
    appt_data *appt;

    appt = g_memdup(xfical_appt_get_internal(ical_uid), sizeof(appt_data));
    appt->uid = g_strdup(appt->uid);
    appt->title = g_strdup(appt->title);
    appt->location = g_strdup(appt->location);
    appt->start_tz_loc = g_strdup(appt->start_tz_loc);
    appt->end_tz_loc = g_strdup(appt->end_tz_loc);
    appt->note = g_strdup(appt->note);
    appt->sound = g_strdup(appt->sound);

    return(appt);
}

void xfical_appt_free(appt_data *appt)
{
    g_free(appt->uid);
    g_free(appt->title);
    g_free(appt->location);
    g_free(appt->start_tz_loc);
    g_free(appt->end_tz_loc);
    g_free(appt->note);
    g_free(appt->sound);
    g_free(appt);
}

 /* modify EVENT type ical appointment in ical file
  * ical_uid: char pointer to ical ical_uid to modify
  * app: pointer to filled appt_data structure, which is stored
  *      Caller is responsible for filling and allocating and freeing it.
  * returns: TRUE is successfull, FALSE if failed
  */
gboolean xfical_appt_mod(char *ical_uid, appt_data *app)
{
    icalcomponent *c;
    char *uid;
    icalproperty *p = NULL;
    gboolean key_found = FALSE;
    struct icaltimetype create_time = icaltime_null_time();

    if (ical_uid == NULL) {
        g_warning("Orage xfical_appt_mod: Got NULL uid. doing nothing");
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
        g_warning("Orage xfical_appt_mod: uid not found. doing nothing");
        return(FALSE);
    }

    appt_add_internal(app, FALSE, ical_uid, create_time);
    return(TRUE);
}

 /* removes EVENT with ical_uid from ical file
  * ical_uid: pointer to ical_uid to be deleted
  * returns: TRUE is successfull, FALSE if failed
  */
gboolean xfical_appt_del(char *ical_uid)
{
    icalcomponent *c;
    char *uid;

    if (ical_uid == NULL) {
        g_warning("Orage xfical_appt_del: Got NULL uid. doing nothing");
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
            xfical_alarm_build_list_internal(FALSE);
            return(TRUE);
        }
    } 
    g_warning("Orage xfical_appt_del: uid not found. doing nothing");
    return(FALSE);
}

 /* Read next EVENT on the specified date from ical datafile.
  * a_day:  start date of ical EVENT appointment which is to be read
  * first:  get first appointment is TRUE, if not get next.
  * days:   how many more days to check forward. 0 = only one day
  * returns: NULL if failed and appt_data pointer to appt_data struct 
  *          filled with data if successfull. 
  *          This appt_data struct is owned by the routine. 
  *          Do not deallocate it.
  *          It will be overdriven by next invocation of this function.
  * Note:   starttimecur and endtimecur are converted to local timezone
  */
appt_data *xfical_appt_get_next_on_day(char *a_day, gboolean first, gint days)
{
    struct icaltimetype 
              asdate, aedate    /* period to check */
            , nsdate, nedate;   /* repeating event occurrency start and end */
    struct icaldurationtype aduration;
    xfical_period per; /* event start and end times with duration */
    icalcomponent *c=NULL;
    icalproperty *p = NULL;
    static icalcompiter ci;
    gboolean date_found=FALSE;
    gboolean date_rec_found=FALSE;
    char *uid;
    static appt_data *appt;
    struct icalrecurrencetype rrule;
    icalrecur_iterator* ri;

    /* setup period to test */
    asdate = icaltime_from_string(a_day);
    if (days) { /* more than one day to check */
        aduration = icaldurationtype_null_duration();
        aduration.days = days;
        aedate = icaltime_add(asdate, aduration);
    }
    else /* only one day */
        aedate = asdate;

    if (first)
        ci = icalcomponent_begin_component(ical, ICAL_VEVENT_COMPONENT);
    for ( ; 
         icalcompiter_deref(&ci) != 0 && !date_found;
         icalcompiter_next(&ci)) {
        c = icalcompiter_deref(&ci);
        per = get_period(c);
        if (local_compare_date_only(per.stime, aedate) <= 0
            && local_compare_date_only(asdate, per.etime) <= 0) {
            date_found = TRUE;
        }
        else if ((p = icalcomponent_get_first_property(c
                , ICAL_RRULE_PROPERTY)) != 0) { /* check recurring EVENTs */
            nsdate = icaltime_null_time();
            rrule = icalproperty_get_rrule(p);
            ri = icalrecur_iterator_new(rrule, per.stime);
            for (nsdate = icalrecur_iterator_next(ri),
                    nedate = icaltime_add(nsdate, per.duration);
                 !icaltime_is_null_time(nsdate)
                    && local_compare_date_only(nedate, asdate) < 0;
                 nsdate = icalrecur_iterator_next(ri),
                    nedate = icaltime_add(nsdate, per.duration)) {
            }
            icalrecur_iterator_free(ri);
            if (local_compare_date_only(nsdate, aedate) <= 0
                && local_compare_date_only(asdate, nedate) <= 0) {
                date_found = TRUE;
                date_rec_found = TRUE;
            }
        }
    }
    if (date_found) {
        uid = (char *)icalcomponent_get_uid(c);
        appt = xfical_appt_get_internal(uid);
        if (date_rec_found) {
            g_strlcpy(appt->starttimecur, icaltime_as_ical_string(nsdate), 17);
            g_strlcpy(appt->endtimecur, icaltime_as_ical_string(nedate), 17);
        }
        else {
            g_strlcpy(appt->starttimecur, icaltime_as_ical_string(per.stime), 17);
            g_strlcpy(appt->endtimecur, icaltime_as_ical_string(per.etime), 17);
        }
        return(appt);
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
    xfical_period per;
    struct icaltimetype nsdate, nedate;
    static icalcomponent *c;
    struct icalrecurrencetype rrule;
    icalrecur_iterator* ri;
    icalproperty *p = NULL;
    gint start_day, day_cnt, end_day;

    gtk_calendar_freeze(gtkcal);
    gtk_calendar_clear_marks(gtkcal);
    for (c = icalcomponent_get_first_component(ical, ICAL_VEVENT_COMPONENT);
         c != 0;
         c = icalcomponent_get_next_component(ical, ICAL_VEVENT_COMPONENT)) {
        per = get_period(c);
        if ((per.stime.year*12+per.stime.month) <= (year*12+month)
                && (year*12+month) <= (per.etime.year*12+per.etime.month)) {
                /* event is in our year+month = visible in calendar */
            if (per.stime.year == year && per.stime.month == month)
                start_day = per.stime.day;
            else
                start_day = 1;
            if (per.etime.year == year && per.etime.month == month)
                end_day = per.etime.day;
            else
                end_day = 31;
            for (day_cnt = start_day; day_cnt <= end_day; day_cnt++)
                gtk_calendar_mark_day(gtkcal, day_cnt);
        }
        if ((p = icalcomponent_get_first_property(c
                , ICAL_RRULE_PROPERTY)) != 0) { /* check recurring EVENTs */
            nsdate = icaltime_null_time();
            rrule = icalproperty_get_rrule(p);
            ri = icalrecur_iterator_new(rrule, per.stime);
            for (nsdate = icalrecur_iterator_next(ri),
                    nedate = icaltime_add(nsdate, per.duration);
                 !icaltime_is_null_time(nsdate)
                    && (nsdate.year*12+nsdate.month) <= (year*12+month);
                 nsdate = icalrecur_iterator_next(ri),
                    nedate = icaltime_add(nsdate, per.duration)) {
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
            icalrecur_iterator_free(ri);
        } 
    } 
    gtk_calendar_thaw(gtkcal);
}

 /* remove all EVENTs starting this day
  * a_day: date to clear (yyyymmdd format)
  */
void xfical_rmday(char *a_day)
{
    xfical_period per;
    struct icaltimetype  adate;
    icalcomponent *c, *c2;

/* Note: remove moves the "c" pointer to next item, so we need to store it 
         first to process all of them or we end up skipping entries */
    adate = icaltime_from_string(a_day);
    for (c = icalcomponent_get_first_component(ical, ICAL_VEVENT_COMPONENT); 
         c != 0;
         c = c2){
        c2 = icalcomponent_get_next_component(ical, ICAL_VEVENT_COMPONENT);
        per = get_period(c);
        if (local_compare_date_only(per.stime, adate) == 0) {
            icalcomponent_remove_component(ical, c);
        }
    } 
    fical_modified = TRUE;
    icalset_mark(fical);
    xfical_alarm_build_list_internal(FALSE);
}

void xfical_icalcomponent_archive_recurrent(icalcomponent *e
        , struct tm *threshold, char *uid)
{
    struct icaltimetype sdate, edate, nsdate, nedate;
    struct icalrecurrencetype rrule;
    struct icaldurationtype duration;
    icalrecur_iterator* ri;
    icalcomponent *d, *a;
    icalparameter *itime_tz;
    gchar *stz_loc = NULL, *etz_loc = NULL;
    icaltimezone *l_icaltimezone = NULL;
    gboolean key_found = FALSE;
    const char *text;
    icalproperty *p, *pdtstart, *pdtend;
    gboolean upd_edate = FALSE;

    /* *** PHASE 1 *** : Add to the archive file */
    /* We must first check that this event has not yet been archived.
     * It is recurrent, so we may have added it earlier already.
     * If it has been added, we do not have to do anything since the
     * one in the archive file must be older than our current event.
     */
    for (a = icalcomponent_get_first_component(aical, ICAL_VEVENT_COMPONENT);
         a != 0 && !key_found;
         a = icalcomponent_get_next_component(aical, ICAL_VEVENT_COMPONENT)) {
        text = icalcomponent_get_uid(a);
        if (strcmp(text, uid) == 0) 
            key_found = TRUE;
    }
    if (!key_found) { /* need to add it */
        d = icalcomponent_new_clone(e);
        icalcomponent_add_component(aical, d);
    }

    /* *** PHASE 2 *** : Update startdate and enddate in the main file */
    /* We must not remove recurrent events, but just modify start- and
     * enddates and actually only the date parts since time will stay.
     * Note that we may need to remove limited recurrency events.
     */
    sdate = icalcomponent_get_dtstart(e);
    pdtstart = icalcomponent_get_first_property(e, ICAL_DTSTART_PROPERTY);
    itime_tz = icalproperty_get_first_parameter(pdtstart, ICAL_TZID_PARAMETER);
    if (itime_tz) {
         stz_loc = (char *) icalparameter_get_tzid(itime_tz);
         l_icaltimezone=icaltimezone_get_builtin_timezone(stz_loc);
         if (!l_icaltimezone) {
            g_warning("Orage xfical_icalcomponent_archive_recurrent: builtin timezone %s not found, conversion failed.", stz_loc);
        }
        sdate = icaltime_convert_to_zone(sdate, l_icaltimezone);
    }

    edate = icalcomponent_get_dtend(e);
    if (icaltime_is_null_time(edate)) {
        edate = sdate;
    }
    pdtend = icalcomponent_get_first_property(e, ICAL_DTEND_PROPERTY);
    if (pdtend) { /* we have DTEND, so we need to adjust it. */
        itime_tz = icalproperty_get_first_parameter(pdtend, ICAL_TZID_PARAMETER);
        if (itime_tz) {
             etz_loc = (char *) icalparameter_get_tzid(itime_tz);
             l_icaltimezone=icaltimezone_get_builtin_timezone(etz_loc);
             if (!l_icaltimezone) {
                g_warning("Orage xfical_icalcomponent_archive_recurrent: builtin timezone %s not found, conversion failed.", etz_loc);
            }
            edate = icaltime_convert_to_zone(edate, l_icaltimezone);
        }
        duration = icaltime_subtract(edate, sdate);
        upd_edate = TRUE;
    }
    else { 
        p = icalcomponent_get_first_property(e, ICAL_DURATION_PROPERTY);
        if (p) { /* we have DURATION, which does not need changes */
            duration = icalproperty_get_duration(p);
        }
        else { /* neither duration, nor dtend, assume dtend=dtstart */
            duration = icaltime_subtract(edate, sdate);
        }
    }

    p = icalcomponent_get_first_property(e, ICAL_RRULE_PROPERTY);
    nsdate = icaltime_null_time();
    rrule = icalproperty_get_rrule(p);
    ri = icalrecur_iterator_new(rrule, sdate);

    /* We must do a loop for finding the first occurence after the threshold */
    for (nsdate = icalrecur_iterator_next(ri),
            nedate = icaltime_add(nsdate, duration);
         !icaltime_is_null_time(nsdate)
         && (nedate.year*12 + nedate.month) 
                < ((threshold->tm_year)*12 + threshold->tm_mon);
         nsdate = icalrecur_iterator_next(ri),
            nedate = icaltime_add(nsdate, duration)){
    }
    icalrecur_iterator_free(ri);

    if (icaltime_is_null_time(nsdate)) { /* remove since it has ended */
        icalcomponent_remove_component(ical, e);
    }
    else { /* modify times*/
        icalcomponent_remove_property(e, pdtstart);
        if (stz_loc == NULL)
            icalcomponent_add_property(e, icalproperty_new_dtstart(nsdate));
        else
            icalcomponent_add_property(e
                    , icalproperty_vanew_dtstart(nsdate
                            , icalparameter_new_tzid(stz_loc)
                            , 0));
        if (upd_edate) {
            icalcomponent_remove_property(e, pdtend);
            if (stz_loc == NULL)
                icalcomponent_add_property(e, icalproperty_new_dtend(nedate));
            else
                icalcomponent_add_property(e
                        , icalproperty_vanew_dtend(nedate
                                , icalparameter_new_tzid(etz_loc)
                                , 0));
        }
    }
}

void xfical_icalcomponent_archive_normal(icalcomponent *e) 
{
    icalcomponent *d;

    /* Add to the archive file */
    d = icalcomponent_new_clone(e);
    icalcomponent_add_component(aical, d);

    /* Remove from the main file */
    icalcomponent_remove_component(ical, e);
}

gboolean xfical_keep_tidy(void)
{
    struct icaltimetype sdate, edate;
    static icalcomponent *c, *c2;
    icalproperty *p;
    struct tm *threshold;
    char *uid;

    if (!xfical_file_open() || !xfical_archive_open()) {
        g_message("Orage xfical_keep_tidy: file open error");
        return;
    }
    threshold = orage_localtime();
    threshold->tm_mday = 1;
    threshold->tm_year += 1900;
    if (threshold->tm_mon > lookback) {
        threshold->tm_mon -= lookback;
    }
    else {
        threshold->tm_mon += (12 - lookback);
        threshold->tm_year--;
    }
    threshold->tm_mon += 1;

    g_message("Orage archiving threshold: %d month(s)", lookback);
    g_message("Orage archiving before: %04d-%02d-%02d", threshold->tm_year
            , threshold->tm_mon, threshold->tm_mday);

    /* Check appointment file for items older than the threshold */
    /* Note: remove moves the "c" pointer to next item, so we need to store it
     *          first to process all of them or we end up skipping entries */
    for (c = icalcomponent_get_first_component(ical, ICAL_VEVENT_COMPONENT);
         c != 0;
         c = c2) {
         c2 = icalcomponent_get_next_component(ical, ICAL_VEVENT_COMPONENT);
        sdate = icalcomponent_get_dtstart(c);
        edate = icalcomponent_get_dtend(c);
        uid = (char*)icalcomponent_get_uid(c);
        if (icaltime_is_null_time(edate)) {
            edate = sdate;
        }
        /* Items with endate before threshold => archived.
         * Recurring items are duplicated in the archive file, and 
         * they are also updated in the current appointment file 
         * with a new startdate and enddate.
         */
        if ((edate.year*12 + edate.month) 
            < ((threshold->tm_year)*12 + (threshold->tm_mon))) {
    /* Read from appointment files and copy into archive file */
            p = icalcomponent_get_first_property(c, ICAL_RRULE_PROPERTY);
            g_message("Archiving uid: %s (%s)", uid, (p) ? "recur" : "normal");
            g_message("\tEnd year: %04d, month: %02d, day: %02d"
                    , edate.year, edate.month, edate.day);
            if (p)  /*  it is recurrent event */
                xfical_icalcomponent_archive_recurrent(c, threshold, uid);
            else 
                xfical_icalcomponent_archive_normal(c);
        }
    }

    icalset_mark(afical);
    icalset_commit(afical);
    icalset_mark(fical);
    icalset_commit(fical);
    g_message("Orage archiving threshold: %d month(s) done", lookback);
    return(TRUE);
}
