/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2005-2007 Juha Kautto  (juha at xfce.org)
 * Copyright (c) 2003-2005 Mickael Graf (korbinus at xfce.org)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the 
       Free Software Foundation
       51 Franklin Street, 5th Floor
       Boston, MA 02110-1301 USA

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
#include <math.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>
#include <ical.h>
#include <icalss.h>

#include "functions.h"
#include "mainbox.h"
#include "reminder.h"
#include "ical-code.h"
#include "event-list.h"
#include "appointment.h"
#include "parameters.h"

typedef struct
{
    struct icaltimetype stime; /* start time */
    struct icaltimetype etime; /* end time */
    struct icaldurationtype duration;
    struct icaltimetype ctime; /* completed time for VTODO appointmnets */
} xfical_period;

typedef struct
{
    int    count;     /* how many timezones we have */
    char **city;      /* pointer to timezone location name strings */
} xfical_timezone_array;

static icalset *fical = NULL,
               *afical = NULL;
static icalcomponent *ical = NULL,
                     *aical = NULL;

typedef struct _foreign_ical_files
{;
    icalset *fical;
    icalcomponent *ical;
} foreign_ical_files;

static foreign_ical_files f_ical[10];

/* timezone handling */
static icaltimezone *utc_icaltimezone = NULL;
static icaltimezone *local_icaltimezone = NULL;

/* Remember to keep this string table in sync with zones.tab
 * This is used only for translations purposes. It makes
 * possible to translate these timezones.
 */
const gchar *trans_timezone[] = {
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

static xfical_timezone_array xfical_get_timezones()
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

gboolean xfical_set_local_timezone()
{
    local_icaltimezone = NULL;
    g_par.local_timezone_utc = FALSE;
    if (!utc_icaltimezone)
            utc_icaltimezone = icaltimezone_get_utc_timezone();

    if (!ORAGE_STR_EXISTS(g_par.local_timezone)) {
        g_warning("xfical_set_local_timezone: empty timezone\n");
        g_par.local_timezone = g_strdup("floating");
    }

    if (strcmp(g_par.local_timezone,"UTC") == 0) {
        g_par.local_timezone_utc = TRUE;
        local_icaltimezone = utc_icaltimezone;
    }
    else if (strcmp(g_par.local_timezone,"floating") == 0) {
        g_warning("Default timezone set to floating. Do not use timezones when setting appointments, it does not make sense without proper local timezone.");
        return(TRUE); /* g_par.local_timezone = NULL */
    }
    else
        local_icaltimezone = 
                icaltimezone_get_builtin_timezone(g_par.local_timezone);

    if (!local_icaltimezone) {
        g_warning("xfical_set_local_timezone: builtin timezone %s not found"
                , g_par.local_timezone);
        return (FALSE);
    }
    return (TRUE); 
}

/*
 * Basically standard says that timezone should be added alwasy
 * when it is used, but in real life these are not needed since
 * all systems have their own timezone data, so let's save time
 * and space and comment this out. 
 */
/*
static void xfical_add_timezone(icalcomponent *p_ical, icalset *p_fical
        , char *loc)
{
    icaltimezone *icaltz=NULL;
    icalcomponent *itimezone=NULL;
                                                                                
    if (!loc) {
        g_warning("xfical_add_timezone: no location defined");
        return;
    }
    else
        orage_message("Adding timezone %s", loc);

    if (strcmp(loc,"UTC") == 0 
    ||  strcmp(loc,"floating") == 0) {
        return;
    }
                                                                                
    icaltz=icaltimezone_get_builtin_timezone(loc);
    if (icaltz==NULL) {
        g_warning("xfical_add_timezone: timezone not found %s", loc);
        return;
    }
    itimezone=icaltimezone_get_component(icaltz);
    if (itimezone != NULL) {
        icalcomponent_add_component(p_ical
            , icalcomponent_new_clone(itimezone));
        icalset_mark(p_fical);
    }
    else
        g_warning("xfical_add_timezone: timezone add failed %s", loc);
}
*/

static gboolean xfical_internal_file_open(icalcomponent **p_ical
        , icalset **p_fical, gchar *file_icalpath, gboolean test)
{
#undef P_N
#define P_N "xfical_internal_file_open: "
    icalcomponent *iter;
    gint cnt=0;

    if (*p_fical != NULL)
        g_warning(P_N "file already open");
    if (!ORAGE_STR_EXISTS(file_icalpath)) {
        if (test)
            g_warning(P_N "file empty");
        else
            g_error(P_N "file empty");
        return(FALSE);
    }
    if ((*p_fical = icalset_new_file(file_icalpath)) == NULL) {
        if (test)
            g_warning(P_N "Could not open ical file (%s) %s\n"
                    , file_icalpath, icalerror_strerror(icalerrno));
        else
            g_error(P_N "Could not open ical file (%s) %s\n"
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
            if (test) { /* failed */
                g_warning(P_N "no top level (VCALENDAR) component in calendar file %s", file_icalpath);
                return(FALSE);
            }
        /* calendar missing, need to add one.  
         * Note: According to standard rfc2445 calendar always needs to
         *       contain at least one other component. So strictly speaking
         *       this is not valid entry before adding an event or timezone
         */
            *p_ical = icalcomponent_vanew(ICAL_VCALENDAR_COMPONENT
                   , icalproperty_new_version("2.0")
                   , icalproperty_new_prodid("-//Xfce//Orage//EN")
                   , NULL);
            /*
            xfical_add_timezone(*p_ical, *p_fical, g_par.local_timezone);
            */
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
                g_warning(P_N "too many top level components in calendar file %s", file_icalpath);
                if (test) { /* failed */
                    return(FALSE);
                }
            }
            if (test 
            && icalcomponent_isa(*p_ical) != ICAL_VCALENDAR_COMPONENT) {
                g_warning(P_N "top level component is not VCALENDAR %s", file_icalpath);
                return(FALSE);
            }
        }
    }
    return(TRUE);
}

gboolean xfical_file_open(gboolean foreign)
{ 
    gboolean ok;
    gint i;

    ok = xfical_internal_file_open(&ical, &fical, g_par.orage_file, FALSE);
    if (ok && foreign) /* let's open foreign files */
        for (i = 0; i < g_par.foreign_count; i++) {
            ok = xfical_internal_file_open(&(f_ical[i].ical), &(f_ical[i].fical)
                    , g_par.foreign_data[i].file, FALSE);
            if (!ok) {
                f_ical[i].ical = NULL;
                f_ical[i].fical = NULL;
            }
        }
    return(ok);
}

gboolean xfical_archive_open(void)
{
    if (!g_par.archive_limit)
        return(FALSE);
    if (!ORAGE_STR_EXISTS(g_par.archive_file))
        return(FALSE);

    return(xfical_internal_file_open(&aical, &afical, g_par.archive_file
            , FALSE));
}

gboolean xfical_file_check(gchar *file_name)
{ 
    icalcomponent *x_ical = NULL;
    icalset *x_fical = NULL;

    return(xfical_internal_file_open(&x_ical, &x_fical, file_name, TRUE));
}

void xfical_file_close(gboolean foreign)
{
#undef  P_N 
#define P_N "xfical_file_close: "
    gint i;

    if (fical == NULL)
        g_warning(P_N "fical is NULL");
    else
        icalset_free(fical);
    fical = NULL;
    
    if (foreign) 
        for (i = 0; i < g_par.foreign_count; i++) {
            if (f_ical[i].fical == NULL)
                g_warning(P_N "foreign fical is NULL");
            else {
                icalset_free(f_ical[i].fical);
                f_ical[i].fical = NULL;
            }
        }
}

void xfical_archive_close(void)
{
#undef  P_N 
#define P_N "xfical_archive_close: "
    if (!ORAGE_STR_EXISTS(g_par.archive_file))
        return;

    if (afical == NULL)
        g_warning(P_N "afical is NULL");
    icalset_free(afical);
    afical = NULL;
}

static struct icaltimetype ical_get_current_local_time()
{
    struct tm *tm;
    static struct icaltimetype ctime;

    if (g_par.local_timezone_utc)
        ctime = icaltime_current_time_with_zone(utc_icaltimezone);
    else if ((g_par.local_timezone)
        &&   (strcmp(g_par.local_timezone, "floating") != 0))
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

static struct icaltimetype convert_to_timezone(struct icaltimetype t
        , icalproperty *p)
{
#undef  P_N 
#define P_N "convert_to_timezone: "
    icalparameter *itime_tz = NULL;
    gchar *tz_loc = NULL;
    icaltimezone *l_icaltimezone = NULL;
    struct icaltimetype tz;

    itime_tz = icalproperty_get_first_parameter(p, ICAL_TZID_PARAMETER);
    if (itime_tz) {
         tz_loc = (char *) icalparameter_get_tzid(itime_tz);
         l_icaltimezone = icaltimezone_get_builtin_timezone(tz_loc);
         if (!l_icaltimezone) {
            g_warning(P_N "builtin timezone %s not found, conversion failed.", tz_loc);
        }
        tz = icaltime_convert_to_zone(t, l_icaltimezone);
    }
    else
        tz = t;

    return (tz);
}

static struct icaltimetype convert_to_local_timezone(struct icaltimetype t
            , icalproperty *p)
{
    struct icaltimetype tl;

    tl = convert_to_timezone(t, p);
    tl = icaltime_convert_to_zone(tl, local_icaltimezone);

    return (tl);
}

static xfical_period get_period(icalcomponent *c) 
{
#undef  P_N 
#define P_N "get_period: "
    icalproperty *p = NULL, *p2 = NULL;
    xfical_period per;
    icalcomponent_kind ikind = ICAL_VEVENT_COMPONENT;
    struct icaldurationtype duration_tmp;
    gint dur_int;

    /* Exactly one start time must be there */
    p = icalcomponent_get_first_property(c, ICAL_DTSTART_PROPERTY);
    if (p != NULL) {
        per.stime = icalproperty_get_dtstart(p);
        per.stime = convert_to_local_timezone(per.stime, p);
    }
    else {
        g_warning(P_N "start time not found (%s)", icalcomponent_get_uid(c));
        per.stime = icaltime_null_time();
    } 

    /* Either endtime/duetime or duration may be there. 
     * But neither is required.
     * VTODO may also have completed time
     */
    ikind = icalcomponent_isa(c);
    if (ikind == ICAL_VEVENT_COMPONENT)
        p = icalcomponent_get_first_property(c, ICAL_DTEND_PROPERTY);
    else if (ikind == ICAL_VTODO_COMPONENT) {
        p = icalcomponent_get_first_property(c, ICAL_DUE_PROPERTY);
        p2 = icalcomponent_get_first_property(c, ICAL_COMPLETED_PROPERTY);
    }
    else if (ikind == ICAL_VJOURNAL_COMPONENT)
        p = NULL; /* does not exist for journal */
    else {
        g_warning(P_N "unknown component type (%s)", icalcomponent_get_uid(c));
        p = NULL;
    }
    if (p != NULL) {
        if (ikind == ICAL_VEVENT_COMPONENT)
            per.etime = icalproperty_get_dtend(p);
        else if (ikind == ICAL_VTODO_COMPONENT)
            per.etime = icalproperty_get_due(p);
        per.etime = convert_to_local_timezone(per.etime, p);
        per.duration = icaltime_subtract(per.etime, per.stime);
        if (icaltime_is_date(per.stime) 
            && icaldurationtype_as_int(per.duration) != 0) {
        /* need to subtract 1 day.
         * read explanation from appt_add_internal: appt->endtime processing */
            duration_tmp = icaldurationtype_from_int(60*60*24);
            dur_int = icaldurationtype_as_int(per.duration);
            dur_int -= icaldurationtype_as_int(duration_tmp);
            per.duration = icaldurationtype_from_int(dur_int);
            per.etime = icaltime_add(per.stime, per.duration);
        }

    }
    else {
        p = icalcomponent_get_first_property(c, ICAL_DURATION_PROPERTY);
        if (p != NULL) {
            per.duration = icalproperty_get_duration(p);
            per.etime = icaltime_add(per.stime, per.duration);
        }
        else {
            per.etime = per.stime;
            per.duration = icaldurationtype_null_duration();
        } 
    } 

    if (p2 != NULL) {
        per.ctime = icalproperty_get_completed(p2);
        per.ctime = convert_to_local_timezone(per.ctime, p2);
    }
    else
        per.ctime = icaltime_null_time();

    return (per);
}

/* basically copied from icaltime_compare, which can't be used
 * because it uses utc
 */
static int local_compare(struct icaltimetype a, struct icaltimetype b)
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
static int local_compare_date_only(struct icaltimetype a
        , struct icaltimetype b)
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

static struct icaltimetype convert_to_zone(struct icaltimetype t, gchar *tz)
{
#undef  P_N 
#define P_N "convert_to_zone: "
    struct icaltimetype wtime = t;
    icaltimezone *l_icaltimezone = NULL;

    if ORAGE_STR_EXISTS(tz) {
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
                g_warning(P_N "builtin timezone %s not found, conversion failed.", tz);
            else
                wtime = icaltime_convert_to_zone(t, l_icaltimezone);
        }
    }
    else  /* floating time */
        if (local_icaltimezone)
            wtime = icaltime_convert_to_zone(t, local_icaltimezone);

    return(wtime);
}

int xfical_compare_times(xfical_appt *appt)
{
#undef  P_N 
#define P_N "xfical_compare_times: "
    struct icaltimetype stime, etime;
    const char *text;
    struct icaldurationtype duration;

    if (appt->use_duration) {
        if (! ORAGE_STR_EXISTS(appt->starttime)) {
            g_warning(P_N "null start time");
            return(0); /* should be error ! */
        }
        stime = icaltime_from_string(appt->starttime);
        duration = icaldurationtype_from_int(appt->duration);
        etime = icaltime_add(stime, duration);
        text  = icaltime_as_ical_string(etime);
        g_strlcpy(appt->endtime, text, 17);
        /*
        g_free(appt->end_tz_loc);
        */
        appt->end_tz_loc = g_strdup(appt->start_tz_loc);
        return(0); /* ok */

    }
    else {
        if (ORAGE_STR_EXISTS(appt->starttime) 
        &&  ORAGE_STR_EXISTS(appt->endtime)) {
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
            g_warning(P_N "null time %s %s"
                    , appt->starttime, appt->endtime);
            return(0); /* should be error ! */
        }
    }
}

 /* allocates memory and initializes it for new ical_type structure
  * returns: NULL if failed and pointer to xfical_appt if successfull.
  *         You must free it after not being used anymore. (g_free())
  */
xfical_appt *xfical_appt_alloc()
{
    xfical_appt *appt;
    int i;

    appt = g_new0(xfical_appt, 1);
    appt->availability = 1;
    appt->alarm_before = TRUE;
    appt->alarm_related_start = TRUE;
    for (i=0; i <= 6; i++)
        appt->recur_byday[i] = TRUE;
    return(appt);
}

static void alarm_free(gpointer galarm, gpointer dummy)
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

static gint alarm_order(gconstpointer a, gconstpointer b)
{
    struct icaltimetype t1, t2;

    t1=icaltime_from_string(((alarm_struct *)a)->alarm_time->str);
    t2=icaltime_from_string(((alarm_struct *)b)->alarm_time->str);

    return(icaltime_compare(t1, t2));
}

static void alarm_add(icalproperty_status action
         , char *uid, char *file_type, char *title, char *description
         , char *sound, gint repeat_cnt, gint repeat_delay
         , struct icaltimetype alarm_time, struct icaltimetype event_time)
{
    alarm_struct *new_alarm;

    new_alarm = g_new(alarm_struct, 1);
    new_alarm->uid = g_string_new(file_type);
    new_alarm->uid = g_string_append(new_alarm->uid, uid);
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

    g_par.alarm_list = g_list_append(g_par.alarm_list, new_alarm);
}

static void xfical_alarm_build_list_internal_real(gboolean first_list_today
        , icalcomponent *base, char *file_type)
{
#undef P_N
#define P_N "xfical_alarm_build_list_internal_real: "
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
    icalparameter *trg_related_par;
    icalparameter_related rel;

    cur_time = ical_get_current_local_time();

    for (c = icalcomponent_get_first_component(base, ICAL_ANY_COMPONENT);
         c != 0;
         c = icalcomponent_get_next_component(base, ICAL_ANY_COMPONENT)) {
        cnt_event++;
        suid = (char*)icalcomponent_get_uid(c);
        /*
        p = icalcomponent_get_first_property(c, ICAL_SUMMARY_PROPERTY);
        if (p)
            ssummary = (char*)icalproperty_get_value_as_string(p);
        p = icalcomponent_get_first_property(c, ICAL_SUMMARY_PROPERTY);
        if (p)
            sdescription = (char*)icalproperty_get_value_as_string(p);
            */
        per = get_period(c);
        if (first_list_today && icaltime_is_date(per.stime)) {
        /* this is special pre 4.3 xfcalendar compatibility alarm:
           Send alarm window always for date type events. */
            ssummary = (char*)icalcomponent_get_summary(c);
            sdescription = (char*)icalcomponent_get_description(c);
            if (local_compare_date_only(per.stime, cur_time) == 0) {
                alarm_add(ICAL_ACTION_DISPLAY
                        , suid, file_type, ssummary, sdescription
                        , NULL, 0, 0, per.stime, per.stime);
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
                            , suid, file_type, ssummary, sdescription
                            , NULL, 0, 0, next_date, per.stime);
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
                        trg_related_par = icalproperty_get_first_parameter(p
                                , ICAL_RELATED_PARAMETER);
                        if (trg_related_par)
                            rel = icalparameter_get_related(trg_related_par);
                        else
                            rel = ICAL_RELATED_START;
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
                        g_warning(P_N "Unknown property (%s) in Alarm",
                            (char *)icalproperty_get_property_name(p));
                        break;
                } 
            } 
        }  /* ALARMS */
        if (trg_found) {
            cnt_alarm++;
            ssummary = (char*)icalcomponent_get_summary(c);
            sdescription = (char*)icalcomponent_get_description(c);
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
            if (rel == ICAL_RELATED_END)
                alarm_time = icaltime_add(per.etime, trg.duration);
            else /* default is ICAL_RELATED_START */
                alarm_time = icaltime_add(per.stime, trg.duration);
            if (icaltime_compare(cur_time, alarm_time) <= 0) { /* active */
                alarm_add(stat, suid, file_type, ssummary, sdescription
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
                    && (local_compare(cur_time, next_date) > 0) ;
                    next_date = icalrecur_iterator_next(ri)) {
                    cnt_repeat++;
                }
                icalrecur_iterator_free(ri);
                if (icaltime_compare(cur_time, next_date) <= 0) {
                    alarm_add(stat, suid, file_type, ssummary, sdescription
                        , ssound, repeat_cnt, repeat_delay
                        , next_date, per.stime);
                    cnt_act_alarm++;
                }
            }
        } /* trg_found */
    }  /* EVENTS */
    g_par.alarm_list = g_list_sort(g_par.alarm_list, alarm_order);
    if (first_list_today) {
        orage_message("Build alarm list: Processed %d events."
                , cnt_event);
        orage_message("\tFound %d alarms of which %d are active. (Searched %d recurring alarms.)"
                , cnt_alarm, cnt_act_alarm, cnt_repeat);
        }
}

static void xfical_alarm_build_list_internal(gboolean first_list_today)
{
    gchar file_type[8];
    gint i;

    /* first remove all old alarms by cleaning the whole structure */
    g_list_foreach(g_par.alarm_list, alarm_free, NULL);
    g_list_free(g_par.alarm_list);
    g_par.alarm_list = NULL;

    /* first search base orage file */
    strcpy(file_type, "O00.");
    xfical_alarm_build_list_internal_real(first_list_today, ical, file_type);
    /* then process all foreign files */
    for (i = 0; i < g_par.foreign_count; i++) {
        g_sprintf(file_type, "F%02d.", i);
        xfical_alarm_build_list_internal_real(first_list_today, f_ical[i].ical
                , file_type);
    }
}

void xfical_alarm_build_list(gboolean first_list_today)
{
    xfical_file_open(TRUE);
    xfical_alarm_build_list_internal(first_list_today);
    xfical_file_close(TRUE);
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

gboolean xfical_duration(char *alarm_stime, int *days, int *hours, int *mins)
{
    struct icaltimetype alarm_time, cur_time;
    struct icaldurationtype duration;

    duration = icaldurationtype_null_duration();
    cur_time = ical_get_current_local_time();
    alarm_time = icaltime_from_string(alarm_stime);
    duration = icaltime_subtract(alarm_time, cur_time);
    if (icaldurationtype_is_bad_duration(duration) || duration.is_neg)
        return(FALSE);
    *days = duration.weeks*7 + duration.days;
    *hours = duration.hours;
    *mins = duration.minutes;
    return(TRUE);
 }

static char *generate_uid()
{
    gchar xf_host[XFICAL_UID_LEN/2+1];
    static gchar xf_uid[XFICAL_UID_LEN+1];
    static int seq = 0;
    struct icaltimetype dtstamp;

    dtstamp = icaltime_current_time_with_zone(utc_icaltimezone);
    gethostname(xf_host, XFICAL_UID_LEN/2);
    xf_host[XFICAL_UID_LEN/2] = '\0';
    g_snprintf(xf_uid, XFICAL_UID_LEN, "Orage-%s%d-%lu@%s"
            , icaltime_as_ical_string(dtstamp), seq, (long) getuid(), xf_host);
    if (++seq > 999)
        seq = 0;
    return(xf_uid);
}

static void appt_add_alarm_internal(xfical_appt *appt, icalcomponent *ievent)
{
    icalcomponent *ialarm;
    gint duration=0;
    struct icaltriggertype trg;
    icalattach *attach;

    duration = appt->alarmtime;
    trg.time = icaltime_null_time();
    if (appt->alarm_before)
        trg.duration = icaldurationtype_from_int(-duration);
    else
        trg.duration = icaldurationtype_from_int(duration);
    /********** DISPLAY **********/
    ialarm = icalcomponent_vanew(ICAL_VALARM_COMPONENT
        , icalproperty_new_action(ICAL_ACTION_DISPLAY)
        , NULL);
    if (appt->alarm_related_start)
        icalcomponent_add_property(ialarm
                , icalproperty_new_trigger(trg));
    else
        icalcomponent_add_property(ialarm
                , icalproperty_vanew_trigger(trg
                        , icalparameter_new_related(ICAL_RELATED_END)
                        , 0));
    if ORAGE_STR_EXISTS(appt->note)
        icalcomponent_add_property(ialarm
            , icalproperty_new_description(appt->note));
    else if ORAGE_STR_EXISTS(appt->title)
        icalcomponent_add_property(ialarm
            , icalproperty_new_description(appt->title));
    else
        icalcomponent_add_property(ialarm
            , icalproperty_new_description(_("Orage default alarm")));
    icalcomponent_add_component(ievent, ialarm);
    /********** AUDIO **********/
    if ORAGE_STR_EXISTS(appt->sound) {
        ialarm = icalcomponent_vanew(ICAL_VALARM_COMPONENT
            , icalproperty_new_action(ICAL_ACTION_AUDIO)
            , NULL);
        if (appt->alarm_related_start)
            icalcomponent_add_property(ialarm
                    , icalproperty_new_trigger(trg));
        else
            icalcomponent_add_property(ialarm
                    , icalproperty_vanew_trigger(trg
                            , icalparameter_new_related(ICAL_RELATED_END)
                            , 0));
        attach = icalattach_new_from_url(appt->sound);
        icalcomponent_add_property(ialarm
            , icalproperty_new_attach(attach));
        if (appt->soundrepeat) {
            icalcomponent_add_property(ialarm
                , icalproperty_new_repeat(appt->soundrepeat_cnt));
            icalcomponent_add_property(ialarm
                , icalproperty_new_duration(
                        icaldurationtype_from_int(appt->soundrepeat_len)));
        }
        icalcomponent_add_component(ievent, ialarm);
    }
}

static void appt_add_recur_internal(xfical_appt *appt, icalcomponent *ievent)
{
#undef P_N
#define P_N "appt_add_recur_internal: "
    gchar recur_str[1001], *recur_p, *recur_p2;
    struct icalrecurrencetype rrule;
    struct icaltimetype wtime;
    gchar *byday_a[] = {"MO", "TU", "WE", "TH", "FR", "SA", "SU"};
    const char *text;
    int i, cnt;
    icaltimezone *l_icaltimezone = NULL;

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
            g_warning(P_N "Unsupported freq");
            icalrecurrencetype_clear(&rrule);
    }
    if (appt->interval > 1) { /* not default, need to insert it */
        recur_p += g_sprintf(recur_p, ";INTERVAL=%d", appt->interval);
    }
    if (appt->recur_limit == 1) {
        recur_p += g_sprintf(recur_p, ";COUNT=%d", appt->recur_count);
    }
    else if (appt->recur_limit == 2) { /* needs to be in UTC */
/* BUG 2937: convert recur_until to utc from start time timezone */
        wtime = icaltime_from_string(appt->recur_until);
        if ORAGE_STR_EXISTS(appt->start_tz_loc) {
        /* Null == floating => no special action needed */
            if (strcmp(appt->start_tz_loc, "floating") == 0) {
                wtime = icaltime_convert_to_zone(wtime
                        , local_icaltimezone);
            }
            else if (strcmp(appt->start_tz_loc, "UTC") != 0) {
            /* FIXME: add this vtimezone to vcalendar if it is not there */
                l_icaltimezone = icaltimezone_get_builtin_timezone(
                        appt->start_tz_loc);
                wtime = icaltime_convert_to_zone(wtime, l_icaltimezone);
            }
        }
        else
            wtime = icaltime_convert_to_zone(wtime, local_icaltimezone);
        wtime = icaltime_convert_to_zone(wtime, utc_icaltimezone);
        text  = icaltime_as_ical_string(wtime);
        recur_p += g_sprintf(recur_p, ";UNTIL=%s", text);
    }
    recur_p2 = recur_p; /* store current pointer */
    for (i = 0, cnt = 0; i <= 6; i++) {
        if (appt->recur_byday[i]) {
            if (cnt == 0) /* first day found */
                recur_p = g_stpcpy(recur_p, ";BYDAY=");
            else /* continue the list */
                recur_p = g_stpcpy(recur_p, ",");
            if ((appt->freq == XFICAL_FREQ_MONTHLY
                    || appt->freq == XFICAL_FREQ_YEARLY)
                && (appt->recur_byday_cnt[i])) /* number defined */
                    recur_p += g_sprintf(recur_p, "%d"
                            , appt->recur_byday_cnt[i]);
            recur_p = g_stpcpy(recur_p, byday_a[i]);
            cnt++;
        }
    }
    if (cnt == 7) { /* all days defined... */
        *recur_p2 = *recur_p; /* ...reset to null... */
        recur_p = recur_p2;   /* ...no need for BYDAY then */
    }
    else if (appt->interval > 1 && appt->freq == XFICAL_FREQ_WEEKLY) {
        /* we have BYDAY rule, let's check week starting date:
         * WKST has meaning only in two cases:
         * 1) WEEKLY rule && interval > 1 && BYDAY rule is in use 
         * 2) YEARLY rule && BYWEEKNO rule is in use 
         * BUT Orage is not using BYWEEKNO rule, so we only check 1)
         * Monday is default, so we can skip that, too
         * */
        if (g_par.ical_weekstartday)
            recur_p += g_sprintf(recur_p, ";WKST=%s"
                    , byday_a[g_par.ical_weekstartday]);
    }
    rrule = icalrecurrencetype_from_string(recur_str);
    icalcomponent_add_property(ievent, icalproperty_new_rrule(rrule));
}

static char *appt_add_internal(xfical_appt *appt, gboolean add, char *uid
        , struct icaltimetype cre_time)
{
#undef P_N
#define P_N "appt_add_internal: "
    icalcomponent *icmp;
    struct icaltimetype dtstamp, create_time, wtime;
    gboolean end_time_done;
    gchar *int_uid, *ext_uid;
    struct icaldurationtype duration;
    int i;
    icalcomponent_kind ikind = ICAL_VEVENT_COMPONENT;
    icaltimezone *l_icaltimezone = NULL;

    dtstamp = icaltime_current_time_with_zone(utc_icaltimezone);
    if (add) {
        int_uid = generate_uid();
        ext_uid = g_strconcat("O00.", int_uid, NULL);
        appt->uid = ext_uid;
        create_time = dtstamp;
    }
    else { /* mod */
        int_uid = uid+4;
        ext_uid = uid;
        if (icaltime_is_null_time(cre_time))
            create_time = dtstamp;
        else
            create_time = cre_time;
    }

    if (appt->type == XFICAL_TYPE_EVENT)
        ikind = ICAL_VEVENT_COMPONENT;
    else if (appt->type == XFICAL_TYPE_TODO)
        ikind = ICAL_VTODO_COMPONENT;
    else if (appt->type == XFICAL_TYPE_JOURNAL)
        ikind = ICAL_VJOURNAL_COMPONENT;
    else
        g_warning(P_N "Unsupported Type");

    icmp = icalcomponent_vanew(ikind
           , icalproperty_new_uid(int_uid)
           , icalproperty_new_categories("ORAGENOTE")
           , icalproperty_new_class(ICAL_CLASS_PUBLIC)
           , icalproperty_new_dtstamp(dtstamp)
           , icalproperty_new_created(create_time)
           , NULL);

    if ORAGE_STR_EXISTS(appt->title)
        icalcomponent_add_property(icmp
                , icalproperty_new_summary(appt->title));
    if ORAGE_STR_EXISTS(appt->note)
        icalcomponent_add_property(icmp
                , icalproperty_new_description(appt->note));
    if (appt->type != XFICAL_TYPE_JOURNAL) {
        if ORAGE_STR_EXISTS(appt->location)
            icalcomponent_add_property(icmp
                    , icalproperty_new_location(appt->location));
    }
    if (appt->type == XFICAL_TYPE_EVENT) {
        if (appt->availability == 0)
            icalcomponent_add_property(icmp
               , icalproperty_new_transp(ICAL_TRANSP_TRANSPARENT));
        else if (appt->availability == 1)
            icalcomponent_add_property(icmp
               , icalproperty_new_transp(ICAL_TRANSP_OPAQUE));
    }
    if (appt->allDay) { /* cut the string after Date: yyyymmdd */
        appt->starttime[8] = '\0';
        appt->endtime[8] = '\0';
    }

    if ORAGE_STR_EXISTS(appt->starttime) {
        wtime=icaltime_from_string(appt->starttime);
        if (appt->allDay) { /* date */
            icalcomponent_add_property(icmp
                    , icalproperty_new_dtstart(wtime));
        }
        else if ORAGE_STR_EXISTS(appt->start_tz_loc) {
            if (strcmp(appt->start_tz_loc, "UTC") == 0) {
                wtime=icaltime_convert_to_zone(wtime, utc_icaltimezone);
                icalcomponent_add_property(icmp
                    , icalproperty_new_dtstart(wtime));
            }
            else if (strcmp(appt->start_tz_loc, "floating") == 0) {
                icalcomponent_add_property(icmp
                    , icalproperty_new_dtstart(wtime));
            }
            else {
            /* FIXME: add this vtimezone to vcalendar if it is not there */
                icalcomponent_add_property(icmp
                    , icalproperty_vanew_dtstart(wtime
                            , icalparameter_new_tzid(appt->start_tz_loc)
                            , 0));
            }
        }
        else { /* floating time */
            icalcomponent_add_property(icmp
                    , icalproperty_new_dtstart(wtime));
        }
    }

    if (appt->type != XFICAL_TYPE_JOURNAL) { 
        /* journal has no duration nor enddate or due */
        if (appt->use_duration) { /* both event and todo can have duration */
            duration = icaldurationtype_from_int(appt->duration);
            icalcomponent_add_property(icmp
                    , icalproperty_new_duration(duration));
        }
        else if ORAGE_STR_EXISTS(appt->endtime) {
            end_time_done = FALSE;
            wtime = icaltime_from_string(appt->endtime);
            if (appt->allDay) { 
                /* need to add 1 day. For example:
                 * DTSTART=20070221 & DTEND=20070223
                 * means only 21 and 22 February */
                duration = icaldurationtype_from_int(60*60*24);
                wtime = icaltime_add(wtime, duration);
            }
            else if ORAGE_STR_EXISTS(appt->end_tz_loc) {
            /* Null == floating => no special action needed */
                if (strcmp(appt->end_tz_loc, "UTC") == 0)
                    wtime = icaltime_convert_to_zone(wtime, utc_icaltimezone);
                else if (strcmp(appt->end_tz_loc, "floating") != 0) {
                /* FIXME: add this vtimezone to vcalendar if it is not there */
                    if (appt->type == XFICAL_TYPE_EVENT)
                        icalcomponent_add_property(icmp
                            , icalproperty_vanew_dtend(wtime
                                    , icalparameter_new_tzid(appt->end_tz_loc)
                                    , 0));
                    else if (appt->type == XFICAL_TYPE_TODO)
                        icalcomponent_add_property(icmp
                            , icalproperty_vanew_due(wtime
                                    , icalparameter_new_tzid(appt->end_tz_loc)
                                    , 0));
                    end_time_done = TRUE;
                }
            }
            if (!end_time_done) {
                if (appt->type == XFICAL_TYPE_EVENT)
                    icalcomponent_add_property(icmp
                            , icalproperty_new_dtend(wtime));
                else if (appt->type == XFICAL_TYPE_TODO)
                    icalcomponent_add_property(icmp
                            , icalproperty_new_due(wtime));
            }
        }
    }

    if (appt->type == XFICAL_TYPE_TODO) {
        if (appt->completed) {
            wtime = icaltime_from_string(appt->completedtime);
            if ORAGE_STR_EXISTS(appt->completed_tz_loc) {
            /* Null == floating => no special action needed */
                if (strcmp(appt->completed_tz_loc, "floating") == 0) {
                    wtime = icaltime_convert_to_zone(wtime
                            , local_icaltimezone);
                }
                else if (strcmp(appt->completed_tz_loc, "UTC") != 0) {
                /* FIXME: add this vtimezone to vcalendar if it is not there */
                    l_icaltimezone = icaltimezone_get_builtin_timezone(
                            appt->start_tz_loc);
                    wtime = icaltime_convert_to_zone(wtime, l_icaltimezone);
                }
            }
            else
                wtime = icaltime_convert_to_zone(wtime, local_icaltimezone);
            wtime = icaltime_convert_to_zone(wtime, utc_icaltimezone);
            icalcomponent_add_property(icmp
                    , icalproperty_new_completed(wtime));
        }
    }

    if (appt->type != XFICAL_TYPE_JOURNAL && appt->freq != XFICAL_FREQ_NONE) {
        /* journal has no recurrency.
         * NOTE: according to standard VJURNAL has this recurrency, 
         * but I can't understand how it coud be usefull, 
         * so Orage takes it away */
        appt_add_recur_internal(appt, icmp);
    }
    if (appt->type != XFICAL_TYPE_JOURNAL && appt->alarmtime != 0) {
        /* journal has no alarms */
        appt_add_alarm_internal(appt, icmp);
    }

    if (ext_uid[0] == 'O') {
        icalcomponent_add_component(ical, icmp);
        icalset_mark(fical);
    }
    else if (ext_uid[0] == 'F') {
        sscanf(ext_uid, "F%02d", &i);
        if (i < g_par.foreign_count && f_ical[i].ical != NULL) {
            icalcomponent_add_component(f_ical[i].ical, icmp);
            icalset_mark(f_ical[i].fical);
        }
        else {
            g_warning(P_N "unknown foreign file number %s", uid);
            return(NULL);
        }
    }
    else { /* note: we never update/add Archive file */ 
        g_warning(P_N "unknown file type %s", ext_uid);
        return(NULL);
    }
    xfical_alarm_build_list_internal(FALSE);
    return(ext_uid);
}

 /* add EVENT/TODO/JOURNAL type ical appointment to ical file
  * app: pointer to filled xfical_appt structure, which is stored
  *      Caller is responsible for filling and allocating and freeing it.
  *  returns: NULL if failed and new ical id if successfully added. 
  *           This ical id is owned by the routine. Do not deallocate it.
  *           It will be overwrittewritten by next invocation of this function.
  */
char *xfical_appt_add(xfical_appt *appt)
{
    /* FIXME: make it possible to add events into foreign files */
    return(appt_add_internal(appt, TRUE, NULL, icaltime_null_time()));
}

void ical_appt_get_alarm_internal(icalcomponent *c,  xfical_appt *appt)
{
#undef P_N
#define P_N "ical_appt_get_alarm_internal: "
    icalcomponent *ca = NULL;
    icalproperty *p = NULL;
    struct icaltriggertype trg;
    icalattach *attach;
    struct icaldurationtype duration;
    icalparameter *trg_related_par;
    icalparameter_related rel;

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
                        appt->alarmtime = icaldurationtype_as_int(trg.duration);
                        if (appt->alarmtime < 0) { /* before */
                            appt->alarmtime *= -1;
                            appt->alarm_before = TRUE;
                        }
                        else
                            appt->alarm_before = FALSE;
                        trg_related_par = icalproperty_get_first_parameter(p
                                , ICAL_RELATED_PARAMETER);
                        if (trg_related_par) {
                            rel = icalparameter_get_related(trg_related_par);
                            if (rel == ICAL_RELATED_END)
                                appt->alarm_related_start = FALSE;
                            else
                                appt->alarm_related_start = TRUE;
                        }
                        else
                            appt->alarm_related_start = TRUE;
                    }
                    else
                        g_warning(P_N "Can not process time triggers");
                    break;
                case ICAL_ATTACH_PROPERTY:
                    attach = icalproperty_get_attach(p);
                    appt->sound = (char *)icalattach_get_url(attach);
                    break;
                case ICAL_REPEAT_PROPERTY:
                    appt->soundrepeat = TRUE;
                    appt->soundrepeat_cnt = icalproperty_get_repeat(p);
                    break;
                case ICAL_DESCRIPTION_PROPERTY:
                    if (appt->note == NULL)
                        appt->note = (char *) icalproperty_get_description(p);
                    break;
                case ICAL_DURATION_PROPERTY:
                    duration = icalproperty_get_duration(p);
                    appt->soundrepeat_len = icaldurationtype_as_int(duration);
                case ICAL_ACTION_PROPERTY:
                /* no actions defined */
                    break;
                default:
                    g_warning(P_N "unknown property %s", (char *)icalproperty_get_property_name(p));
                    break;
            }
        }
    }
}

static void process_start_date(xfical_appt *appt, icalproperty *p
        , struct icaltimetype *itime, struct icaltimetype *stime
        , struct icaltimetype *sltime, struct icaltimetype *etime)
{
    const char *text;
    icalparameter *itime_tz;

    text = icalproperty_get_value_as_string(p);
    *itime = icaltime_from_string(text);
    *stime = convert_to_timezone(*itime, p);
    *sltime = convert_to_local_timezone(*itime, p);
    /*
    g_print("PROPERTY: %s\n", text);
    text  = icaltime_as_ical_string(*itime);
    */
    g_strlcpy(appt->starttime, text, 17);
    if (icaltime_is_date(*itime))
        appt->allDay = TRUE;
    else if (icaltime_is_utc(*itime))
        appt->start_tz_loc = "UTC";
    else { /* let's check timezone */
        itime_tz = icalproperty_get_first_parameter(p, ICAL_TZID_PARAMETER);
        if (itime_tz)
            appt->start_tz_loc = (char *)icalparameter_get_tzid(itime_tz);
    }
    if (appt->endtime[0] == '\0') {
        g_strlcpy(appt->endtime,  appt->starttime, 17);
        appt->end_tz_loc = appt->start_tz_loc;
        etime = stime;
    }
}

static void process_end_date(xfical_appt *appt, icalproperty *p
        , struct icaltimetype *itime, struct icaltimetype *etime
        , struct icaltimetype *eltime)
{
    const char *text;
    icalparameter *itime_tz;

    text = icalproperty_get_value_as_string(p);
    *itime = icaltime_from_string(text);
    *eltime = convert_to_local_timezone(*itime, p);
    /*
    text  = icaltime_as_ical_string(*itime);
    */
    g_strlcpy(appt->endtime, text, 17);
    if (icaltime_is_date(*itime)) {
        appt->allDay = TRUE;
    }
    else if (icaltime_is_utc(*itime))
        appt->end_tz_loc = "UTC";
    else { /* let's check timezone */
        itime_tz = icalproperty_get_first_parameter(p, ICAL_TZID_PARAMETER);
        if (itime_tz)
            appt->end_tz_loc = (char *)icalparameter_get_tzid(itime_tz);
        else
            appt->end_tz_loc = NULL;
    }
}

static void process_completed_date(xfical_appt *appt, icalproperty *p)
{
    const char *text;
    icalparameter *itime_tz;
    struct icaltimetype itime;
    struct icaltimetype eltime;

    /* FIXME: convert to local timezone */
    text = icalproperty_get_value_as_string(p);
    itime = icaltime_from_string(text);
    eltime = convert_to_local_timezone(itime, p);
    text  = icaltime_as_ical_string(eltime);
    appt->completed_tz_loc = g_par.local_timezone;
    g_strlcpy(appt->completedtime, text, 17);
    /*
    g_strlcpy(appt->completedtime, text, 17);
    appt->completed_tz_loc = "UTC";
    */
    appt->completed = TRUE;
}

 /* Read EVENT/TODO/JOURNAL component from ical datafile.
  * ical_uid:  key of ical comp appt-> is to be read
  * returns: if failed: NULL
  *          if successfull: xfical_appt pointer to xfical_appt struct 
  *              filled with data.
  *              This xfical_appt struct is owned by the routine.
  *              Do not deallocate it.
  *             It will be overdriven by next invocation of this function.
  *          NOTE: This routine does not fill starttimecur nor endtimecur,
  *                Those are always initialized to null string
  */
static xfical_appt *xfical_appt_get_internal(char *ical_uid
        , icalcomponent *base)
{
#undef P_N
#define P_N "xfical_appt_get_internal: "
    static xfical_appt appt;
    icalcomponent *c = NULL;
    icalproperty *p = NULL;
    gboolean key_found = FALSE;
    const char *text;
    struct icaltimetype itime, stime, etime, sltime, eltime;
    icalparameter *itime_tz;
    icalproperty_transp xf_transp;
    struct icalrecurrencetype rrule;
    struct icaldurationtype duration, duration_tmp;
    int i, cnt, day;
    gboolean stime_found = FALSE, etime_found = FALSE, valid_comp = FALSE;

    for (c = icalcomponent_get_first_component(base, ICAL_ANY_COMPONENT); 
         c != 0 && !key_found;
         c = icalcomponent_get_next_component(base, ICAL_ANY_COMPONENT)) {
        if (icalcomponent_isa(c) == ICAL_VEVENT_COMPONENT) {
            appt.type = XFICAL_TYPE_EVENT;
            valid_comp = TRUE;
        }
        else if (icalcomponent_isa(c) == ICAL_VTODO_COMPONENT) {
            appt.type = XFICAL_TYPE_TODO;
            valid_comp = TRUE;
        }
        else if (icalcomponent_isa(c) == ICAL_VJOURNAL_COMPONENT) {
            appt.type = XFICAL_TYPE_JOURNAL;
            valid_comp = TRUE;
        }
        else {
            g_warning(P_N "Unknown component");
            valid_comp = FALSE;
        }
        text = icalcomponent_get_uid(c);

        if (ORAGE_STR_EXISTS(text) && strcmp(text, ical_uid) == 0
                && valid_comp) { 
            /* we found our uid (=component) */
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
            appt.soundrepeat = FALSE;
            appt.soundrepeat_cnt = 500;
            appt.soundrepeat_len = 2;
            appt.note = NULL;
            appt.sound = NULL;
            appt.uid = NULL;
            appt.starttime[0] = '\0';
            appt.start_tz_loc = NULL;
            appt.endtime[0] = '\0';
            appt.end_tz_loc = NULL;
            appt.completed = FALSE;
            appt.completedtime[0] = '\0';
            appt.completed_tz_loc = NULL;
            appt.use_duration = FALSE;
            appt.duration = 0;
            appt.starttimecur[0] = '\0';
            appt.endtimecur[0] = '\0';
            appt.freq = XFICAL_FREQ_NONE;
            appt.interval = 1;
            appt.recur_limit = 0;
            appt.recur_count = 0;
            appt.recur_until[0] = '\0';
            for (i=0; i <= 6; i++) {
                appt.recur_byday[i] = TRUE;
                appt.recur_byday_cnt[i] = 0;
            }
        /*********** Properties ***********/
            for (p = icalcomponent_get_first_property(c, ICAL_ANY_PROPERTY);
                 p != 0;
                 p = icalcomponent_get_next_property(c, ICAL_ANY_PROPERTY)) {
                /* these are in icalderivedproperty.h */
                switch (icalproperty_isa(p)) {
                    case ICAL_SUMMARY_PROPERTY:
                        /* FIXME JK001 */
                        appt.title = (char*)icalproperty_get_value_as_string(p);
                        if (!appt.title)
                            g_print("xfical_appt_get_internal: summary is null. uid=%s\n", (ical_uid));
                        else
                            appt.title = (char *)icalproperty_get_summary(p);
                        break;
                    case ICAL_LOCATION_PROPERTY:
                        appt.location = (char *) icalproperty_get_value_as_string(p);
                        if (!appt.location)
                            g_print("xfical_appt_get_internal: location is null. uid=%s\n", (ical_uid));
                        /*
                        appt.location = (char *) icalproperty_get_location(p);
                        */
                        break;
                    case ICAL_DESCRIPTION_PROPERTY:
                        /* FIXME JK001 */
                        appt.note = (char *)icalproperty_get_value_as_string(p);
                        /*
                        g_print ("note string %s\n", appt.note);
                        */
                        if (!appt.note)
                            g_print("xfical_appt_get_internal: description is null. uid=%s\n", (ical_uid));
                        else
                            appt.note = (char *)icalproperty_get_description(p);
                        /*
                        g_print ("note description %s\n", appt.note);
                        */
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
                        if (!stime_found)
                            process_start_date(&appt, p 
                                , &itime, &stime, &sltime, &etime);
                        break;
                    case ICAL_DTEND_PROPERTY:
                    case ICAL_DUE_PROPERTY:
                        if (!etime_found)
                            process_end_date(&appt, p 
                                , &itime, &etime, &eltime);
                        break;
                    case ICAL_COMPLETED_PROPERTY:
                        process_completed_date(&appt, p);
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
                        if ((appt.recur_count = rrule.count))
                            appt.recur_limit = 1;
                        else if(! icaltime_is_null_time(rrule.until)) {
                            appt.recur_limit = 2;
                            text  = icaltime_as_ical_string(rrule.until);
                            g_strlcpy(appt.recur_until, text, 17);
                        }
                        if (rrule.by_day[0] != ICAL_RECURRENCE_ARRAY_MAX) {
                            for (i=0; i <= 6; i++)
                                appt.recur_byday[i] = FALSE;
                            for (i=0; i <= 6 && 
                                 rrule.by_day[i] != ICAL_RECURRENCE_ARRAY_MAX; 
                                 i++) {
                                cnt = (int)floor((double)(rrule.by_day[i]/8));
                                day = abs(rrule.by_day[i]-8*cnt);
                                switch (day) {
                                    case ICAL_MONDAY_WEEKDAY:
                                        appt.recur_byday[0] = TRUE;
                                        appt.recur_byday_cnt[0] = cnt;
                                        break;
                                    case ICAL_TUESDAY_WEEKDAY:
                                        appt.recur_byday[1] = TRUE;
                                        appt.recur_byday_cnt[1] = cnt;
                                        break;
                                    case ICAL_WEDNESDAY_WEEKDAY:
                                        appt.recur_byday[2] = TRUE;
                                        appt.recur_byday_cnt[2] = cnt;
                                        break;
                                    case ICAL_THURSDAY_WEEKDAY:
                                        appt.recur_byday[3] = TRUE;
                                        appt.recur_byday_cnt[3] = cnt;
                                        break;
                                    case ICAL_FRIDAY_WEEKDAY:
                                        appt.recur_byday[4] = TRUE;
                                        appt.recur_byday_cnt[4] = cnt;
                                        break;
                                    case ICAL_SATURDAY_WEEKDAY:
                                        appt.recur_byday[5] = TRUE;
                                        appt.recur_byday_cnt[5] = cnt;
                                        break;
                                    case ICAL_SUNDAY_WEEKDAY:
                                        appt.recur_byday[6] = TRUE;
                                        appt.recur_byday_cnt[6] = cnt;
                                        break;
                                    case ICAL_NO_WEEKDAY:
                                        break;
                                    default:
                                        g_warning(P_N "unknown weekday %s: %d/%d (%x)"
                                                , ical_uid, rrule.by_day[i]
                                                , i, rrule.by_day[i]);
                                        break;
                                }
                            }
                        }
                        appt.interval = rrule.interval;
                        break;
                    case ICAL_X_PROPERTY:
                        /*
    text = icalproperty_get_value_as_string(p);
    g_print("X PROPERTY: %s\n", text);
    */
                        text = icalproperty_get_x_name(p);
                        if (g_str_has_prefix(text, "X-ORAGE-ORIG-DTSTART")) {
                            process_start_date(&appt, p 
                                , &itime, &stime, &sltime, &etime);
                            stime_found = TRUE;
                            break;
                        }
                        if (g_str_has_prefix(text, "X-ORAGE-ORIG-DTEND")) {
                            process_end_date(&appt, p 
                                , &itime, &etime, &eltime);
                            etime_found = TRUE;
                            break;
                        }
                        g_warning(P_N "unknown X property %s"
                                , (char *)text);
                    case ICAL_CATEGORIES_PROPERTY:
                    case ICAL_CLASS_PROPERTY:
                    case ICAL_DTSTAMP_PROPERTY:
                    case ICAL_CREATED_PROPERTY:
                        break;
                    default:
                        orage_message(P_N "unknown property %s"
                                , (char *)icalproperty_get_property_name(p));
                        break;
                }
            }
        ical_appt_get_alarm_internal(c, &appt);
        }
    } 

    if (key_found) {
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
            if (appt.allDay && appt.duration) {
        /* need to subtract 1 day.
         * read explanation from appt_add_internal: appt->endtime processing */
                duration_tmp = icaldurationtype_from_int(60*60*24);
                appt.duration -= icaldurationtype_as_int(duration_tmp);
                duration = icaldurationtype_from_int(appt.duration);
                etime = icaltime_add(stime, duration);
                text  = icaltime_as_ical_string(etime);
                g_strlcpy(appt.endtime, text, 17);
            }
        }
        return(&appt);
    }
    else
        return(NULL);
}

static void xfical_appt_get_fill_internal(xfical_appt *appt, char *file_type)
{
    if (appt) {
        appt->uid = g_strconcat(file_type, appt->uid, NULL);
        appt->title = g_strdup(appt->title);
        appt->location = g_strdup(appt->location);
        if (appt->start_tz_loc)
            appt->start_tz_loc = g_strdup(appt->start_tz_loc);
        else
            appt->start_tz_loc = g_strdup("floating");
        if (appt->end_tz_loc)
            appt->end_tz_loc = g_strdup(appt->end_tz_loc);
        else
            appt->end_tz_loc = g_strdup("floating");
        if (appt->completed_tz_loc)
            appt->completed_tz_loc = g_strdup(appt->completed_tz_loc);
        else
            appt->completed_tz_loc = g_strdup("floating");
        appt->note = g_strdup(appt->note);
        appt->sound = g_strdup(appt->sound);
    }
}

static xfical_appt *appt_get_any(char *ical_uid, icalcomponent *base
        , char *file_type)
{
    xfical_appt *appt;

    if ((appt = g_memdup(xfical_appt_get_internal(ical_uid, base)
            , sizeof(xfical_appt)))) {
        xfical_appt_get_fill_internal(appt, file_type);
    }
    return(appt);
}
/*
static xfical_appt *xfical_appt_get_archive(char *ical_uid)
{
    xfical_appt *appt;

    if ((appt = g_memdup(xfical_appt_get_internal(ical_uid, aical)
            , sizeof(xfical_appt)))) {
        xfical_appt_get_fill_internal(appt);
    }
    return(appt);
}
*/
 /* Read EVENT from ical datafile.
  * ical_uid:  key of ical EVENT appt-> is to be read
  * returns: if failed: NULL
  *          if successfull: xfical_appt pointer to xfical_appt struct 
  *             filled with data.
  *             You must free it after not being used anymore
  *             using xfical_appt_free.
  *          NOTE: This routine does not fill starttimecur nor endtimecur,
  *                Those are always initialized to null string
  */
xfical_appt *xfical_appt_get(char *uid)
{
#undef P_N
#define P_N "xfical_appt_get: "
    xfical_appt *appt;
    char *ical_uid;
    char file_type[8];
    gint i;

    strncpy(file_type, uid, 4); /* file id */
    file_type[4] = '\0';
    ical_uid = uid+4; /* skip file id */
    if (uid[0] == 'O') {
        return(appt_get_any(ical_uid, ical, file_type));
    }
    else if (uid[0] == 'A') {
        return(appt_get_any(ical_uid, aical, file_type));
    }
    else if (uid[0] == 'F') {
        sscanf(uid, "F%02d", &i);
        if (i < g_par.foreign_count && f_ical[i].ical != NULL)
            return(appt_get_any(ical_uid, f_ical[i].ical, file_type));
        else {
            g_warning(P_N "unknown foreign file number %s", uid);
            return(NULL);
        }
    }
    else {
        g_warning(P_N "unknown file type %s", uid);
        return(NULL);
    }
    /*
    if ((appt = g_memdup(xfical_appt_get_internal(ical_uid, ical)
            , sizeof(xfical_appt)))) {
        xfical_appt_get_fill_internal(appt);
    }
    */
    return(appt);
}

void xfical_appt_free(xfical_appt *appt)
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
  * app: pointer to filled xfical_appt structure, which is stored
  *      Caller is responsible for filling and allocating and freeing it.
  * returns: TRUE is successfull, FALSE if failed
  */
gboolean xfical_appt_mod(char *ical_uid, xfical_appt *app)
{
#undef P_N
#define P_N "xfical_appt_mod: "
    icalcomponent *c, *base;
    char *uid, *int_uid;
    icalproperty *p = NULL;
    gboolean key_found = FALSE;
    struct icaltimetype create_time = icaltime_null_time();
    int i;

    if (ical_uid == NULL) {
        g_warning(P_N "Got NULL uid. doing nothing");
        return(FALSE);
    }
    int_uid = ical_uid+4;
    if (ical_uid[0] == 'O')
        base = ical;
    else if (ical_uid[0] == 'F') {
        sscanf(ical_uid, "F%02d", &i);
        if (i < g_par.foreign_count && f_ical[i].ical != NULL)
            base = f_ical[i].ical;
        else {
            g_warning(P_N "unknown foreign file number %s", ical_uid);
            return(FALSE);
        }
        if (g_par.foreign_data[i].read_only) {
            g_warning(P_N "foreign file %s is READ only. Not modified.", g_par.foreign_data[i].file);
            return(FALSE);
        }
    }
    else { /* note: we never update/add Archive file */ 
        g_warning(P_N "unknown file type %s", ical_uid);
        return(FALSE);
    }
    for (c = icalcomponent_get_first_component(base, ICAL_ANY_COMPONENT); 
         c != 0 && !key_found;
         c = icalcomponent_get_next_component(base, ICAL_ANY_COMPONENT)) {
        uid = (char *)icalcomponent_get_uid(c);
        if (strcmp(uid, int_uid) == 0) {
            if ((p = icalcomponent_get_first_property(c,
                            ICAL_CREATED_PROPERTY)))
                create_time = icalproperty_get_created(p);
            icalcomponent_remove_component(base, c);
            key_found = TRUE;
        }
    } 
    if (!key_found) {
        g_warning(P_N "uid %s not found. Doing nothing", ical_uid);
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
#undef P_N
#define P_N "xfical_appt_del: "
    icalcomponent *c, *base;
    icalset *fbase;
    char *uid, *int_uid;
    int i;

    if (ical_uid == NULL) {
        g_warning(P_N "Got NULL uid. doing nothing");
        return(FALSE);
    }
    int_uid = ical_uid+4;
    if (ical_uid[0] == 'O') {
        base = ical;
        fbase = fical;
    }
    else if (ical_uid[0] == 'F') {
        sscanf(ical_uid, "F%02d", &i);
        if (i < g_par.foreign_count && f_ical[i].ical != NULL) {
            base = f_ical[i].ical;
            fbase = f_ical[i].fical;
        }
        else {
            g_warning(P_N "unknown foreign file number %s", ical_uid);
            return(FALSE);
        }
        if (g_par.foreign_data[i].read_only) {
            g_warning(P_N "foreign file %s is READ only. Not modified.", g_par.foreign_data[i].file);
            return(FALSE);
        }
    }
    else { /* note: we never update/add Archive file */ 
        g_warning(P_N "unknown file type %s", ical_uid);
        return(FALSE);
    }
    if (ical_uid == NULL) {
        g_warning(P_N "Got NULL uid. doing nothing");
        return(FALSE);
     }
    for (c = icalcomponent_get_first_component(base, ICAL_ANY_COMPONENT); 
         c != 0;
         c = icalcomponent_get_next_component(base, ICAL_ANY_COMPONENT)) {
        uid = (char *)icalcomponent_get_uid(c);
        if (strcmp(uid, int_uid) == 0) {
            icalcomponent_remove_component(base, c);
            icalset_mark(fbase);
            xfical_alarm_build_list_internal(FALSE);
            return(TRUE);
        }
    } 
    g_warning(P_N "uid %s not found. Doing nothing", ical_uid);
    return(FALSE);
}

 /* Read next EVENT/TODO/JOURNAL component on the specified date from 
  * ical datafile.
  * a_day:  start date of ical component which is to be read
  * first:  get first appointment is TRUE, if not get next.
  * days:   how many more days to check forward. 0 = only one day
  * type:   EVENT/TODO/JOURNAL to be read
  * returns: NULL if failed and xfical_appt pointer to xfical_appt struct 
  *          filled with data if successfull. 
  *          This xfical_appt struct is owned by the routine. 
  *          Do not deallocate it.
  *          It will be overdriven by next invocation of this function.
  * Note:   starttimecur and endtimecur are converted to local timezone
  */
static xfical_appt *xfical_appt_get_next_on_day_internal(char *a_day
        , gboolean first, gint days, xfical_type type, icalcomponent *base
        , gchar *file_type)
{
#undef P_N
#define P_N "xfical_appt_get_next_on_day: "
    struct icaltimetype asdate, aedate    /* period to check */
            , nsdate, nedate;   /* repeating event occurrency start and end */
    xfical_period per; /* event start and end times with duration */
    icalcomponent *c=NULL;
    icalproperty *p = NULL;
    static icalcompiter ci;
    gboolean date_found=FALSE;
    gboolean date_rec_found=FALSE;
    char *uid;
    xfical_appt *appt;
    struct icalrecurrencetype rrule;
    icalrecur_iterator* ri;
    icalcomponent_kind ikind = ICAL_VEVENT_COMPONENT;

    /* setup period to test */
    asdate = icaltime_from_string(a_day);
    aedate = asdate;
    if (days)
        icaltime_adjust(&aedate, days, 0, 0, 0);

    if (first) {
        if (type == XFICAL_TYPE_EVENT)
            ikind = ICAL_VEVENT_COMPONENT;
        else if (type == XFICAL_TYPE_TODO)
            ikind = ICAL_VTODO_COMPONENT;
        else if (type == XFICAL_TYPE_JOURNAL)
            ikind = ICAL_VJOURNAL_COMPONENT;
        else
            g_warning(P_N "Unsupported Type");

        ci = icalcomponent_begin_component(base, ikind);
    }
    for ( ; 
         icalcompiter_deref(&ci) != 0 && !date_found;
         icalcompiter_next(&ci)) {
        /* next VTODO appointment loop. check if it is ok */
        c = icalcompiter_deref(&ci);
        per = get_period(c);
        /*
        g_print("alku %s, loppu %s, period alku %s, period loppu %s, period completed %s\n"
                , icaltime_as_ical_string(asdate)
                , icaltime_as_ical_string(aedate)
                , icaltime_as_ical_string(per.stime)
                , icaltime_as_ical_string(per.etime)
                , icaltime_as_ical_string(per.ctime));
                */
        if (type == XFICAL_TYPE_TODO) {
            if (icaltime_is_null_time(per.ctime))
            /* VTODO is never completed  */
                date_found = TRUE;
            else if (local_compare(per.ctime, per.stime) <= 0)
            /* this is VTODO and it has completed before start, so
             * this one is not done and needs to be counted */
                date_found = TRUE;
        }
        else { /* VEVENT or VJOURNAL */
            if (local_compare_date_only(per.stime, aedate) <= 0
                && local_compare_date_only(asdate, per.etime) <= 0) {
                    date_found = TRUE;
            }
        }
        if (!date_found && (p = icalcomponent_get_first_property(c
                , ICAL_RRULE_PROPERTY)) != 0) { /* check recurring */
            nsdate = icaltime_null_time();
            rrule = icalproperty_get_rrule(p);
            ri = icalrecur_iterator_new(rrule, per.stime);
            for (nsdate = icalrecur_iterator_next(ri),
                    nedate = icaltime_add(nsdate, per.duration);
                 !icaltime_is_null_time(nsdate)
                 && ((type == XFICAL_TYPE_TODO
                        && local_compare(nsdate, per.ctime) < 0)
                     || (type != XFICAL_TYPE_TODO
                         && local_compare_date_only(nedate, asdate) < 0));
                 nsdate = icalrecur_iterator_next(ri),
                    nedate = icaltime_add(nsdate, per.duration)) {
                /*
        g_print("loop %s, loppu %s, period alku %s, period loppu %s, period completed %s loop alku %s loop loppu %s\n"
                , icaltime_as_ical_string(asdate)
                , icaltime_as_ical_string(aedate)
                , icaltime_as_ical_string(per.stime)
                , icaltime_as_ical_string(per.etime)
                , icaltime_as_ical_string(per.ctime)
                , icaltime_as_ical_string(nsdate)
                , icaltime_as_ical_string(nedate));
            */
            }
            icalrecur_iterator_free(ri);
            if (type == XFICAL_TYPE_TODO) {
                if (!icaltime_is_null_time(nsdate)) {
                    date_found = TRUE;
                    date_rec_found = TRUE;
                }
            }
            else if (local_compare_date_only(nsdate, aedate) <= 0
                && local_compare_date_only(asdate, nedate) <= 0) {
                date_found = TRUE;
                date_rec_found = TRUE;
            }
        }
    }
    if (date_found) {
        uid = (char *)icalcomponent_get_uid(c);
        appt = appt_get_any(uid, base, file_type);
        if (date_rec_found) {
            g_strlcpy(appt->starttimecur, icaltime_as_ical_string(nsdate), 17);
            g_strlcpy(appt->endtimecur, icaltime_as_ical_string(nedate), 17);
        }
        else {
            g_strlcpy(appt->starttimecur, icaltime_as_ical_string(per.stime)
                    , 17);
            g_strlcpy(appt->endtimecur, icaltime_as_ical_string(per.etime)
                    , 17);
        }
        return(appt);
    }
    else
        return(0);
}

 /* Read next EVENT/TODO/JOURNAL component on the specified date from 
  * ical datafile.
  * a_day:  start date of ical component which is to be read
  * first:  get first appointment is TRUE, if not get next.
  * days:   how many more days to check forward. 0 = only one day
  * type:   EVENT/TODO/JOURNAL to be read
  * returns: NULL if failed and xfical_appt pointer to xfical_appt struct 
  *          filled with data if successfull. 
  *          This xfical_appt struct is owned by the routine. 
  *          Do not deallocate it.
  *          It will be overdriven by next invocation of this function.
  * Note:   starttimecur and endtimecur are converted to local timezone
  */
xfical_appt *xfical_appt_get_next_on_day(char *a_day, gboolean first, gint days
        , xfical_type type, gchar *file_type)
{
#undef P_N
#define P_N "xfical_appt_get_next_on_day: "
    gint i;

    if (file_type[0] == 'O') {
        return(xfical_appt_get_next_on_day_internal(a_day, first
                , days, type, ical, file_type));
    }
    else if (file_type[0] == 'A') {
        return(xfical_appt_get_next_on_day_internal(a_day, first
                , days, type, aical, file_type));
    }
    else if (file_type[0] == 'F') {
        sscanf(file_type, "F%02d", &i);
        if (i < g_par.foreign_count && f_ical[i].ical != NULL)
            return(xfical_appt_get_next_on_day_internal(a_day, first
                    , days, type, f_ical[i].ical, file_type));
         else {
             g_warning(P_N "unknown foreign file number %s", file_type);
             return(NULL);
         }
    }
    else {
        g_warning(P_N "unknown file type");
        return(NULL);
    }

}

 /* Mark days with appointments into calendar
  * year: Year to be searched
  * month: Month to be searched
  */
void xfical_mark_calendar_internal(GtkCalendar *gtkcal, icalcomponent *base
        , int year, int month)
{
    xfical_period per;
    struct icaltimetype nsdate, nedate;
    static icalcomponent *c;
    struct icalrecurrencetype rrule;
    icalrecur_iterator* ri;
    icalproperty *p = NULL;
    gint start_day, day_cnt, end_day;

    for (c = icalcomponent_get_first_component(base, ICAL_VEVENT_COMPONENT);
         c != 0;
         c = icalcomponent_get_next_component(base, ICAL_VEVENT_COMPONENT)) {
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
            {
                gtk_calendar_mark_day(gtkcal, day_cnt);
            }
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
                    {
                        gtk_calendar_mark_day(gtkcal, day_cnt);
                    }
                }
            }
            icalrecur_iterator_free(ri);
        } 
    } 
}

void xfical_mark_calendar(GtkCalendar *gtkcal, int year, int month)
{
    gint i;

    gtk_calendar_freeze(gtkcal);
    gtk_calendar_clear_marks(gtkcal);
    xfical_mark_calendar_internal(gtkcal, ical, year, month);
    for (i = 0; i < g_par.foreign_count; i++) {
        xfical_mark_calendar_internal(gtkcal, f_ical[i].ical, year, month);
    }
    gtk_calendar_thaw(gtkcal);
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

void xfical_icalcomponent_archive_recurrent(icalcomponent *e
        , struct tm *threshold, char *uid)
{
#undef P_N
#define P_N "xfical_icalcomponent_archive_recurrent: "
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
    char *text2;
    icalproperty *p, *pdtstart, *pdtend;
    gboolean upd_edate = FALSE;

    /* *** PHASE 1 *** : Add to the archive file */
    /* We must first check that this event has not yet been archived.
     * It is recurrent, so we may have added it earlier already.
     * If it has been added, we do not have to do anything since the
     * one in the archive file must be older than our current event.
     */
    /*
    for (a = icalcomponent_get_first_component(aical, ICAL_VEVENT_COMPONENT);
         a != 0 && !key_found;
         a = icalcomponent_get_next_component(aical, ICAL_VEVENT_COMPONENT)) {
        text = icalcomponent_get_uid(a);
        if (strcmp(text, uid) == 0) 
            key_found = TRUE;
    }
    if (!key_found) {
        d = icalcomponent_new_clone(e);
        icalcomponent_add_component(aical, d);
    }
    */

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
         l_icaltimezone = icaltimezone_get_builtin_timezone(stz_loc);
         if (!l_icaltimezone) {
            g_warning(P_N "builtin timezone %s not found, conversion failed.", stz_loc);
        }
        sdate = icaltime_convert_to_zone(sdate, l_icaltimezone);
    }

    edate = icalcomponent_get_dtend(e);
    if (icaltime_is_null_time(edate)) {
        edate = sdate;
    }
    pdtend = icalcomponent_get_first_property(e, ICAL_DTEND_PROPERTY);
    if (pdtend) { /* we have DTEND, so we need to adjust it. */
        itime_tz = icalproperty_get_first_parameter(pdtend
                , ICAL_TZID_PARAMETER);
        if (itime_tz) {
             etz_loc = (char *) icalparameter_get_tzid(itime_tz);
             l_icaltimezone = icaltimezone_get_builtin_timezone(etz_loc);
             if (!l_icaltimezone) {
                g_warning(P_N "builtin timezone %s not found, conversion failed.", etz_loc);
            }
            edate = icaltime_convert_to_zone(edate, l_icaltimezone);
        }
        duration = icaltime_subtract(edate, sdate);
        upd_edate = TRUE;
    }
    else { 
        p = icalcomponent_get_first_property(e, ICAL_DURATION_PROPERTY);
        if (p) /* we have DURATION, which does not need changes */
            duration = icalproperty_get_duration(p);
        else  /* neither duration, nor dtend, assume dtend=dtstart */
            duration = icaltime_subtract(edate, sdate);
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

    if (icaltime_is_null_time(nsdate))  /* remove since it has ended */
        xfical_icalcomponent_archive_normal(e);
    else { /* modify times*/
        text = g_strdup(icalproperty_as_ical_string(pdtstart));
        text2 = g_strjoin(NULL, "X-ORAGE-ORIG-", text, NULL);
        p = icalproperty_new_from_string(text2);
        g_free((gchar *)text2);
        g_free((gchar *)text);
        icalcomponent_add_property(e, p);
        icalcomponent_remove_property(e, pdtstart);
        if (stz_loc == NULL)
            icalcomponent_add_property(e, icalproperty_new_dtstart(nsdate));
        else
            icalcomponent_add_property(e
                    , icalproperty_vanew_dtstart(nsdate
                            , icalparameter_new_tzid(stz_loc)
                            , 0));
        if (upd_edate) {
            text = g_strdup(icalproperty_as_ical_string(pdtend));
            text2 = g_strjoin(NULL, "X-ORAGE-ORIG-", text, NULL);
            p = icalproperty_new_from_string(text2);
            g_free((gchar *)text2);
            g_free((gchar *)text);
            icalcomponent_add_property(e, p);
            icalcomponent_remove_property(e, pdtend);
            if (etz_loc == NULL)
                icalcomponent_add_property(e, icalproperty_new_dtend(nedate));
            else
                icalcomponent_add_property(e
                        , icalproperty_vanew_dtend(nedate
                                , icalparameter_new_tzid(etz_loc)
                                , 0));
        }
    }
}

gboolean xfical_archive(void)
{
#undef P_N
#define P_N "xfical_archive: "
    struct icaltimetype sdate, edate;
    static icalcomponent *c, *c2;
    icalproperty *p;
    struct tm *threshold;
    char *uid;

    if (g_par.archive_limit == 0) {
        orage_message(_("Archiving not enabled. Exiting"));
        return(TRUE);
    }
    if (!xfical_file_open(FALSE) || !xfical_archive_open()) {
        g_warning(P_N "file open error");
        return(FALSE);
    }
    threshold = orage_localtime();
    threshold->tm_mday = 1;
    threshold->tm_year += 1900;
    if (threshold->tm_mon > g_par.archive_limit) {
        threshold->tm_mon -= g_par.archive_limit;
    }
    else {
        threshold->tm_mon += (12 - g_par.archive_limit);
        threshold->tm_year--;
    }
    threshold->tm_mon += 1;

    orage_message(_("Archiving threshold: %d month(s)")
            , g_par.archive_limit);
    orage_message(_("\tArchiving events, which are older than: %04d-%02d-%02d")
            , threshold->tm_year, threshold->tm_mon, threshold->tm_mday);

    /* Check appointment file for items older than the threshold */
    /* Note: remove moves the "c" pointer to next item, so we need to store it
     *          first to process all of them or we end up skipping entries */
    for (c = icalcomponent_get_first_component(ical, ICAL_ANY_COMPONENT);
         c != 0;
         c = c2) {
        c2 = icalcomponent_get_next_component(ical, ICAL_ANY_COMPONENT);
        sdate = icalcomponent_get_dtstart(c);
        edate = icalcomponent_get_dtend(c);
        uid = (char *)icalcomponent_get_uid(c);
        if (icaltime_is_null_time(edate)) {
            edate = sdate;
        }
        /* Items with endate before threshold => archived.
         * Recurring events are marked in the main file by adding special
         * X-ORAGE_ORIG-DTSTART/X-ORAGE_ORIG-DTEND to save the original
         * start/end dates. Then start_date is changed. These are NOT
         * written in archive file (unless of course they really have
         * ended).
         */
        if ((edate.year*12 + edate.month) 
            < (threshold->tm_year*12 + threshold->tm_mon)) {
            p = icalcomponent_get_first_property(c, ICAL_RRULE_PROPERTY);
            orage_message(_("Archiving uid: %s (%s)")
                    , uid, (p) ? _("recur") : _("normal"));
            orage_message(_("\tEnd year: %04d, month: %02d, day: %02d")
                    , edate.year, edate.month, edate.day);
            if (p)  /*  it is recurrent event */
                xfical_icalcomponent_archive_recurrent(c, threshold, uid);
            else 
                xfical_icalcomponent_archive_normal(c);
        }
    }

    icalset_mark(afical);
    icalset_commit(afical);
    xfical_archive_close();
    icalset_mark(fical);
    icalset_commit(fical);
    xfical_file_close(FALSE);
    orage_message(_("Archiving done\n"));
    return(TRUE);
}

icalproperty *replace_repeating(icalcomponent *c, icalproperty *p
        , icalproperty_kind k)
{
    icalproperty *s, *n;
    const char *text;
    const gint x_len = strlen("X-ORAGE-ORIG-");

    text = g_strdup(icalproperty_as_ical_string(p));
    n = icalproperty_new_from_string(text + x_len);
    g_free((gchar *)text);
    s = icalcomponent_get_first_property(c, k);
    /* remove X-ORAGE-ORIG...*/
    icalcomponent_remove_property(c, p);
    /* remove old k (=either DTSTART or DTEND) */
    icalcomponent_remove_property(c, s);
    /* add new DTSTART or DTEND */
    icalcomponent_add_property(c, n);
    /* we need to start again from the first since we messed the order,
     * but there are not so many X- propoerties that this is worth worring */
    return(icalcomponent_get_first_property(c, ICAL_X_PROPERTY));
}

gboolean xfical_unarchive(void)
{
#undef P_N
#define P_N "xfical_unarchive: "
    icalcomponent *c, *d;
    icalproperty *p;
    const char *text;

    /* PHASE 1: go through base orage file and remove "repeat" shortcuts */
    orage_message(_("Starting archive removal: PHASE 1: reset recurring appointments"));
    if (!xfical_file_open(FALSE)) {
        g_warning(P_N "file open error");
        return(FALSE);
    }

    for (c = icalcomponent_get_first_component(ical, ICAL_VEVENT_COMPONENT);
         c != 0;
         c = icalcomponent_get_next_component(ical, ICAL_VEVENT_COMPONENT)) {
         p = icalcomponent_get_first_property(c, ICAL_X_PROPERTY);
         while (p) {
            text = icalproperty_get_x_name(p);
            if (g_str_has_prefix(text, "X-ORAGE-ORIG-DTSTART"))
                p = replace_repeating(c, p, ICAL_DTSTART_PROPERTY);
            else if (g_str_has_prefix(text, "X-ORAGE-ORIG-DTEND"))
                p = replace_repeating(c, p, ICAL_DTEND_PROPERTY);
            else /* it was not our X-PROPERTY */
                p = icalcomponent_get_next_property(c, ICAL_X_PROPERTY);
        }
    }
    /* PHASE 2: go through archive file and add everything back to base orage.
     * After that delete the whole arch file */
    if (!xfical_archive_open()) {
        g_warning(P_N "archive file open error");
        icalset_mark(fical);
        icalset_commit(fical);
        xfical_file_close(FALSE);
        return(FALSE);
    }
    for (c = icalcomponent_get_first_component(aical, ICAL_VEVENT_COMPONENT);
         c != 0;
         c = icalcomponent_get_next_component(aical, ICAL_VEVENT_COMPONENT)) {
    /* Add to the base file */
        d = icalcomponent_new_clone(c);
        icalcomponent_add_component(ical, d);
    }
    xfical_archive_close();
    if (g_remove(g_par.archive_file) == -1) {
        g_warning(P_N "Failed to remove archive file %s"
                , g_par.archive_file);
    }
    icalset_mark(fical);
    icalset_commit(fical);
    xfical_file_close(FALSE);
    orage_message(_("Archive removal done\n"));
    return(TRUE);
}

gboolean xfical_unarchive_uid(char *uid)
{
#undef P_N
#define P_N "xfical_unarchive_uid: "
    icalcomponent *c, *d;
    gboolean key_found = FALSE;
    const char *text;
    char *ical_uid;

    ical_uid = uid+4; /* skip file id (which is A00. now)*/
    if (!xfical_file_open(FALSE) || !xfical_archive_open()) {
        g_warning(P_N "file open error");
        return(FALSE);
    } 
    for (c = icalcomponent_get_first_component(aical, ICAL_ANY_COMPONENT);
         c != 0 && !key_found;
         c = icalcomponent_get_next_component(aical, ICAL_ANY_COMPONENT)) {
        text = icalcomponent_get_uid(c);
        if (strcmp(text, ical_uid) == 0) { /* we found our uid (=event) */
            /* Add to the base file */
            d = icalcomponent_new_clone(c);
            icalcomponent_add_component(ical, d);
            /* Remove from the archive file */
            icalcomponent_remove_component(aical, c);
            key_found = TRUE;
        }
    }
    icalset_mark(fical);
    icalset_commit(fical);
    xfical_file_close(FALSE);
    icalset_mark(afical);
    icalset_commit(afical);
    xfical_archive_close();

    return(TRUE);
}

static gboolean add_event(icalcomponent *c)
{
#undef P_N
#define P_N "add_event: "
    icalcomponent *ca = NULL;
    icalproperty *p = NULL;
    int prop_cnt = 0;
    icalparameter *par = NULL;
    int par_cnt = 0;
    char *uid;

    ca = icalcomponent_new_clone(c);
    if ((uid = (char *)icalcomponent_get_uid(ca)) == NULL) {
        uid = generate_uid();
        icalcomponent_add_property(ca,  icalproperty_new_uid(uid));
        orage_message("Generated UID %s\n", uid);

    }
    if (!xfical_file_open(FALSE)) {
        g_warning(P_N "ical file open failed");
        return(FALSE);
    }
    icalcomponent_add_component(ical, ca);
    /*********** Properties ***********/
    /*
    for (p = icalcomponent_get_first_property(c, ICAL_ANY_PROPERTY);
         p != 0;
         p = icalcomponent_get_next_property(c, ICAL_ANY_PROPERTY)) {
         */
        /* these are in icalderivedproperty.h */
    /*
        g_print("\t***add_event: property %s\n", (char *)icalproperty_get_property_name(p));
        g_print("\t***add_event: property value%s\n", (char *)icalproperty_get_value_as_string(p));
        g_print("\t***add_event: continue...\n"); 
        */
        /*********** Parameters ***********/
        /*
        for (par = icalproperty_get_first_parameter(p, ICAL_ANY_PARAMETER);
             par != 0;
             par = icalproperty_get_next_parameter(p, ICAL_ANY_PARAMETER)) {
            g_print("\t\tadd_event: parameter %s\n", icalparameter_as_ical_string(par));
             }
    }
             */

    icalset_mark(fical);
    icalset_commit(fical);
    xfical_file_close(FALSE);
    return(TRUE);
}

static gboolean pre_format(char *file_name_in, char *file_name_out)
{
#undef P_N
#define P_N "pre_format: "
    gchar *text, *tmp, *tmp2, *tmp3;
    gsize text_len, cnt;
    GError *error = NULL;
    /* pre process the file to rule out some features, which orage does not
     * support so that we can do better conversion 
     */

    orage_message("Starting import file preprocessing");
    if (!g_file_get_contents(file_name_in, &text, &text_len, &error)) {
        g_warning(P_N "Could not open ical file (%s) error:%s"
                , file_name_in, error->message);
        g_error_free(error);
        return(FALSE);
    }
    /*
    g_print("read %d bytes\n", text_len);
    */
    /* Then convert to utf8 if needed */
    /*
    g_print("checking utf8 status\n");
    */
    if (!g_utf8_validate(text, -1, NULL)) {
        g_warning(P_N "is not in utf8 format. Conversion needed.\n (Use iconv and convert it into UTF-8 and import it again.)\n");
        return(FALSE);
        /* we don't know the real characterset, so we can't convert
        tmp = g_locale_to_utf8(text, -1, NULL, &cnt, NULL);
    g_print("written %d bytes\n", cnt);
        if (tmp) {
            g_strlcpy(text, tmp, text_len);
            g_free(tmp);
        }*/
    }
    /*
    g_print("utf8 safe now\n");
    */

    /* 1: remove ;CHARSET=...: parameter */
    for (tmp = g_strrstr(text, ";CHARSET="); 
         tmp != NULL;
         tmp = g_strrstr(text, ";CHARSET="))  {
        for (tmp2 = tmp; *tmp2 != ':'; tmp2++) {
            *tmp2 = ' ';
            if (tmp2 >= text + text_len) {
                g_warning(P_N "failed. CHARSET missing :");
                return(FALSE);
            }
        }
        for (tmp2 = tmp; *tmp2 != '\n'; tmp2--) {
            if (tmp2 <= text) {
                g_warning(P_N "failed. CHARSET missing property");
                return(FALSE);
            }
        }
        tmp3 = g_strndup(tmp2+1, tmp-tmp2);
        orage_message("... Removed CHARSET from %s", tmp3);
        g_free(tmp3);
    }
    /* 2: change DCREATED to CREATED */
    for (tmp = g_strrstr(text, "DCREATED:"); 
         tmp != NULL;
         tmp = g_strrstr(text, "DCREATED:"))  {
        tmp2 = tmp+strlen("DCREATED:yyyymmddThhmmss");
        if (*tmp2 == 'Z') {
            /* it is already in UTC, so just fix the parameter name */
            *tmp = ' ';
        }
        else {
            /* needs to be converted to UTC also */
            for (tmp3 = tmp; tmp3 < tmp2; tmp3++) {
                *tmp3 = *(tmp3+1); 
            }
            *(tmp3-1) = 'Z'; /* this is 'bad'...but who cares...it is fast */
        }
        tmp3 = g_strndup(tmp, tmp2-tmp);
        orage_message("... Patched DCREATED to be CREATED (%s)", tmp3);
        g_free(tmp3);
    }
    /* write file */
    if (!g_file_set_contents(file_name_out, text, -1, NULL)) {
        g_warning(P_N "Could not write ical file (%s)", file_name_out);
        return(FALSE);
    }
    g_free(text);
    orage_message("Import file preprocessing done");
    return(TRUE);
}

gboolean xfical_import_file(char *file_name)
{
#undef P_N
#define P_N "xfical_import_file: "
    static icalset *file_ical = NULL;
    char *ical_file_name = NULL;
    icalcomponent *c1, *c2;
    int cnt1 = 0, cnt2 = 0;

    ical_file_name = g_strdup_printf("%s.orage", file_name);
    if (!pre_format(file_name, ical_file_name)) {
        return(FALSE);
    }
    if ((file_ical = icalset_new_file(ical_file_name)) == NULL) {
        g_warning(P_N "Could not open ical file (%s) %s\n"
                , ical_file_name, icalerror_strerror(icalerrno));
        return(FALSE);
    }
    for (c1 = icalset_get_first_component(file_ical);
         c1 != 0;
         c1 = icalset_get_next_component(file_ical)) {
        if (icalcomponent_isa(c1) == ICAL_VCALENDAR_COMPONENT) {
            cnt1++;
            for (c2=icalcomponent_get_first_component(c1, ICAL_ANY_COMPONENT);
                 c2 != 0;
                 c2=icalcomponent_get_next_component(c1, ICAL_ANY_COMPONENT)){
                if (icalcomponent_isa(c2) == ICAL_VEVENT_COMPONENT) {
                    cnt2++;
                    add_event(c2);
                }
                else
                    g_warning(P_N "unknown component %s %s"
                            , icalcomponent_kind_to_string(
                                    icalcomponent_isa(c2))
                            , ical_file_name);
            }

        }
        else
            g_warning(P_N "unknown icalset component %s in %s"
                    , icalcomponent_kind_to_string(icalcomponent_isa(c1))
                    , ical_file_name);
    }
    if (cnt1 == 0) {
        g_warning(P_N "No valid icalset components found");
        return(FALSE);
    }
    if (cnt2 == 0) {
        g_warning(P_N "No valid ical components found");
        return(FALSE);
    }

    g_free(ical_file_name);
    return(TRUE);
}

static gboolean export_prepare_write_file(char *file_name)
{
#undef P_N
#define P_N "export_prepare_write_file: "
    gchar *tmp;

    if (strcmp(file_name, g_par.orage_file) == 0) {
        g_warning(P_N "You do not want to overwrite Orage ical file! (%s)"
                , file_name);
        return(FALSE);
    }
    tmp = g_path_get_dirname(file_name);
    if (g_mkdir_with_parents(tmp, 0755)) { /* octal */
        g_warning(P_N "Could not create directories (%s)"
                , file_name);
        return(FALSE);
    }
    g_free(tmp);
    if (g_remove(file_name) == -1) { /* note that it may not exist */
        g_warning(P_N "Failed to remove export file %s"
                , file_name);
    }
    return(TRUE);
}

static gboolean export_all(char *file_name)
{
#undef P_N
#define P_N "export_all: "
    gchar *text;
    gsize text_len;

    if (!export_prepare_write_file(file_name))
        return(FALSE);
    /* read Orage's ical file */
    if (!g_file_get_contents(g_par.orage_file, &text, &text_len, NULL)) {
        g_warning(P_N "Could not open Orage ical file (%s)"
                , g_par.orage_file);
        return(FALSE);
    }
    if (!g_file_set_contents(file_name, text, -1, NULL)) {
        g_warning(P_N "Could not write file (%s)"
                , file_name);
        g_free(text);
        return(FALSE);
    }
    g_free(text);
    return(TRUE);
}

static void export_selected_uid(icalcomponent *base, gchar *uid_int
        , icalcomponent *x_ical)
{
#undef P_N
#define P_N "export_selected_uid: "
    icalcomponent *c, *d;
    gboolean key_found = FALSE;
    gchar *uid_ical;

    key_found = FALSE;
    for (c = icalcomponent_get_first_component(base, ICAL_ANY_COMPONENT);
         c != 0 && !key_found;
         c = icalcomponent_get_next_component(base, ICAL_ANY_COMPONENT)) {
        uid_ical = (char *)icalcomponent_get_uid(c);
        if (strcmp(uid_int, uid_ical) == 0) { /* we found our uid */
            d = icalcomponent_new_clone(c);
            icalcomponent_add_component(x_ical, d);
            key_found = TRUE;
        }
    }
    if (!key_found)
        g_warning(P_N "not found %s from Orage", uid_int);
}

static gboolean export_selected(char *file_name, char *uids)
{
#undef P_N
#define P_N "export_selected: "
    icalcomponent *x_ical = NULL;
    icalset *x_fical = NULL;
    gchar *uid_ical, *uid, *uid_end, *uid_int;
    gboolean more_uids;
    int i;

    if (!export_prepare_write_file(file_name))
        return(FALSE);
    if (!ORAGE_STR_EXISTS(uids)) {
        g_warning(P_N "UID list is empty");
        return(FALSE);
    }
    if (!xfical_internal_file_open(&x_ical, &x_fical, file_name, FALSE)) {
        g_warning(P_N "Failed to create export file %s"
                , file_name);
        return(FALSE);
    }
    if (!xfical_file_open(TRUE)) {
        return(FALSE);
    }

    /* checks done, let's start the real work */
    more_uids = TRUE;
    for (uid = uids; more_uids; ) {
        uid_int = uid+4;
        uid_end = g_strstr_len((const gchar *)uid, strlen(uid), ",");
        if (uid_end != NULL)
            *uid_end = 0; /* uid ends here */
        /* FIXME: proper messages to screen */
        if (uid[0] == 'O') {
            export_selected_uid(ical, uid_int, x_ical);
        }
        else if (uid[0] == 'F') {
            sscanf(uid, "F%02d", &i);
            if (i < g_par.foreign_count && f_ical[i].ical != NULL) {
                export_selected_uid(f_ical[i].ical, uid_int, x_ical);
            }
            else {
                g_warning(P_N "unknown foreign file number %s", uid);
                return(FALSE);
            }

        }
        else {
            g_warning(P_N "Unknown uid type %s\n", uid);
        }
        
        if (uid_end != NULL)  /* we have more uids */
            uid = uid_end+1;  /* next uid starts here */
        else
            more_uids = FALSE;
    }

    xfical_file_close(TRUE);
    icalset_mark(x_fical);
    icalset_commit(x_fical);
    icalset_free(x_fical);
    return(TRUE);
}

gboolean xfical_export_file(char *file_name, int type, char *uids)
{
#undef P_N
#define P_N "xfical_export_file: "
    if (type == 0) { /* copy the whole file */
        return(export_all(file_name));
    }
    else if (type == 1) { /* copy only selected appointments */
        return(export_selected(file_name, uids));
    }
    else {
        g_warning(P_N "Unknown app count");
        return(FALSE);
    }
}

/* let's find next VEVENT or VTODO or VJOURNAL BEGIN or END
 * We rely that str is either BEGIN: or END: to show if we search the
 * beginning or the end */
gchar *find_next(gchar *cur, gchar *end, gchar *str)
{
    gchar *next;
    gboolean found_valid = FALSE;

    next = cur;
    do {
        next = g_strstr_len(next, end-next, str);
        if (next) {
         /* we found it. Need to check that it is valid.
          * First skip BEGIN: or END: and point to the component type */
            next += strlen(str);
            if (g_str_has_prefix(next, "VEVENT\n")
            ||  g_str_has_prefix(next, "VTODO\n")
            ||  g_str_has_prefix(next, "VJOURNAL\n"))
                found_valid = TRUE;
        }
    } while (next && !found_valid);
    return(next);
}

/* find the last str before cur. start from beg.
 * Idea is to find str, which is actually always BEGIN:
 * and then check that it is the beginning of VEVENT/VTODO/VJOURNAL
 * if it is not, repeat search, but move cur up to found string by
 * making len (prev-beg) smaller
 */
gchar *find_prev(gchar *beg, gchar *cur, gchar *str)
{
    gchar *prev;
    gboolean found_valid = FALSE;

    prev = cur;
    do {
         prev = g_strrstr_len(beg, prev-beg, str);
         if (prev) {
         /* we found it. Need to check that it is valid.
          * First skip BEGIN: or END: and point to the component type */
            prev += strlen(str);
            if (g_str_has_prefix(prev, "VEVENT\n")
            ||  g_str_has_prefix(prev, "VTODO\n")
            ||  g_str_has_prefix(prev, "VJOURNAL\n"))
                found_valid = TRUE;
            else
                prev -= strlen(str);
         }
    } while (prev && !found_valid);
    return(prev);
}

 /* Read next EVENT/TODO/JOURNAL which contains the specified string 
  * from ical datafile.
  * str:     string to search
  * first:   get first appointment is TRUE, if not get next.
  * returns: NULL if failed.
  *          xfical_appt pointer to xfical_appt struct filled with data if 
  *          successfull. 
  *          You must deallocate the appt after the call
  */
xfical_appt *xfical_appt_get_next_with_string_internal(char *str
        , gboolean first, char *search_file, icalcomponent *base
        , gchar *file_type)
{
#undef P_N
#define P_N "xfical_appt_get_next_with_string_internal: "
    static gchar *text, *beg, *end;
    gchar *cur, *tmp, mem;
    gsize text_len;
    char *uid, ical_uid[XFICAL_UID_LEN+1];
    xfical_appt *appt;
    gboolean found_valid, search_done = FALSE;
    struct icaltimetype it;
    const char *stime;

    if (!ORAGE_STR_EXISTS(str))
        return(NULL);
    if (first) {
        /* FIXME: it would be good idea to convert to upper case.
         * then we should not need to care about case and search would
         * be more user friendly
         */
        if (!g_file_get_contents(search_file, &text, &text_len, NULL)) {
            g_warning(P_N "Could not open Orage ical file (%s)", search_file);
            return(NULL);
        }
        /* let's find first BEGIN:VEVENT or BEGIN:VTODO or BEGIN:VJOURNAL
         * to start our search */
        end = text+text_len;
        beg = find_next(text, end, "\nBEGIN:");
        if (!beg) {
            g_warning(P_N "Could not find initial BEGIN:V-type from Orage ical file (%s)"
                    , search_file);
            return(NULL);
        }
        beg -= strlen("\nBEGIN:"); /* we need to be able to find first, too */
        first = FALSE;
        cur = beg;
    }

    if (!first) {
        for (cur = g_strstr_len(beg, end-beg, str), found_valid = FALSE; 
             cur && !found_valid; ) {
            /* We have found a match, let's check that it is valid. 
             * We only accept SUMMARY, DESCRIPTION and LOCATION ical strings
             * to be valid. */
            /* First we need to find the beginning of our row */
            for (tmp = cur; tmp > beg && *tmp != '\n'; tmp--)
                ;
            tmp++; /* skip the '\n' */
            /* Then let's check that it is valid. Mark the end temporarily. */
            mem = *cur;
            *cur = '\0';
            /* FIXME: vcs files may contain charset like this:
             * SUMMARY;CHARSET=UTF-8:Text of summary
             * so matching to : is not ok 
             */
            if (g_str_has_prefix(tmp, "SUMMARY")
            ||  g_str_has_prefix(tmp, "DESCRIPTION")
            ||  g_str_has_prefix(tmp, "LOCATION")) 
                found_valid = TRUE;
            *cur = mem;
            if (!found_valid)
                 cur = g_strstr_len(++cur, end-cur, str);
        }
        if (!cur) {
            search_done = TRUE;
        }
        else {
            /* first find ^BEGIN:<type> which shows the start 
             * of the component.
             * then find UID: and get the appointment and finally locate the
             * ^END:<type> and setup search for he next time 
             */
            beg = find_prev(beg, cur, "\nBEGIN:");
            if (!beg) {
                g_warning(P_N "BEGIN:V-type not found %s", str);
                search_done = TRUE;
            }
            else {
                uid = g_strstr_len(beg, end-beg, "UID:");
                if (!uid) {
                    g_warning(P_N "UID not found %s", str);
                    search_done = TRUE;
                }
                else {
                    sscanf(uid, "UID:%sXFICAL_UID_LEN", ical_uid);
                    if (strlen(ical_uid) > XFICAL_UID_LEN-2) {
                        g_warning(P_N "too long UID %s", ical_uid);
                        return(NULL);
                    }
                    appt = appt_get_any(ical_uid, base, file_type);
                    if (!appt) {
                        g_warning(P_N "UID not found in ical file %s", ical_uid);
                        search_done = TRUE;
                    }
                    else {
                        if (strcmp(g_par.local_timezone, "floating") == 0) {
                            g_strlcpy(appt->starttimecur, appt->starttime, 17);
                            g_strlcpy(appt->endtimecur, appt->endtime, 17);
                        }
                        else {
                            it = icaltime_from_string(appt->starttime);
                            it = convert_to_zone(it, appt->start_tz_loc);
                            it = icaltime_convert_to_zone(it
                                    , local_icaltimezone);
                            stime = icaltime_as_ical_string(it);
                            g_strlcpy(appt->starttimecur, stime, 17);
                            it = icaltime_from_string(appt->endtime);
                            it = convert_to_zone(it, appt->end_tz_loc);
                            it = icaltime_convert_to_zone(it
                                    , local_icaltimezone);
                            stime = icaltime_as_ical_string(it);
                            g_strlcpy(appt->endtimecur, stime, 17);
                        }
                        beg = find_next(uid, end, "\nEND:");
                        if (!beg) {
                            g_warning(P_N "END:V-type not found %s", str);
                            search_done = TRUE;
                        }
                        else {
                            return(appt);
                        }
                    }
                }
            }
        }
    }

    g_free(text);
    if (!search_done) {
        g_warning(P_N "illegal ending %s", str);
    }
    return(NULL);
}

 /* Read next EVENT/TODO/JOURNAL which contains the specified string 
  * from ical datafile.
  * str:     string to search
  * first:   get first appointment is TRUE, if not get next.
  * returns: NULL if failed.
  *          xfical_appt pointer to xfical_appt struct filled with data if 
  *          successfull. 
  *          You must deallocate the appt after the call
  */
xfical_appt *xfical_appt_get_next_with_string(char *str, gboolean first
        , gchar *file_type)
{
#undef P_N
#define P_N "xfical_appt_get_next_with_string: "
    gint i;

    if (file_type[0] == 'O') {
        return(xfical_appt_get_next_with_string_internal(str, first
                , g_par.orage_file , ical, file_type));
    }
    else if (file_type[0] == 'A') {
        return(xfical_appt_get_next_with_string_internal(str, first
                , g_par.archive_file, aical, file_type));
    }
    else if (file_type[0] == 'F') {
        sscanf(file_type, "F%02d", &i);
        if (i < g_par.foreign_count && f_ical[i].ical != NULL)
            return(xfical_appt_get_next_with_string_internal(str, first
                    , g_par.foreign_data[i].file, f_ical[i].ical, file_type));
        else {
            g_warning(P_N "unknown foreign file number %s", file_type);
            return(NULL);
        }
    }
    else {
        g_warning(P_N "unknown file type");
        return(NULL);
    }
}

gboolean xfical_timezone_button_clicked(GtkButton *button, GtkWindow *parent
        , gchar **tz)
{
#undef P_N
#define P_N "xfical_timezone_button_clicked: "
#define MAX_AREA_LENGTH 20

enum {
    LOCATION,
    LOCATION_ENG,
    N_COLUMNS
};

    GtkTreeStore *store;
    GtkTreeIter iter1, iter2;
    GtkWidget *tree;
    GtkCellRenderer *rend;
    GtkTreeViewColumn *col;
    GtkWidget *window;
    GtkWidget *sw;
    xfical_timezone_array tz_a;
    int i, j, result;
    char area_old[MAX_AREA_LENGTH], *loc, *loc_eng;
    GtkTreeSelection *sel;
    GtkTreeModel     *model;
    GtkTreeIter       iter;
    gboolean    changed = FALSE;

    /* enter data */
    store = gtk_tree_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);
    strcpy(area_old, "S T a R T");
    tz_a = xfical_get_timezones();
    for (i=0; i < tz_a.count-2; i++) {
        /* first area */
        if (! g_str_has_prefix(tz_a.city[i], area_old)) {
            for (j=0; tz_a.city[i][j] != '/' && j < MAX_AREA_LENGTH; j++) {
                area_old[j] = tz_a.city[i][j];
            }
            if (j < MAX_AREA_LENGTH)
                area_old[j] = 0;
            else
                g_warning(P_N "too long line in zones.tab %s", tz_a.city[i]);

            gtk_tree_store_append(store, &iter1, NULL);
            gtk_tree_store_set(store, &iter1
                    , LOCATION, _(area_old)
                    , LOCATION_ENG, area_old
                    , -1);
        }
        /* then city translated and in base form used internally */
        gtk_tree_store_append(store, &iter2, &iter1);
        gtk_tree_store_set(store, &iter2
                , LOCATION, _(tz_a.city[i])
                , LOCATION_ENG, tz_a.city[i]
                , -1);
    }

    /* create view */
    tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    rend = gtk_cell_renderer_text_new();
    col  = gtk_tree_view_column_new_with_attributes(_("Location")
                , rend, "text", LOCATION, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);

    rend = gtk_cell_renderer_text_new();
    col  = gtk_tree_view_column_new_with_attributes(_("Location")
                , rend, "text", LOCATION_ENG, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
    gtk_tree_view_column_set_visible(col, FALSE);

    /* show it */
    window =  gtk_dialog_new_with_buttons(_("Pick timezone")
            , parent
            , GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT
            , _("UTC"), 1
            , _("floating"), 2
            , GTK_STOCK_OK, GTK_RESPONSE_ACCEPT
            , NULL);
    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(sw), tree);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), sw, TRUE, TRUE, 0);
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 500);

    gtk_widget_show_all(window);
    do {
        result = gtk_dialog_run(GTK_DIALOG(window));
        switch (result) {
            case GTK_RESPONSE_ACCEPT:
                sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
                if (gtk_tree_selection_get_selected(sel, &model, &iter))
                    if (gtk_tree_model_iter_has_child(model, &iter))
                        result = 0;
                    else {
                        gtk_tree_model_get(model, &iter, LOCATION, &loc, -1);
                        gtk_tree_model_get(model, &iter, LOCATION_ENG, &loc_eng
                                , -1);                     }
                else {
                    loc = g_strdup(_(*tz));
                    loc_eng = g_strdup(*tz);
                }
                break;
            case 1:
                loc = g_strdup(_("UTC"));
                loc_eng = g_strdup("UTC");
                break;
            case 2:
                loc = g_strdup(_("floating"));
                loc_eng = g_strdup("floating");
                break;
            default:
                loc = g_strdup(_(*tz));
                loc_eng = g_strdup(*tz);
                break;
        }
    } while (result == 0);
    if (g_ascii_strcasecmp(loc, (gchar *)gtk_button_get_label(button)) != 0)
        changed = TRUE;
    gtk_button_set_label(button, loc);

    if (*tz)
        g_free(*tz);
    *tz = g_strdup(loc_eng);
    g_free(loc);
    g_free(loc_eng);
    gtk_widget_destroy(window);
    return(changed);
}
