/* ical-code.h
 *
 * Copyright (C) 2005 Juha Kautto <juha@xfce.org>
 *                    Mickaël Graf <korbinus@lunar-linux.org>
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

gboolean open_ical_file(void);

void close_ical_file(void);

appt_type *xf_alloc_ical_app();

char *xf_add_ical_app(appt_type *app);

appt_type *xf_get_ical_app(char *ical_id);

gboolean xf_del_ical_app(char *ical_id);

void add_ical_app(appt_type *app);

gboolean get_ical_app(appt_type *app, char *a_day, char *hh_mm);

int getnextday_ical_app(int year, int month, int day);

void rm_ical_app(char *a_day) ;

