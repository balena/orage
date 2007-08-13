/*  xfce4
 *  Copyright (C) 2006-2007 Juha Kautto (juha@xfce.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This pibrary is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#define OC_NAME "orageclock panel plugin"
#define OC_MAX_LINES 3 /* changing this causes trouble. don't do it */
#define OC_MAX_LINE_LENGTH 100 
#define OC_BASE_INTERVAL 1000  /* best accurance of the clock */
#define OC_CONFIG_INTERVAL 200 /* special fast interval used in setup */

typedef struct _timetuning
{
    gint  level; /* 0 = seconds, 1 = minutes, 2 = hours */
    gint  cnt;
    gboolean changed_once; 
    gint  timeout_id;
    gchar prev[OC_MAX_LINES][OC_MAX_LINE_LENGTH+1];
    gchar tooltip_prev[OC_MAX_LINE_LENGTH+1];
} TimeTuning;

typedef struct _clockline
{
    GtkWidget *label;
    gboolean   show;
    GString   *data; /* the time formatting data */
    GString   *font;
    gchar      prev[OC_MAX_LINE_LENGTH+1];
} ClockLine;

typedef struct _clock
{
    XfcePanelPlugin *plugin;

    GtkWidget *ebox;
    GtkWidget *frame;
    GtkWidget *vbox;
    gboolean   width_set;
    gint       width;
    gboolean   height_set;
    gint       height;
    gboolean   show_frame;
    gboolean   fg_set;
    GdkColor   fg;
    gboolean   bg_set;
    GdkColor   bg;
    GString   *timezone;
    gchar     *TZ_orig;
    GtkWidget *tz_entry;
    ClockLine  line[OC_MAX_LINES];
    GString   *tooltip_data;
    gchar      tooltip_prev[OC_MAX_LINE_LENGTH+1];

    GtkTooltips *tips;
    int timeout_id;
    int delay_timeout_id;
    int adjust_timeout_id;
    int interval;
    struct tm   now;

    TimeTuning  *tune;
} Clock;

void oc_properties_dialog(XfcePanelPlugin *plugin, Clock *clock);

void oc_show_frame_set(Clock *clock);
void oc_fg_set(Clock *clock);
void oc_bg_set(Clock *clock);
void oc_size_set(Clock *clock);
void oc_timezone_set(Clock *clock);
void oc_show_line_set(Clock *clock, gint lno);
void oc_line_font_set(Clock *clock, gint lno);
void oc_write_rc_file(XfcePanelPlugin *plugin, Clock *clock);
gboolean oc_start_timer(Clock *clock);
void oc_start_tuning(Clock *clock);
void oc_disable_tuning(Clock *clock);
void oc_init_timer(Clock *clock);

