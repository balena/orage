/* reminder.h
 *
 * (C) 2004-2005 Mickaël Graf
 * (C) 2005 Juha Kautto
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
    GString *uid;
    GString *alarm_time;
    GString *event_time;
    GString *title;
    gboolean display;
    GString *description;
    gboolean audio;
    GString *sound;
    gint     repeat_cnt;
    gint     repeat_delay;
} alarm_struct;

typedef struct {
    gchar *play_cmd;
    gint cnt;
    gint delay;
} xfce_audio_alarm_type;


void create_wReminder(alarm_struct *alarm);

gboolean xfcalendar_alarm_clock(gpointer user_data);

