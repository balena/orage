/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2005-2011 Juha Kautto  (juha at xfce.org)
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

#define _XOPEN_SOURCE /* glibc2 needs this */
#define _XOPEN_SOURCE_EXTENDED 1 /* strptime needs this in posix systems */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
/*
#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>
*/

#include "orage-i18n.h"
#include "functions.h"
#include "parameters.h"

/* message logging level */
int g_log_level=0;

/**************************************
 *  Debugging helping functions       *
 **************************************/
/* this is for testing only. it can be used to see where time is spent.
 * Add call program_log("dbus started") in the code and run orage like:
 * strace -ttt -f -o /tmp/logfile.strace ./orage
 * And then you can check results:
 * grep MARK /tmp/logfile.strace
 * grep MARK /tmp/logfile.strace|sed s/", F_OK) = -1 ENOENT (No such file or directory)"/\)/
 * */
/*
void program_log (const char *format, ...)
{
        va_list args;
        char *formatted, *str;

        va_start (args, format);
        formatted = g_strdup_vprintf (format, args);
        va_end (args);

        str = g_strdup_printf ("MARK: %s: %s", g_get_prgname(), formatted);
        g_free (formatted);

        access (str, F_OK);
        g_free (str);
}
*/

/* Print message at various level:
 * < 0     = Debug (Use only in special debug code which should not be in 
 *                  normal code.)
 * 0-99    = Message
 * 100-199 = Warning
 * 200-299 = Critical warning
 * 300-    = Error
 * variable g_log_level can be used to control how much data is printed
 */
void orage_message(gint level, const char *format, ...)
{
    va_list args;
    char *formatted;
    struct tm *t = orage_localtime();

    if (level < g_log_level)
        return;
    va_start(args, format);
    formatted = g_strdup_vprintf(format, args);
    va_end(args);

    g_print("%02d:%02d:%02d ", t->tm_hour, t->tm_min, t->tm_sec);
    if (level < 0)
        g_debug("%s", formatted);
    else if (level < 100) 
        g_message("Orage **: %s", formatted);
    else if (level < 200) 
        g_warning("%s", formatted);
    else if (level < 300) 
        g_critical("%s", formatted);
    else
        g_error("Orage **: %s", formatted);
    g_free(formatted);
}

/**************************************
 *  General purpose helper functions  *
 **************************************/

GtkWidget *orage_create_combo_box_with_content(char *text[], int size)
{
    register int i;
    GtkWidget *combo_box;

    combo_box = gtk_combo_box_new_text();
    for (i = 0; i < size; i++) {
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo_box)
                , (const gchar *)text[i]);
    }
    return(combo_box);
}

gboolean orage_date_button_clicked(GtkWidget *button, GtkWidget *win)
{
    GtkWidget *selDate_dialog;
    GtkWidget *selDate_calendar;
    gint result;
    char *new_date=NULL, *cur_date;
    struct tm cur_t;
    gboolean changed, allocated=FALSE;

    selDate_dialog = gtk_dialog_new_with_buttons(
            _("Pick the date"), GTK_WINDOW(win),
            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
            _("Today"),
            1,
            GTK_STOCK_OK,
            GTK_RESPONSE_ACCEPT,
            NULL);

    selDate_calendar = gtk_calendar_new();
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(selDate_dialog)->vbox)
            , selDate_calendar);

    cur_date = (char *)gtk_button_get_label(GTK_BUTTON(button));
    if (cur_date)
        cur_t = orage_i18_date_to_tm_date(cur_date);
    else /* something was wrong. let's return some valid value */
        cur_t = orage_i18_date_to_tm_date(orage_localdate_i18());

    orage_select_date(GTK_CALENDAR(selDate_calendar)
            , cur_t.tm_year+1900, cur_t.tm_mon, cur_t.tm_mday);
    gtk_widget_show_all(selDate_dialog);

    result = gtk_dialog_run(GTK_DIALOG(selDate_dialog));
    switch(result){
        case GTK_RESPONSE_ACCEPT:
            new_date = orage_cal_to_i18_date(GTK_CALENDAR(selDate_calendar));
            break;
        case 1:
            new_date = orage_localdate_i18();
            break;
        case GTK_RESPONSE_DELETE_EVENT:
        default:
            new_date = g_strdup(cur_date);
            allocated = TRUE;
            break;
    }
    if (g_ascii_strcasecmp(new_date, cur_date) != 0)
        changed = TRUE;
    else
        changed = FALSE;
    gtk_button_set_label(GTK_BUTTON(button), (const gchar *)new_date);
    if (allocated)
        g_free(new_date);
    gtk_widget_destroy(selDate_dialog);
    return(changed);
}

static void child_setup_async(gpointer user_data)
{
#if defined(HAVE_SETSID) && !defined(G_OS_WIN32)
    setsid();
#endif
}

static void child_watch_cb(GPid pid, gint status, gpointer data)
{
    gboolean *cmd_active = (gboolean *)data;

    waitpid(pid, NULL, 0);
    g_spawn_close_pid(pid);
    *cmd_active = FALSE;
}

gboolean orage_exec(const char *cmd, gboolean *cmd_active, GError **error)
{
    char **argv;
    gboolean success;
    int spawn_flags = G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD;
    GPid pid;

    if (!g_shell_parse_argv(cmd, NULL, &argv, error))
        return(FALSE);

    if (!argv || !argv[0])
        return(FALSE);

    success = g_spawn_async(NULL, argv, NULL, spawn_flags
            , child_setup_async, NULL, &pid, error);
    if (cmd_active) {
        if (success)
            *cmd_active = TRUE;
        g_child_watch_add(pid, child_watch_cb, cmd_active);
    }
    g_strfreev(argv);

    return(success);
}

GtkWidget *orage_toolbar_append_button(GtkWidget *toolbar
    , const gchar *stock_id, GtkTooltips *tooltips, const char *tooltip_text
    , gint pos)
{
    GtkWidget *button;

    button = (GtkWidget *)gtk_tool_button_new_from_stock(stock_id);
    gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(button), tooltips
            , (const gchar *) tooltip_text, NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), pos);
    return button;
}

GtkWidget *orage_toolbar_append_separator(GtkWidget *toolbar, gint pos)
{
    GtkWidget *separator;

    separator = (GtkWidget *)gtk_separator_tool_item_new();
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(separator), pos);

    return separator;
}

GtkWidget *orage_table_new(guint rows, guint border)
{
    GtkWidget *table;

    table = gtk_table_new(rows, 2, FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(table), border);
    gtk_table_set_row_spacings(GTK_TABLE(table), 6);
    gtk_table_set_col_spacings(GTK_TABLE(table), 6);
    return table;
}

void orage_table_add_row(GtkWidget *table, GtkWidget *label
        , GtkWidget *input, guint row
        , GtkAttachOptions input_x_option
        , GtkAttachOptions input_y_option)
{
    if (label) {
        gtk_table_attach(GTK_TABLE (table), label, 0, 1, row, row+1
                , (GTK_FILL), (0), 0, 0);
        gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    }

    if (input) {
        gtk_table_attach(GTK_TABLE(table), input, 1, 2, row, row+1,
                  input_x_option, input_y_option, 0, 0);
    }
}

GtkWidget *orage_menu_new(const gchar *menu_header_title
    , GtkWidget *menu_bar)
{
    GtkWidget *menu_header, *menu;

    menu_header = gtk_menu_item_new_with_mnemonic(menu_header_title);
    gtk_container_add(GTK_CONTAINER(menu_bar), menu_header);

    menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_header), menu);

    return menu;
}

GtkWidget *orage_image_menu_item_new_from_stock(const gchar *stock_id
    , GtkWidget *menu, GtkAccelGroup *ag)
{
    GtkWidget *menu_item;

    menu_item = gtk_image_menu_item_new_from_stock(stock_id, ag);
    gtk_container_add(GTK_CONTAINER(menu), menu_item);
    return menu_item;
}

GtkWidget *orage_separator_menu_item_new(GtkWidget *menu)
{
    GtkWidget *menu_item;

    menu_item = gtk_separator_menu_item_new();
    gtk_container_add(GTK_CONTAINER(menu), menu_item);
    return menu_item;
}

GtkWidget *orage_menu_item_new_with_mnemonic(const gchar *label
    , GtkWidget *menu)
{
    GtkWidget *menu_item;

    menu_item = gtk_menu_item_new_with_mnemonic(label);
    gtk_container_add(GTK_CONTAINER(menu), menu_item);
    return menu_item;
}

/* replace old with new string in text.
   if changes are done, returns newly allocated char *, which needs to be freed
   if there are no changes, it returns the original string without freeing it.
   You can always use this like 
   str=orage_replace_text(str, old, new);
   but it may point to new place.
*/
char *orage_replace_text(char *text, char *old, char *new)
{
#undef P_N
#define P_N "orage_replace_text: "
    /* these point to the original string and travel it until no more commands 
     * are found:
     * cur points to the current head (after each old if any found)
     * cmd points to the start of next old in text */
    char *cur, *cmd;
    char *beq=NULL; /* beq is the total new string. */
    char *tmp;      /* temporary pointer to handle freeing */

    for (cur = text; cur && (cmd = strstr(cur, old)); cur = cmd + strlen(old)) {
        cmd[0] = '\0'; /* temporarily */
        if (beq) { /* we already have done changes */
            tmp = beq;
            beq = g_strconcat(tmp, cur, new, NULL);
            g_free(tmp);
        }
        else /* first round */
            beq = g_strconcat(cur, new, NULL);
        cmd[0] = old[0]; /* back to real value */
    }

    if (beq) {
        /* we found and processed at least one command, 
         * add the remaining fragment and return it */
        tmp = beq;
        beq = g_strconcat(tmp, cur, NULL);
        g_free(tmp);
        g_free(text); /* free original string as we changed it */
    }
    else {
        /* we did not find any commands,
         * so just return the original string */
        beq = text;
    }
    return(beq);
}

/* this will change <&Xnnn> type commands to numbers or text as defined.
 * Currently the only command is 
 * <&Ynnnn> which calculates years between current year and nnnn */
char *orage_process_text_commands(char *text)
{
#undef P_N
#define P_N "process_text_commands: "
    /* these point to the original string and travel it until no more commands 
     * are found:
     * cur points to the current head
     * cmd points to the start of new command
     * end points to the end of new command */
    char *cur, *end, *cmd;
    /* these point to the new string, which has commands in processed form:
     * new is the new fragment to be added
     * beq is the total new string. */
    char *new=NULL, *beq=NULL;
    char *tmp; /* temporary pointer to handle freeing */
    int start_year = -1, year_diff, res;
    struct tm *cur_time;

    /**** RULE <&Ynnnn> difference of the nnnn year and current year *****/
    /* This is usefull in birthdays for example: I will be <&Y1980>
     * translates to "I will be 29" if the alarm is raised on 2009 */
    for (cur = text; cur && (cmd = strstr(cur, "<&Y")); cur = end) {
        if ((end = strstr(cmd, ">"))) {
            end[0] = '\0'; /* temporarily. */
            res = sscanf(cmd, "<&Y%d", &start_year);
            end[0] = '>'; /* put it back. */
            if (res == 1 && start_year > 0) { /* we assume success */
                cur_time = orage_localtime();
                year_diff = cur_time->tm_year + 1900 - start_year;
                if (year_diff > 0) { /* sane value */
                    end++; /* next char after > */
                    cmd[0] = '\0'; /* temporarily. (this ends cur) */
                    new = g_strdup_printf("%s%d", cur, year_diff);
                    cmd[0] = '<'; /* put it back */
                    if (beq) { /* this is normal round after first */
                        tmp = beq;
                        beq = g_strdup_printf("%s%s", tmp, new);
                        g_free(tmp);
                    }
                    else { /* first round, we do not have beq yet */
                        beq = g_strdup(new);
                    }
                    g_free(new);
                }
                else
                    orage_message(150, P_N "start year is too big (%d)."
                            , start_year);
            }
            else
                orage_message(150, P_N "failed to understand parameter (%s)."
                        , cmd);
        }
        else
            orage_message(150, P_N "parameter (%s) misses ending >.", cmd);
    }

    if (beq) {
        /* we found and processed at least one command, 
         * add the remaining fragment and return it */
        tmp = beq;
        beq = g_strdup_printf("%s%s", tmp, cur);
        g_free(tmp);
    }
    else {
        /* we did not find any commands,
         * so just return duplicate of the original string */
        beq = g_strdup(text);
    }
    return(beq);
}

GtkWidget *orage_period_hbox_new(gboolean head_space, gboolean tail_space
        , GtkWidget *spin_dd, GtkWidget *dd_label
        , GtkWidget *spin_hh, GtkWidget *hh_label
        , GtkWidget *spin_mm, GtkWidget *mm_label)
{
    GtkWidget *hbox, *space_label;

    hbox = gtk_hbox_new(FALSE, 0);

    if (head_space) {
        space_label = gtk_label_new("   ");
        gtk_box_pack_start(GTK_BOX(hbox), space_label, FALSE, FALSE, 0);
    }

    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(spin_dd), TRUE);
    gtk_box_pack_start(GTK_BOX(hbox), spin_dd, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(hbox), dd_label, FALSE, FALSE, 5);

    space_label = gtk_label_new("   ");
    gtk_box_pack_start(GTK_BOX(hbox), space_label, FALSE, FALSE, 0);

    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(spin_hh), TRUE);
    /* gtk_widget_set_size_request(spin_hh, 40, -1); */
    gtk_box_pack_start(GTK_BOX(hbox), spin_hh, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(hbox), hh_label, FALSE, FALSE, 5);

    space_label = gtk_label_new("   ");
    gtk_box_pack_start(GTK_BOX(hbox), space_label, FALSE, FALSE, 0);

    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(spin_mm), TRUE);
    gtk_spin_button_set_increments(GTK_SPIN_BUTTON(spin_mm), 5, 10);
    /* gtk_widget_set_size_request(spin_mm, 40, -1); */
    gtk_box_pack_start(GTK_BOX(hbox), spin_mm, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(hbox), mm_label, FALSE, FALSE, 5);

    if (tail_space) {
        space_label = gtk_label_new("   ");
        gtk_box_pack_start(GTK_BOX(hbox), space_label, FALSE, FALSE, 0);
    }

    return hbox;
}

GtkWidget *orage_create_framebox_with_content(const gchar *title
        , GtkWidget *content)
{
    GtkWidget *framebox;
    GtkWidget *frame_bin;
    gchar *tmp;
    GtkWidget *label;

    framebox = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(framebox), GTK_SHADOW_NONE);
    gtk_frame_set_label_align(GTK_FRAME(framebox), 0.0, 1.0);
    gtk_widget_show(framebox);

    if (title) {
        tmp = g_strdup_printf("<b>%s</b>", title);
        label = gtk_label_new(tmp);
        gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
        gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
        gtk_widget_show(label);
        gtk_frame_set_label_widget(GTK_FRAME(framebox), label);
        g_free(tmp);
    }

    frame_bin = gtk_alignment_new(0.0, 0.0, 1.0, 1.0);
    gtk_alignment_set_padding(GTK_ALIGNMENT(frame_bin), 5, 5, 21, 5);
    gtk_widget_show(frame_bin);
    gtk_container_add(GTK_CONTAINER(framebox), frame_bin);
    gtk_container_add(GTK_CONTAINER(frame_bin), content);

    return(framebox);
}

/*******************************************************
 * time convert and manipulation functions
 *******************************************************/

struct tm orage_i18_time_to_tm_time(const char *i18_time)
{
    char *ret;
    struct tm tm_time = {0,0,0,0,0,0,0,0,0};

    ret = (char *)strptime(i18_time, "%x %R", &tm_time);
    if (ret == NULL)
        g_error("Orage: orage_i18_time_to_tm_time wrong format (%s)", i18_time);
    else if (ret[0] != '\0')
        g_warning("Orage: orage_i18_time_to_tm_time too long format (%s). Ignoring:%s)"
                , i18_time, ret);
    return(tm_time);
}

struct tm orage_i18_date_to_tm_date(const char *i18_date)
{
    char *ret;
    struct tm tm_date = {0,0,0,0,0,0,0,0,0};

    ret = (char *)strptime(i18_date, "%x", &tm_date);
    if (ret == NULL)
        g_error("Orage: orage_i18_date_to_tm_date wrong format (%s)", i18_date);
    else if (ret[0] != '\0')
        g_warning("Orage: orage_i18_date_to_tm_date too long format (%s). Ignoring:%s)"
                , i18_date, ret);
    return(tm_date);
}

char *orage_tm_time_to_i18_time(struct tm *tm_time)
{
    static char i18_time[40];

    if (strftime(i18_time, 40, "%x %R", tm_time) == 0)
        g_error("Orage: orage_tm_time_to_i18_time too long string in strftime");
    return(i18_time);
}

char *orage_tm_date_to_i18_date(struct tm *tm_date)
{
    static char i18_date[32];

    if (strftime(i18_date, 32, "%x", tm_date) == 0)
        g_error("Orage: orage_tm_date_to_i18_date too long string in strftime");
    return(i18_date);
}

struct tm orage_cal_to_tm_time(GtkCalendar *cal, gint hh, gint mm)
{
    /* dst needs to -1 or mktime adjusts time if we are in another
     * dst setting */
    struct tm tm_date = {0,0,0,0,0,0,0,0,-1};

    gtk_calendar_get_date(cal
            , (unsigned int *)&tm_date.tm_year
            , (unsigned int *)&tm_date.tm_mon
            , (unsigned int *)&tm_date.tm_mday);
    tm_date.tm_year -= 1900;
    tm_date.tm_hour = hh;
    tm_date.tm_min = mm;
    /* need to fill missing tm_wday and tm_yday, which are in use 
     * in some locale's default date. For example in en_IN. mktime does it */
    if (mktime(&tm_date) == (time_t) -1) {
        g_warning("orage: orage_cal_to_tm_time mktime failed %d %d %d"
                , tm_date.tm_year, tm_date.tm_mon, tm_date.tm_mday);
    }
    return(tm_date);
}

char *orage_cal_to_i18_time(GtkCalendar *cal, gint hh, gint mm)
{
    struct tm tm_date = {0,0,0,0,0,0,0,0,-1};

    tm_date = orage_cal_to_tm_time(cal, hh, mm);
    return(orage_tm_time_to_i18_time(&tm_date));
}

char *orage_cal_to_i18_date(GtkCalendar *cal)
{
    struct tm tm_date = {0,0,0,0,0,0,0,0,-1};

    tm_date = orage_cal_to_tm_time(cal, 1, 1);
    return(orage_tm_date_to_i18_date(&tm_date));
}

char *orage_cal_to_icaldate(GtkCalendar *cal)
{
    struct tm tm_date = {0,0,0,0,0,0,0,0,-1};
    char *icalt;

    tm_date = orage_cal_to_tm_time(cal, 1, 1);
    icalt = orage_tm_time_to_icaltime(&tm_date);
    icalt[8] = '\0'; /* we know it is date */
    return(icalt);
}

struct tm orage_icaltime_to_tm_time(const char *icaltime, gboolean real_tm)
{
    struct tm t = {0,0,0,0,0,0,0,0,0};
    char *ret;

    ret = strptime(icaltime, "%Y%m%dT%H%M%S", &t);
    if (ret == NULL) {
        /* not all format string matched, so it must be DATE */
        /* and tm_wday is not calculated ! */
    /* need to fill missing tm_wday and tm_yday, which are in use 
     * in some locale's default date. For example in en_IN. mktime does it */
        if (mktime(&t) == (time_t) -1) {
            g_warning("orage: orage_icaltime_to_tm_time mktime failed %d %d %d"
                    , t.tm_year, t.tm_mon, t.tm_mday);
        }
        t.tm_hour = -1;
        t.tm_min = -1;
        t.tm_sec = -1;
    }
    else if (ret[0] != 0) { /* icaltime was not processed completely */
        /* UTC times end to Z, which is ok */
        if (ret[0] != 'Z' || ret[1] != 0) /* real error */
            g_error("orage: orage_icaltime_to_tm_time error %s %s", icaltime
                    , ret);
    }

    if (!real_tm) { /* convert from standard tm format to "normal" format */
        t.tm_year += 1900;
        t.tm_mon += 1;
    }
    return(t);
}

char *orage_tm_time_to_icaltime(struct tm *t)
{
    static char icaltime[XFICAL_APPT_TIME_FORMAT_LEN];

    g_sprintf(icaltime, XFICAL_APPT_TIME_FORMAT
            , t->tm_year + 1900, t->tm_mon + 1, t->tm_mday
            , t->tm_hour, t->tm_min, t->tm_sec);

    return(icaltime);
}

char *orage_icaltime_to_i18_time(const char *icaltime)
{ /* timezone is not converted */
    struct tm t;
    char      *ct;

    t = orage_icaltime_to_tm_time(icaltime, TRUE);
    if (t.tm_hour == -1)
        ct = orage_tm_date_to_i18_date(&t);
    else
        ct = orage_tm_time_to_i18_time(&t);
    return(ct);
}

char *orage_icaltime_to_i18_time_only(const char *icaltime)
{
    struct tm t;
    static char i18_time[10];

    t = orage_icaltime_to_tm_time(icaltime, TRUE);
    if (strftime(i18_time, 10, "%R", &t) == 0)
        g_error("Orage: orage_icaltime_to_i18_time_short too long string in strftime");
    return(i18_time);
}

char *orage_i18_time_to_icaltime(const char *i18_time)
{
    struct tm t;
    char      *ct;

    t = orage_i18_time_to_tm_time(i18_time);
    ct = orage_tm_time_to_icaltime(&t);
    return(ct);
}

char *orage_i18_date_to_icaldate(const char *i18_date)
{
    struct tm t;
    char      *icalt;

    t = orage_i18_date_to_tm_date(i18_date);
    icalt = orage_tm_time_to_icaltime(&t);
    icalt[8] = '\0'; /* we know it is date */
    return(icalt);
}

struct tm *orage_localtime(void)
{
    time_t tt;

    tt = time(NULL);
    return(localtime(&tt));
}

char *orage_localdate_i18(void)
{
    struct tm *t;

    t = orage_localtime();
    return(orage_tm_date_to_i18_date(t));
}

/* move one day forward or backward */
void orage_move_day(struct tm *t, int day)
{
    gint monthdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (day < -1 || day > 1) {
        g_warning("orage: orage_move_day wrong parameter %d", day);
    }
    t->tm_year += 1900;
    if (((t->tm_year%4) == 0) 
    && (((t->tm_year%100) != 0) || ((t->tm_year%400) == 0)))
        ++monthdays[1]; /* leap year, february has 29 days */

    t->tm_mday += day; /* may be negative */
    if (t->tm_mday == 0) { /*  we went to previous month */
        if (--t->tm_mon == -1) { /* previous year */
            --t->tm_year;
            t->tm_mon = 11;
        }
        t->tm_mday = monthdays[t->tm_mon];
    }
    else if (t->tm_mday > (monthdays[t->tm_mon])) { /* next month */
        if (++t->tm_mon == 12) {
            ++t->tm_year;
            t->tm_mon = 0;
        }
        t->tm_mday = 1;
    }
    t->tm_year -= 1900;
    /* need to fill missing tm_wday and tm_yday, which are in use 
     * in some locale's default date. For example in en_IN. mktime does it */
    if (mktime(t) == (time_t) -1) {
        g_warning("orage: orage_move_day mktime failed %d %d %d"
                , t->tm_year, t->tm_mon, t->tm_mday);
    }
}

gint orage_days_between(struct tm *t1, struct tm *t2)
{
    GDate *g_t1, *g_t2;
    gint dd;

    g_t1 = g_date_new_dmy(t1->tm_mday, t1->tm_mon, t1->tm_year);
    g_t2 = g_date_new_dmy(t2->tm_mday, t2->tm_mon, t2->tm_year);
    dd = g_date_days_between(g_t1, g_t2);
    g_date_free(g_t1);
    g_date_free(g_t2);
    return(dd);
}

void orage_select_date(GtkCalendar *cal
    , guint year, guint month, guint day)
{
    guint cur_year, cur_month, cur_mday;

    gtk_calendar_get_date(cal, &cur_year, &cur_month, &cur_mday);

    if (cur_year == year && cur_month == month)
        gtk_calendar_select_day(cal, day);
    else {
        gtk_calendar_select_day(cal, 0); /* need to avoid illegal day/month */
        gtk_calendar_select_month(cal, month, year);
        gtk_calendar_select_day(cal, day);
    }
}

void orage_select_today(GtkCalendar *cal)
{
    struct tm *t;

    t = orage_localtime();
    orage_select_date(cal, t->tm_year+1900, t->tm_mon, t->tm_mday);
}

/*******************************************************
 * data and config file locations
 *******************************************************/

gboolean orage_copy_file(gchar *source, gchar *target)
{
    gchar *text;
    gsize text_len;
    GError *error = NULL;
    gboolean ok = TRUE;

    /* read file */
    if (!g_file_get_contents(source, &text, &text_len, &error)) {
        orage_message(150, "orage_copy_file: Could not open file (%s) error:%s"
                , source, error->message);
        g_error_free(error);
        ok = FALSE;
    }
    /* write file */
    if (!g_file_set_contents(target, text, -1, &error)) {
        orage_message(150, "orage_copy_file: Could not write file (%s) error:%s"
                , target, error->message);
        g_error_free(error);
        ok = FALSE;
    }
    g_free(text);
    return(ok);
}

gchar *orage_xdg_system_data_file_location(char *name)
{
    char *file_name;
    const gchar * const *base_dirs;
    int i;

    base_dirs = g_get_system_data_dirs();
    for (i = 0; base_dirs[i] != NULL; i++) {
        file_name = g_build_filename(base_dirs[i], name, NULL);
        if (g_file_test(file_name, G_FILE_TEST_IS_REGULAR)) {
            return(file_name);
        }
        g_free(file_name);
    }

    /* no system wide data file */
    return(NULL);
}

/* Returns valid xdg local data file name. 
   If the file did not exist, it checks if there are system defaults 
   and creates it from those */
gchar *orage_data_file_location(char *name)
{
    char *file_name, *dir_name, *sys_name;
    const char *base_dir;
    int mode = 0700;

    base_dir = g_get_user_data_dir();
    file_name = g_build_filename(base_dir, name, NULL);
    if (!g_file_test(file_name, G_FILE_TEST_IS_REGULAR)) {
        /* it does not exist, let's try to create it */
        dir_name = g_path_get_dirname((const gchar *)file_name);
        if (g_mkdir_with_parents(dir_name, mode)) {
            orage_message(150, "orage_data_file_location: (%s) (%s) directory creation failed.\n", base_dir, file_name);
        }
        g_free(dir_name);
        /* now we have the directories ready, let's check for system default */
        sys_name = orage_xdg_system_data_file_location(name);
        if (sys_name) { /* we found it, we need to copy it */
            orage_copy_file(sys_name, file_name);
        }
    }

    return(file_name);
}

/* these are default files and must only be read */
gchar *orage_xdg_system_config_file_location(char *name)
{
    char *file_name;
    const gchar * const *base_dirs;
    int i;

    base_dirs = g_get_system_config_dirs();
    for (i = 0; base_dirs[i] != NULL; i++) {
        file_name = g_build_filename(base_dirs[i], name, NULL);
        if (g_file_test(file_name, G_FILE_TEST_IS_REGULAR)) {
            return(file_name);
        }
        g_free(file_name);
    }

    /* no system wide config file */
    return(NULL);
}

/* Returns valid xdg local config file name. 
   If the file did not exist, it checks if there are system defaults 
   and creates it from those */
gchar *orage_config_file_location(char *name)
{
    char *file_name, *dir_name, *sys_name;
    const char *base_dir;
    int mode = 0700;

    base_dir = g_get_user_config_dir();
    file_name = g_build_filename(base_dir, name, NULL);
    if (!g_file_test(file_name, G_FILE_TEST_IS_REGULAR)) {
        /* it does not exist, let's try to create it */
        dir_name = g_path_get_dirname((const gchar *)file_name);
        if (g_mkdir_with_parents(dir_name, mode)) {
            orage_message(150, "orage_config_file_location: (%s) (%s) directory creation failed.\n", base_dir, file_name);
        }
        g_free(dir_name);
        /* now we have the directories ready, let's check for system default */
        sys_name = orage_xdg_system_config_file_location(name);
        if (sys_name) { /* we found it, we need to copy it */
            orage_copy_file(sys_name, file_name);
        }
    }

    return(file_name);
}

/*******************************************************
 * rc file interface
 *******************************************************/

OrageRc *orage_rc_file_open(char *fpath, gboolean read_only)
{
    /* XfceRc *rc; */
    OrageRc *orc = NULL;
    GKeyFile *grc;
    GError *error = NULL;

    grc = g_key_file_new();
    if (g_key_file_load_from_file(grc, (const gchar *)fpath
            , G_KEY_FILE_KEEP_COMMENTS, &error)) { /* success */
        orc = g_new(OrageRc, 1);
        orc->rc = grc;
        orc->read_only = read_only;
        orc->file_name = g_strdup(fpath);
        orc->cur_group = NULL;
    }
    else {
#ifdef ORAGE_DEBUG
        orage_message(-90, "orage_rc_file_open: Unable to open RC file (%s). Creating it. (%s)", fpath, error->message);
#endif
        g_clear_error(&error);
        if (g_file_set_contents((const gchar *)fpath, "#Created by Orage", -1
                    , &error)) { /* successfully created new file */
            orc = g_new(OrageRc, 1);
            orc->rc = grc;
            orc->read_only = read_only;
            orc->file_name = g_strdup(fpath);
            orc->cur_group = NULL;
        }
        else {
#ifdef ORAGE_DEBUG
            orage_message(150, "orage_rc_file_open: Unable to open (create) RC file (%s). (%s)", fpath, error->message);
#endif
            g_key_file_free(grc);
        }
    }

    return(orc);
}

void orage_rc_file_close(OrageRc *orc)
    /* FIXME: check if file contents have been changed and only write when
       needed or build separate save function */
{
    GError *error = NULL;
    gchar *file_content = NULL;
    gsize length;

    if (orc) {
        /* xfce_rc_close((XfceRc *)orc->rc); */
        if (!orc->read_only) { /* need to write it */
            file_content = g_key_file_to_data(orc->rc, &length, NULL);
            if (file_content 
            && !g_file_set_contents(orc->file_name, file_content, -1
                , &error)) { /* write needed and failed */
                orage_message(150, "orage_rc_file_close: File save failed. RC file (%s). (%s)", orc->file_name, error->message);
            }
            g_free(file_content);
        }
        g_key_file_free(orc->rc);
        g_free((void *)orc->file_name);
        g_free((void *)orc->cur_group);
        g_free(orc);
    }
    else {
#ifdef ORAGE_DEBUG
        orage_message(-90, "orage_rc_file_close: closing empty file.");
#endif
    }
}

gchar **orage_rc_get_groups(OrageRc *orc)
{
    return(g_key_file_get_groups((GKeyFile *)orc->rc, NULL));
}

void orage_rc_set_group(OrageRc *orc, char *grp)
{
    g_free((void *)orc->cur_group);
    orc->cur_group = g_strdup(grp);
}

void orage_rc_del_group(OrageRc *orc, char *grp)
{
    GError *error = NULL;

    if (!g_key_file_remove_group((GKeyFile *)orc->rc, (const gchar *)grp
                , &error)) {
#ifdef ORAGE_DEBUG
        orage_message(150, "orage_rc_del_group: Group remove failed. RC file (%s). group (%s) (%s)", orc->file_name, grp, error->message);
#endif
    }
}

gchar *orage_rc_get_group(OrageRc *orc)
{
    return(g_strdup(orc->cur_group));
}

gchar *orage_rc_get_str(OrageRc *orc, char *key, char *def)
{
    GError *error = NULL;
    gchar *ret;

    ret = g_key_file_get_string((GKeyFile *)orc->rc, orc->cur_group
            , (const gchar *)key, &error);
    if (!ret && error) {
        ret = g_strdup(def);
#ifdef ORAGE_DEBUG
        orage_message(-90, "orage_rc_get_str: str (%s) group (%s) in RC file (%s) not found, using default (%s)", key, orc->cur_group, orc->file_name, ret);
#endif
    }
    return(ret);
}

gint orage_rc_get_int(OrageRc *orc, char *key, gint def)
{
    GError *error = NULL;
    gint ret;

    ret = g_key_file_get_integer((GKeyFile *)orc->rc, orc->cur_group
            , (const gchar *)key, &error);
    if (!ret && error) {
        ret = def;
#ifdef ORAGE_DEBUG
        orage_message(-90, "orage_rc_get_int: str (%s) group (%s) in RC file (%s) not found, using default (%d)", key, orc->cur_group, orc->file_name, ret);
#endif
    }
    return(ret);
}

gboolean orage_rc_get_bool(OrageRc *orc, char *key, gboolean def)
{
    GError *error = NULL;
    gboolean ret;

    ret = g_key_file_get_boolean((GKeyFile *)orc->rc, orc->cur_group
            , (const gchar *)key, &error);
    if (!ret && error) {
        ret = def;
#ifdef ORAGE_DEBUG
        orage_message(-90, "orage_rc_get_bool: str (%s) group (%s) in RC file (%s) not found, using default (%d)", key, orc->cur_group, orc->file_name, ret);
#endif
    }
    return(ret);
}

void orage_rc_put_str(OrageRc *orc, char *key, char *val)
{
    if (ORAGE_STR_EXISTS(val))
        g_key_file_set_string((GKeyFile *)orc->rc, orc->cur_group
                , (const gchar *)key, (const gchar *)val);
}

void orage_rc_put_int(OrageRc *orc, char *key, gint val)
{
    g_key_file_set_integer((GKeyFile *)orc->rc, orc->cur_group
            , (const gchar *)key, val);
}

void orage_rc_put_bool(OrageRc *orc, char *key, gboolean val)
{
    g_key_file_set_boolean((GKeyFile *)orc->rc, orc->cur_group
            , (const gchar *)key, val);
}

gboolean orage_rc_exists_item(OrageRc *orc, char *key)
{
    return(g_key_file_has_key((GKeyFile *)orc->rc, orc->cur_group
            , (const gchar *)key, NULL));
}

void orage_rc_del_item(OrageRc *orc, char *key)
{
    g_key_file_remove_key((GKeyFile *)orc->rc, orc->cur_group
            , (const gchar *)key, NULL);
}

/*******************************************************
 * dialog functions
 *******************************************************/

gint orage_info_dialog(GtkWindow *parent
        , char *primary_text, char *secondary_text)
{
    GtkWidget *dialog;
    gint result;

    dialog = gtk_message_dialog_new(parent
            , GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT
            , GTK_MESSAGE_INFO
            , GTK_BUTTONS_OK
            , "%s", primary_text);
    if (secondary_text) 
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog)
                , "%s", secondary_text);
    result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return(result);
}

gint orage_warning_dialog(GtkWindow *parent
        , char *primary_text, char *secondary_text)
{
    GtkWidget *dialog;
    gint result;

    dialog = gtk_message_dialog_new(parent
            , GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT
            , GTK_MESSAGE_WARNING
            , GTK_BUTTONS_YES_NO
            , "%s", primary_text);
    if (secondary_text) 
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog)
                , "%s", secondary_text);
    result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return(result);
}

gint orage_error_dialog(GtkWindow *parent
        , char *primary_text, char *secondary_text)
{
    GtkWidget *dialog;
    gint result;

    dialog = gtk_message_dialog_new(parent
            , GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT
            , GTK_MESSAGE_ERROR
            , GTK_BUTTONS_OK
            , "%s", primary_text);
    if (secondary_text) 
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog)
                , "%s", secondary_text);
    result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return(result);
}
