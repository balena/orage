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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <unistd.h>
#include <time.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <glib/gprintf.h>

#include "functions.h"
#include "mainbox.h"
#include "reminder.h"
#include "about-xfcalendar.h"
#include "ical-code.h"
#include "event-list.h"
#include "appointment.h"
#include "parameters.h"


/* Direction for changing day to look at */
enum {
    PREVIOUS,
    NEXT
};

enum {
    COL_TIME = 0
   ,COL_FLAGS
   ,COL_HEAD
   ,COL_UID
   ,COL_SORT
   ,NUM_COLS
};

enum {
    DRAG_TARGET_STRING = 0
   ,DRAG_TARGET_COUNT
};
static const GtkTargetEntry drag_targets[] =
{
    { "STRING", 0, DRAG_TARGET_STRING }
};

static void title_to_ical(char *title, char *ical)
{ /* yyyy-mm-dd\0 -> yyyymmdd\0 */
    gint i, j;

    for (i = 0, j = 0; i <= 8; i++) {
        if (i == 4 || i == 6)
            j++;
        ical[i] = title[j++];
    }
}

static void editEvent(GtkTreeView *view, GtkTreePath *path
        , GtkTreeViewColumn *col, gpointer user_data)
{
    el_win *el = (el_win *)user_data;
    appt_win *apptw;
    GtkTreeModel *model;
    GtkTreeIter   iter;
    gchar *uid = NULL, *flags = NULL;

    model = gtk_tree_view_get_model(view);
    if (gtk_tree_model_get_iter(model, &iter, path)) {
        gtk_tree_model_get(model, &iter, COL_UID, &uid, -1);
        gtk_tree_model_get(model, &iter, COL_FLAGS, &flags, -1);
        if (flags && flags[3] == 'A')
            xfical_unarchive_uid(uid);
        apptw = create_appt_win("UPDATE", uid, el);
        g_free(uid);
    }
}

static gint sortEvent_comp(GtkTreeModel *model
        , GtkTreeIter *i1, GtkTreeIter *i2, gpointer data)
{
    gint col = GPOINTER_TO_INT(data);
    gint ret;
    gchar *text1, *text2;

    gtk_tree_model_get(model, i1, col, &text1, -1);
    gtk_tree_model_get(model, i2, col, &text2, -1);
    ret = strcmp(text1, text2);
    g_free(text1);
    g_free(text2);
    return(ret);
}

static int append_date(char *result, char *ical_time, int i)
{
    result[i++] = ical_time[6];
    result[i++] = ical_time[7];
    result[i++] = '.';
    result[i++] = ical_time[4];
    result[i++] = ical_time[5];
    result[i++] = '.';
    result[i++] = ical_time[0];
    result[i++] = ical_time[1];
    result[i++] = ical_time[2];
    result[i++] = ical_time[3];
    result[i++] = ' ';
    return(11);
}

static int append_time(char *result, char *ical_time, int i)
{
    result[i++] = ical_time[9];
    result[i++] = ical_time[10];
    result[i++] = ':';
    result[i++] = ical_time[11];
    result[i++] = ical_time[12];
    result[i++] = ' ';
    return(6);
}

static int append_special_time(char *result, char *str, int i)
{
    result[i++] = str[0];
    result[i++] = str[1];
    result[i++] = str[2];
    result[i++] = str[3];
    result[i++] = str[4];
    result[i++] = str[5];
    return(6);
}

static char *format_time(el_win *el, xfical_appt *appt, char *par)
{
    static char result[40];
    int i = 0;
    char *start_ical_time;
    char *end_ical_time;
    gboolean same_date;

    start_ical_time = appt->starttimecur;
    end_ical_time = appt->endtimecur;
    same_date = !strncmp(start_ical_time, end_ical_time, 8);

    if (el->page == EVENT_PAGE && el->days == 0) { 
        /* special formatting for 1 day VEVENTS */
        if (start_ical_time[8] == 'T') { /* time part available */
            if (strncmp(start_ical_time, par, 8) < 0)
                i += append_special_time(result, "+00:00", i);
            else
                i += append_time(result, start_ical_time, i);
            result[i++] = '-';
            result[i++] = ' ';
            if (strncmp(par, end_ical_time , 8) < 0)
                i += append_special_time(result, "24:00+", i);
            else
                i += append_time(result, end_ical_time, i);
        }
        else {/* date only appointment */
            strncpy(result, _("All day"), 39);
            i = strlen(result); /* because we add null to i-pos */
        }
    }
    else { /* normally show date and time */
        i += append_date(result, start_ical_time, i);
        if (start_ical_time[8] == 'T') { /* time part available */
            i += append_time(result, start_ical_time, i);
            result[i++] = '-';
            result[i++] = ' ';
            if (!same_date) {
                i += append_date(result, end_ical_time, i);
                result[i++] = ' ';
            }
            i += append_time(result, end_ical_time, i);
        }
        else {/* date only */
            result[i++] = '-';
            result[i++] = ' ';
            i += append_date(result, end_ical_time, i);
        }
    }

    result[i++] = '\0';
    return(result);
}

static void start_time_data_func(GtkTreeViewColumn *col, GtkCellRenderer *rend
        , GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
    el_win *el = (el_win *)user_data;
    gchar *stime, *etime;

    if (!el->today || el->days != 0) { /* reset */
        g_object_set(rend
                 , "foreground-set",    FALSE
                 , "strikethrough-set", FALSE
                 , "weight-set",        FALSE
                 , NULL); 
        return;
    }

    gtk_tree_model_get(model, iter, COL_TIME, &stime, -1);
    if (stime[0] == '+')
        stime++;
    etime = stime + 8; /* hh:mm - hh:mm */
    /* only add special highlight if we are on today (=start with time)
     * and we process VEVENT 
     */
    if (stime[2] != ':' || el->page != EVENT_PAGE) { 
        g_object_set(rend
                 , "foreground-set",    FALSE
                 , "strikethrough-set", FALSE
                 , "weight-set",        FALSE
                 , NULL); 
    }
    else if (strncmp(etime, el->time_now, 5) < 0) { /* gone */
        g_object_set(rend
                 , "foreground-set",    FALSE
                 , "strikethrough",     TRUE
                 , "strikethrough-set", TRUE
                 , "weight",            PANGO_WEIGHT_LIGHT
                 , "weight-set",        TRUE
                 , NULL);
    }
    else if (strncmp(stime, el->time_now, 5) <= 0 
          && strncmp(etime, el->time_now, 5) >= 0) { /* current */
        g_object_set(rend
                 , "foreground",        "Red"
                 , "foreground-set",    TRUE
                 , "strikethrough-set", FALSE
                 , "weight",            PANGO_WEIGHT_BOLD
                 , "weight-set",        TRUE
                 , NULL);
    }
    else { /* future */
        g_object_set(rend
                 , "foreground-set",    FALSE
                 , "strikethrough-set", FALSE
                 , "weight",            PANGO_WEIGHT_BOLD
                 , "weight-set",        TRUE
                 , NULL);
    }
}

static void add_el_row(el_win *el, xfical_appt *appt, char *par)
{
    GtkTreeIter     iter1;
    GtkListStore   *list1;
    gchar          *title = NULL;
    gchar           flags[5]; 
    gchar          *stime;
    gchar          *s_sort, *s_sort1;
    gint            len = 50;

    stime = format_time(el, appt, par);
    if (appt->alarmtime != 0)
        if (appt->sound != NULL)
            flags[0] = 'S';
        else
            flags[0] = 'A';
    else
        flags[0] = 'n';

    if (appt->freq == XFICAL_FREQ_NONE)
        flags[1] = 'n';
    else if (appt->freq == XFICAL_FREQ_DAILY)
        flags[1] = 'D';
    else if (appt->freq == XFICAL_FREQ_WEEKLY)
        flags[1] = 'W';
    else if (appt->freq == XFICAL_FREQ_MONTHLY)
        flags[1] = 'M';
    else if (appt->freq == XFICAL_FREQ_YEARLY)
        flags[1] = 'Y';
    else
        flags[1] = 'n';

    if (appt->availability != 0)
        flags[2] = 'B';
    else
        flags[2] = 'f';

    if (strcmp(par, "archive") == 0)
        flags[3] = 'A';
    else
        flags[3] = 'n';

    flags[4] = '\0';

    if (appt->title != NULL)
        title = g_strdup(appt->title);
    else if (appt->note != NULL) { 
    /* let's take len chars of the first line from the text */
        if ((title = g_strstr_len(appt->note, strlen(appt->note), "\n")) 
            != NULL) {
            if ((strlen(appt->note)-strlen(title)) < len)
                len = strlen(appt->note)-strlen(title);
        }
        title = g_strndup(appt->note, len);
    }

    s_sort1 = g_strconcat(appt->starttimecur, appt->endtimecur, NULL);
    s_sort = g_utf8_collate_key(s_sort1, -1);

    list1 = el->ListStore;
    gtk_list_store_append(list1, &iter1);
    gtk_list_store_set(list1, &iter1
            , COL_TIME,  stime
            , COL_FLAGS, flags
            , COL_HEAD,  title
            , COL_UID,   appt->uid
            , COL_SORT,  s_sort
            , -1);
    g_free(title);
    g_free(s_sort1);
    g_free(s_sort);
}

static void search_data(el_win *el)
{
    gchar *search_string = NULL;
    gchar *text;
    gsize text_len;
    xfical_appt *appt;

    if (!xfical_file_open())
        return;
    search_string = g_strdup(gtk_entry_get_text((GtkEntry *)el->search_entry));
    for (appt = xfical_appt_get_next_with_string(search_string, TRUE, FALSE);
         appt;
         appt = xfical_appt_get_next_with_string(search_string, FALSE, FALSE)){
        add_el_row(el, appt, "main");
        xfical_appt_free(appt);
    }
    xfical_file_close();
    /* process always archive file also */
    if (!xfical_archive_open()) {
        g_free(search_string);
        return;
    }
    for (appt = xfical_appt_get_next_with_string(search_string, TRUE, TRUE);
         appt;
         appt = xfical_appt_get_next_with_string(search_string, FALSE, TRUE)) {
        add_el_row(el, appt, "archive");
        xfical_appt_free(appt);
    }
    xfical_archive_close();
    g_free(search_string);
}

static void get_data_rows(el_win *el, char *a_day, gboolean arch, char *par)
{
    xfical_appt *appt;
    xfical_type ical_type;

    if (!arch) {
        if (!xfical_file_open())
            return;
    }
    else {
        if (!xfical_archive_open())
            return;
    }

    switch (el->page) {
        case EVENT_PAGE:
            ical_type = XFICAL_TYPE_EVENT;
            break;
        case TODO_PAGE:
            ical_type = XFICAL_TYPE_TODO;
            break;
        case JOURNAL_PAGE:
            ical_type = XFICAL_TYPE_JOURNAL;
            break;
        default:
            g_error("wrong page in get_data_rows (%d)\n", el->page);
    }

    for (appt = xfical_appt_get_next_on_day(a_day, TRUE, el->days, ical_type
                , arch);
         appt;
         appt = xfical_appt_get_next_on_day(a_day, FALSE, el->days, ical_type
                , arch)) {
        add_el_row(el, appt, par);
        xfical_appt_free(appt);
    }

    if (!arch)
        xfical_file_close();
    else
        xfical_archive_close();
}

static void refresh_time_field(el_win *el)
{
    GtkCellRenderer *rend;
    GtkTreeViewColumn *col;

/* this is needed if we want to make time field smaller again */
    col = gtk_tree_view_get_column(GTK_TREE_VIEW(el->TreeView), 0);
    gtk_tree_view_remove_column(GTK_TREE_VIEW(el->TreeView), col);
    rend = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes( _("Time"), rend
            , "text", COL_TIME, NULL);
    gtk_tree_view_column_set_cell_data_func(col, rend
            , start_time_data_func, el, NULL);
    gtk_tree_view_insert_column(GTK_TREE_VIEW(el->TreeView), col, 0);
}

static void event_data(el_win *el)
{
    guint year, month, day;
    char      *title;
    char      a_day[9];  /* yyyymmdd */
    struct tm *t;

    if (el->days == 0)
        refresh_time_field(el);
    el->days = gtk_spin_button_get_value(GTK_SPIN_BUTTON(el->event_spin));
    title = (char *)gtk_window_get_title(GTK_WINDOW(el->Window));
    title_to_ical(title, a_day);
    if (sscanf(title, "%d-%d-%d", &year, &month, &day) != 3)
        g_warning("time_data: title conversion error\n");
    t = orage_localtime();
    g_sprintf(el->time_now, "%02d:%02d", t->tm_hour, t->tm_min);
    if (   year  == t->tm_year + 1900
        && month == t->tm_mon + 1
        && day   == t->tm_mday)
        el->today = TRUE;
    else
        el->today = FALSE; 

    get_data_rows(el, a_day, FALSE, a_day);
}

static void todo_data(el_win *el)
{
    guint year, month, day;
    char      *title;
    char      a_day[9];  /* yyyymmdd */
    struct tm *t;

    el->days = 10*365; /* long enough time to get everything from future */
    t = orage_localtime();
    g_sprintf(a_day, "%04d%02d%02d"
            , t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
    get_data_rows(el, a_day, FALSE, a_day);
}

void journal_data(el_win *el)
{
    guint year, month, day;
    char      a_day[9];  /* yyyymmdd */
    xfical_appt *appt;
    struct tm t;

    el->days = 10*365; /* long enough time to get everything from future */
    t = orage_i18_date_to_tm_date(gtk_button_get_label(
            GTK_BUTTON(el->journal_start_button)));
    g_sprintf(a_day, "%04d%02d%02d"
            , t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

    get_data_rows(el, a_day, FALSE, "main");
    get_data_rows(el, a_day, TRUE, "archive");
}

void refresh_el_win(el_win *el)
{
    if (el->Window && el->ListStore && el->TreeView) {
        gtk_list_store_clear(el->ListStore);
        el->page = gtk_notebook_get_current_page(GTK_NOTEBOOK(el->Notebook));
        switch (el->page) {
            case EVENT_PAGE:
                event_data(el);
                break;
            case TODO_PAGE:
                todo_data(el);
                break;
            case JOURNAL_PAGE:
                journal_data(el);
                break;
            case SEARCH_PAGE:
                search_data(el);
                break;
            default:
                g_warning("refresh_el_win: unknown tab");
                break;
        }
    }
}

static gboolean upd_notebook(el_win *el)
{
    static gint prev_page = -1;
    gint cur_page;

    /* we only show the page IF it is different than the last
     * page changed with this method
     */
    cur_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(el->Notebook));
    if (cur_page != prev_page) {
        prev_page = cur_page;
        refresh_el_win(el);
    }
    return(FALSE); /* we do this only once */
}

static void on_notebook_page_switch(GtkNotebook *notebook
        , GtkNotebookPage *page, guint page_num, gpointer user_data)
{
    el_win *el = (el_win *)user_data;

    /* we can't do refresh_el_win here directly since it is slow and
     * gtk_notebook_get_current_page points to old page at this time,
     * so we end up showing wrong page.
     * This timeout also makes it possible to avoid unnecessary 
     * refreshes when tab is change quickly; like with mouse roll
     */
    g_timeout_add(300, (GtkFunction)upd_notebook, el);
}

static void on_Search_clicked(GtkButton *b, gpointer user_data)
{
    el_win *el = (el_win *)user_data;

    gtk_notebook_set_current_page(GTK_NOTEBOOK(el->Notebook), SEARCH_PAGE);
    refresh_el_win((el_win *)user_data);
}

static void on_View_search_activate_cb(GtkMenuItem *mi, gpointer user_data)
{
    el_win *el = (el_win *)user_data;

    gtk_notebook_set_current_page(GTK_NOTEBOOK(el->Notebook), SEARCH_PAGE);
    refresh_el_win((el_win *)user_data);
}

static void set_el_data_from_cal(el_win *el)
{
    char title[12];
    guint year, month, day;

    gtk_calendar_get_date(GTK_CALENDAR(g_par.xfcal->mCalendar)
            , &year, &month, &day);
    g_sprintf(title, "%04d-%02d-%02d", year, month+1, day);
    gtk_window_set_title(GTK_WINDOW(el->Window), (const gchar*)title);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(el->Notebook), EVENT_PAGE);
    refresh_el_win(el);
}

static void duplicate_appointment(el_win *el)
{
    GtkTreeSelection *sel;
    GtkTreeModel     *model;
    GtkTreePath      *path;
    GtkTreeIter       iter;
    GList *list;
    gint  list_len, i;
    gchar *uid = NULL, *flags = NULL;
    appt_win *apptw;

    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(el->TreeView));
    list = gtk_tree_selection_get_selected_rows(sel, &model);
    list_len = g_list_length(list);
    if (list_len > 0) {
        if (list_len > 1)
            g_warning("Copy: too many rows selected\n");
        path = (GtkTreePath *)g_list_nth_data(list, 0);
        if (gtk_tree_model_get_iter(model, &iter, path)) {
            gtk_tree_model_get(model, &iter, COL_UID, &uid, -1);
            gtk_tree_model_get(model, &iter, COL_FLAGS, &flags, -1);
            if (flags && flags[3] == 'A')
                xfical_unarchive_uid(uid);
            apptw = create_appt_win("COPY", uid, el);
            g_free(uid);
        }
    }
    else
        g_warning("Copy: No row selected\n");
    g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
    g_list_free(list);
}

static void on_Copy_clicked(GtkButton *b, gpointer user_data)
{
    duplicate_appointment((el_win *)user_data);
}

static void on_File_duplicate_activate_cb(GtkMenuItem *mi, gpointer user_data)
{
    duplicate_appointment((el_win *)user_data);
}

static void close_window(el_win *el)
{
    gtk_window_get_size(GTK_WINDOW(el->Window)
            , &g_par.el_size_x, &g_par.el_size_y);
    write_parameters();

    gtk_widget_destroy(el->Window); /* destroy the eventlist window */
    g_free(el);
    el = NULL;
}

static void on_Close_clicked(GtkButton *b, gpointer user_data)
{
    close_window((el_win *)user_data);
}

static void on_File_close_activate_cb(GtkMenuItem *mi, gpointer user_data)
{
    close_window((el_win *)user_data);
}

static gboolean on_Window_delete_event(GtkWidget *w, GdkEvent *e
        , gpointer user_data)
{
    close_window((el_win *)user_data);
    return(FALSE);
}

static void on_Refresh_clicked(GtkButton *b, gpointer user_data)
{
    refresh_el_win((el_win*)user_data);
}

static void on_View_refresh_activate_cb(GtkMenuItem *mi, gpointer user_data)
{
    refresh_el_win((el_win*)user_data);
}

static void changeSelectedDate(el_win *el, gint direction)
{
    guint year, month, day;
    guint monthdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    char *title;

    title = (char *)gtk_window_get_title(GTK_WINDOW(el->Window));
    if (sscanf(title, "%d-%d-%d", &year, &month, &day) != 3)
        g_warning("changeSelectedDate: title conversion error\n");
    month--; /* gtk calendar starts from 0  month */
    if (((year%4) == 0) && (((year%100) != 0) || ((year%400) == 0)))
        ++monthdays[1];

    switch(direction) {
        case PREVIOUS:
            if (--day == 0) {
                if (--month == -1) {
                    --year;
                    month = 11;
                }
                day = monthdays[month];
            }
        break;
        case NEXT:
            if (++day == (monthdays[month]+1)) {
                if (++month == 12) {
                    ++year;
                    month = 0;
                }
                day = 1;
            }
        break;
        default:
        break;
    }
    orage_select_date(GTK_CALENDAR(g_par.xfcal->mCalendar), year, month, day);
    set_el_data_from_cal(el);
}

static void on_Previous_clicked(GtkButton *b, gpointer user_data)
{
  changeSelectedDate((el_win *)user_data, PREVIOUS);
}

static void on_Go_previous_activate_cb(GtkMenuItem *mi, gpointer user_data)
{
  changeSelectedDate((el_win *)user_data, PREVIOUS);
}

static void go_to_today(el_win *el)
{
    orage_select_today(GTK_CALENDAR(g_par.xfcal->mCalendar));
    set_el_data_from_cal(el);
}

static void on_Today_clicked(GtkButton *b, gpointer user_data)
{
    go_to_today((el_win *)user_data);
}

static void on_Go_today_activate_cb(GtkMenuItem *mi, gpointer user_data)
{
    go_to_today((el_win *)user_data);
}

static void on_Next_clicked(GtkButton *b, gpointer user_data)
{
  changeSelectedDate((el_win *)user_data, NEXT);
}

static void on_Go_next_activate_cb(GtkMenuItem *mi, gpointer user_data)
{
  changeSelectedDate((el_win *)user_data, NEXT);
}

static void create_new_appointment(el_win *el)
{
    appt_win *apptw;
    char *title, a_day[10];

    title = (char *)gtk_window_get_title(GTK_WINDOW(el->Window));
    title_to_ical(title, a_day);

    apptw = create_appt_win("NEW", a_day, el);
    gtk_widget_show(apptw->Window);
}

static void on_File_newApp_activate_cb(GtkMenuItem *mi, gpointer user_data)
{
    create_new_appointment((el_win *)user_data);
}

static void on_Create_toolbutton_clicked_cb(GtkButton *b, gpointer user_data)
{
    create_new_appointment((el_win *)user_data);
}

static void on_spin_changed(GtkSpinButton *b, gpointer user_data)
{
    refresh_el_win((el_win *)user_data);
}

static void delete_appointment(el_win *el)
{
    gint result;
    GtkTreeSelection *sel;
    GtkTreeModel     *model;
    GtkTreePath      *path;
    GtkTreeIter       iter;
    GList *list;
    gint  list_len, i;
    gchar *uid = NULL;

    result = xfce_message_dialog(GTK_WINDOW(el->Window),
             _("Warning"),
             GTK_STOCK_DIALOG_WARNING,
             _("You will permanently remove all\nselected appointments."),
             _("Do you want to continue?"),
             GTK_STOCK_YES, GTK_RESPONSE_ACCEPT,
             GTK_STOCK_NO, GTK_RESPONSE_CANCEL,
             NULL);

    if (result == GTK_RESPONSE_ACCEPT) {
        if (!xfical_file_open())
            return;
        sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(el->TreeView));
        list = gtk_tree_selection_get_selected_rows(sel, &model);
        list_len = g_list_length(list);
        for (i = 0; i < list_len; i++) {
            path = (GtkTreePath *)g_list_nth_data(list, i);
            if (gtk_tree_model_get_iter(model, &iter, path)) {
                gtk_tree_model_get(model, &iter, COL_UID, &uid, -1);
                result = xfical_appt_del(uid);
                if (result)
                    g_message("Orage **: Removed: %s", uid);
                else
                    g_warning("Removal failed: %s", uid);
                g_free(uid);
            }
        }
        xfical_file_close();
        refresh_el_win(el);
        orage_mark_appointments();
        g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
        g_list_free(list);
    }
}

static void on_Delete_clicked(GtkButton *b, gpointer user_data)
{
    delete_appointment((el_win *)user_data);
}

static void on_File_delete_activate_cb(GtkMenuItem *mi, gpointer user_data)
{
    delete_appointment((el_win *)user_data);
}

static void on_journal_start_button_clicked(GtkButton *button
        , gpointer *user_data)
{
    el_win *el = (el_win *)user_data;
    GtkWidget *selDate_Window_dialog;
    GtkWidget *selDate_Calendar_calendar;
    gint result;
    guint year, month, day;
    char *date_to_display = NULL;
    struct tm *t, cur_t;

    selDate_Window_dialog = gtk_dialog_new_with_buttons(
            _("Pick the date"), GTK_WINDOW(el->Window),
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
            gtk_calendar_get_date(GTK_CALENDAR(selDate_Calendar_calendar)
                    , (guint *)&cur_t.tm_year, (guint *)&cur_t.tm_mon
                    , (guint *)&cur_t.tm_mday);
            cur_t.tm_year -= 1900;
            date_to_display = orage_tm_date_to_i18_date(cur_t);
            break;
        case 1:
            t = orage_localtime();
            date_to_display = orage_tm_date_to_i18_date(*t);
            break;
        case GTK_RESPONSE_DELETE_EVENT:
        default:
            date_to_display = (gchar *)gtk_button_get_label(button);
            break;
    }
    gtk_button_set_label(button, (const gchar *)date_to_display);
    refresh_el_win(el);
    gtk_widget_destroy(selDate_Window_dialog);
}

static void drag_data_get(GtkWidget *widget, GdkDragContext *context
        , GtkSelectionData *selection_data, guint info, guint time
        , gpointer user_data)
{
    GtkTreeSelection *sel;
    GtkTreeModel     *model;
    GtkTreePath      *path;
    GtkTreeIter       iter;
    GList *list;
    gint  list_len, i;
    gchar *uid = NULL;
    GString *result = NULL;

    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
    list = gtk_tree_selection_get_selected_rows(sel, &model);
    list_len = g_list_length(list);
    for (i = 0; i < list_len; i++) {
        path = (GtkTreePath *)g_list_nth_data(list, i);
        if (gtk_tree_model_get_iter(model, &iter, path)) {
            gtk_tree_model_get(model, &iter, COL_UID, &uid, -1);
            if (i == 0) { /* first */
                result = g_string_new(uid);
            }
            else {
                g_string_append(result, ",");
                g_string_append(result, uid);
            }
            g_free(uid);
        }
    }
    if (!gtk_selection_data_set_text(selection_data, result->str, -1))
        g_warning("drag_data_get failed\n");
    g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
    g_list_free(list);
    g_string_free(result, TRUE);
}

static void build_menu(el_win *el)
{
    GtkWidget *menu_separator;

    el->Menubar = gtk_menu_bar_new();
    gtk_box_pack_start(GTK_BOX(el->Vbox), el->Menubar, FALSE, FALSE, 0);

    /* File menu */
    el->File_menu = orage_menu_new(_("_File"), el->Menubar);
    el->File_menu_new = orage_image_menu_item_new_from_stock("gtk-new"
            , el->File_menu, el->accel_group);

    menu_separator = orage_separator_menu_item_new(el->File_menu);

    /* add event copying and day cleaning */
    el->File_menu_duplicate = orage_menu_item_new_with_mnemonic(_("D_uplicate")
            , el->File_menu);
    gtk_widget_add_accelerator(el->File_menu_duplicate
            , "activate", el->accel_group
            , GDK_d, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    menu_separator = orage_separator_menu_item_new(el->File_menu);

    el->File_menu_delete = orage_image_menu_item_new_from_stock("gtk-delete"
            , el->File_menu, el->accel_group);

    menu_separator = orage_separator_menu_item_new(el->File_menu);

    el->File_menu_close = orage_image_menu_item_new_from_stock("gtk-close"
            , el->File_menu, el->accel_group);

    /* View menu */
    el->View_menu = orage_menu_new(_("_View"), el->Menubar);
    el->View_menu_refresh = orage_image_menu_item_new_from_stock ("gtk-refresh"
            , el->View_menu, el->accel_group);
    gtk_widget_add_accelerator(el->View_menu_refresh
            , "activate", el->accel_group
            , GDK_r, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(el->View_menu_refresh
            , "activate", el->accel_group
            , GDK_Return, 0, 0);
    gtk_widget_add_accelerator(el->View_menu_refresh
            , "activate", el->accel_group
            , GDK_KP_Enter, 0, 0);

    menu_separator = orage_separator_menu_item_new(el->View_menu);

    el->View_menu_search = orage_image_menu_item_new_from_stock("gtk-find"
            , el->View_menu, el->accel_group);

    /* Go menu   */
    el->Go_menu = orage_menu_new(_("_Go"), el->Menubar);
    el->Go_menu_today = orage_image_menu_item_new_from_stock("gtk-home"
            , el->Go_menu, el->accel_group);
    gtk_widget_add_accelerator(el->Go_menu_today
            , "activate", el->accel_group
            , GDK_Home, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
    el->Go_menu_prev = orage_image_menu_item_new_from_stock("gtk-go-back"
            , el->Go_menu, el->accel_group);
    gtk_widget_add_accelerator(el->Go_menu_prev
            , "activate", el->accel_group
            , GDK_b, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    el->Go_menu_next = orage_image_menu_item_new_from_stock("gtk-go-forward"
            , el->Go_menu, el->accel_group);
    gtk_widget_add_accelerator(el->Go_menu_next
            , "activate", el->accel_group
            , GDK_f, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    g_signal_connect((gpointer)el->File_menu_new, "activate"
            , G_CALLBACK(on_File_newApp_activate_cb), el);
    g_signal_connect((gpointer)el->File_menu_duplicate, "activate"
            , G_CALLBACK(on_File_duplicate_activate_cb), el);
    g_signal_connect((gpointer)el->File_menu_delete, "activate"
            , G_CALLBACK(on_File_delete_activate_cb), el);
    g_signal_connect((gpointer)el->File_menu_close, "activate"
            , G_CALLBACK(on_File_close_activate_cb), el);
    g_signal_connect((gpointer)el->View_menu_refresh, "activate"
            , G_CALLBACK(on_View_refresh_activate_cb), el);
    g_signal_connect((gpointer)el->View_menu_search, "activate"
            , G_CALLBACK(on_View_search_activate_cb), el);
    g_signal_connect((gpointer)el->Go_menu_today, "activate"
            , G_CALLBACK(on_Go_today_activate_cb), el);
    g_signal_connect((gpointer)el->Go_menu_prev, "activate"
            , G_CALLBACK(on_Go_previous_activate_cb), el);
    g_signal_connect((gpointer)el->Go_menu_next, "activate"
            , G_CALLBACK(on_Go_next_activate_cb), el);
}

static void build_toolbar(el_win *el)
{
    GtkWidget *toolbar_separator;
    gint i = 0;

    el->Toolbar = gtk_toolbar_new();
    gtk_box_pack_start(GTK_BOX(el->Vbox), el->Toolbar, FALSE, FALSE, 0);
    gtk_toolbar_set_tooltips(GTK_TOOLBAR(el->Toolbar), TRUE);

    el->Create_toolbutton = orage_toolbar_append_button(el->Toolbar
            , "gtk-new", el->Tooltips, _("New"), i++);
    el->Copy_toolbutton = orage_toolbar_append_button(el->Toolbar
            , "gtk-copy", el->Tooltips, _("Duplicate"), i++);
    el->Delete_toolbutton = orage_toolbar_append_button(el->Toolbar
            , "gtk-delete", el->Tooltips, _("Delete"), i++);

    toolbar_separator = orage_toolbar_append_separator(el->Toolbar, i++);
    
    el->Previous_toolbutton = orage_toolbar_append_button(el->Toolbar
            , "gtk-go-back", el->Tooltips, _("Back"), i++);
    el->Today_toolbutton = orage_toolbar_append_button(el->Toolbar
            , "gtk-home", el->Tooltips, _("Today"), i++);
    el->Next_toolbutton = orage_toolbar_append_button(el->Toolbar
            , "gtk-go-forward", el->Tooltips, _("Forward"), i++);

    toolbar_separator = orage_toolbar_append_separator(el->Toolbar, i++);

    el->Refresh_toolbutton = orage_toolbar_append_button(el->Toolbar
            , "gtk-refresh", el->Tooltips, _("Refresh"), i++);
    el->Search_toolbutton = orage_toolbar_append_button(el->Toolbar
            , "gtk-find", el->Tooltips, _("Find"), i++);

    toolbar_separator = orage_toolbar_append_separator(el->Toolbar, i++);

    el->Close_toolbutton = orage_toolbar_append_button(el->Toolbar
            , "gtk-close", el->Tooltips, _("Close"), i++);

    g_signal_connect((gpointer)el->Create_toolbutton, "clicked"
            , G_CALLBACK(on_Create_toolbutton_clicked_cb), el);
    g_signal_connect((gpointer)el->Copy_toolbutton, "clicked"
            , G_CALLBACK(on_Copy_clicked), el);
    g_signal_connect((gpointer)el->Delete_toolbutton, "clicked"
            , G_CALLBACK(on_Delete_clicked), el);
    g_signal_connect((gpointer)el->Previous_toolbutton, "clicked"
            , G_CALLBACK(on_Previous_clicked), el);
    g_signal_connect((gpointer)el->Today_toolbutton, "clicked"
            , G_CALLBACK(on_Today_clicked), el);
    g_signal_connect((gpointer)el->Next_toolbutton, "clicked"
            , G_CALLBACK(on_Next_clicked), el);
    g_signal_connect((gpointer)el->Refresh_toolbutton, "clicked"
            , G_CALLBACK(on_Refresh_clicked), el);
    g_signal_connect((gpointer)el->Search_toolbutton, "clicked"
            , G_CALLBACK(on_Search_clicked), el);
    g_signal_connect((gpointer)el->Close_toolbutton, "clicked"
            , G_CALLBACK(on_Close_clicked), el);
}

static void build_event_tab(el_win *el)
{
    gint row;
    GtkWidget *label, *hbox;

    el->event_tab_label = gtk_label_new(_("Event"));
    el->event_notebook_page = orage_table_new(2, 10);

    label = gtk_label_new(_("Extra days to show "));
    hbox =  gtk_hbox_new(FALSE, 0);
    el->event_spin = gtk_spin_button_new_with_range(0, 31, 1);
    gtk_box_pack_start(GTK_BOX(hbox), el->event_spin, FALSE, FALSE, 15);
    orage_table_add_row(el->event_notebook_page
            , label, hbox
            , row = 0, (GTK_FILL), (0));

    gtk_notebook_append_page(GTK_NOTEBOOK(el->Notebook)
            , el->event_notebook_page, el->event_tab_label);

    g_signal_connect((gpointer)el->event_spin, "value-changed"
            , G_CALLBACK(on_spin_changed), el);
}

static void build_todo_tab(el_win *el)
{
    el->todo_tab_label = gtk_label_new(_("Todo"));
    el->todo_notebook_page = orage_table_new(2, 10);

    gtk_notebook_append_page(GTK_NOTEBOOK(el->Notebook)
            , el->todo_notebook_page, el->todo_tab_label);
}

static void build_journal_tab(el_win *el)
{
    gint row;
    GtkWidget *label, *hbox;
    struct tm *tm;
    gchar *sdate;

    el->journal_tab_label = gtk_label_new(_("Journal"));
    el->journal_notebook_page = orage_table_new(2, 10);

    label = gtk_label_new(_("Journal entries starting from:"));
    hbox =  gtk_hbox_new(FALSE, 0);
    el->journal_start_button = gtk_button_new();
    tm = orage_localtime();
    tm->tm_year -= 1;
    sdate = orage_tm_date_to_i18_date(*tm);
    gtk_button_set_label(GTK_BUTTON(el->journal_start_button)
            , (const gchar *)sdate);
    gtk_box_pack_start(GTK_BOX(hbox), el->journal_start_button
            , FALSE, FALSE, 15);
    orage_table_add_row(el->journal_notebook_page
            , label, hbox
            , row = 0, (GTK_FILL), (0));

    gtk_notebook_append_page(GTK_NOTEBOOK(el->Notebook)
            , el->journal_notebook_page, el->journal_tab_label);
    g_signal_connect((gpointer)el->journal_start_button, "clicked"
            , G_CALLBACK(on_journal_start_button_clicked), el);
}

static void build_search_tab(el_win *el)
{
    gint row;
    GtkWidget *label, *hbox;

    el->search_tab_label = gtk_label_new(_("Search"));
    el->search_notebook_page = orage_table_new(2, 10);

    label = gtk_label_new(_("Search text "));
    el->search_entry = gtk_entry_new();
    orage_table_add_row(el->search_notebook_page
            , label, el->search_entry
            , row = 0, (GTK_EXPAND | GTK_FILL), (0));

    gtk_notebook_append_page(GTK_NOTEBOOK(el->Notebook)
            , el->search_notebook_page, el->search_tab_label);
}

static void build_notebook(el_win *el)
{
    el->Notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(el->Vbox), el->Notebook, FALSE, FALSE, 0);

    build_event_tab(el);
    build_todo_tab(el);
    build_journal_tab(el);
    build_search_tab(el);

    g_signal_connect((gpointer)el->Notebook, "switch-page"
            , G_CALLBACK(on_notebook_page_switch), el);
}

static void build_event_list(el_win *el)
{
    GtkCellRenderer *rend;
    GtkTreeViewColumn *col;

    /* Scrolled window */
    el->ScrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_box_pack_start(GTK_BOX(el->Vbox), el->ScrolledWindow
            , TRUE, TRUE, 0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(el->ScrolledWindow)
            , GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    /* Tree view */
    el->ListStore = gtk_list_store_new(NUM_COLS
            , G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING
            , G_TYPE_STRING);
    el->TreeView = gtk_tree_view_new();
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(el->TreeView), TRUE);

    el->TreeSelection = 
            gtk_tree_view_get_selection(GTK_TREE_VIEW(el->TreeView)); 
    gtk_tree_selection_set_mode(el->TreeSelection, GTK_SELECTION_MULTIPLE);

    el->TreeSortable = GTK_TREE_SORTABLE(el->ListStore);
    gtk_tree_sortable_set_sort_func(el->TreeSortable, COL_SORT
            , sortEvent_comp, GINT_TO_POINTER(COL_SORT), NULL);
    gtk_tree_sortable_set_sort_column_id(el->TreeSortable
            , COL_SORT, GTK_SORT_ASCENDING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(el->TreeView)
            , GTK_TREE_MODEL(el->ListStore));

    gtk_container_add(GTK_CONTAINER(el->ScrolledWindow), el->TreeView);

    rend = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes( _("Time"), rend
                , "text", COL_TIME
                , NULL);
    gtk_tree_view_column_set_cell_data_func(col, rend, start_time_data_func
                , el, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(el->TreeView), col);

    rend = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes( _("Flags"), rend
                , "text", COL_FLAGS
                , NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(el->TreeView), col);

    rend = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes( _("Title"), rend
                , "text", COL_HEAD
                , NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(el->TreeView), col);

    rend = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("uid", rend
                , "text", COL_UID
                , NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(el->TreeView), col);
    gtk_tree_view_column_set_visible(col, FALSE);

    rend = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("sort", rend
                , "text", COL_SORT
                , NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(el->TreeView), col);
    gtk_tree_view_column_set_visible(col, FALSE);

    gtk_tooltips_set_tip(el->Tooltips, el->TreeView, _("Double click line to edit it.\n\nFlags in order:\n\t 1. Alarm: n=no alarm\n\t\tA=visual Alarm S=also Sound alarm\n\t 2. Recurrence: n=no recurrence\n\t\t D=Daily W=Weekly M=Monthly Y=Yearly\n\t 3. Type: f=free B=Busy\n\t 4.Archived: A=Archived n=not archived"), NULL);

    g_signal_connect(el->TreeView, "row-activated",
            G_CALLBACK(editEvent), el);
}

el_win *create_el_win()
{
    GdkPixbuf *pixbuf;
    el_win *el;

    /* initialisation + main window + base vbox */
    el = g_new(el_win, 1);
    el->today = FALSE;
    el->days = 0;
    el->time_now[0] = 0;
    el->Tooltips = gtk_tooltips_new();
    el->accel_group = gtk_accel_group_new();

    el->Window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(el->Window)
            , g_par.el_size_x, g_par.el_size_y);
    gtk_window_add_accel_group(GTK_WINDOW(el->Window), el->accel_group);

    el->Vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(el->Window), el->Vbox);

    build_menu(el);
    build_toolbar(el);
    build_notebook(el);
    build_event_list(el);

    g_signal_connect((gpointer)el->Window, "delete_event"
            , G_CALLBACK(on_Window_delete_event), el);

    gtk_widget_show_all(el->Window);
    set_el_data_from_cal(el);

    gtk_drag_source_set(el->TreeView, GDK_BUTTON1_MASK
            , drag_targets, DRAG_TARGET_COUNT, GDK_ACTION_COPY);
    pixbuf = xfce_themed_icon_load("xfcalendar", 16);
    gtk_drag_source_set_icon_pixbuf(el->TreeView, pixbuf);
    g_object_unref(pixbuf);
    g_signal_connect(el->TreeView, "drag_data_get"
            , G_CALLBACK(drag_data_get), NULL);

    return(el);
}
