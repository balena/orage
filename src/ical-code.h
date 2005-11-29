/* ical-code.h
 *
 * Copyright (C) 2005 Juha Kautto <juha@xfce.org>
 *                    Mickaël Graf <korbinus@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

typedef struct
{
    int    count;      /* how many timezones we have */
    char **city;      /* pointer to timezone location name strings */
} xfical_timezone_array;

xfical_timezone_array xfical_get_timezones();

void set_default_ical_path (void);

void set_ical_path (gchar *path);

void set_aical_path (gchar *path);

void set_lookback (int i);

gboolean xfical_file_open(void);

void xfical_file_close(void);

appt_data *xfical_app_alloc();

char *xfical_app_add(appt_data *app);

appt_data *xfical_app_get(char *ical_id);

gboolean xfical_app_mod(char *ical_id, appt_data *app);

gboolean xfical_app_del(char *ical_id);

struct icaltimetype ical_get_current_local_time();

appt_data * xfical_app_get_next_on_day(char *a_day, gboolean first, gint days);

void xfical_mark_calendar(GtkCalendar *gtkcal, int year, int month);

void rmday_ical_app(char *a_day);

void xfical_alarm_build_list(gboolean first_list_today);

gboolean xfical_alarm_passed(char *alarm_stime);

gboolean xfical_keep_tidy(void);
