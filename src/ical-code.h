/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2005-2007 Juha Kautto  (juha at xfce.org)
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

#ifndef __ICAL_CODE_H__
#define __ICAL_CODE_H__

#include "appointment.h"

#define ORAGE_UID_LEN 200
typedef struct
{
    int    count;      /* how many timezones we have */
    char **city;      /* pointer to timezone location name strings */
} xfical_timezone_array;

xfical_timezone_array xfical_get_timezones();

gboolean xfical_set_local_timezone();

gboolean xfical_file_open(void);
void xfical_file_close(void);

appt_data *xfical_appt_alloc();
char *xfical_appt_add(appt_data *app);
appt_data *xfical_appt_get(char *ical_id);
void xfical_appt_free(appt_data *appt);
gboolean xfical_appt_mod(char *ical_id, appt_data *app);
gboolean xfical_appt_del(char *ical_id);
appt_data *xfical_appt_get_next_on_day(char *a_day, gboolean first, gint days
        , xfical_type type, gboolean arch);
appt_data *xfical_appt_get_next_with_string(char *a_day, gboolean first
        , gboolean arch);

void xfical_mark_calendar(GtkCalendar *gtkcal, int year, int month);

void xfical_alarm_build_list(gboolean first_list_today);
gboolean xfical_alarm_passed(char *alarm_stime);

gboolean xfical_duration(char *alarm_stime, int *days, int *hours, int *mins);
int xfical_compare_times(appt_data *appt);
gboolean xfical_archive(void);
gboolean xfical_unarchive(void);
gboolean xfical_unarchive_uid(char *uid);

gboolean xfical_import_file(char *file_name);
gboolean xfical_export_file(char *file_name, int type, char *uids);

#endif /* !__ICAL_CODE_H__ */
