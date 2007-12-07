/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2005-2007 Juha Kautto  (juha at xfce.org)
 * Copyright (c) 2004-2006 Mickael Graf (korbinus at xfce.org)
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

#ifndef __REMINDER_H__
#define __REMINDER_H__

typedef struct _active_alarm_struct
{
    gboolean sound_active; /* sound is currently being played */
    GtkWidget *stop_noise_reminder;
    gpointer active_notify; /* this is NotifyNotification, but it may not be
                               linked in, so need to be done like this */
    gboolean notify_stop_noise_action;
} active_alarm_struct;

typedef struct _alarm_struct
{
    gchar   *alarm_time;
    gchar   *uid;
    gchar   *title;
    gchar   *description;
    gboolean persistent;

    gboolean display_orage;
    gboolean display_notify;
    gboolean notify_refresh;
    gint     notify_timeout;

    gboolean audio;
    gchar   *sound;
    gint     repeat_cnt;
    gint     repeat_delay;

    gboolean procedure;
    gchar   *cmd;
    /*
    gboolean email;
    */
    /* this is used to control active alarms */
    active_alarm_struct *active_alarm;
} alarm_struct;

gboolean orage_day_change(gpointer user_data);
gboolean setup_orage_alarm_clock(void);
void alarm_read();
void alarm_list_free();

#endif /* !__REMINDER_H__ */
