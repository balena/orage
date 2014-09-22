/*  xfce4
 *  Copyright (C) 2006-2011 Juha Kautto (juha@xfce.org)
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
#define OC_MAX_LINES 10             /* max number of lines in the plugin */
#define OC_MAX_LINE_LENGTH 100 
#define OC_BASE_INTERVAL 1000       /* best accurance of the clock = 1 sec */
#define OC_CONFIG_INTERVAL 200      /* special fast interval used in setup */
#define OC_RC_COLOR "%uR %uG %uB"   /* this is how we store colors */

typedef struct _clock
{
    XfcePanelPlugin *plugin;

    GtkWidget *ebox;
    GtkWidget *frame;
    GtkWidget *mbox; /* main box. Either vertical or horizontal */
    gboolean   show_frame;
    gboolean   fg_set;
    GdkColor   fg;
    gboolean   bg_set;
    GdkColor   bg;
    gboolean   width_set;
    gint       width;
    gboolean   height_set;
    gint       height;
    gboolean   lines_vertically;
    gint       rotation;
    GString   *timezone;
    gchar     *TZ_orig;
    GList     *lines;
    gint       orig_line_cnt;
    GString   *tooltip_data;
    gchar      tooltip_prev[OC_MAX_LINE_LENGTH+1];
    gboolean   hib_timing;

    GtkTooltips *tips;
    int timeout_id;  /* timer id for the clock */
    int delay_timeout_id;
    int interval;
    struct tm  now;
    gboolean first_call; /* set defaults correct when clock is created */
} Clock;

typedef struct _clockline
{
    GtkWidget *label;
    GString   *data; /* the time formatting data */
    GString   *font;
    gchar      prev[OC_MAX_LINE_LENGTH+1];
    Clock     *clock; /* pointer back to main clock structure */
} ClockLine;

void oc_properties_dialog(XfcePanelPlugin *plugin, Clock *clock);

void oc_show_frame_set(Clock *clock);
void oc_fg_set(Clock *clock);
void oc_bg_set(Clock *clock);
void oc_size_set(Clock *clock);
void oc_timezone_set(Clock *clock);
void oc_line_font_set(ClockLine *line);
void oc_line_rotate(Clock *clock, ClockLine *line);
void oc_write_rc_file(XfcePanelPlugin *plugin, Clock *clock);
void oc_start_timer(Clock *clock);
void oc_init_timer(Clock *clock);
void oc_set_line(Clock *clock, ClockLine *clock_line, int pos);
ClockLine * oc_add_new_line(Clock *clock, const char *data, const char *font, int pos);
void oc_reorganize_lines(Clock *clock);
