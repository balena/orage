/* vim: set expandtab ts=4 sw=4: */
/*
 *
 *  Copyright © 2006-2015 Juha Kautto <juha@xfce.org>
 *
 *  it under the terms of the GNU Library General Public License as published 
 *  by the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  Authors:
 *      Juha Kautto <juha@xfce.org>
 *      Based on XFce panel plugin clock and date-time plugin
 */

#include <config.h>
#include <sys/stat.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkevents.h>
#include <gdk/gdkx.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>

#include "../src/functions.h"
#include "xfce4-orageclock-plugin.h"

/* -------------------------------------------------------------------- *
 *                               Clock                                  *
 * -------------------------------------------------------------------- */

static void oc_utf8_strftime(char *res, int res_l, char *format, struct tm *tm)
{
    char *tmp = NULL;

    /* strftime is nasty. It returns formatted characters (%A...) in utf8
     * but it does not convert plain characters so they will be in locale 
     * charset. 
     * It expects format to be in locale charset, so we need to convert 
     * that first (it may contain utf8).
     * We need then convert the results finally to utf8.
     * */
    tmp = g_locale_from_utf8(format, -1, NULL, NULL, NULL);
    strftime(res, res_l, tmp, tm);
    g_free(tmp);
    /* Then convert to utf8 if needed */
    if (!g_utf8_validate(res, -1, NULL)) {
        tmp = g_locale_to_utf8(res, -1, NULL, NULL, NULL);
        if (tmp) {
            g_strlcpy(res, tmp, res_l);
            g_free(tmp);
        }
    }
}

void oc_line_font_set(ClockLine *line)
{
    PangoFontDescription *font;

    if (line->font->str) {
        font = pango_font_description_from_string(line->font->str);
        gtk_widget_modify_font(line->label, font);
        pango_font_description_free(font);
    }
    else
        gtk_widget_modify_font(line->label, NULL);
}

void oc_line_rotate(Clock *clock, ClockLine *line)
{
    switch (clock->rotation) {
        case 0:
            gtk_label_set_angle(GTK_LABEL(line->label), 0);
            break;
        case 1:
            gtk_label_set_angle(GTK_LABEL(line->label), 90);
            break;
        case 2:
            gtk_label_set_angle(GTK_LABEL(line->label), 270);
            break;
    }
}

void oc_set_line(Clock *clock, ClockLine *clock_line, int pos)
{
    clock_line->label = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(clock->mbox), clock_line->label
            , FALSE, FALSE, 0);
    gtk_box_reorder_child(GTK_BOX(clock->mbox), clock_line->label, pos);
    oc_line_font_set(clock_line);
    oc_line_rotate(clock, clock_line);
    gtk_widget_show(clock_line->label);
    /* clicking does not work after this
    gtk_label_set_selectable(GTK_LABEL(clock_line->label), TRUE);
    */
}

static void oc_set_lines_to_panel(Clock *clock)
{
    ClockLine *clock_line;
    GList   *tmp_list;

    if (clock->lines_vertically)
        clock->mbox = gtk_vbox_new(TRUE, 0);
    else
        clock->mbox = gtk_hbox_new(TRUE, 0);
    gtk_widget_show(clock->mbox);
    gtk_container_add(GTK_CONTAINER(clock->frame), clock->mbox);

    for (tmp_list = g_list_first(clock->lines); 
            tmp_list;
         tmp_list = g_list_next(tmp_list)) {
        clock_line = tmp_list->data;
        /* make sure clock face is updated */
        strcpy(clock_line->prev, "New line");
        oc_set_line(clock, clock_line, -1);
    }
}

void oc_reorganize_lines(Clock *clock)
{
    /* let's just do this easily as it is very seldom called: 
       delete and recreate lines */
    gtk_widget_destroy(GTK_WIDGET(clock->mbox));
    oc_set_lines_to_panel(clock);
    oc_fg_set(clock);
    oc_size_set(clock);
}

static void oc_tooltip_set(Clock *clock)
{
    char res[OC_MAX_LINE_LENGTH-1];

    oc_utf8_strftime(res, sizeof(res), clock->tooltip_data->str, &clock->now);
    if (strcmp(res,  clock->tooltip_prev)) {
        gtk_tooltips_set_tip(clock->tips, GTK_WIDGET(clock->plugin),res, NULL);
        strcpy(clock->tooltip_prev, res);
    }
}

static gboolean oc_get_time(Clock *clock)
{
    time_t  t;
    char    res[OC_MAX_LINE_LENGTH-1];
    ClockLine *line;
    GList   *tmp_list;

    time(&t);
    localtime_r(&t, &clock->now);
    for (tmp_list = g_list_first(clock->lines); 
            tmp_list;
         tmp_list = g_list_next(tmp_list)) {
        line = tmp_list->data;
        oc_utf8_strftime(res, sizeof(res), line->data->str, &clock->now);
        /* gtk_label_set_text call takes almost
         * 100 % of the time used in this procedure.
         * Note that even though we only wake up when needed, we 
         * may not have to update all lines, so this check still
         * probably is worth doing. Specially after we added the
         * hibernate update option.
         * */
        if (strcmp(res, line->prev)) {
            gtk_label_set_text(GTK_LABEL(line->label), res);
            strcpy(line->prev, res);
        }
    }
    oc_tooltip_set(clock);

    return(TRUE);
}

static gboolean oc_get_time_and_tune(Clock *clock)
{
    oc_get_time(clock);
    if (clock->now.tm_sec > 1) {
        /* we are more than 1 sec off => fix the timing */
        oc_start_timer(clock);
    }
    else if (clock->interval > 60000 && clock->now.tm_min != 0) {
        /* we need to check also minutes if we are using hour timer */
        oc_start_timer(clock);
    }
    return(TRUE);
}

static gboolean oc_get_time_delay(Clock *clock)
{
    oc_get_time(clock); /* update clock */
    /* now we really start the clock */
    clock->timeout_id = g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE
            , clock->interval, (GSourceFunc)oc_get_time_and_tune, clock, NULL);
    return(FALSE); /* this is one time only timer */
}

void oc_start_timer(Clock *clock)
{
    gint delay_time; /* this is used to set the clock start time correct */

    /*
    g_message("oc_start_timer: (%s) interval %d  %d:%d:%d", clock->tooltip_prev, clock->interval, clock->now.tm_hour, clock->now.tm_min, clock->now.tm_sec);
    */
    /* stop the clock refresh since we will start it again here soon */
    if (clock->timeout_id) {
        g_source_remove(clock->timeout_id);
        clock->timeout_id = 0;
    }
    if (clock->delay_timeout_id) {
        g_source_remove(clock->delay_timeout_id);
        clock->delay_timeout_id = 0;
    }
    oc_get_time(clock); /* put time on the clock and also fill clock->now */
    /* if we are using longer than 1 second (== 1000) interval, we need
     * to delay the first start so that clock changes when minute or hour
     * changes */
    if (clock->interval <= 1000) { /* no adjustment needed, we show seconds */
        clock->timeout_id = g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE
                , clock->interval, (GSourceFunc)oc_get_time, clock, NULL);
    }
    else { /* need to tune time */
        if (clock->interval <= 60000) /* adjust to next full minute */
            delay_time = (clock->interval - clock->now.tm_sec*1000);
        else /* adjust to next full hour */
            delay_time = (clock->interval -
                    (clock->now.tm_min*60000 + clock->now.tm_sec*1000));

        clock->delay_timeout_id = g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE
                , delay_time, (GSourceFunc)oc_get_time_delay, clock, NULL);
    }
}

gboolean oc_check_if_same(Clock *clock, int diff)
{
    /* we compare if clock would change after diff seconds */
    /* instead of waiting for the time to really pass, we just move the clock
     * and see what would happen in the future. No need to wait for hours. */
    time_t  t, t_next;
    struct tm tm, tm_next;
    char    res[OC_MAX_LINE_LENGTH-1], res_next[OC_MAX_LINE_LENGTH-1];
    int     max_len;
    ClockLine *line;
    GList   *tmp_list;
    gboolean same_time = TRUE, first_check = TRUE, result_known = FALSE;
    
    max_len = sizeof(res); 
    while (!result_known) {
        time(&t);
        t_next = t + diff;  /* diff secs forward */
        localtime_r(&t, &tm);
        localtime_r(&t_next, &tm_next);
        for (tmp_list = g_list_first(clock->lines);
                (tmp_list && same_time);
             tmp_list = g_list_next(tmp_list)) {
            line = tmp_list->data;
            oc_utf8_strftime(res, max_len, line->data->str, &tm);
            oc_utf8_strftime(res_next, max_len, line->data->str, &tm_next);
            if (strcmp(res, res_next)) { /* differ */
                same_time = FALSE;
            }
        }
        /* Need to check also tooltip */
        if (same_time) { /* we only check tooltip if needed */
            oc_utf8_strftime(res, max_len, clock->tooltip_data->str, &tm);
            oc_utf8_strftime(res_next, max_len, clock->tooltip_data->str
                    , &tm_next);
            if (strcmp(res, res_next)) { /* differ */
                same_time = FALSE;
            }
        }

        if (!same_time) {
            if (first_check) {
                /* change detected, but it can be that bigger unit 
                 * like hour or day happened to change, so we need to check 
                 * again to be sure */
                first_check = FALSE;
                same_time = TRUE;
            }
            else { /* second check, now we are sure the clock has changed */
                result_known = TRUE;   /* no need to check more */
            }
        }
        else { /* clock did not change */
            result_known = TRUE;   /* no need to check more */
        }
    }
    return(same_time);
}

void oc_tune_interval(Clock *clock)
{
    /* check if clock changes after 2 secs */
    if (oc_check_if_same(clock, 2)) { /* Continue checking */
        /* We know now that clock does not change every second. 
         * Let's check 2 minutes next: */
        if (oc_check_if_same(clock, 2*60)) {
            /* We know now that clock does not change every minute. 
             * We could check hours next, but cpu saving between 1 hour and 24
             * hours would be minimal. But keeping 24 hour wake up time clock
             * in accurate time would be much more difficult, so we end here 
             * and schedule clock to fire every hour. */
            clock->interval = 3600000;
        }
        else { /* we schedule clock to fire every minute */
            clock->interval = 60000;
        }
    }
}

void oc_init_timer(Clock *clock)
{
    /* Fix for bug 7232. Need to make sure timezone is correct. */
    tzset(); 
    clock->interval = OC_BASE_INTERVAL;
    if (!clock->hib_timing) /* using suspend/hibernate, do not tune time */
        oc_tune_interval(clock);
    oc_start_timer(clock);
}

static void oc_update_size(Clock *clock, int size)
{
    if (size > 26) {
        gtk_container_set_border_width(GTK_CONTAINER(clock->frame), 2);
    }
    else {
        gtk_container_set_border_width(GTK_CONTAINER(clock->frame), 0);
    }
}

static gboolean popup_program(GtkWidget *widget, gchar *program, Clock *clock
        , guint event_time)
{
    GdkAtom atom;
    Window xwindow;
    GError *error = NULL;
    GdkEventClient gev;
    gchar *check, *popup; /* atom names to use */

    if (strcmp(program, "orage") == 0) {
        check = "_XFCE_CALENDAR_RUNNING";
        popup = "_XFCE_CALENDAR_TOGGLE_HERE";
    }
    else if (strcmp(program, "globaltime") == 0) {
        check = "_XFCE_GLOBALTIME_RUNNING";
        popup = "_XFCE_GLOBALTIME_TOGGLE_HERE";
    }
    else {
        g_warning("unknown program to start %s", program);
        return(FALSE);
    }

    /* send message to program to check if it is running */
    atom = gdk_atom_intern(check, FALSE);
    if ((xwindow = XGetSelectionOwner(GDK_DISPLAY(),
            gdk_x11_atom_to_xatom(atom))) != None) { /* yes, then toggle */
        gev.type = GDK_CLIENT_EVENT;
        gev.window = widget->window;
        gev.send_event = TRUE;
        gev.message_type = gdk_atom_intern(popup, FALSE);
        gev.data_format = 8;

        if (!gdk_event_send_client_message((GdkEvent *) &gev,
                (GdkNativeWindow)xwindow)) 
             g_message("%s: send message to %s failed", OC_NAME, program);
        gdk_flush();

        return(TRUE);
    }
    else { /* not running, let's try to start it. Need to reset TZ! */
        static guint prev_event_time = 0; /* prevents double start (BUG 4096) */

        if (prev_event_time && ((event_time - prev_event_time) < 1000)) {
            g_message("%s: double start of %s prevented", OC_NAME, program);
            return(FALSE);
        }
            
        prev_event_time = event_time;
        if (clock->TZ_orig != NULL)  /* we had TZ when we started */
            g_setenv("TZ", clock->TZ_orig, 1);
        else  /* TZ was not set so take it out */
            g_unsetenv("TZ");
        tzset();

        if (!orage_exec(program, FALSE, &error)) 
            g_message("%s: start of %s failed", OC_NAME, program);

        if ((clock->timezone->str != NULL) && (clock->timezone->len > 0)) {
        /* user has set timezone, so let's set TZ */
            g_setenv("TZ", clock->timezone->str, 1);
            tzset();
        }

        return(TRUE);
    }

    return(FALSE);
}

static gboolean on_button_press_event_cb(GtkWidget *widget
        , GdkEventButton *event, Clock *clock)
{
    /* Fix for bug 7232. Need to make sure timezone is correct. */
    tzset(); 
    if (event->type != GDK_BUTTON_PRESS) /* double or triple click */
        return(FALSE); /* ignore */
    if (event->button == 1)
        return(popup_program(widget, "orage", clock, event->time));
    else if (event->button == 2)
        return(popup_program(widget, "globaltime", clock, event->time));

    return(FALSE);
}


/* -------------------------------------------------------------------- *
 *                     Panel Plugin Interface                           *
 * -------------------------------------------------------------------- */


/* Interface Implementation */

static gboolean oc_set_size(XfcePanelPlugin *plugin, int size, Clock *clock)
{
    oc_update_size(clock, size);
    if (clock->first_call) {
    /* default is horizontal panel. 
       we need to check and change if it is vertical */
        if (xfce_panel_plugin_get_mode(plugin) 
                == XFCE_PANEL_PLUGIN_MODE_VERTICAL) {
            clock->lines_vertically = FALSE;
        /* check rotation handling from oc_rotation_changed in oc_config.c */
            if (xfce_screen_position_is_right(
                        xfce_panel_plugin_get_screen_position(plugin)))
                clock->rotation = 2;
            else
                clock->rotation = 1;
            oc_reorganize_lines(clock);
        }

    }

    return(TRUE);
}

static void oc_free_data(XfcePanelPlugin *plugin, Clock *clock)
{
    GtkWidget *dlg = g_object_get_data(G_OBJECT(plugin), "dialog");

    if (dlg)
        gtk_widget_destroy(dlg);
    
    if (clock->timeout_id) {
        g_source_remove(clock->timeout_id);
    }
    g_list_free(clock->lines);
    g_free(clock->TZ_orig);
    g_object_unref(clock->tips);
    g_free(clock);
}

static GdkColor oc_rc_read_color(XfceRc *rc, char *par, char *def)
{
    const gchar *ret;
    GdkColor color;

    ret = xfce_rc_read_entry(rc, par, def);
    color.pixel = 0;
    if (!strcmp(ret, def)
    ||  sscanf(ret, OC_RC_COLOR
                , (unsigned int *)&color.red
                , (unsigned int *)&color.green
                , (unsigned int *)&color.blue) != 3) {
        gint i = sscanf(ret, OC_RC_COLOR , (unsigned int *)&color.red , (unsigned int *)&color.green , (unsigned int *)&color.blue);
        g_warning("unable to read %s colour from rc file ret=(%s) def=(%s) cnt=%d", par, ret, def, i);
        gdk_color_parse(ret, &color);
    }
    return(color);
}

ClockLine * oc_add_new_line(Clock *clock, const char *data, const char *font, int pos)
{
    ClockLine *clock_line = g_new0(ClockLine, 1);

    clock_line->data = g_string_new(data);
    clock_line->font = g_string_new(font);
    strcpy(clock_line->prev, "New line");
    clock_line->clock = clock;
    clock->lines = g_list_insert(clock->lines, clock_line, pos);
    return(clock_line);
}

static void oc_read_rc_file(XfcePanelPlugin *plugin, Clock *clock)
{
    gchar  *file;
    XfceRc *rc;
    const gchar  *ret, *data, *font;
    gchar tmp[100];
    gboolean more_lines;
    gint i;

    if (!(file = xfce_panel_plugin_lookup_rc_file(plugin)))
        return; /* if it does not exist, we use defaults from orage_oc_new */
    if (!(rc = xfce_rc_simple_open(file, TRUE))) {
        g_warning("unable to read-open rc file (%s)", file);
        return;
    }
    clock->first_call = FALSE;

    clock->show_frame = xfce_rc_read_bool_entry(rc, "show_frame", TRUE);

    clock->fg_set = xfce_rc_read_bool_entry(rc, "fg_set", FALSE);
    if (clock->fg_set) {
        clock->fg = oc_rc_read_color(rc, "fg", "black");
    }

    clock->bg_set = xfce_rc_read_bool_entry(rc, "bg_set", FALSE);
    if (clock->bg_set) {
        clock->bg = oc_rc_read_color(rc, "bg", "white");
    }
    g_free(file);

    ret = xfce_rc_read_entry(rc, "timezone", NULL);
    g_string_assign(clock->timezone, ret); 

    clock->width_set = xfce_rc_read_bool_entry(rc, "width_set", FALSE);
    if (clock->width_set) {
        clock->width = xfce_rc_read_int_entry(rc, "width", -1);
    }
    clock->height_set = xfce_rc_read_bool_entry(rc, "height_set", FALSE);
    if (clock->height_set) {
        clock->height = xfce_rc_read_int_entry(rc, "height", -1);
    }

    clock->lines_vertically = xfce_rc_read_bool_entry(rc, "lines_vertically", FALSE);
    clock->rotation = xfce_rc_read_int_entry(rc, "rotation", 0);
    
    for (i = 0, more_lines = TRUE; more_lines; i++) {
        sprintf(tmp, "data%d", i);
        data = xfce_rc_read_entry(rc, tmp, NULL);
        if (data) { /* let's add it */
            sprintf(tmp, "font%d", i);
            font = xfce_rc_read_entry(rc, tmp, NULL);
            oc_add_new_line(clock, data, font, -1);
        }
        else { /* no more clock lines */
            more_lines = FALSE;
        }
    }
    clock->orig_line_cnt = i; 

    if ((ret = xfce_rc_read_entry(rc, "tooltip", NULL)))
        g_string_assign(clock->tooltip_data, ret); 

    clock->hib_timing = xfce_rc_read_bool_entry(rc, "hib_timing", FALSE);

    xfce_rc_close(rc);
}

void oc_write_rc_file(XfcePanelPlugin *plugin, Clock *clock)
{
    gchar  *file;
    XfceRc *rc;
    gchar   tmp[100];
    int     i;
    ClockLine *line;
    GList   *tmp_list;

    if (!(file = xfce_panel_plugin_save_location(plugin, TRUE))) {
        g_warning("unable to write rc file");
        return;
    }
    if (!(rc = xfce_rc_simple_open(file, FALSE))) {
        g_warning("unable to read-open rc file (%s)", file);
        return;
    }
    g_free(file);

    xfce_rc_write_bool_entry(rc, "show_frame", clock->show_frame);

    xfce_rc_write_bool_entry(rc, "fg_set", clock->fg_set);
    if (clock->fg_set) {
        sprintf(tmp, "%uR %uG %uB"
                , clock->fg.red, clock->fg.green, clock->fg.blue);
        xfce_rc_write_entry(rc, "fg", tmp);
    }
    else {
        xfce_rc_delete_entry(rc, "fg", TRUE);
    }

    xfce_rc_write_bool_entry(rc, "bg_set", clock->bg_set);
    if (clock->bg_set) {
        sprintf(tmp, "%uR %uG %uB"
                , clock->bg.red, clock->bg.green, clock->bg.blue);
        xfce_rc_write_entry(rc, "bg", tmp);
    }
    else {
        xfce_rc_delete_entry(rc, "bg", TRUE);
    }

    xfce_rc_write_entry(rc, "timezone",  clock->timezone->str);

    xfce_rc_write_bool_entry(rc, "width_set", clock->width_set);
    if (clock->width_set) {
        xfce_rc_write_int_entry(rc, "width", clock->width);
    }
    else {
        xfce_rc_delete_entry(rc, "width", TRUE);
    }

    xfce_rc_write_bool_entry(rc, "height_set", clock->height_set);
    if (clock->height_set) {
        xfce_rc_write_int_entry(rc, "height", clock->height);
    }
    else {
        xfce_rc_delete_entry(rc, "height", TRUE);
    }

    xfce_rc_write_bool_entry(rc, "lines_vertically", clock->lines_vertically);
    xfce_rc_write_int_entry(rc, "rotation", clock->rotation);

    for (i = 0, tmp_list = g_list_first(clock->lines);
            tmp_list;
         i++, tmp_list = g_list_next(tmp_list)) {
        line = tmp_list->data;
        sprintf(tmp, "data%d", i);
        xfce_rc_write_entry(rc, tmp,  line->data->str);
        sprintf(tmp, "font%d", i);
        xfce_rc_write_entry(rc, tmp,  line->font->str);
    }
    /* delete extra lines */
    for (; i <= clock->orig_line_cnt; i++) {
        sprintf(tmp, "data%d", i);
        xfce_rc_delete_entry(rc, tmp,  FALSE);
        sprintf(tmp, "font%d", i);
        xfce_rc_delete_entry(rc, tmp,  FALSE);
    }

    xfce_rc_write_entry(rc, "tooltip",  clock->tooltip_data->str);

    xfce_rc_write_bool_entry(rc, "hib_timing", clock->hib_timing);

    xfce_rc_close(rc);
}

/* Create widgets and connect to signals */
Clock *orage_oc_new(XfcePanelPlugin *plugin)
{
    Clock *clock = g_new0(Clock, 1);

    clock->first_call = TRUE; /* this is starting point */
    clock->plugin = plugin;

    clock->ebox = gtk_event_box_new();
    gtk_widget_show(clock->ebox);

    clock->frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(clock->ebox), clock->frame);
    gtk_widget_show(clock->frame);

    gtk_container_add(GTK_CONTAINER(plugin), clock->ebox);

    clock->show_frame = TRUE;
    clock->fg_set = FALSE;
    clock->bg_set = FALSE;
    clock->width_set = FALSE;
    clock->height_set = FALSE;
    clock->lines_vertically = TRUE;
    clock->rotation = 0; /* no rotation */

    clock->timezone = g_string_new(""); /* = not set */
    clock->TZ_orig = g_strdup(g_getenv("TZ"));

    clock->lines = NULL; /* no lines yet */
    clock->orig_line_cnt = 0;

    /* TRANSLATORS: Use format characters from strftime(3)
     * to get the proper string for your locale.
     * I used these:
     * %A  : full weekday name
     * %d  : day of the month
     * %B  : full month name
     * %Y  : four digit year
     * %V  : ISO week number
     */
    clock->tooltip_data = g_string_new(_("%A %d %B %Y/%V"));

    clock->hib_timing = FALSE;

    clock->tips = gtk_tooltips_new();
    g_object_ref(clock->tips);
    gtk_object_sink(GTK_OBJECT(clock->tips));
        
    return(clock);
}

void oc_show_frame_set(Clock *clock)
{
    gtk_frame_set_shadow_type(GTK_FRAME(clock->frame)
            , clock->show_frame ? GTK_SHADOW_IN : GTK_SHADOW_NONE);
}

void oc_fg_set(Clock *clock)
{
    GdkColor *fg = NULL;
    ClockLine *line;
    GList   *tmp_list;

    if (clock->fg_set)
        fg = &clock->fg;

    for (tmp_list = g_list_first(clock->lines);
            tmp_list;
         tmp_list = g_list_next(tmp_list)) {
        line = tmp_list->data;
        gtk_widget_modify_fg(line->label, GTK_STATE_NORMAL, fg);
    }
}

void oc_bg_set(Clock *clock)
{
    GdkColor *bg = NULL;

    if (clock->bg_set)
        bg = &clock->bg;

    gtk_widget_modify_bg(clock->ebox, GTK_STATE_NORMAL, bg);
}

void oc_timezone_set(Clock *clock)
{
    if ((clock->timezone->str != NULL) && (clock->timezone->len > 0)) {
        /* user has set timezone, so let's set TZ */
        g_setenv("TZ", clock->timezone->str, 1);
    }
    else if (clock->TZ_orig != NULL) { /* we had TZ when we started */
        g_setenv("TZ", clock->TZ_orig, 1);
    }
    else { /* TZ was not set so take it out */
        g_unsetenv("TZ");
    }
    tzset();
}

void oc_size_set(Clock *clock)
{
    gint w, h;

    w = clock->width_set ? clock->width : -1;
    h = clock->height_set ? clock->height : -1;
    gtk_widget_set_size_request(clock->mbox, w, h);
}

static void oc_construct(XfcePanelPlugin *plugin)
{
    Clock *clock;

    xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    clock = orage_oc_new(plugin);

    oc_read_rc_file(plugin, clock);
    if (clock->lines == NULL) { /* Let's setup a default clock_line */
        oc_add_new_line(clock, "%X", "", -1);
    }

    oc_set_lines_to_panel(clock);
    oc_show_frame_set(clock);
    oc_fg_set(clock);
    oc_bg_set(clock);
    oc_timezone_set(clock);
    oc_size_set(clock);

    oc_init_timer(clock);

    /* we are called through size-changed trigger
    oc_update_size(clock, xfce_panel_plugin_get_size(plugin));
    */

    xfce_panel_plugin_add_action_widget(plugin, clock->ebox);
    
    xfce_panel_plugin_menu_show_configure(plugin);

    g_signal_connect(plugin, "configure-plugin", 
            G_CALLBACK(oc_properties_dialog), clock);

    g_signal_connect(plugin, "size-changed", 
            G_CALLBACK(oc_set_size), clock);
    
    g_signal_connect(plugin, "free-data", 
            G_CALLBACK(oc_free_data), clock);
    
    g_signal_connect(plugin, "save", 
            G_CALLBACK(oc_write_rc_file), clock);

/* callback for calendar and globaltime popup */
    g_signal_connect(clock->ebox, "button-press-event",
            G_CALLBACK(on_button_press_event_cb), clock);
}

/* Register with the panel */

XFCE_PANEL_PLUGIN_REGISTER(oc_construct);

