/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2005-2011 Juha Kautto  (juha at xfce.org)
 * Copyright (c) 2004-2005 Mickael Graf (korbinus at xfce.org)
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

#ifndef __ICAL_INTERNAL_H__
#define __ICAL_INTERNAL_H__

typedef struct
{
    struct icaltimetype stime; /* start time */
    struct icaltimetype etime; /* end time */
    struct icaldurationtype duration;
    struct icaltimetype ctime; /* completed time for VTODO appointmnets */
    icalcomponent_kind ikind;  /* type of component, VEVENt, VTODO... */
} xfical_period;

gboolean ic_internal_file_open(icalcomponent **p_ical
        , icalset **p_fical, gchar *file_icalpath, gboolean read_only
        , gboolean test);
char *ic_get_char_timezone(icalproperty *p);
xfical_period ic_get_period(icalcomponent *c, gboolean local);
char *ic_generate_uid(void);
struct icaltimetype ic_convert_to_timezone(struct icaltimetype t
        , icalproperty *p);

#endif /* !__ICAL_INTERNAL_H__ */
