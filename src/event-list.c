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

GtkWidget*
create_wAppointment (void)
{
  GtkWidget *wAppointment;
  GtkWidget *vbox2;
  GtkWidget *handlebox1;
  GtkWidget *toolbar1;
  GtkWidget *tmp_toolbar_icon;
  GtkWidget *btClose;
  GtkWidget *btDelete;
  GtkWidget *btCreate;
  GtkWidget *btPrevious;
  GtkWidget *btToday;
  GtkWidget *btNext;
  GtkWidget *scrolledwindow1;
  GtkAccelGroup *accel_group;
  GtkWidget *spin1, *spin2;

  accel_group = gtk_accel_group_new();

  wAppointment = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(wAppointment)
        , event_win_size_x, event_win_size_y);
  /*
  gtk_window_set_position(GTK_WINDOW(wAppointment), GTK_WIN_POS_NONE);
  */

  vbox2 = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox2);
  gtk_container_add(GTK_CONTAINER(wAppointment), vbox2);

  /*
  handlebox1 = gtk_handle_box_new();
  gtk_widget_show(handlebox1);
  gtk_box_pack_start(GTK_BOX(vbox2), handlebox1, FALSE, FALSE, 0);
  */

  toolbar1 = gtk_toolbar_new();
  gtk_widget_show(toolbar1);
  /*
  gtk_container_add(GTK_CONTAINER(handlebox1), toolbar1);
  */
  gtk_box_pack_start(GTK_BOX(vbox2), toolbar1, FALSE, FALSE, 0);
  gtk_toolbar_set_style(GTK_TOOLBAR(toolbar1), GTK_TOOLBAR_ICONS);

  tmp_toolbar_icon = gtk_image_new_from_stock("gtk-new", gtk_toolbar_get_icon_size(GTK_TOOLBAR(toolbar1)));
  btCreate = gtk_toolbar_append_element(GTK_TOOLBAR(toolbar1),
                                GTK_TOOLBAR_CHILD_BUTTON, NULL,
                                "btCreate", _("Add (Ctrl+a)"), NULL,
                                tmp_toolbar_icon, NULL, NULL);
  /*
  gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(toolbar1)->children)->data))->label), TRUE);
*/
  gtk_widget_show(btCreate);
  gtk_widget_add_accelerator(btCreate, "clicked", accel_group, GDK_a, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  tmp_toolbar_icon = gtk_image_new_from_stock("gtk-go-back", gtk_toolbar_get_icon_size(GTK_TOOLBAR(toolbar1)));
  btPrevious = gtk_toolbar_append_element(GTK_TOOLBAR(toolbar1),
                                GTK_TOOLBAR_CHILD_BUTTON, NULL,
                                "btPrevious", _("Previous day (Ctrl+p)"), NULL,
                                tmp_toolbar_icon, NULL, NULL);
  /*
  gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(toolbar1)->children)->data))->label), TRUE);
  */
  gtk_widget_show(btPrevious);
  gtk_widget_add_accelerator(btPrevious, "clicked", accel_group, GDK_p, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  tmp_toolbar_icon = gtk_image_new_from_stock("gtk-home", gtk_toolbar_get_icon_size(GTK_TOOLBAR(toolbar1)));
  btToday = gtk_toolbar_append_element(GTK_TOOLBAR(toolbar1),
                                GTK_TOOLBAR_CHILD_BUTTON, NULL,
                                "btToday", _("Today (Alt+Home)"), NULL,
                                tmp_toolbar_icon, NULL, NULL);
  /*
  gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(toolbar1)->children)->data))->label), TRUE);
  */
  gtk_widget_show(btToday);
  gtk_widget_add_accelerator(btToday, "clicked", accel_group, GDK_Home, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator(btToday, "clicked", accel_group, GDK_h, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  tmp_toolbar_icon = gtk_image_new_from_stock("gtk-go-forward", gtk_toolbar_get_icon_size(GTK_TOOLBAR(toolbar1)));
  btNext = gtk_toolbar_append_element(GTK_TOOLBAR(toolbar1),
                                GTK_TOOLBAR_CHILD_BUTTON, NULL,
                                "btNext", _("Next day (Ctrl+n)"), NULL,
                                tmp_toolbar_icon, NULL, NULL);
  /*
  gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(toolbar1)->children)->data))->label), TRUE);
  */
  gtk_widget_show(btNext);
  gtk_widget_add_accelerator(btNext, "clicked", accel_group, GDK_n, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  gtk_toolbar_append_space(GTK_TOOLBAR(toolbar1));

  tmp_toolbar_icon = gtk_image_new_from_stock("gtk-close", gtk_toolbar_get_icon_size(GTK_TOOLBAR(toolbar1)));
  btClose = gtk_toolbar_append_element(GTK_TOOLBAR(toolbar1),
                                GTK_TOOLBAR_CHILD_BUTTON, NULL,
                                "btClose", _("Close (Ctrl+w)"), NULL,
                                tmp_toolbar_icon, NULL, NULL);
  /*
  gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(toolbar1)->children)->data))->label), TRUE);
  */
  gtk_widget_show(btClose);
  gtk_widget_add_accelerator(btClose, "clicked", accel_group, GDK_w, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  tmp_toolbar_icon = gtk_image_new_from_stock("gtk-clear", gtk_toolbar_get_icon_size(GTK_TOOLBAR(toolbar1)));
  btDelete = gtk_toolbar_append_element(GTK_TOOLBAR(toolbar1),
                                GTK_TOOLBAR_CHILD_BUTTON, NULL,
                                "btDelete", _("Clear (Ctrl+l)"), NULL,
                                tmp_toolbar_icon, NULL, NULL);
  /*
  gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(toolbar1)->children)->data))->label), TRUE);
  */
  gtk_widget_show(btDelete);
  gtk_widget_add_accelerator(btDelete, "clicked", accel_group, GDK_l, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  spin1 = gtk_spin_button_new_with_range(0, 31, 1);
  spin2 = gtk_toolbar_append_element(GTK_TOOLBAR(toolbar1),
                                GTK_TOOLBAR_CHILD_WIDGET, spin1,
                                "spin", _("Show more days"), NULL,
                                NULL, NULL, NULL);
  gtk_widget_show(spin1);

  scrolledwindow1 = gtk_scrolled_window_new(NULL, NULL);
  gtk_widget_show(scrolledwindow1);
  gtk_box_pack_start(GTK_BOX(vbox2), scrolledwindow1, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow1), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  g_signal_connect((gpointer) wAppointment, "delete_event",
                    G_CALLBACK(on_wAppointment_delete_event), NULL);
  g_signal_connect((gpointer) btPrevious, "clicked",
                    G_CALLBACK(on_btPrevious_clicked), (gpointer) wAppointment);
  g_signal_connect((gpointer) btToday, "clicked",
                    G_CALLBACK(on_btToday_clicked), (gpointer) wAppointment);
  g_signal_connect((gpointer) btNext, "clicked",
                    G_CALLBACK(on_btNext_clicked), (gpointer) wAppointment);
  g_signal_connect((gpointer) btClose, "clicked",
                    G_CALLBACK(on_btClose_clicked), (gpointer) wAppointment);
  g_signal_connect((gpointer) btDelete, "clicked",
                    G_CALLBACK(on_btDelete_clicked), (gpointer) wAppointment);
  g_signal_connect((gpointer) btCreate, "clicked",
                    G_CALLBACK(on_btCreate_clicked), (gpointer) wAppointment);
  g_signal_connect((gpointer) spin1, "value-changed",
                    G_CALLBACK(on_btspin_changed), (gpointer) wAppointment);

  /* Store pointers to widgets, for later use */
  g_object_set_data(G_OBJECT(wAppointment), "wAppointment", wAppointment);
  g_object_set_data(G_OBJECT(wAppointment), "vbox2", vbox2);
  g_object_set_data(G_OBJECT(wAppointment), "scrolledwindow1", scrolledwindow1);
  g_object_set_data(G_OBJECT(wAppointment), "spin1", spin1);

  gtk_window_add_accel_group(GTK_WINDOW(wAppointment), accel_group);

  return wAppointment;
}

GtkWidget*
recreate_wAppointment(GtkWidget *wAppointment)
{
    GtkWidget *scrolledwindow1;
    GtkWidget *vbox2;

    vbox2 = (GtkWidget*) g_object_get_data(G_OBJECT(wAppointment), "vbox2");
    scrolledwindow1 = (GtkWidget*) g_object_get_data(G_OBJECT(wAppointment)
            , "scrolledwindow1");    
    gtk_widget_destroy(scrolledwindow1);
    scrolledwindow1 = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_show(scrolledwindow1);
    gtk_box_pack_start(GTK_BOX(vbox2), scrolledwindow1, TRUE, TRUE, 0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scrolledwindow1)
            , GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    g_object_set_data(G_OBJECT(wAppointment), "scrolledwindow1"
            , scrolledwindow1);

    manageAppointment(GTK_CALENDAR(xfcal->mCalendar), wAppointment);

    return wAppointment;
}

GtkWidget*
create_wClearWarn (GtkWidget *parent)
{
  GtkWidget *wClearWarn;
  GtkWidget *dialog_vbox2;
  GtkWidget *hbox1;
  GtkWidget *image1;
  GtkWidget *lbClearWarn;
  GtkWidget *dialog_action_area2;
  GtkWidget *cancelbutton1;
  GtkWidget *okbutton2;

  wClearWarn = gtk_dialog_new ();
  gtk_widget_set_size_request (wClearWarn, 250, 120);
  gtk_window_set_title (GTK_WINDOW (wClearWarn), _("Warning"));
  gtk_window_set_transient_for(GTK_WINDOW (wClearWarn), GTK_WINDOW(parent));
  gtk_window_set_position (GTK_WINDOW (wClearWarn), GTK_WIN_POS_CENTER_ON_PARENT);
  gtk_window_set_modal (GTK_WINDOW (wClearWarn), TRUE);
  gtk_window_set_resizable (GTK_WINDOW (wClearWarn), FALSE);

  dialog_vbox2 = GTK_DIALOG (wClearWarn)->vbox;
  gtk_widget_show (dialog_vbox2);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox1);
  gtk_box_pack_start (GTK_BOX (dialog_vbox2), hbox1, TRUE, TRUE, 0);

  image1 = gtk_image_new_from_stock ("gtk-dialog-warning", GTK_ICON_SIZE_DIALOG);
  gtk_widget_show (image1);
  gtk_box_pack_start (GTK_BOX (hbox1), image1, TRUE, TRUE, 0);

  lbClearWarn = gtk_label_new (_("You will remove all information \nassociated with this date."));
  gtk_widget_show (lbClearWarn);
  gtk_box_pack_start (GTK_BOX (hbox1), lbClearWarn, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (lbClearWarn), GTK_JUSTIFY_LEFT);

  dialog_action_area2 = GTK_DIALOG (wClearWarn)->action_area;
  gtk_widget_show (dialog_action_area2);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area2), GTK_BUTTONBOX_END);

  cancelbutton1 = gtk_button_new_from_stock ("gtk-cancel");
  gtk_widget_show (cancelbutton1);
  gtk_dialog_add_action_widget (GTK_DIALOG (wClearWarn), cancelbutton1, GTK_RESPONSE_CANCEL);
  GTK_WIDGET_SET_FLAGS (cancelbutton1, GTK_CAN_DEFAULT);

  okbutton2 = gtk_button_new_from_stock ("gtk-ok");
  gtk_widget_show (okbutton2);
  gtk_dialog_add_action_widget (GTK_DIALOG (wClearWarn), okbutton2, GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS (okbutton2, GTK_CAN_DEFAULT);

  g_signal_connect ((gpointer) cancelbutton1, "clicked",
                    G_CALLBACK (on_cancelbutton1_clicked),
                    NULL);

  g_object_set_data(G_OBJECT(wClearWarn), "wClearWarn", wClearWarn);
  g_object_set_data(G_OBJECT(wClearWarn), "okbutton2", okbutton2);

  return wClearWarn;
}

void editAppointment(GtkTreeView *view, GtkTreePath *path
                   , GtkTreeViewColumn *col, gpointer wAppointment)
{
    appt_win *app;
    GtkTreeModel *model;
    GtkTreeIter   iter;
    gchar *uid = NULL;

    model = gtk_tree_view_get_model(view);
    if (gtk_tree_model_get_iter(model, &iter, path)) {
        gtk_tree_model_get(model, &iter, COL_UID, &uid, -1);
    }
    app = create_appt_win("UPDATE", uid, GTK_WIDGET(wAppointment));
    if (uid) 
        g_free(uid);
    gtk_widget_show(app->appWindow);
}

gint sortAppointment_comp(GtkTreeModel *model
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
    int i = 0;

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

void start_time_data_func(GtkTreeViewColumn *col, GtkCellRenderer *rend,
                          GtkTreeModel      *model, GtkTreeIter   *iter, 
                          gpointer           user_data) 
{
    gchar *stime, *etime;
    struct tm *t;
    time_t tt;
    gchar time_now[6];

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

void addAppointment(GtkListStore *list1, appt_type *app
        , char *header, gint days)
{
    GtkTreeIter     iter1;
    gchar           *title = NULL;
    gchar           flags[4]; 
    gchar           stime[40]; /* 0 12345678901 2 34
                                 [+]hh:mm-hh:mm[+]+<null> */
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

void addAppointment_init(GtkWidget *view, GtkWidget *wAppointment
            , gboolean today, gint days)
{
    GtkTreeViewColumn   *col;
    GtkCellRenderer     *rend;

    rend = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes( _("Time"), rend
                , "text", COL_TIME
                , NULL);
    /*
    g_object_set(rend 
            , "weight", PANGO_WEIGHT_BOLD
            , "weight-set", TRUE
            , NULL);
            */
    if (today && days == 0)
        gtk_tree_view_column_set_cell_data_func(col, rend, start_time_data_func
                , NULL, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

    rend = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes( _("Flags"), rend
                , "text", COL_FLAGS
                , NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

    rend = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes( _("Title"), rend
                , "text", COL_HEAD
                , NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

    rend = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes( _("uid"), rend
                , "text", COL_UID
                , NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
    gtk_tree_view_column_set_visible(col, FALSE);

    g_signal_connect (view, "row-activated",
            G_CALLBACK(editAppointment), wAppointment);
}

void manageAppointment(GtkCalendar *calendar, GtkWidget *wAppointment)
{
    guint year, month, day;
    char            title[12];
    char            a_day[9];  /* yyyymmdd */
    GtkWidget       *swin;
    appt_type       *app;
    GtkWidget       *view;
    GtkListStore    *list;
    GtkTreeSortable *sort;
    struct tm *t;
    time_t tt;
    gboolean today;
    GtkTooltips *tooltips = gtk_tooltips_new();
    gint days = 0;
    GtkSpinButton   *spin;

    gtk_calendar_get_date(calendar, &year, &month, &day);
    g_sprintf(title, "%04d-%02d-%02d", year, month+1, day);
    gtk_window_set_title(GTK_WINDOW(wAppointment), _(title));
    spin = (GtkSpinButton *) g_object_get_data(G_OBJECT(wAppointment), "spin1");
    days = gtk_spin_button_get_value(spin);

    if (xfical_file_open()){
        g_sprintf(a_day, XFICAL_APP_DATE_FORMAT, year, month+1, day);
        if ((app = xfical_app_get_next_on_day(a_day, TRUE, days))) {
            tt = time(NULL);
            t  = localtime(&tt);
            if (year  == t->tm_year + 1900 
            &&  month == t->tm_mon 
            &&  day   == t->tm_mday)
                today = TRUE;
            else
                today = FALSE;
  
            list = gtk_list_store_new(NUM_COLS
                , G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
            view = gtk_tree_view_new();
            addAppointment_init(view, wAppointment, today, days);
            do {
                addAppointment(list, app, title, days);
            } while ((app = xfical_app_get_next_on_day(a_day, FALSE, days)));
            sort = GTK_TREE_SORTABLE(list);
            gtk_tree_sortable_set_sort_func(sort, COL_TIME
                , sortAppointment_comp, GINT_TO_POINTER(COL_TIME), NULL);
            gtk_tree_sortable_set_sort_column_id(sort
                , COL_TIME, GTK_SORT_ASCENDING);
            gtk_tree_view_set_model(GTK_TREE_VIEW(view),  GTK_TREE_MODEL(list));
            g_object_unref(list); /* model is destroyed together with view */
            swin = (GtkWidget*)g_object_get_data(G_OBJECT(wAppointment)
                , "scrolledwindow1");
            gtk_container_add(GTK_CONTAINER(swin), view);
            gtk_tooltips_set_tip(tooltips, view, "Click line to edit it.\n\nFlags in order:\n\t 1. Alarm: n=no alarm\n\t\tA=visual Alarm S=also Sound alarm\n\t 2. Recurrency: n=no recurrency\n\t\t D=Daily W=Weekly M=Monthly\n\t 3. Type: f=free B=Busy", NULL);
            gtk_widget_show(view);
        }
        xfical_file_close();
    }
}

void
on_btClose_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *wAppointment;

    wAppointment = (GtkWidget *) user_data;
    gtk_window_get_size(GTK_WINDOW(wAppointment)
        , &event_win_size_x, &event_win_size_y);
    apply_settings();

    gtk_widget_destroy(wAppointment); /* destroy the appointment window */
}

gint 
dialogWin(gpointer user_data)
{
  GtkWidget *dialog, *message;
  dialog = gtk_dialog_new_with_buttons (_("Question"),
                    GTK_WINDOW(user_data),
                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                    GTK_STOCK_NO,
                    GTK_RESPONSE_REJECT,
                    GTK_STOCK_YES,
                    GTK_RESPONSE_ACCEPT,
                    NULL);

  message = gtk_label_new(_("\nThe information has been modified.\n Do you want to continue ?\n"));

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), message);
  gtk_widget_show_all(dialog);
  gint result = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  return result;
}

gboolean 
on_wAppointment_delete_event(GtkWidget *widget, GdkEvent *event,
                             gpointer user_data)
{
  gtk_window_get_size(GTK_WINDOW(widget), &event_win_size_x, &event_win_size_y);
  apply_settings();
  gtk_widget_destroy(widget); /* destroy the appointment window */
  return(FALSE);
}

void
on_btPrevious_clicked(GtkButton *button, gpointer user_data)
{
  changeSelectedDate(button, user_data, PREVIOUS);
}

void
on_btToday_clicked(GtkButton *button, gpointer user_data)
{
  struct tm *t;
  time_t tt;
  GtkWidget *wAppointment; 

  tt=time(NULL);
  t=localtime(&tt);

  gtk_calendar_select_month(GTK_CALENDAR(xfcal->mCalendar), t->tm_mon, t->tm_year+1900);
  gtk_calendar_select_day(GTK_CALENDAR(xfcal->mCalendar), t->tm_mday);

  wAppointment = (GtkWidget*) user_data;
  recreate_wAppointment(wAppointment);
}

void
on_btNext_clicked(GtkButton *button, gpointer user_data)
{
  changeSelectedDate(button, user_data, NEXT);
}

gboolean
bisextile(guint year)
{
  return ((year%4)==0)&&(((year%100)!=0)||((year%400)==0));
}

void
changeSelectedDate(GtkButton *button, gpointer user_data, gint direction)
{
    guint year, month, day;
    guint monthdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    GtkWidget *wAppointment; 
    char *title;

    wAppointment = (GtkWidget*) user_data;
    title = (char*)gtk_window_get_title(GTK_WINDOW(wAppointment));
    if (sscanf(title, "%d-%d-%d", &year, &month, &day) != 3)
        g_warning("changeSelectedDate: title conversion error\n");
    month--; /* gtk calendar starts from 0  month */
    if(bisextile(year))
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

    recreate_wAppointment(wAppointment);
}

void
on_btDelete_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *w;
    GtkWidget *wAppointment; 
    
    wAppointment = (GtkWidget *) user_data;

    clearwarn = create_wClearWarn(wAppointment);
    w=(GtkWidget*) g_object_get_data (G_OBJECT(clearwarn),"okbutton2");
    g_signal_connect ((gpointer) w, "clicked",
                    G_CALLBACK (on_okbutton2_clicked),
                    (gpointer)wAppointment);
    gtk_widget_show(clearwarn);
}

void
on_btCreate_clicked(GtkButton *button, gpointer user_data)
{
    appt_win *app;
    GtkWidget *wAppointment; 
    char *title;
    char a_day[10];

    wAppointment = (GtkWidget*) user_data;
    title = (char*)gtk_window_get_title(GTK_WINDOW(wAppointment));
    title_to_ical(title, a_day);

    app = create_appt_win("NEW", a_day, wAppointment);
    gtk_widget_show(app->appWindow);
}

void
on_btspin_changed(GtkSpinButton *button, gpointer user_data)
{
    GtkWidget *wAppointment; 

    wAppointment = (GtkWidget*) user_data;
    recreate_wAppointment(wAppointment);
}

void
on_cancelbutton1_clicked(GtkButton *button, gpointer user_data)
{
    gtk_widget_destroy(clearwarn);
}

void
on_okbutton2_clicked(GtkButton *button, gpointer user_data)
{
    char a_day[10];
    GtkWidget *wAppointment;
    char *title;
    guint day;

    gtk_widget_destroy(clearwarn);

    if (xfical_file_open()){
        wAppointment = (GtkWidget *) user_data;
        title = (char*)gtk_window_get_title(GTK_WINDOW (wAppointment));
        title_to_ical(title, a_day);
        rmday_ical_app(a_day);
        xfical_file_close();
        day = atoi(a_day+6);
        gtk_calendar_unmark_day(GTK_CALENDAR(xfcal->mCalendar), day);
        recreate_wAppointment(wAppointment);
    }
}
