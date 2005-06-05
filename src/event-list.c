/* xfcalendar
 *
 * Copyright (C) 2003-2005 Mickael Graf (korbinus at xfce.org)
 * Copyright (C) 2005 Juha Kautto (juha at xfce.org)
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  This program is distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.  You
 * should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 
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
#include <libxfcegui4/netk-trayicon.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <glib/gprintf.h>

#include "event-list.h"
#include "reminder.h"
#include "about-xfcalendar.h"
#include "mainbox.h"
#include "appointment.h"
#include "ical-code.h"

static GtkWidget *clearwarn;

extern CalWin *xfcal;
extern gint event_win_size_x, event_win_size_y;

/* Direction for changing day to look at */
enum{
  PREVIOUS,
    NEXT
    };

enum {
    COL_TIME = 0
   ,COL_FLAGS
   ,COL_HEAD
   ,COL_UID
   ,NUM_COLS
};

void title_to_ical(char *title, char *ical)
{ /* yyyy-mm-dd\0 -> yyyymmdd\0 */
    gint i, j;

    for (i = 0, j = 0; i <= 8; i++) {
        if (i == 4 || i == 6)
            j++;
        ical[i] = title[j++];
    }
}

void editEvent(GtkTreeView *view, GtkTreePath *path
             , GtkTreeViewColumn *col, gpointer eventlist)
{
    appt_win *wApp;
    GtkTreeModel *model;
    GtkTreeIter   iter;
    gchar *uid = NULL;

    model = gtk_tree_view_get_model(view);
    if (gtk_tree_model_get_iter(model, &iter, path)) {
        gtk_tree_model_get(model, &iter, COL_UID, &uid, -1);
    }
    wApp = create_appt_win("UPDATE", uid, eventlist);
    if (uid) 
        g_free(uid);
    gtk_widget_show(wApp->appWindow);
}

gint sortEvent_comp(GtkTreeModel *model
        , GtkTreeIter *i1, GtkTreeIter *i2, gpointer data)
{
    gint col = GPOINTER_TO_INT(data);
    gint ret;
    gchar *text1, *text2;

    gtk_tree_model_get(model, i1, col, &text1, -1);
    gtk_tree_model_get(model, i2, col, &text2, -1);
    ret = g_utf8_collate(text1, text2);
    g_free(text1);
    g_free(text2);
    return(ret);
}

char *format_time(char *start_ical_time, char *end_ical_time
                , char *title, gint days)
{
    static char result[40];
    char title_ical_date[10];
    register int i = 0;

    if (days) { /* date needs to be visible */
        result[i++] = start_ical_time[6];
        result[i++] = start_ical_time[7];
        result[i++] = '.';
        result[i++] = start_ical_time[4];
        result[i++] = start_ical_time[5];
        result[i++] = '.';
        result[i++] = start_ical_time[0];
        result[i++] = start_ical_time[1];
        result[i++] = start_ical_time[2];
        result[i++] = start_ical_time[3];

        if (start_ical_time[8] == 'T') { /* time part available */
            result[i++] = ' ';
            result[i++] = start_ical_time[9];
            result[i++] = start_ical_time[10];
            result[i++] = ':';
            result[i++] = start_ical_time[11];
            result[i++] = start_ical_time[12];
            result[i++] = '-';
            result[i++] = end_ical_time[9];
            result[i++] = end_ical_time[10];
            result[i++] = ':';
            result[i++] = end_ical_time[11];
            result[i++] = end_ical_time[12];
            if (strncmp(start_ical_time, end_ical_time, 8) != 0)
                result[i++] = '+';
        }
        else {/* date */
            result[i++] = '-';
            result[i++] = end_ical_time[6];
            result[i++] = end_ical_time[7];
            result[i++] = '.';
            result[i++] = end_ical_time[4];
            result[i++] = end_ical_time[5];
            result[i++] = '.';
            result[i++] = end_ical_time[0];
            result[i++] = end_ical_time[1];
            result[i++] = end_ical_time[2];
            result[i++] = end_ical_time[3];
        }
        result[i++] = '\0';
    }
    else /* only show one day */
        if (start_ical_time[8] == 'T') { /* time part available */
            title_to_ical(title, title_ical_date);
            if (strncmp(start_ical_time, title_ical_date, 8) < 0) {
                strcpy(result, "+00:00");
                i = 6;
            }
            else {
                result[i++] = start_ical_time[9];
                result[i++] = start_ical_time[10];
                result[i++] = ':';
                result[i++] = start_ical_time[11];
                result[i++] = start_ical_time[12];
            }
            result[i++] = '-';
            if (strncmp(title_ical_date, end_ical_time , 8) < 0) {
                strcpy(result+i, "24:00+");
            }
            else {
                result[i++] = end_ical_time[9];
                result[i++] = end_ical_time[10];
                result[i++] = ':';
                result[i++] = end_ical_time[11];
                result[i++] = end_ical_time[12];
                result[i++] = '\0';
            }
        }
        else /* date only appointment */
            strcpy(result, _("today"));

    return(result);
}

void start_time_data_func_(GtkTreeViewColumn *col, GtkCellRenderer *rend,
                           GtkTreeModel      *model, GtkTreeIter   *iter,
                           gpointer           user_data)
{
    gchar *stime, *etime;
    struct tm *t;
    time_t tt;
    gchar time_now[6];
    gint            days = 0;
    eventlist_win *el = (eventlist_win *)user_data;
    days = gtk_spin_button_get_value(GTK_SPIN_BUTTON(el->elSpin1));

    if (!el->elToday || days != 0){
        return;
    }

    tt = time(NULL);
    t  = localtime(&tt);
    sprintf(time_now, "%02d:%02d", t->tm_hour, t->tm_min);

    gtk_tree_model_get(model, iter, COL_TIME, &stime, -1);
    if (stime[0] == '+')
        stime++;
    etime = stime + 6;
    if (stime[2] != ':') { /* catch "today" */
        g_object_set(rend
                 , "foreground-set",    FALSE
                 , "strikethrough-set", FALSE
                 , "weight-set",        FALSE
                 , NULL); 
    }
    else if (strncmp(etime, time_now, 5) < 0) { /* gone */
        g_object_set(rend
                 , "foreground-set",    FALSE
                 , "strikethrough",     TRUE
                 , "strikethrough-set", TRUE
                 , "weight",            PANGO_WEIGHT_LIGHT
                 , "weight-set",        TRUE
                 , NULL);
    }
    else if (strncmp(stime, time_now, 5) <= 0 
          && strncmp(etime, time_now, 5) >= 0) { /* current */
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

void
recreate_eventlist_win(eventlist_win *el)
{
    GtkCellRenderer *rend;
    GtkTreeViewColumn *col;

    gtk_list_store_clear(el->elListStore);
    col = gtk_tree_view_get_column(GTK_TREE_VIEW(el->elTreeView), 0);
    gtk_tree_view_remove_column(GTK_TREE_VIEW(el->elTreeView), col);
    rend = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes( _("Time"), rend
                , "text", COL_TIME
                , NULL);
    gtk_tree_view_column_set_cell_data_func(col, rend, start_time_data_func_
                , el, NULL);
    gtk_tree_view_insert_column(GTK_TREE_VIEW(el->elTreeView), col, 0);
    manage_eventlist_win(GTK_CALENDAR(xfcal->mCalendar), el);
}

void addEvent(GtkListStore *list1, appt_type *app, char *header, gint days)
{
    GtkTreeIter     iter1;
    gchar           *title = NULL;
    gchar           flags[4]; 
    gchar           stime[40];
    gint            len = 50;

    g_strlcpy(stime, format_time(app->starttimecur, app->endtimecur
                               , header, days), 40);

    if (app->alarmtime != 0)
        if (app->sound != NULL)
            flags[0] = 'S';
        else
            flags[0] = 'A';
    else
        flags[0] = 'n';
    if (app->freq == XFICAL_FREQ_NONE)
        flags[1] = 'n';
    else if (app->freq == XFICAL_FREQ_DAILY)
        flags[1] = 'D';
    else if (app->freq == XFICAL_FREQ_WEEKLY)
        flags[1] = 'W';
    else if (app->freq == XFICAL_FREQ_MONTHLY)
        flags[1] = 'M';
    else
        flags[1] = 'n';
    if (app->availability != 0)
        flags[2] = 'B';
    else
        flags[2] = 'f';
    flags[3] = '\0';

    if (app->title != NULL)
        title = g_strdup(app->title);
    else if (app->note != NULL) { 
    /* let's take len chars of the first line from the text */
        if ((title = g_strstr_len(app->note, strlen(app->note), "\n")) != NULL){
            if ((strlen(app->note)-strlen(title)) < len)
                len=strlen(app->note)-strlen(title);
        }
        title = g_strndup(app->note, len);
    }

    gtk_list_store_append(list1, &iter1);
    gtk_list_store_set(list1, &iter1
                , COL_TIME,  stime
                , COL_FLAGS, flags
                , COL_HEAD,  title
                , COL_UID,   app->uid
                , -1);
    g_free(title);
}

void manage_eventlist_win(GtkCalendar *calendar, eventlist_win *el)
{
    guint year, month, day;
    char            title[12];
    char            a_day[9];  /* yyyymmdd */
    appt_type       *app;
    struct tm       *t;
    time_t          tt;
    gint            days = 0;

    gtk_calendar_get_date(calendar, &year, &month, &day);
    g_sprintf(title, "%04d-%02d-%02d", year, month+1, day);
    gtk_window_set_title(GTK_WINDOW(el->elWindow), (const gchar*)title);
    days = gtk_spin_button_get_value(GTK_SPIN_BUTTON(el->elSpin1));

    if (xfical_file_open()){
        g_sprintf(a_day, XFICAL_APP_DATE_FORMAT, year, month+1, day);
        if ((app = xfical_app_get_next_on_day(a_day, TRUE, days))) {
            tt = time(NULL);
            t  = localtime(&tt);
            if (   year  == t->tm_year + 1900
                && month == t->tm_mon
                && day   == t->tm_mday)
                el->elToday = TRUE;
            else
                el->elToday = FALSE; 

            do {
                addEvent(el->elListStore, app, title, days);
            } while ((app = xfical_app_get_next_on_day(a_day, FALSE, days)));
        }
        xfical_file_close();
    }
}

void
on_elCopy_clicked(GtkButton *button, gpointer user_data)
{
    eventlist_win *el;
    GtkTreeSelection *sel;
    GtkTreeModel     *model;
    GtkTreeIter       iter;
    gchar *uid = NULL;
    appt_win *wApp;

    el = (eventlist_win *) user_data;
    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(el->elTreeView));
    if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
         gtk_tree_model_get(model, &iter, COL_UID, &uid, -1);
         wApp = create_appt_win("COPY", uid, GTK_WIDGET(el->elWindow));
         gtk_widget_show(wApp->appWindow);
         g_free(uid);

    }
    else
        g_warning("Copy: No row selected\n");
}

void
on_elClose_clicked(GtkButton *button, gpointer user_data)
{
    eventlist_win *el = (eventlist_win *) user_data;
    gtk_window_get_size(GTK_WINDOW(el->elWindow)
        , &event_win_size_x, &event_win_size_y);
    apply_settings();

    gtk_widget_destroy(el->elWindow); /* destroy the eventlist window */
    g_free(el);
}

gboolean 
on_elWindow_delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    gtk_window_get_size(GTK_WINDOW(widget), &event_win_size_x, &event_win_size_y);
    apply_settings();
    gtk_widget_destroy(widget); /* destroy the eventlist window */
    g_free(data);
    return(FALSE);
}

void
changeSelectedDate_(GtkButton *button, gpointer user_data, gint direction)
{
    guint year, month, day;
    guint monthdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    eventlist_win *el;
    char *title;

    el = (eventlist_win *)user_data;
    title = (char*)gtk_window_get_title(GTK_WINDOW(el->elWindow));
    if (sscanf(title, "%d-%d-%d", &year, &month, &day) != 3)
        g_warning("changeSelectedDate: title conversion error\n");
    month--; /* gtk calendar starts from 0  month */
    if (((year%4)==0)&&(((year%100)!=0)||((year%400)==0)))
        ++monthdays[1];

    switch(direction){
    case PREVIOUS:
        if(--day == 0){
            if(--month == -1){
                --year;
                month = 11;
            }
            day = monthdays[month];
        }
        break;
    case NEXT:
        if(++day == (monthdays[month]+1)){
            if(++month == 12){
                ++year;
                month = 0;
            }
            day = 1;
        }
        break;
    default:
        break;
    }
    gtk_calendar_select_month(GTK_CALENDAR(xfcal->mCalendar), month, year);
    gtk_calendar_select_day(GTK_CALENDAR(xfcal->mCalendar), day);

    recreate_eventlist_win(el);
}

void
on_elPrevious_clicked(GtkButton *button, gpointer user_data)
{
  changeSelectedDate_(button, user_data, PREVIOUS);
}

void
on_elToday_clicked(GtkButton *button, gpointer user_data)
{
  struct tm *t;
  time_t tt;

  tt = time(NULL);
  t = localtime(&tt);

  gtk_calendar_select_month(GTK_CALENDAR(xfcal->mCalendar), t->tm_mon, t->tm_year+1900);
  gtk_calendar_select_day(GTK_CALENDAR(xfcal->mCalendar), t->tm_mday);

  recreate_eventlist_win((eventlist_win*)user_data);
}

void
on_elNext_clicked(GtkButton *button, gpointer user_data)
{
  changeSelectedDate_(button, user_data, NEXT);
}

void
on_elCreate_toolbutton_clicked_cb(GtkButton *button, gpointer user_data)
{
    appt_win *app;
    eventlist_win *el = (eventlist_win *)user_data;
    char *title;
    char a_day[10];

    title = (char*)gtk_window_get_title(GTK_WINDOW(el->elWindow));
    title_to_ical(title, a_day);

    app = create_appt_win("NEW", a_day, el);
    gtk_widget_show(app->appWindow);
}

void
on_elSpin1_changed(GtkSpinButton *button, gpointer user_data)
{
    recreate_eventlist_win((eventlist_win *)user_data);
}

void
on_elDelete_clicked(GtkButton *button, gpointer user_data)
{
    char a_day[10];
    char *title;
    guint day;
    gint result;
    eventlist_win *el = (eventlist_win *) user_data;

    result = xfce_message_dialog(GTK_WINDOW(el->elWindow),
                                 _("Warning"),
                                 GTK_STOCK_DIALOG_WARNING,
                                 _("You will remove all information \nassociated with this date."),
                                 _("Do you want to continue?"),
                                 GTK_STOCK_YES,
                                 GTK_RESPONSE_ACCEPT,
                                 GTK_STOCK_NO,
                                 GTK_RESPONSE_CANCEL,
                                 NULL);

    if (result == GTK_RESPONSE_ACCEPT){
        if (xfical_file_open()){
            title = (char*)gtk_window_get_title(GTK_WINDOW (el->elWindow));
            title_to_ical(title, a_day);
            rmday_ical_app(a_day);
            xfical_file_close();
            day = atoi(a_day+6);
            gtk_calendar_unmark_day(GTK_CALENDAR(xfcal->mCalendar), day);
            recreate_eventlist_win(el);
        }
    }
}

eventlist_win
*create_eventlist_win(void)
{
    GtkWidget *tmp_toolbar_icon;
    GtkCellRenderer *rend;
    GtkTreeViewColumn *col;

    eventlist_win *el = g_new(eventlist_win, 1);
    el->elToday = FALSE;
    el->elTooltips = gtk_tooltips_new();
    el->accel_group = gtk_accel_group_new();

    el->elWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_print("create_eventlist_win (ze niou oane): x, y: %d, %d\n", event_win_size_x,event_win_size_y);
    gtk_window_set_default_size(GTK_WINDOW(el->elWindow), event_win_size_x, event_win_size_y);

    el->elVbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(el->elWindow), el->elVbox);

    el->elToolbar = gtk_toolbar_new();
    gtk_box_pack_start(GTK_BOX(el->elVbox), el->elToolbar, FALSE, FALSE, 0);
    /* gtk_toolbar_set_style(GTK_TOOLBAR(el->elToolbar), GTK_TOOLBAR_ICONS);*/

    tmp_toolbar_icon = gtk_image_new_from_stock("gtk-new", gtk_toolbar_get_icon_size(GTK_TOOLBAR(el->elToolbar)));
    el->elCreate_toolbutton = gtk_toolbar_append_element(GTK_TOOLBAR(el->elToolbar),
                                                         GTK_TOOLBAR_CHILD_BUTTON, NULL,
                                                         _("Add"), _("Add (Ctrl+a)"), NULL,
                                                         tmp_toolbar_icon, NULL, NULL);
    gtk_widget_add_accelerator(el->elCreate_toolbutton, "clicked", el->accel_group, GDK_a, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    tmp_toolbar_icon = gtk_image_new_from_stock("gtk-go-back", gtk_toolbar_get_icon_size(GTK_TOOLBAR(el->elToolbar)));
    el->elPrevious_toolbutton = gtk_toolbar_append_element(GTK_TOOLBAR(el->elToolbar),
                                                           GTK_TOOLBAR_CHILD_BUTTON, NULL,
                                                           _("Previous"), _("Previous day (Ctrl+p)"), NULL,
                                                           tmp_toolbar_icon, NULL, NULL);
    gtk_widget_add_accelerator(el->elPrevious_toolbutton, "clicked", el->accel_group, GDK_p, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    tmp_toolbar_icon = gtk_image_new_from_stock("gtk-home", gtk_toolbar_get_icon_size(GTK_TOOLBAR(el->elToolbar)));
    el->elToday_toolbutton = gtk_toolbar_append_element(GTK_TOOLBAR(el->elToolbar),
                                                        GTK_TOOLBAR_CHILD_BUTTON, NULL,
                                                        _("Today"), _("Today (Alt+Home)"), NULL,
                                                        tmp_toolbar_icon, NULL, NULL);
    gtk_widget_add_accelerator(el->elToday_toolbutton, "clicked", el->accel_group, GDK_Home, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(el->elToday_toolbutton, "clicked", el->accel_group, GDK_h, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    tmp_toolbar_icon = gtk_image_new_from_stock("gtk-go-forward", gtk_toolbar_get_icon_size(GTK_TOOLBAR(el->elToolbar)));
    el->elNext_toolbutton = gtk_toolbar_append_element(GTK_TOOLBAR(el->elToolbar),
                                                       GTK_TOOLBAR_CHILD_BUTTON, NULL,
                                                       _("Next"), _("Next day (Ctrl+n)"), NULL,
                                                       tmp_toolbar_icon, NULL, NULL);
    gtk_widget_add_accelerator(el->elNext_toolbutton, "clicked", el->accel_group, GDK_n, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    gtk_toolbar_append_space(GTK_TOOLBAR(el->elToolbar));

    tmp_toolbar_icon = gtk_image_new_from_stock("gtk-copy", gtk_toolbar_get_icon_size(GTK_TOOLBAR(el->elToolbar)));
    el->elCopy_toolbutton = gtk_toolbar_append_element(GTK_TOOLBAR(el->elToolbar),
                                                       GTK_TOOLBAR_CHILD_BUTTON, NULL,
                                                      _("Duplicate"), _("Copy (Ctrl+c)"), NULL,
                                                      tmp_toolbar_icon, NULL, NULL);
    gtk_widget_add_accelerator(el->elCopy_toolbutton, "clicked", el->accel_group, GDK_c, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    tmp_toolbar_icon = gtk_image_new_from_stock("gtk-close", gtk_toolbar_get_icon_size(GTK_TOOLBAR(el->elToolbar)));
    el->elClose_toolbutton = gtk_toolbar_append_element(GTK_TOOLBAR(el->elToolbar),
                                                        GTK_TOOLBAR_CHILD_BUTTON, NULL,
                                                        _("Close"), _("Close (Ctrl+w)"), NULL,
                                                        tmp_toolbar_icon, NULL, NULL);
    gtk_widget_add_accelerator(el->elClose_toolbutton, "clicked", el->accel_group, GDK_w, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    tmp_toolbar_icon = gtk_image_new_from_stock("gtk-clear", gtk_toolbar_get_icon_size(GTK_TOOLBAR(el->elToolbar)));
    el->elDelete_toolbutton = gtk_toolbar_append_element(GTK_TOOLBAR(el->elToolbar),
                                                         GTK_TOOLBAR_CHILD_BUTTON, NULL,
                                                         _("Clear"), _("Clear (Ctrl+l)"), NULL,
                                                         tmp_toolbar_icon, NULL, NULL);
    gtk_widget_add_accelerator(el->elDelete_toolbutton, "clicked", el->accel_group, GDK_l, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);


    el->elSpin1 = gtk_spin_button_new_with_range(0, 31, 1);
    el->elSpin2 = gtk_toolbar_append_element(GTK_TOOLBAR(el->elToolbar),
                                             GTK_TOOLBAR_CHILD_WIDGET, el->elSpin1,
                                             "spin", _("Show more days"), NULL,
                                             NULL, NULL, NULL);

    el->elScrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_box_pack_start(GTK_BOX(el->elVbox), el->elScrolledWindow, TRUE, TRUE, 0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(el->elScrolledWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    el->elListStore = gtk_list_store_new(NUM_COLS
                , G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    el->elTreeView = gtk_tree_view_new();

    el->elTreeSelection = gtk_tree_view_get_selection(GTK_TREE_VIEW(el->elTreeView)); /* Might be useless here... */
    gtk_tree_selection_set_mode(el->elTreeSelection, GTK_SELECTION_BROWSE);

    el->elTreeSortable = GTK_TREE_SORTABLE(el->elListStore);
    gtk_tree_sortable_set_sort_func(el->elTreeSortable, COL_TIME
                , sortEvent_comp, GINT_TO_POINTER(COL_TIME), NULL);
    gtk_tree_sortable_set_sort_column_id(el->elTreeSortable
                , COL_TIME, GTK_SORT_ASCENDING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(el->elTreeView), GTK_TREE_MODEL(el->elListStore));

    gtk_container_add(GTK_CONTAINER(el->elScrolledWindow), el->elTreeView);

    rend = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes( _("Time"), rend
                , "text", COL_TIME
                , NULL);
    gtk_tree_view_column_set_cell_data_func(col, rend, start_time_data_func_
                , el, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(el->elTreeView), col);

    rend = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes( _("Flags"), rend
                , "text", COL_FLAGS
                , NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(el->elTreeView), col);

    rend = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes( _("Title"), rend
                , "text", COL_HEAD
                , NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(el->elTreeView), col);

    rend = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes( _("uid"), rend
                , "text", COL_UID
                , NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(el->elTreeView), col);
    gtk_tree_view_column_set_visible(col, FALSE);

    g_signal_connect (el->elTreeView, "row-activated",
            G_CALLBACK(editEvent), el);

    gtk_tooltips_set_tip(el->elTooltips, el->elTreeView, "Double click line to edit it.\n\nFlags in order:\n\t 1. Alarm: n=no alarm\n\t\tA=visual Alarm S=also Sound alarm\n\t 2. Recurrency: n=no recurrency\n\t\t D=Daily W=Weekly M=Monthly\n\t 3. Type: f=free B=Busy", NULL);

  g_signal_connect((gpointer)el->elWindow, "delete_event",
                    G_CALLBACK(on_elWindow_delete_event), el);
  g_signal_connect((gpointer)el->elCreate_toolbutton, "clicked",
                    G_CALLBACK(on_elCreate_toolbutton_clicked_cb), el);
  g_signal_connect((gpointer)el->elPrevious_toolbutton, "clicked",
                    G_CALLBACK(on_elPrevious_clicked), el);
  g_signal_connect((gpointer)el->elToday_toolbutton, "clicked",
                    G_CALLBACK(on_elToday_clicked), el);
  g_signal_connect((gpointer)el->elNext_toolbutton, "clicked",
                    G_CALLBACK(on_elNext_clicked), el);
  g_signal_connect((gpointer)el->elCopy_toolbutton, "clicked",
                    G_CALLBACK(on_elCopy_clicked), el);
  g_signal_connect((gpointer)el->elClose_toolbutton, "clicked",
                    G_CALLBACK(on_elClose_clicked), el);
  g_signal_connect((gpointer)el->elDelete_toolbutton, "clicked",
                    G_CALLBACK(on_elDelete_clicked), el);
  g_signal_connect((gpointer)el->elSpin1, "value-changed",
                    G_CALLBACK(on_elSpin1_changed), el);

    gtk_window_add_accel_group(GTK_WINDOW(el->elWindow), el->accel_group);

    gtk_widget_show_all(el->elWindow);

  return el;
}
