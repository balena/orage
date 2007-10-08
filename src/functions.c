/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2005-2007 Juha Kautto  (juha at xfce.org)
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

#include <stdio.h>
#include <stdlib.h>
#define _XOPEN_SOURCE /* glibc2 needs this */
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

/*
#include <libxfce4util/libxfce4util.h>
*/

#include "orage-i18n.h"
#include "functions.h"

/**************************************
 *  General purpose helper functions  *
 **************************************/

gboolean orage_date_button_clicked(GtkWidget *button, GtkWidget *win)
{
    GtkWidget *selDate_Window_dialog;
    GtkWidget *selDate_Calendar_calendar;
    gint result;
    char *date_to_display=NULL;
    /*
    struct tm *t;
    */
    struct tm cur_t;
    gboolean changed;

    selDate_Window_dialog = gtk_dialog_new_with_buttons(
            _("Pick the date"), GTK_WINDOW(win),
            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
            _("Today"),
            1,
            GTK_STOCK_OK,
            GTK_RESPONSE_ACCEPT,
            NULL);

    selDate_Calendar_calendar = gtk_calendar_new();
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(selDate_Window_dialog)->vbox)
            , selDate_Calendar_calendar);

    cur_t = orage_i18_date_to_tm_date(gtk_button_get_label(
            GTK_BUTTON(button)));
    orage_select_date(GTK_CALENDAR(selDate_Calendar_calendar)
            , cur_t.tm_year+1900, cur_t.tm_mon, cur_t.tm_mday);
    gtk_widget_show_all(selDate_Window_dialog);

    result = gtk_dialog_run(GTK_DIALOG(selDate_Window_dialog));
    switch(result){
        case GTK_RESPONSE_ACCEPT:
            /*
            gtk_calendar_get_date(GTK_CALENDAR(selDate_Calendar_calendar)
                    , (guint *)&cur_t.tm_year, (guint *)&cur_t.tm_mon
                    , (guint *)&cur_t.tm_mday);
            cur_t.tm_year -= 1900;
            date_to_display = orage_tm_date_to_i18_date(&cur_t);
            */
            date_to_display = orage_cal_to_i18_date(selDate_Calendar_calendar);
            break;
        case 1:
            /*
            t = orage_localtime();
            date_to_display = orage_tm_date_to_i18_date(t);
            */
            date_to_display = orage_localdate_i18();
            break;
        case GTK_RESPONSE_DELETE_EVENT:
        default:
            date_to_display = (gchar *)gtk_button_get_label(
                    GTK_BUTTON(button));
            break;
    }
    if (g_ascii_strcasecmp((gchar *)date_to_display
            , (gchar *)gtk_button_get_label(GTK_BUTTON(button))) != 0)
        changed = TRUE;
    else
        changed = FALSE;
    gtk_button_set_label(GTK_BUTTON(button), (const gchar *)date_to_display);
    gtk_widget_destroy(selDate_Window_dialog);
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
        return FALSE;

    if (!argv || !argv[0])
        return FALSE;

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

struct tm orage_i18_date_to_tm_date(const char *i18_date)
{
    char *ret;
    struct tm tm_date = {0,0,0,0,0,0,0,0,0};

    ret = (char *)strptime(i18_date, "%x", &tm_date);
    if (ret == NULL)
        g_error("Orage: orage_i18_date_to_tm_date wrong format (%s)", i18_date);
    else if (ret[0] != '\0')
        g_error("Orage: orage_i18_date_to_tm_date too long format (%s-%s)"
                , i18_date, ret);
    return(tm_date);
}

char *orage_tm_date_to_i18_date(struct tm *tm_date)
{
    static char i18_date[32];

    if (strftime(i18_date, 32, "%x", tm_date) == 0)
        g_error("Orage: orage_tm_date_to_i18_date too long string in strftime");
    return(i18_date);
}

char *orage_cal_to_i18_date(GtkCalendar *cal)
{
    struct tm tm_date = {0,0,0,0,0,0,0,0,0};

    gtk_calendar_get_date(cal
            , (unsigned int *)&tm_date.tm_year
            , (unsigned int *)&tm_date.tm_mon
            , (unsigned int *)&tm_date.tm_mday);
    tm_date.tm_year -= 1900;
    return(orage_tm_date_to_i18_date(&tm_date));
}

struct tm orage_icaltime_to_tm_time(const char *icaltime, gboolean real_tm)
{
    int i;
    struct tm t = {0,0,0,0,0,0,0,0,0};

    i = sscanf(icaltime, XFICAL_APPT_TIME_FORMAT
            , &t.tm_year, &t.tm_mon, &t.tm_mday
            , &t.tm_hour, &t.tm_min, &t.tm_sec);
    switch (i) {
        case 3: /* date */
            t.tm_hour = -1;
            t.tm_min = -1;
            t.tm_sec = -1;
            break;
        case 6: /* time */
            break;
        default: /* error */
            g_error("orage: orage_icaltime_to_tm_time error %s %d", icaltime, i);
            break;
    }
    if (real_tm) { /* normalise to standard tm format */
        t.tm_year -= 1900;
        t.tm_mon -= 1;
    }
    return(t);
}

char *orage_tm_time_to_icaltime(struct tm *t)
{
    static char icaltime[32];

    g_sprintf(icaltime, XFICAL_APPT_TIME_FORMAT
            , t->tm_year + 1900, t->tm_mon + 1, t->tm_mday
            , t->tm_hour, t->tm_min, t->tm_sec);

    return(icaltime);
}

char *orage_i18_date_to_icaltime(const char *i18_date)
{
    struct tm t;
    char      *ct;

    t = orage_i18_date_to_tm_date(i18_date);
    ct = orage_tm_time_to_icaltime(&t);
    ct[8] = '\0'; /* we know it is date */
    return(ct);
}

void orage_message(const char *format, ...)
{
    va_list args;
    char *formatted, *str;

    va_start(args, format);
    formatted = g_strdup_vprintf(format, args);
    va_end(args);

    g_message("Orage **: %s", formatted);
    g_free(formatted);
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

struct tm *orage_localtime()
{
    time_t tt;

    tt = time(NULL);
    return(localtime(&tt));
}

char *orage_localdate_i18()
{
    struct tm *t;

    t = orage_localtime();
    return(orage_tm_date_to_i18_date(t));
}

void orage_select_date(GtkCalendar *cal
    , guint year, guint month, guint day)
{
    gtk_calendar_select_day(cal, 0); /* needed to avoid illegal day/month */
    gtk_calendar_select_month(cal, month, year);
    gtk_calendar_select_day(cal, day);
}

void orage_select_today(GtkCalendar *cal)
{
    struct tm *t;

    t = orage_localtime();
    orage_select_date(cal, t->tm_year+1900, t->tm_mon, t->tm_mday);
}
