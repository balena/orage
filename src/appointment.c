/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2005-2008 Juha Kautto  (juha at xfce.org)
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
#  include <config.h>
#endif

#define _XOPEN_SOURCE /* glibc2 needs this */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <locale.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>

#include "orage-i18n.h"
#include "functions.h"
#include "mainbox.h"
#include "ical-code.h"
#include "timezone_selection.h"
#include "event-list.h"
#include "day-view.h"
#include "appointment.h"
#include "parameters.h"
#include "reminder.h"

#define BORDER_SIZE 20
#define FILETYPE_SIZE 38


typedef struct _orage_category_win
{
    GtkWidget *window;
    GtkWidget *vbox;

    GtkWidget *new_frame;
    GtkWidget *new_entry;
    GtkWidget *new_color_button;
    GtkWidget *new_add_button;

    GtkWidget *cur_frame;
    GtkWidget *cur_frame_vbox;

    GtkTooltips *tooltips;
    GtkAccelGroup *accelgroup;

    gpointer *apptw;
} category_win_struct;

static void refresh_categories(category_win_struct *catw);
static void read_default_alarm(xfical_appt *appt);

/*  
 *  these are the main functions in this file:
 */ 
static void fill_appt_window(appt_win *apptw, char *action, char *par);
static gboolean fill_appt_from_apptw(xfical_appt *appt, appt_win *apptw);


static GtkWidget *datetime_hbox_new(GtkWidget *date_button
        , GtkWidget *spin_hh, GtkWidget *spin_mm
        , GtkWidget *timezone_button)
{
    GtkWidget *hbox, *space_label;

    hbox = gtk_hbox_new(FALSE, 0);

    gtk_box_pack_start(GTK_BOX(hbox), date_button, FALSE, FALSE, 0);

    space_label = gtk_label_new("  ");
    gtk_box_pack_start(GTK_BOX(hbox), space_label, FALSE, FALSE, 0);

    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(spin_hh), TRUE);
    /* gtk_widget_set_size_request(spin_hh, 40, -1); */
    gtk_box_pack_start(GTK_BOX(hbox), spin_hh, FALSE, FALSE, 0);

    space_label = gtk_label_new(":");
    gtk_box_pack_start(GTK_BOX(hbox), space_label, FALSE, FALSE, 0);

    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(spin_mm), TRUE);
    /* gtk_widget_set_size_request(spin_mm, 40, -1); */
    gtk_box_pack_start(GTK_BOX(hbox), spin_mm, FALSE, FALSE, 0);

    if (timezone_button) {
        space_label = gtk_label_new("  ");
        gtk_box_pack_start(GTK_BOX(hbox), space_label, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), timezone_button, TRUE, TRUE, 0);
    }

    return(hbox);
}

static void mark_appointment_changed(appt_win *apptw)
{
    if (!apptw->appointment_changed) {
        apptw->appointment_changed = TRUE;
        gtk_widget_set_sensitive(apptw->Revert, TRUE);
        gtk_widget_set_sensitive(apptw->File_menu_revert, TRUE);
    }
}

static void mark_appointment_unchanged(appt_win *apptw)
{
    if (apptw->appointment_changed) {
        apptw->appointment_changed = FALSE;
        gtk_widget_set_sensitive(apptw->Revert, FALSE);
        gtk_widget_set_sensitive(apptw->File_menu_revert, FALSE);
    }
}

static void set_time_sensitivity(appt_win *apptw)
{
    gboolean dur_act, allDay_act, comp_act, due_act;

    dur_act = gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(apptw->Dur_checkbutton));
    allDay_act = gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(apptw->AllDay_checkbutton));
    comp_act = gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(apptw->Completed_checkbutton));
        /* todo can be without due date, but event always has end date.
         * Journal has none, but we show it similarly than event */
    if (gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(apptw->Type_todo_rb)))
        due_act = gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(apptw->End_checkbutton));
    else 
        due_act = TRUE;

    gtk_widget_set_sensitive(apptw->Dur_checkbutton, due_act);
    if (allDay_act) {
        gtk_widget_set_sensitive(apptw->StartTime_spin_hh, FALSE);
        gtk_widget_set_sensitive(apptw->StartTime_spin_mm, FALSE);
        gtk_widget_set_sensitive(apptw->StartTimezone_button, FALSE);
        gtk_widget_set_sensitive(apptw->EndTime_spin_hh, FALSE);
        gtk_widget_set_sensitive(apptw->EndTime_spin_mm, FALSE);
        gtk_widget_set_sensitive(apptw->EndTimezone_button, FALSE);
        gtk_widget_set_sensitive(apptw->Dur_spin_hh, FALSE);
        gtk_widget_set_sensitive(apptw->Dur_spin_hh_label, FALSE);
        gtk_widget_set_sensitive(apptw->Dur_spin_mm, FALSE);
        gtk_widget_set_sensitive(apptw->Dur_spin_mm_label, FALSE);
        if (dur_act) {
            gtk_widget_set_sensitive(apptw->EndDate_button, FALSE);
            gtk_widget_set_sensitive(apptw->Dur_spin_dd, due_act);
            gtk_widget_set_sensitive(apptw->Dur_spin_dd_label, due_act);
        }
        else {
            gtk_widget_set_sensitive(apptw->EndDate_button, due_act);
            gtk_widget_set_sensitive(apptw->Dur_spin_dd, FALSE);
            gtk_widget_set_sensitive(apptw->Dur_spin_dd_label, FALSE);
        }
    }
    else {
        gtk_widget_set_sensitive(apptw->StartTime_spin_hh, TRUE);
        gtk_widget_set_sensitive(apptw->StartTime_spin_mm, TRUE);
        gtk_widget_set_sensitive(apptw->StartTimezone_button, TRUE);
        if (dur_act) {
            gtk_widget_set_sensitive(apptw->EndDate_button, FALSE);
            gtk_widget_set_sensitive(apptw->EndTime_spin_hh, FALSE);
            gtk_widget_set_sensitive(apptw->EndTime_spin_mm, FALSE);
            gtk_widget_set_sensitive(apptw->EndTimezone_button, FALSE);
            gtk_widget_set_sensitive(apptw->Dur_spin_dd, due_act);
            gtk_widget_set_sensitive(apptw->Dur_spin_dd_label, due_act);
            gtk_widget_set_sensitive(apptw->Dur_spin_hh, due_act);
            gtk_widget_set_sensitive(apptw->Dur_spin_hh_label, due_act);
            gtk_widget_set_sensitive(apptw->Dur_spin_mm, due_act);
            gtk_widget_set_sensitive(apptw->Dur_spin_mm_label, due_act);
        }
        else {
            gtk_widget_set_sensitive(apptw->EndDate_button, due_act);
            gtk_widget_set_sensitive(apptw->EndTime_spin_hh, due_act);
            gtk_widget_set_sensitive(apptw->EndTime_spin_mm, due_act);
            gtk_widget_set_sensitive(apptw->EndTimezone_button, due_act);
            gtk_widget_set_sensitive(apptw->Dur_spin_dd, FALSE);
            gtk_widget_set_sensitive(apptw->Dur_spin_dd_label, FALSE);
            gtk_widget_set_sensitive(apptw->Dur_spin_hh, FALSE);
            gtk_widget_set_sensitive(apptw->Dur_spin_hh_label, FALSE);
            gtk_widget_set_sensitive(apptw->Dur_spin_mm, FALSE);
            gtk_widget_set_sensitive(apptw->Dur_spin_mm_label, FALSE);
        }
    }
    gtk_widget_set_sensitive(apptw->CompletedDate_button, comp_act);
    gtk_widget_set_sensitive(apptw->CompletedTime_spin_hh, comp_act);
    gtk_widget_set_sensitive(apptw->CompletedTime_spin_mm, comp_act);
    gtk_widget_set_sensitive(apptw->CompletedTimezone_button, comp_act);
}

static void set_repeat_sensitivity(appt_win *apptw)
{
    gint freq, i;
    
    freq = gtk_combo_box_get_active(GTK_COMBO_BOX(apptw->Recur_freq_cb));
    if (freq == XFICAL_FREQ_NONE) {
        gtk_widget_set_sensitive(apptw->Recur_limit_rb, FALSE);
        gtk_widget_set_sensitive(apptw->Recur_count_rb, FALSE);
        gtk_widget_set_sensitive(apptw->Recur_count_spin, FALSE);
        gtk_widget_set_sensitive(apptw->Recur_count_label, FALSE);
        gtk_widget_set_sensitive(apptw->Recur_until_rb, FALSE);
        gtk_widget_set_sensitive(apptw->Recur_until_button, FALSE);
        for (i=0; i <= 6; i++) {
            gtk_widget_set_sensitive(apptw->Recur_byday_cb[i], FALSE);
            gtk_widget_set_sensitive(apptw->Recur_byday_spin[i], FALSE);
        }
        gtk_widget_set_sensitive(apptw->Recur_int_spin, FALSE);
        gtk_widget_set_sensitive(apptw->Recur_int_spin_label1, FALSE);
        gtk_widget_set_sensitive(apptw->Recur_int_spin_label2, FALSE);
        gtk_widget_set_sensitive(apptw->Recur_todo_base_hbox, FALSE);
        /*
        gtk_widget_set_sensitive(apptw->Recur_exception_hbox, FALSE);
        gtk_widget_set_sensitive(apptw->Recur_calendar_hbox, FALSE);
        */
    }
    else {
        gtk_widget_set_sensitive(apptw->Recur_limit_rb, TRUE);
        gtk_widget_set_sensitive(apptw->Recur_count_rb, TRUE);
        if (gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(apptw->Recur_count_rb))) {
            gtk_widget_set_sensitive(apptw->Recur_count_spin, TRUE);
            gtk_widget_set_sensitive(apptw->Recur_count_label, TRUE);
        }
        else {
            gtk_widget_set_sensitive(apptw->Recur_count_spin, FALSE);
            gtk_widget_set_sensitive(apptw->Recur_count_label, FALSE);
        }
        gtk_widget_set_sensitive(apptw->Recur_until_rb, TRUE);
        if (gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(apptw->Recur_until_rb))) {
            gtk_widget_set_sensitive(apptw->Recur_until_button, TRUE);
        }
        else {
            gtk_widget_set_sensitive(apptw->Recur_until_button, FALSE);
        }
        for (i=0; i <= 6; i++) {
            gtk_widget_set_sensitive(apptw->Recur_byday_cb[i], TRUE);
        }
        if (freq == XFICAL_FREQ_MONTHLY || freq == XFICAL_FREQ_YEARLY) {
            for (i=0; i <= 6; i++) {
                gtk_widget_set_sensitive(apptw->Recur_byday_spin[i], TRUE);
            }
        }
        else {
            for (i=0; i <= 6; i++) {
                gtk_widget_set_sensitive(apptw->Recur_byday_spin[i], FALSE);
            }
        }
        gtk_widget_set_sensitive(apptw->Recur_int_spin, TRUE);
        gtk_widget_set_sensitive(apptw->Recur_int_spin_label1, TRUE);
        gtk_widget_set_sensitive(apptw->Recur_int_spin_label2, TRUE);
        gtk_widget_set_sensitive(apptw->Recur_todo_base_hbox, TRUE);
        /*
        gtk_widget_set_sensitive(apptw->Recur_exception_hbox, TRUE);
        gtk_widget_set_sensitive(apptw->Recur_calendar_hbox, TRUE);
        */
    }
}

static void recur_hide_show(appt_win *apptw)
{
    if (gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(apptw->Recur_feature_normal_rb))) {
        gtk_widget_hide(apptw->Recur_byday_label);
        gtk_widget_hide(apptw->Recur_byday_hbox);
        gtk_widget_hide(apptw->Recur_byday_spin_label);
        gtk_widget_hide(apptw->Recur_byday_spin_hbox);
    }
    else {
        gtk_widget_show(apptw->Recur_byday_label);
        gtk_widget_show(apptw->Recur_byday_hbox);
        gtk_widget_show(apptw->Recur_byday_spin_label);
        gtk_widget_show(apptw->Recur_byday_spin_hbox);
    }
}

static void type_hide_show(appt_win *apptw)
{
    if (gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(apptw->Type_event_rb))) {
        gtk_label_set_text(GTK_LABEL(apptw->End_label), _("End"));
        gtk_widget_show(apptw->Availability_label);
        gtk_widget_show(apptw->Availability_cb);
        gtk_widget_hide(apptw->End_checkbutton);
        gtk_widget_hide(apptw->Completed_label);
        gtk_widget_hide(apptw->Completed_hbox);
        gtk_widget_set_sensitive(apptw->Alarm_notebook_page, TRUE);
        gtk_widget_set_sensitive(apptw->Alarm_tab_label, TRUE);
        gtk_widget_set_sensitive(apptw->Recur_notebook_page, TRUE);
        gtk_widget_set_sensitive(apptw->Recur_tab_label, TRUE);
        gtk_widget_set_sensitive(apptw->Location_label, TRUE);
        gtk_widget_set_sensitive(apptw->Location_entry, TRUE);
        gtk_widget_set_sensitive(apptw->End_label, TRUE);
        gtk_widget_set_sensitive(apptw->EndTime_hbox, TRUE);
        gtk_widget_set_sensitive(apptw->Dur_hbox, TRUE);
        gtk_widget_hide(apptw->Recur_todo_base_label);
        gtk_widget_hide(apptw->Recur_todo_base_hbox);
    }
    else if (gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(apptw->Type_todo_rb))) {
        gtk_label_set_text(GTK_LABEL(apptw->End_label), _("Due"));
        gtk_widget_hide(apptw->Availability_label);
        gtk_widget_hide(apptw->Availability_cb);
        gtk_widget_show(apptw->End_checkbutton);
        gtk_widget_show(apptw->Completed_label);
        gtk_widget_show(apptw->Completed_hbox);
        gtk_widget_set_sensitive(apptw->Alarm_notebook_page, TRUE);
        gtk_widget_set_sensitive(apptw->Alarm_tab_label, TRUE);
        gtk_widget_set_sensitive(apptw->Recur_notebook_page, TRUE);
        gtk_widget_set_sensitive(apptw->Recur_tab_label, TRUE);
        gtk_widget_set_sensitive(apptw->Location_label, TRUE);
        gtk_widget_set_sensitive(apptw->Location_entry, TRUE);
        gtk_widget_set_sensitive(apptw->End_label, TRUE);
        gtk_widget_set_sensitive(apptw->EndTime_hbox, TRUE);
        gtk_widget_set_sensitive(apptw->Dur_hbox, TRUE);
        gtk_widget_show(apptw->Recur_todo_base_label);
        gtk_widget_show(apptw->Recur_todo_base_hbox);
    }
    else /* if (gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(apptw->Type_journal_rb))) */ {
        gtk_label_set_text(GTK_LABEL(apptw->End_label), _("End"));
        gtk_widget_hide(apptw->Availability_label);
        gtk_widget_hide(apptw->Availability_cb);
        gtk_widget_hide(apptw->End_checkbutton);
        gtk_widget_hide(apptw->Completed_label);
        gtk_widget_hide(apptw->Completed_hbox);
        gtk_widget_set_sensitive(apptw->Alarm_notebook_page, FALSE);
        gtk_widget_set_sensitive(apptw->Alarm_tab_label, FALSE);
        gtk_widget_set_sensitive(apptw->Recur_notebook_page, FALSE);
        gtk_widget_set_sensitive(apptw->Recur_tab_label, FALSE);
        gtk_widget_set_sensitive(apptw->Location_label, FALSE);
        gtk_widget_set_sensitive(apptw->Location_entry, FALSE);
        gtk_widget_set_sensitive(apptw->End_label, FALSE);
        gtk_widget_set_sensitive(apptw->EndTime_hbox, FALSE);
        gtk_widget_set_sensitive(apptw->Dur_hbox, FALSE);
        gtk_widget_hide(apptw->Recur_todo_base_label);
        gtk_widget_hide(apptw->Recur_todo_base_hbox);
    }
    set_time_sensitivity(apptw); /* todo has different settings */
}

static void set_sound_sensitivity(appt_win *apptw)
{
    gboolean sound_act, repeat_act;

    sound_act = gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(apptw->Sound_checkbutton));
    repeat_act = gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(apptw->SoundRepeat_checkbutton));

    if (sound_act) {
        gtk_widget_set_sensitive(apptw->Sound_entry, TRUE);
        gtk_widget_set_sensitive(apptw->Sound_button, TRUE);
        gtk_widget_set_sensitive(apptw->SoundRepeat_hbox, TRUE);
        gtk_widget_set_sensitive(apptw->SoundRepeat_checkbutton, TRUE);
        if (repeat_act) {
            gtk_widget_set_sensitive(apptw->SoundRepeat_spin_cnt, TRUE);
            gtk_widget_set_sensitive(apptw->SoundRepeat_spin_cnt_label, TRUE);
            gtk_widget_set_sensitive(apptw->SoundRepeat_spin_len, TRUE);
            gtk_widget_set_sensitive(apptw->SoundRepeat_spin_len_label, TRUE);
        }
        else {
            gtk_widget_set_sensitive(apptw->SoundRepeat_spin_cnt, FALSE);
            gtk_widget_set_sensitive(apptw->SoundRepeat_spin_cnt_label, FALSE);
            gtk_widget_set_sensitive(apptw->SoundRepeat_spin_len, FALSE);
            gtk_widget_set_sensitive(apptw->SoundRepeat_spin_len_label, FALSE);
        }
    }
    else {
        gtk_widget_set_sensitive(apptw->Sound_entry, FALSE);
        gtk_widget_set_sensitive(apptw->Sound_button, FALSE);
        gtk_widget_set_sensitive(apptw->SoundRepeat_hbox, FALSE); /* enough */
    }
}

static void set_notify_sensitivity(appt_win *apptw)
{
#ifdef HAVE_NOTIFY
    gboolean notify_act, expire_act;

    notify_act = gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(apptw->Display_checkbutton_notify));
    expire_act = gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(apptw->Display_checkbutton_expire_notify));

    if (notify_act) {
        gtk_widget_set_sensitive(apptw->Display_checkbutton_expire_notify
                , TRUE);
        if (expire_act) {
            gtk_widget_set_sensitive(apptw->Display_spin_expire_notify, TRUE);
            gtk_widget_set_sensitive(apptw->Display_spin_expire_notify_label
                    , TRUE);
        }
        else {
            gtk_widget_set_sensitive(apptw->Display_spin_expire_notify, FALSE);
            gtk_widget_set_sensitive(apptw->Display_spin_expire_notify_label
                    , FALSE);
        }
    }
    else {
        gtk_widget_set_sensitive(apptw->Display_checkbutton_expire_notify
                , FALSE);
        gtk_widget_set_sensitive(apptw->Display_spin_expire_notify, FALSE);
        gtk_widget_set_sensitive(apptw->Display_spin_expire_notify_label
                , FALSE);
    }
#endif
}

static void set_proc_sensitivity(appt_win *apptw)
{
    gboolean proc_act;

    proc_act = gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(apptw->Proc_checkbutton));

    if (proc_act) {
        gtk_widget_set_sensitive(apptw->Proc_entry, TRUE);
    }
    else {
        gtk_widget_set_sensitive(apptw->Proc_entry, FALSE);
    }
}

static void app_type_checkbutton_clicked_cb(GtkCheckButton *cb
        , gpointer user_data)
{
    type_hide_show((appt_win *)user_data);
    mark_appointment_changed((appt_win *)user_data);
}

static void on_appTitle_entry_changed_cb(GtkEditable *entry
        , gpointer user_data)
{
    gchar *title, *application_name;
    const gchar *appointment_name;
    appt_win *apptw = (appt_win *)user_data;

    appointment_name = gtk_entry_get_text((GtkEntry *)apptw->Title_entry);
    application_name = _("Orage");

    if (ORAGE_STR_EXISTS(appointment_name))
        title = g_strdup_printf("%s - %s", appointment_name, application_name);
    else
        title = g_strdup_printf("%s", application_name);

    gtk_window_set_title(GTK_WINDOW(apptw->Window), (const gchar *)title);

    g_free(title);
    mark_appointment_changed((appt_win *)user_data);
}

static void app_time_checkbutton_clicked_cb(GtkCheckButton *cb
        , gpointer user_data)
{
    set_time_sensitivity((appt_win *)user_data);
    mark_appointment_changed((appt_win *)user_data);
}

static void refresh_recur_calendars(appt_win *apptw)
{
    xfical_appt *appt;

    appt = (xfical_appt *)apptw->xf_appt;
    if (apptw->appointment_changed) {
        fill_appt_from_apptw(appt, apptw);
    }
    xfical_mark_calendar_recur(GTK_CALENDAR(apptw->Recur_calendar1), appt);
    xfical_mark_calendar_recur(GTK_CALENDAR(apptw->Recur_calendar2), appt);
    xfical_mark_calendar_recur(GTK_CALENDAR(apptw->Recur_calendar3), appt);
}

static void on_notebook_page_switch(GtkNotebook *notebook
        , GtkNotebookPage *page, guint page_num, gpointer user_data)
{
    if (page_num == 2)
        refresh_recur_calendars((appt_win *)user_data);
}

static void app_recur_checkbutton_clicked_cb(GtkCheckButton *checkbutton
        , gpointer user_data)
{
    set_repeat_sensitivity((appt_win *)user_data);
    mark_appointment_changed((appt_win *)user_data);
    refresh_recur_calendars((appt_win *)user_data);
}

static void app_recur_feature_checkbutton_clicked_cb(GtkCheckButton *cb
        , gpointer user_data)
{
    recur_hide_show((appt_win *)user_data);
    mark_appointment_changed((appt_win *)user_data);
}

static void app_sound_checkbutton_clicked_cb(GtkCheckButton *cb
        , gpointer user_data)
{
    set_sound_sensitivity((appt_win *)user_data);
    mark_appointment_changed((appt_win *)user_data);
}

#ifdef HAVE_NOTIFY
static void app_notify_checkbutton_clicked_cb(GtkCheckButton *cb
        , gpointer user_data)
{
    set_notify_sensitivity((appt_win *)user_data);
    mark_appointment_changed((appt_win *)user_data);
}
#endif

static void app_proc_checkbutton_clicked_cb(GtkCheckButton *cb
        , gpointer user_data)
{
    set_proc_sensitivity((appt_win *)user_data);
    mark_appointment_changed((appt_win *)user_data);
}

static void app_checkbutton_clicked_cb(GtkCheckButton *cb, gpointer user_data)
{
    mark_appointment_changed((appt_win *)user_data);
}

static void refresh_dependent_data(appt_win *apptw)
{
    if (apptw->el != NULL)
        refresh_el_win((el_win *)apptw->el);
    if (apptw->dw != NULL)
        refresh_day_win((day_win *)apptw->dw);
    orage_mark_appointments();
}

static void on_appNote_buffer_changed_cb(GtkTextBuffer *b, gpointer user_data)
{
    appt_win *apptw = (appt_win *)user_data;
    GtkTextIter start, end, match_start, match_end;
    GtkTextBuffer *tb;
    gchar *cdate, ctime[6], *cdatetime;
    struct tm *tm;

    tb = apptw->Note_buffer;
    gtk_text_buffer_get_bounds(tb, &start, &end);
    if (gtk_text_iter_forward_search(&start, "<D>"
                , GTK_TEXT_SEARCH_TEXT_ONLY
                , &match_start, &match_end, &end)) { /* found it */
        cdate = orage_localdate_i18();
        gtk_text_buffer_delete(tb, &match_start, &match_end);
        gtk_text_buffer_insert(tb, &match_start, cdate, -1);
    }
    else if (gtk_text_iter_forward_search(&start, "<T>"
                , GTK_TEXT_SEARCH_TEXT_ONLY
                , &match_start, &match_end, &end)) { /* found it */
        tm = orage_localtime();
        g_sprintf(ctime, "%02d:%02d", tm->tm_hour, tm->tm_min);
        gtk_text_buffer_delete(tb, &match_start, &match_end);
        gtk_text_buffer_insert(tb, &match_start, ctime, -1);
    }
    else if (gtk_text_iter_forward_search(&start, "<DT>"
                , GTK_TEXT_SEARCH_TEXT_ONLY
                , &match_start, &match_end, &end)) { /* found it */
        tm = orage_localtime();
        cdate = orage_tm_date_to_i18_date(tm);
        g_sprintf(ctime, "%02d:%02d", tm->tm_hour, tm->tm_min);
        cdatetime = g_strconcat(cdate, " ", ctime, NULL);
        gtk_text_buffer_delete(tb, &match_start, &match_end);
        gtk_text_buffer_insert(tb, &match_start, cdatetime, -1);
        g_free(cdatetime);
    }
   
    mark_appointment_changed((appt_win *)user_data);
}

static void on_app_entry_changed_cb(GtkEditable *entry, gpointer user_data)
{
    mark_appointment_changed((appt_win *)user_data);
}

static void on_freq_combobox_changed_cb(GtkComboBox *cb, gpointer user_data)
{
    set_repeat_sensitivity((appt_win *)user_data);
    mark_appointment_changed((appt_win *)user_data);
    refresh_recur_calendars((appt_win *)user_data);
}

static void on_app_combobox_changed_cb(GtkComboBox *cb, gpointer user_data)
{
    mark_appointment_changed((appt_win *)user_data);
}

static void on_app_spin_button_changed_cb(GtkSpinButton *sb, gpointer user_data)
{
    mark_appointment_changed((appt_win *)user_data);
}

static void on_recur_spin_button_changed_cb(GtkSpinButton *sb
        , gpointer user_data)
{
    mark_appointment_changed((appt_win *)user_data);
    refresh_recur_calendars((appt_win *)user_data);
}

static void on_appSound_button_clicked_cb(GtkButton *button, gpointer user_data)
{
    appt_win *apptw = (appt_win *)user_data;
    GtkWidget *file_chooser;
    GtkFileFilter *filter;
    gchar *appSound_entry_filename;
    gchar *sound_file;
    const gchar *filetype[FILETYPE_SIZE] = {
        "*.aiff", "*.al", "*.alsa", "*.au", "*.auto", "*.avr",
        "*.cdr", "*.cvs", "*.dat", "*.vms", "*.gsm", "*.hcom", 
        "*.la", "*.lu", "*.maud", "*.mp3", "*.nul", "*.ogg", "*.ossdsp",
        "*.prc", "*.raw", "*.sb", "*.sf", "*.sl", "*.smp", "*.sndt", 
        "*.sph", "*.8svx", "*.sw", "*.txw", "*.ub", "*.ul", "*.uw",
        "*.voc", "*.vorbis", "*.vox", "*.wav", "*.wve"};
    register int i;

    appSound_entry_filename = g_strdup(gtk_entry_get_text(
            (GtkEntry *)apptw->Sound_entry));
    file_chooser = gtk_file_chooser_dialog_new(_("Select a file..."),
            GTK_WINDOW(apptw->Window),
            GTK_FILE_CHOOSER_ACTION_OPEN,
            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
            GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
            NULL);

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, _("Sound Files"));
    for (i = 0; i < FILETYPE_SIZE; i++) {
        gtk_file_filter_add_pattern(filter, filetype[i]);
    }
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(file_chooser), filter);

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, _("All Files"));
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(file_chooser), filter);

    gtk_file_chooser_add_shortcut_folder(GTK_FILE_CHOOSER(file_chooser)
            , PACKAGE_DATA_DIR "/orage/sounds", NULL);

    if (strlen(appSound_entry_filename) > 0)
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(file_chooser)
                , (const gchar *) appSound_entry_filename);
    else
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(file_chooser)
            , PACKAGE_DATA_DIR "/orage/sounds");

    if (gtk_dialog_run(GTK_DIALOG(file_chooser)) == GTK_RESPONSE_ACCEPT) {
        sound_file = 
            gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser));
        if (sound_file) {
            gtk_entry_set_text(GTK_ENTRY(apptw->Sound_entry), sound_file);
            gtk_editable_set_position(GTK_EDITABLE(apptw->Sound_entry), -1);
        }
    }

    gtk_widget_destroy(file_chooser);
    g_free(appSound_entry_filename);
}

static void app_free_memory(appt_win *apptw)
{
    /* remove myself from event list appointment monitoring list */
    if (apptw->el)
        ((el_win *)apptw->el)->apptw_list = 
                g_list_remove(((el_win *)apptw->el)->apptw_list, apptw);
    /* remove myself from day list appointment monitoring list */
    else if (apptw->dw)
        ((day_win *)apptw->dw)->apptw_list = 
                g_list_remove(((day_win *)apptw->dw)->apptw_list, apptw);
    gtk_widget_destroy(apptw->Window);
    gtk_object_destroy(GTK_OBJECT(apptw->Tooltips));
    g_free(apptw->xf_uid);
    g_free(apptw->par);
    xfical_appt_free((xfical_appt *)apptw->xf_appt);
    g_free(apptw);
}

static gboolean appWindow_check_and_close(appt_win *apptw)
{
    gint result;

    if (apptw->appointment_changed == TRUE) {
        result = orage_warning_dialog(GTK_WINDOW(apptw->Window)
                , _("The appointment information has been modified.")
                , _("Do you want to continue?"));

        if (result == GTK_RESPONSE_ACCEPT) {
            app_free_memory(apptw);
        }
    }
    else {
        app_free_memory(apptw);
    }
    return(TRUE);
}

static gboolean on_appWindow_delete_event_cb(GtkWidget *widget, GdkEvent *event
    , gpointer user_data)
{
    return(appWindow_check_and_close((appt_win *)user_data));
}

static gboolean orage_validate_datetime(appt_win *apptw, xfical_appt *appt)
{
    gint result;

    /* Journal does not have end time so no need to check */
    if (appt->type == XFICAL_TYPE_JOURNAL
    ||  (appt->type == XFICAL_TYPE_TODO && !appt->use_due_time))
        return(TRUE);

    if (xfical_compare_times(appt) > 0) {
        result = orage_error_dialog(GTK_WINDOW(apptw->Window)
                , _("The end of this appointment is earlier than the beginning.")
                , NULL);
        return(FALSE);
    }
    else {
        return(TRUE);
    }
}

static void fill_appt_from_apptw_alarm(xfical_appt *appt, appt_win *apptw)
{
    gint i, j, k;
    gchar *tmp;

    /* reminder time */
    appt->alarmtime = gtk_spin_button_get_value_as_int(
            GTK_SPIN_BUTTON(apptw->Alarm_spin_dd)) * 24*60*60
                    + gtk_spin_button_get_value_as_int(
            GTK_SPIN_BUTTON(apptw->Alarm_spin_hh)) *    60*60
                    + gtk_spin_button_get_value_as_int(
            GTK_SPIN_BUTTON(apptw->Alarm_spin_mm)) *       60
                    ;
    appt->display_alarm_orage = appt->alarmtime ? TRUE : FALSE;

    /* reminder before/after related to start/end */
    /*
    char *when_array[4] = {_("Before Start"), _("Before End")
        , _("After Start"), _("After End")};
        */
    switch (gtk_combo_box_get_active(GTK_COMBO_BOX(apptw->Alarm_when_cb))) {
        case 0:
            appt->alarm_before = TRUE;
            appt->alarm_related_start = TRUE;
            break;
        case 1:
            appt->alarm_before = TRUE;
            appt->alarm_related_start = FALSE;
            break;
        case 2:
            appt->alarm_before = FALSE;
            appt->alarm_related_start = TRUE;
            break;
        case 3:
            appt->alarm_before = FALSE;
            appt->alarm_related_start = FALSE;
            break;
        default:
            appt->alarm_before = TRUE;
            appt->alarm_related_start = TRUE;
            break;
    }

    /* Do we use persistent alarm */
    appt->alarm_persistent = gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(apptw->Per_checkbutton));

    /* Do we use audio alarm */
    appt->sound_alarm = gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(apptw->Sound_checkbutton));

    /* Which sound file will be played */
    if (appt->sound) {
        g_free(appt->sound);
        appt->sound = NULL;
    }
    appt->sound = g_strdup(gtk_entry_get_text(GTK_ENTRY(apptw->Sound_entry)));

    /* the alarm will repeat until someone shuts it off 
     * or it has been played soundrepeat_cnt times */
    appt->soundrepeat = gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(apptw->SoundRepeat_checkbutton));
    appt->soundrepeat_cnt = gtk_spin_button_get_value_as_int(
            GTK_SPIN_BUTTON(apptw->SoundRepeat_spin_cnt));
    appt->soundrepeat_len = gtk_spin_button_get_value_as_int(
            GTK_SPIN_BUTTON(apptw->SoundRepeat_spin_len));

    /* Do we use orage display alarm */
    appt->display_alarm_orage = gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(apptw->Display_checkbutton_orage));

    /* Do we use notify display alarm */
#ifdef HAVE_NOTIFY
    appt->display_alarm_notify = gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(apptw->Display_checkbutton_notify));
#else
    appt->display_alarm_notify = FALSE;
#endif

    /* notify display alarm timeout */
#ifdef HAVE_NOTIFY
    if (gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(apptw->Display_checkbutton_expire_notify)))
        appt->display_notify_timeout = gtk_spin_button_get_value_as_int(
                GTK_SPIN_BUTTON(apptw->Display_spin_expire_notify));
    else 
        appt->display_notify_timeout = -1;
#else
    appt->display_notify_timeout = -1;
#endif

    /* Do we use procedure alarm */
    appt->procedure_alarm = gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(apptw->Proc_checkbutton));

    /* The actual command. 
     * Note that we need to split this into cmd and the parameters 
     * since that is how libical stores it */
    if (appt->procedure_cmd) {
        g_free(appt->procedure_cmd);
        appt->procedure_cmd = NULL;
    }
    if (appt->procedure_params) {
        g_free(appt->procedure_params);
        appt->procedure_params = NULL;
    }
    tmp = (char *)gtk_entry_get_text(GTK_ENTRY(apptw->Proc_entry));
    j = strlen(tmp);
    for (i = 0; i < j && g_ascii_isspace(tmp[i]); i++)
        ; /* skip blanks */
    for (k = i; k < j && !g_ascii_isspace(tmp[k]); k++)
        ; /* find first blank after the cmd */
        /* now i points to start of cmd and k points to end of cmd */
    if (k-i)
        appt->procedure_cmd = g_strndup(tmp+i, k-i);
    if (j-k)
        appt->procedure_params = g_strndup(tmp+k, j-k);
}

/*
 * Function fills an appointment with the contents of an appointment 
 * window
 */
static gboolean fill_appt_from_apptw(xfical_appt *appt, appt_win *apptw)
{
    GtkTextIter start, end;
    const char *time_format="%H:%M";
    struct tm current_t;
    gchar starttime[6], endtime[6], completedtime[6];
    gint i;
    gchar *tmp, *tmp2;
    /*
    GList *tmp_gl;
    */

/* Next line is fix for bug 2811.
 * We need to make sure spin buttons do not have values which are not
 * yet updated = visible.
 * gtk_spin_button_update call would do it, but it seems to cause
 * crash if done in on_app_spin_button_changed_cb and here we should
 * either do it for all spin fields, which takes too many lines of code.
 * if (GTK_WIDGET_HAS_FOCUS(apptw->StartTime_spin_hh))
 *      gtk_spin_button_update;
 * So we just change the focus field and then spin button value gets set.
 * It would be nice to not to have to change the field though.
 */
    gtk_widget_grab_focus(apptw->Title_entry);

            /*********** GENERAL TAB ***********/
    /* type */
    if (gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(apptw->Type_event_rb)))
        appt->type = XFICAL_TYPE_EVENT;
    else if (gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(apptw->Type_todo_rb)))
        appt->type = XFICAL_TYPE_TODO;
    else if (gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(apptw->Type_journal_rb)))
        appt->type = XFICAL_TYPE_JOURNAL;
    else
        g_warning("fill_appt_from_apptw: coding error, illegal type");

    /* title */
    appt->title = g_strdup(gtk_entry_get_text(GTK_ENTRY(apptw->Title_entry)));

    /* location */
    appt->location = g_strdup(gtk_entry_get_text(
            GTK_ENTRY(apptw->Location_entry)));

    /* all day */
    appt->allDay = gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(apptw->AllDay_checkbutton));

    /* start date and time. 
     * Note that timezone is kept upto date all the time 
     */
    current_t = orage_i18_date_to_tm_date(gtk_button_get_label(
            GTK_BUTTON(apptw->StartDate_button)));
    g_sprintf(starttime, "%02d:%02d"
            , gtk_spin_button_get_value_as_int(
                    GTK_SPIN_BUTTON(apptw->StartTime_spin_hh))
            , gtk_spin_button_get_value_as_int(
                    GTK_SPIN_BUTTON(apptw->StartTime_spin_mm)));
    strptime(starttime, time_format, &current_t);
    g_sprintf(appt->starttime, XFICAL_APPT_TIME_FORMAT
            , current_t.tm_year + 1900, current_t.tm_mon + 1, current_t.tm_mday
            , current_t.tm_hour, current_t.tm_min, 0);

    /* end date and time. 
     * Note that timezone is kept upto date all the time 
     */ 
    appt->use_due_time = gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(apptw->End_checkbutton));
    current_t = orage_i18_date_to_tm_date(gtk_button_get_label(
            GTK_BUTTON(apptw->EndDate_button)));
    g_sprintf(endtime, "%02d:%02d"
            , gtk_spin_button_get_value_as_int(
                    GTK_SPIN_BUTTON(apptw->EndTime_spin_hh))
            , gtk_spin_button_get_value_as_int(
                    GTK_SPIN_BUTTON(apptw->EndTime_spin_mm)));
    strptime(endtime, time_format, &current_t);
    g_sprintf(appt->endtime, XFICAL_APPT_TIME_FORMAT
            , current_t.tm_year + 1900, current_t.tm_mon + 1, current_t.tm_mday
            , current_t.tm_hour, current_t.tm_min, 0);

    /* duration */
    appt->use_duration = gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(apptw->Dur_checkbutton));
    if (appt->allDay)
        appt->duration = gtk_spin_button_get_value_as_int(
                GTK_SPIN_BUTTON(apptw->Dur_spin_dd)) * 24*60*60;
    else
        appt->duration = gtk_spin_button_get_value_as_int(
                GTK_SPIN_BUTTON(apptw->Dur_spin_dd)) * 24*60*60
                        + gtk_spin_button_get_value_as_int(
                GTK_SPIN_BUTTON(apptw->Dur_spin_hh)) *    60*60
                        + gtk_spin_button_get_value_as_int(
                GTK_SPIN_BUTTON(apptw->Dur_spin_mm)) *       60;

    /* Check that end time is after start time. */
    if (!orage_validate_datetime(apptw, appt))
        return(FALSE);

    /* completed date and time. 
     * Note that timezone is kept upto date all the time 
     */ 
    appt->completed = gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(apptw->Completed_checkbutton));
    current_t = orage_i18_date_to_tm_date(gtk_button_get_label(
            GTK_BUTTON(apptw->CompletedDate_button)));
    g_sprintf(completedtime, "%02d:%02d"
            , gtk_spin_button_get_value_as_int(
                    GTK_SPIN_BUTTON(apptw->CompletedTime_spin_hh))
            , gtk_spin_button_get_value_as_int(
                    GTK_SPIN_BUTTON(apptw->CompletedTime_spin_mm)));
    strptime(completedtime, time_format, &current_t);
    g_sprintf(appt->completedtime, XFICAL_APPT_TIME_FORMAT
            , current_t.tm_year + 1900, current_t.tm_mon + 1, current_t.tm_mday
            , current_t.tm_hour, current_t.tm_min, 0);

    /* availability */
    appt->availability = gtk_combo_box_get_active(
            GTK_COMBO_BOX(apptw->Availability_cb));

    /* categories */
    tmp = g_strdup(gtk_entry_get_text(GTK_ENTRY(apptw->Categories_entry)));
    tmp2 = gtk_combo_box_get_active_text(GTK_COMBO_BOX(apptw->Categories_cb));
    if (!strcmp(tmp2, _("Not set"))) {
        g_free(tmp2);
        tmp2 = NULL;
    }
    if (ORAGE_STR_EXISTS(tmp)) {
        appt->categories = g_strjoin(",", tmp, tmp2, NULL);
        g_free(tmp);
        g_free(tmp2);
    }
    else
        appt->categories = tmp2;

    /* priority */
    appt->priority = gtk_spin_button_get_value_as_int(
            GTK_SPIN_BUTTON(apptw->Priority_spin));

    /* notes */
    gtk_text_buffer_get_bounds(apptw->Note_buffer, &start, &end);
    appt->note = gtk_text_iter_get_text(&start, &end);

            /*********** ALARM TAB ***********/
    fill_appt_from_apptw_alarm(appt, apptw);

            /*********** RECURRENCE TAB ***********/
    /* recurrence */
    appt->freq = gtk_combo_box_get_active(GTK_COMBO_BOX(apptw->Recur_freq_cb));

    /* recurrence interval */
    appt->interval = gtk_spin_button_get_value_as_int(
            GTK_SPIN_BUTTON(apptw->Recur_int_spin));

    /* recurrence ending */
    if (gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(apptw->Recur_limit_rb))) {
        appt->recur_limit = 0;    /* no limit */
        appt->recur_count = 0;    /* special: means no repeat count limit */
        appt->recur_until[0] = 0; /* special: means no until time limit */
    }
    else if (gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(apptw->Recur_count_rb))) {
        appt->recur_limit = 1;    /* count limit */
        appt->recur_count = gtk_spin_button_get_value_as_int(
                GTK_SPIN_BUTTON(apptw->Recur_count_spin));
        appt->recur_until[0] = 0; /* special: means no until time limit */
    }
    else if (gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(apptw->Recur_until_rb))) {
        appt->recur_limit = 2;    /* until limit */
        appt->recur_count = 0;    /* special: means no repeat count limit */

        current_t = orage_i18_date_to_tm_date(gtk_button_get_label(
                GTK_BUTTON(apptw->Recur_until_button)));
        g_sprintf(appt->recur_until, XFICAL_APPT_TIME_FORMAT
                , current_t.tm_year + 1900, current_t.tm_mon + 1
                , current_t.tm_mday, 23, 59, 10);
    }
    else
        g_warning("fill_appt_from_apptw: coding error, illegal recurrence");

    /* recurrence weekdays */
    for (i=0; i <= 6; i++) {
        appt->recur_byday[i] = gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(apptw->Recur_byday_cb[i]));
    /* recurrence weekday selection for month/yearly case */
        appt->recur_byday_cnt[i] = gtk_spin_button_get_value_as_int(
                GTK_SPIN_BUTTON(apptw->Recur_byday_spin[i]));
    }

    /* recurrence todo base */
    appt->recur_todo_base_start = gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(apptw->Recur_todo_base_start_rb));

    /* recurrence exceptions */
    /* is kept upto date all the time */
    /*
    g_print("fill_appt_from_apptw: checking data start\n");
    for (tmp_gl = g_list_first(appt->recur_exceptions);
         tmp_gl != NULL;
         tmp_gl = g_list_next(tmp_gl)) {
        g_print("fill_appt_from_apptw: checking data (%s)\n", ((xfical_exception *)tmp_gl->data)->time);
    }
    */

    return(TRUE);
}

static gboolean save_xfical_from_appt_win(appt_win *apptw)
{
    gboolean ok = FALSE;
    xfical_appt *appt = (xfical_appt *)apptw->xf_appt;

    if (fill_appt_from_apptw(appt, apptw)) {
        /* Here we try to save the event... */
        if (!xfical_file_open(TRUE))
            return(FALSE);
        if (apptw->appointment_add) {
            apptw->xf_uid = g_strdup(xfical_appt_add(appt));
            ok = (apptw->xf_uid ? TRUE : FALSE);
            if (ok) {
                apptw->appointment_add = FALSE;
                gtk_widget_set_sensitive(apptw->Duplicate, TRUE);
                gtk_widget_set_sensitive(apptw->File_menu_duplicate, TRUE);
                orage_message(10, "Added: %s", apptw->xf_uid);
            }
            else
                orage_message(150, "Addition failed: %s", apptw->xf_uid);
            }
        else {
            ok = xfical_appt_mod(apptw->xf_uid, appt);
            if (ok)
                orage_message(10, "Modified: %s", apptw->xf_uid);
            else
                orage_message(150, "Modification failed: %s", apptw->xf_uid);
        }
        xfical_file_close(TRUE);
        if (ok) {
            apptw->appointment_new = FALSE;
            mark_appointment_unchanged(apptw);
            refresh_dependent_data(apptw);
        }
    }
    return(ok);
}

static void on_appFileSave_menu_activate_cb(GtkMenuItem *mi, gpointer user_data)
{
    save_xfical_from_appt_win((appt_win *)user_data);
}

static void on_appSave_clicked_cb(GtkButton *b, gpointer user_data)
{
    save_xfical_from_appt_win((appt_win *)user_data);
}

static void save_xfical_from_appt_win_and_close(appt_win *apptw)
{
    if (save_xfical_from_appt_win(apptw)) {
        app_free_memory(apptw);
    }
}

static void on_appFileSaveClose_menu_activate_cb(GtkMenuItem *mi
        , gpointer user_data)
{
    save_xfical_from_appt_win_and_close((appt_win *)user_data);
}

static void on_appSaveClose_clicked_cb(GtkButton *b, gpointer user_data)
{
    save_xfical_from_appt_win_and_close((appt_win *)user_data);
}

static void delete_xfical_from_appt_win(appt_win *apptw)
{
    gint result;

    result = orage_warning_dialog(GTK_WINDOW(apptw->Window)
            , _("This appointment will be permanently removed.")
            , _("Do you want to continue?"));
                                 
    if (result == GTK_RESPONSE_ACCEPT) {
        if (!apptw->appointment_add) {
            if (!xfical_file_open(TRUE))
                    return;
            result = xfical_appt_del(apptw->xf_uid);
            if (result)
                orage_message(10, "Removed: %s", apptw->xf_uid);
            else
                g_warning("Removal failed: %s", apptw->xf_uid);
            xfical_file_close(TRUE);
        }

        refresh_dependent_data(apptw);
        app_free_memory(apptw);
    }
}

static void on_appDelete_clicked_cb(GtkButton *b, gpointer user_data)
{
    delete_xfical_from_appt_win((appt_win *)user_data);
}

static void on_appFileDelete_menu_activate_cb(GtkMenuItem *mi
        , gpointer user_data)
{
    delete_xfical_from_appt_win((appt_win *)user_data);
}

static void on_appFileClose_menu_activate_cb(GtkMenuItem *mi
        , gpointer user_data)
{
    appWindow_check_and_close((appt_win *)user_data);
}

static void duplicate_xfical_from_appt_win(appt_win *apptw)
{
    gint x, y;
    appt_win *apptw2;

    /* do not keep track of appointments created here */
    apptw2 = create_appt_win("COPY", apptw->xf_uid);
    gtk_window_get_position(GTK_WINDOW(apptw->Window), &x, &y);
    gtk_window_move(GTK_WINDOW(apptw2->Window), x+20, y+20);
}

static void on_appFileDuplicate_menu_activate_cb(GtkMenuItem *mi
        , gpointer user_data)
{
    duplicate_xfical_from_appt_win((appt_win *)user_data);
}

static void on_appDuplicate_clicked_cb(GtkButton *b, gpointer user_data)
{
    duplicate_xfical_from_appt_win((appt_win *)user_data);
}

static void revert_xfical_to_last_saved(appt_win *apptw)
{
    if (!apptw->appointment_new) {
        fill_appt_window(apptw, "UPDATE", apptw->xf_uid);
    }
    else {
        fill_appt_window(apptw, "NEW", apptw->par);
    }
}

static void on_appFileRevert_menu_activate_cb(GtkMenuItem *mi
        , gpointer user_data)
{
    revert_xfical_to_last_saved((appt_win *)user_data);
}

static void on_appRevert_clicked_cb(GtkWidget *b, gpointer *user_data)
{
    revert_xfical_to_last_saved((appt_win *)user_data);
}

static void on_Date_button_clicked_cb(GtkWidget *button, gpointer *user_data)
{
    appt_win *apptw = (appt_win *)user_data;

    if (orage_date_button_clicked(button, apptw->Window))
        mark_appointment_changed(apptw);
}

static void on_recur_Date_button_clicked_cb(GtkWidget *button
        , gpointer *user_data)
{
    appt_win *apptw = (appt_win *)user_data;

    if (orage_date_button_clicked(button, apptw->Window))
        mark_appointment_changed(apptw);
    refresh_recur_calendars((appt_win *)user_data);
}

static void on_appStartTimezone_clicked_cb(GtkButton *button
        , gpointer *user_data)
{
    appt_win *apptw = (appt_win *)user_data;
    xfical_appt *appt;

    appt = (xfical_appt *)apptw->xf_appt;
    if (orage_timezone_button_clicked(button, GTK_WINDOW(apptw->Window)
            , &appt->start_tz_loc, TRUE, g_par.local_timezone))
        mark_appointment_changed(apptw);
}

static void on_appEndTimezone_clicked_cb(GtkButton *button
        , gpointer *user_data)
{
    appt_win *apptw = (appt_win *)user_data;
    xfical_appt *appt;

    appt = (xfical_appt *)apptw->xf_appt;
    if (orage_timezone_button_clicked(button, GTK_WINDOW(apptw->Window)
            , &appt->end_tz_loc, TRUE, g_par.local_timezone))
        mark_appointment_changed(apptw);
}

static void on_appCompletedTimezone_clicked_cb(GtkButton *button
        , gpointer *user_data)
{
    appt_win *apptw = (appt_win *)user_data;
    xfical_appt *appt;

    appt = (xfical_appt *)apptw->xf_appt;
    if (orage_timezone_button_clicked(button, GTK_WINDOW(apptw->Window)
            , &appt->completed_tz_loc, TRUE, g_par.local_timezone))
        mark_appointment_changed(apptw);
}

static gint check_exists(gconstpointer a, gconstpointer b)
{
   /*  We actually only care about match or no match.*/
    if (!strcmp(((xfical_exception *)a)->time
              , ((xfical_exception *)b)->time)) {
        return(strcmp(((xfical_exception *)a)->type
                    , ((xfical_exception *)b)->type));
    }
    else {
        return(1); /* does not matter if it is smaller or bigger */
    }
}

static xfical_exception *new_exception(char *text)
{
    xfical_exception *recur_exception;
    gint i;

    recur_exception = g_new(xfical_exception, 1);
    i = strlen(text);
    text[i-2] = '\0';
    if (text[i-1] == '+') {
        strcpy(recur_exception->type, "RDATE");
        strcpy(recur_exception->time, orage_i18_time_to_icaltime(text));
    }
    else {
        strcpy(recur_exception->type, "EXDATE");
#ifdef HAVE_LIBICAL
        /* need to add time also as standard libical can not handle dates
           correctly yet. Check more from BUG 5764.
           We use start time from appointment. */
        strcpy(recur_exception->time, orage_i18_time_to_icaltime(text));
#else
        strcpy(recur_exception->time, orage_i18_date_to_icaldate(text));
#endif
    }
    text[i-2] = ' ';
    return(recur_exception);
}

static void recur_row_clicked(GtkWidget *widget
        , GdkEventButton *event, gpointer *user_data)
{
    appt_win *apptw = (appt_win *)user_data;
    xfical_appt *appt;
    gchar *text;
    GList *children;
    GtkWidget *lab;
    xfical_exception *recur_exception, *recur_exception_cur;
    GList *gl_pos;

    if (event->type == GDK_2BUTTON_PRESS) {
        /* first find the text */
        children = gtk_container_get_children(GTK_CONTAINER(widget));
        children = g_list_first(children);
        lab = (GtkWidget *)children->data;
        text = g_strdup(gtk_label_get_text(GTK_LABEL(lab)));

         /* Then, let's keep the GList updated */
        recur_exception = new_exception(text);
        appt = (xfical_appt *)apptw->xf_appt;
        g_free(text);
        if ((gl_pos = g_list_find_custom(appt->recur_exceptions
                    , recur_exception, check_exists))) {
            /* let's remove it */
            recur_exception_cur = gl_pos->data;
            appt->recur_exceptions = 
                    g_list_remove(appt->recur_exceptions, recur_exception_cur);
            g_free(recur_exception_cur);
        }
        else { 
            g_warning("recur_row_clicked: non existent row (%s)\n", recur_exception->time);
        }
        g_free(recur_exception);

        /* and finally update the display */
        gtk_widget_destroy(widget);
        mark_appointment_changed(apptw);
        refresh_recur_calendars(apptw);
    }
}


static gboolean add_recur_exception_row(char *p_time, char *p_type
        , appt_win *apptw, gboolean only_window)
{
    GtkWidget *ev, *label;
    gchar *text, tmp_type[2];
    xfical_appt *appt;
    xfical_exception *recur_exception;

    /* First build the data */
    if (!strcmp(p_type, "EXDATE"))
        strcpy(tmp_type, "-");
    else if (!strcmp(p_type, "RDATE"))
        strcpy(tmp_type, "+");
    else
        strcpy(tmp_type, p_type);
    text = g_strdup_printf("%s %s", p_time, tmp_type);

    /* Then, let's keep the GList updated */
    if (!only_window) {
        recur_exception = new_exception(text);
        appt = (xfical_appt *)apptw->xf_appt;
        if (g_list_find_custom(appt->recur_exceptions, recur_exception
                    , check_exists)) {
            /* this element is already in the list, so no need to add it again.
             * we just clean the memory and leave */
            g_free(recur_exception);
            g_free(text);
            return(FALSE);
        }
        else { /* need to add it */
            appt->recur_exceptions = g_list_prepend(appt->recur_exceptions
                    , recur_exception);
        }
    }

    /* finally put the value visible also */
    label = gtk_label_new(text);
    g_free(text);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    ev = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(ev), label);
    gtk_box_pack_start(GTK_BOX(apptw->Recur_exception_rows_vbox), ev
            , FALSE, FALSE, 0);
    g_signal_connect((gpointer)ev, "button-press-event"
            , G_CALLBACK(recur_row_clicked), apptw);
    gtk_widget_show(label);
    gtk_widget_show(ev);
    return(TRUE); /* we added the value */
}


static void recur_month_changed_cb(GtkCalendar *calendar, gpointer user_data)
{
    appt_win *apptw = (appt_win *)user_data;
    xfical_appt *appt;

    appt = (xfical_appt *)apptw->xf_appt;
    /* actually we do not have to do fill_appt_from_apptw always, 
     * but as we are not keeping track of changes, we just do it always */
    fill_appt_from_apptw(appt, apptw);
    xfical_mark_calendar_recur(calendar, appt);
}

static void recur_day_selected_double_click_cb(GtkCalendar *calendar
        , gpointer user_data)
{
    appt_win *apptw = (appt_win *)user_data;
    char *cal_date, *type;
    gint hh, mm;

    if (gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(apptw->Recur_exception_excl_rb))) {
        type = "-";
#ifdef HAVE_LIBICAL
        /* need to add time also as standard libical can not handle dates
           correctly yet. Check more from BUG 5764.
           We use start time from appointment. */
        hh =  gtk_spin_button_get_value_as_int(
                GTK_SPIN_BUTTON(apptw->StartTime_spin_hh));
        mm =  gtk_spin_button_get_value_as_int(
                GTK_SPIN_BUTTON(apptw->StartTime_spin_mm));
        cal_date = g_strdup(orage_cal_to_i18_time(calendar, hh, mm));
#else
        /* date is enough */
        cal_date = g_strdup(orage_cal_to_i18_date(calendar));
#endif
    }
    else { /* extra day. This needs also time */
        type = "+";
        hh =  gtk_spin_button_get_value_as_int(
                GTK_SPIN_BUTTON(apptw->Recur_exception_incl_spin_hh));
        mm =  gtk_spin_button_get_value_as_int(
                GTK_SPIN_BUTTON(apptw->Recur_exception_incl_spin_mm));
        cal_date = g_strdup(orage_cal_to_i18_time(calendar, hh, mm));
    }

    if (add_recur_exception_row(cal_date, type, apptw, FALSE)) { /* new data */
        mark_appointment_changed((appt_win *)user_data);
        refresh_recur_calendars((appt_win *)user_data);
    }
    g_free(cal_date);
}

static void fill_appt_window_times(appt_win *apptw, xfical_appt *appt)
{
    char *startdate_to_display, *enddate_to_display, *completeddate_to_display;
    int days, hours, minutes;
    struct tm tm_date;

    /* all day ? */
    gtk_toggle_button_set_active(
            GTK_TOGGLE_BUTTON(apptw->AllDay_checkbutton), appt->allDay);

    /* start time */
    if (strlen(appt->starttime) > 6 ) {
        tm_date = orage_icaltime_to_tm_time(appt->starttime, TRUE);
        startdate_to_display = orage_tm_date_to_i18_date(&tm_date);
        gtk_button_set_label(GTK_BUTTON(apptw->StartDate_button)
                , (const gchar *)startdate_to_display);
        gtk_spin_button_set_value(
                GTK_SPIN_BUTTON(apptw->StartTime_spin_hh)
                        , (gdouble)tm_date.tm_hour);
        gtk_spin_button_set_value(
                GTK_SPIN_BUTTON(apptw->Recur_exception_incl_spin_hh)
                        , (gdouble)tm_date.tm_hour);
        gtk_spin_button_set_value(
                GTK_SPIN_BUTTON(apptw->StartTime_spin_mm)
                        , (gdouble)tm_date.tm_min);
        gtk_spin_button_set_value(
                GTK_SPIN_BUTTON(apptw->Recur_exception_incl_spin_mm)
                        , (gdouble)tm_date.tm_min);
        if (appt->start_tz_loc)
            gtk_button_set_label(GTK_BUTTON(apptw->StartTimezone_button)
                    , _(appt->start_tz_loc));
        else /* we should never get here */
            g_warning("fill_appt_window_times: start_tz_loc is null");
    }
    else
        g_warning("fill_appt_window_times: starttime wrong %s", appt->uid);

    /* end time */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
            apptw->End_checkbutton), appt->use_due_time);
    if (strlen(appt->endtime) > 6 ) {
        tm_date = orage_icaltime_to_tm_time(appt->endtime, TRUE);
        enddate_to_display = orage_tm_date_to_i18_date(&tm_date);
        gtk_button_set_label(GTK_BUTTON(apptw->EndDate_button)
                , (const gchar *)enddate_to_display);
        gtk_spin_button_set_value(
                GTK_SPIN_BUTTON(apptw->EndTime_spin_hh)
                        , (gdouble)tm_date.tm_hour);
        gtk_spin_button_set_value(
                GTK_SPIN_BUTTON(apptw->EndTime_spin_mm)
                        , (gdouble)tm_date.tm_min);
        if (appt->end_tz_loc) {
            gtk_button_set_label(GTK_BUTTON(apptw->EndTimezone_button)
                    , _(appt->end_tz_loc));
        }
        else /* we should never get here */
            g_warning("fill_appt_window_times: end_tz_loc is null");
    }
    else
        g_warning("fill_appt_window_times: endtime wrong %s", appt->uid);

    /* duration */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(apptw->Dur_checkbutton)
            , appt->use_duration);
    days = appt->duration/(24*60*60);
    hours = (appt->duration-days*(24*60*60))/(60*60);
    minutes = (appt->duration-days*(24*60*60)-hours*(60*60))/(60);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(apptw->Dur_spin_dd)
            , (gdouble)days);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(apptw->Dur_spin_hh)
            , (gdouble)hours);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(apptw->Dur_spin_mm)
            , (gdouble)minutes);

    /* completed time (only available for todo) */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
            apptw->Completed_checkbutton), appt->completed);
    if (strlen(appt->completedtime) > 6 ) {
        tm_date = orage_icaltime_to_tm_time(appt->completedtime, TRUE);
        completeddate_to_display = orage_tm_date_to_i18_date(&tm_date);
        gtk_button_set_label(GTK_BUTTON(apptw->CompletedDate_button)
                , (const gchar *)completeddate_to_display);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(apptw->CompletedTime_spin_hh)
                , (gdouble)tm_date.tm_hour);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(apptw->CompletedTime_spin_mm)
                , (gdouble)tm_date.tm_min);
        if (appt->completed_tz_loc) {
            gtk_button_set_label(GTK_BUTTON(apptw->CompletedTimezone_button)
                    , _(appt->completed_tz_loc));
        }
        else /* we should never get here */
            g_warning("fill_appt_window_times: completed_tz_loc is null");
    }
    else
        g_warning("fill_appt_window_times: completedtime wrong %s", appt->uid);
}

static xfical_appt *fill_appt_window_get_appt(appt_win *apptw
    , char *action, char *par)
{
    xfical_appt *appt=NULL;
    struct tm *t;
    gchar today[9];

    if (strcmp(action, "NEW") == 0) {
/* par contains XFICAL_APPT_DATE_FORMAT (yyyymmdd) date for NEW appointment */
        appt = xfical_appt_alloc();
        t = orage_localtime();
        g_sprintf(today, "%04d%02d%02d", t->tm_year+1900, t->tm_mon+1
                , t->tm_mday);
        /* If we're today, we propose an appointment the next half-hour */
        /* hour 24 is wrong, we use 00 */
        if (strcmp(par, today) == 0 && t->tm_hour < 23) { 
            if (t->tm_min <= 30) {
                g_sprintf(appt->starttime,"%sT%02d%02d00"
                        , par, t->tm_hour, 30);
                g_sprintf(appt->endtime,"%sT%02d%02d00"
                        , par, t->tm_hour + 1, 00);
            }
            else {
                g_sprintf(appt->starttime,"%sT%02d%02d00"
                        , par, t->tm_hour + 1, 00);
                g_sprintf(appt->endtime,"%sT%02d%02d00"
                        , par, t->tm_hour + 1, 30);
            }
        }
        /* otherwise we suggest it at 09:00 in the morning. */
        else {
            g_sprintf(appt->starttime,"%sT090000", par);
            g_sprintf(appt->endtime,"%sT093000", par);
        }
        if (g_par.local_timezone_utc)
            appt->start_tz_loc = g_strdup("UTC");
        else if (g_par.local_timezone) 
            appt->start_tz_loc = g_strdup(g_par.local_timezone);
        else
            appt->start_tz_loc = g_strdup("floating");
        appt->end_tz_loc = g_strdup(appt->start_tz_loc);
        appt->duration = 30*60;
        /* use completed by default for new TODO */
        appt->completed = TRUE;
        /* use duration by default for new appointments */
        appt->use_duration = TRUE;
        g_sprintf(appt->completedtime,"%sT%02d%02d00"
                    , today, t->tm_hour, t->tm_min);
        appt->completed_tz_loc = g_strdup(appt->start_tz_loc);

        read_default_alarm(appt);
    }
    else if ((strcmp(action, "UPDATE") == 0) || (strcmp(action, "COPY") == 0)) {
        /* par contains ical uid */
        if (!par) {
            orage_message(10, "%s appointment with null id. Ending.", action);
            return(NULL);
        }
        if (!xfical_file_open(TRUE))
            return(NULL);
        if ((appt = xfical_appt_get(par)) == NULL) {
            orage_info_dialog(GTK_WINDOW(apptw->Window)
                , _("This appointment does not exist.")
                , _("It was probably removed, please refresh your screen."));
        }
        xfical_file_close(TRUE);
    }
    else {
        g_error("unknown parameter\n");
    }

    return(appt);
}

/************************************************************/
/* categories start.                                        */
/************************************************************/

typedef struct _orage_category
{
    gchar *category;
    GdkColor color;
} orage_category_struct;

GList *orage_category_list = NULL;

static OrageRc *orage_category_file_open(gboolean read_only)
{
    gchar *fpath;
    OrageRc *orc;

    fpath = orage_data_file_location(ORAGE_CATEGORIES_FILE);
    if ((orc = (OrageRc *)orage_rc_file_open(fpath, read_only)) == NULL) {
        orage_message(150, "orage_category_file_open: category file open failed.");
    }
    g_free(fpath);

    return(orc);
}

GdkColor *orage_category_list_contains(char *categories)
{
    GList *cat_l;
    orage_category_struct *cat;
    
    if (categories == NULL)
        return(NULL);
    for (cat_l = g_list_first(orage_category_list);
         cat_l != NULL;
         cat_l = g_list_next(cat_l)) {
        cat = (orage_category_struct *)cat_l->data;
        if (g_str_has_suffix(categories, cat->category)) {
            return(&cat->color);
        }
    }
    /* not found */
    return(NULL);
}

static void orage_category_free(gpointer gcat, gpointer dummy)
{
    orage_category_struct *cat = (orage_category_struct *)gcat;

    g_free(cat->category);
    g_free(cat);
}

void orage_category_free_list()
{
    g_list_foreach(orage_category_list, orage_category_free, NULL);
    g_list_free(orage_category_list);
    orage_category_list = NULL;
}

void orage_category_get_list()
{
    OrageRc *orc;
    gchar **cat_groups, *color;
    gint i;
    orage_category_struct *cat;
    GdkColormap *pic1_cmap;
    unsigned int red, green, blue;

    if (orage_category_list != NULL)
        orage_category_free_list();
    pic1_cmap = gdk_colormap_get_system();
    orc = orage_category_file_open(TRUE);
    cat_groups = orage_rc_get_groups(orc);
    for (i=1; cat_groups[i] != NULL; i++) {
        orage_rc_set_group(orc, cat_groups[i]);
        color = orage_rc_get_str(orc, "Color", NULL);
        if (color) {
            cat = g_new(orage_category_struct, 1);
            cat->category = g_strdup(cat_groups[i]);
            /*
            sscanf(color, ORAGE_COLOR_FORMAT
                    , (unsigned int *)&(cat->color.red)
                    , (unsigned int *)&(cat->color.green)
                    , (unsigned int *)&(cat->color.blue));
                    */
            sscanf(color, ORAGE_COLOR_FORMAT, &red, &green, &blue);
            cat->color.red = red;
            cat->color.green = green;
            cat->color.blue = blue;
            gdk_colormap_alloc_color(pic1_cmap, &cat->color, FALSE, TRUE);
            orage_category_list = g_list_prepend(orage_category_list, cat);
            g_free(color);
        }
    }
    g_strfreev(cat_groups);
    orage_rc_file_close(orc);
}

gboolean category_fill_cb(GtkComboBox *cb, char *select)
{
    OrageRc *orc;
    gchar **cat_gourps;
    gint i;
    gboolean found=FALSE;

    orc = orage_category_file_open(TRUE);
    cat_gourps = orage_rc_get_groups(orc);
    /* cat_groups[0] is special [NULL] entry always */
    gtk_combo_box_append_text(cb, _("Not set"));
    gtk_combo_box_set_active(cb, 0);
    for (i=1; cat_gourps[i] != NULL; i++) {
        gtk_combo_box_append_text(cb, (const gchar *)cat_gourps[i]);
        if (!found && select && !strcmp(select, cat_gourps[i])) {
            gtk_combo_box_set_active(cb, i);
            found = TRUE;
        }
    }
    g_strfreev(cat_gourps);
    orage_rc_file_close(orc);
    return(found);
}

void orage_category_refill_cb(appt_win *apptw)
{
    gchar *tmp;

    /* first remember the currently selected value */
    tmp = gtk_combo_box_get_active_text(GTK_COMBO_BOX(apptw->Categories_cb));

    /* then clear the values by removing the widget and putting it back */
    gtk_widget_destroy(apptw->Categories_cb);
    apptw->Categories_cb = gtk_combo_box_new_text();
    gtk_container_add(GTK_CONTAINER(apptw->Categories_cb_event)
            , apptw->Categories_cb);

    /* and finally fill it with new values */
    category_fill_cb(GTK_COMBO_BOX(apptw->Categories_cb), tmp);

    g_free(tmp);
    gtk_widget_show(apptw->Categories_cb);
}

void fill_category_data(appt_win *apptw, xfical_appt *appt)
{
    gchar *tmp = NULL;

    /* first search the last entry. which is the special color value */
    if (appt->categories) {
        tmp = g_strrstr(appt->categories, ",");
        if (!tmp) /* , not found, let's take the whole string */
            tmp = appt->categories;
        while (*(tmp) == ' ' || *(tmp) == ',') /* skip blanks and , */
            tmp++; 
    }
    if (category_fill_cb(GTK_COMBO_BOX(apptw->Categories_cb), tmp)) {
        /* we found match. Let's try to hide that from the entry text */
        while (tmp != appt->categories 
                && (*(tmp-1) == ' ' || *(tmp-1) == ','))
            tmp--;
        *tmp = '\0'; /* note that this goes to appt->categories */
    }
    gtk_entry_set_text(GTK_ENTRY(apptw->Categories_entry)
            , (appt->categories ? appt->categories : ""));
}

void orage_category_write_entry(gchar *category, GdkColor *color)
{
    OrageRc *orc;
    char *color_str;

    if (!ORAGE_STR_EXISTS(category)) {
        orage_message(50, "orage_category_write_entry: empty category. Not written");
        return;
    }
    color_str = g_strdup_printf(ORAGE_COLOR_FORMAT
            , color->red, color->green, color->blue);
    orc = orage_category_file_open(FALSE);
    orage_rc_set_group(orc, category);
    orage_rc_put_str(orc, "Color", color_str);
    g_free(color_str);
    orage_rc_file_close(orc);
}

static void orage_category_remove_entry(gchar *category)
{
    OrageRc *orc;

    if (!ORAGE_STR_EXISTS(category)) {
        orage_message(50, "orage_category_remove_entry: empty category. Not removed");
        return;
    }
    orc = orage_category_file_open(FALSE);
    orage_rc_del_group(orc, category);
    orage_rc_file_close(orc);
}

static void close_cat_window(gpointer user_data)
{
    category_win_struct *catw = (category_win_struct *)user_data;

    orage_category_refill_cb((appt_win *)catw->apptw);
    gtk_widget_destroy(catw->window);
    gtk_object_destroy(GTK_OBJECT(catw->tooltips));
    g_free(catw);
}

static gboolean on_cat_window_delete_event(GtkWidget *w, GdkEvent *e
        , gpointer user_data)
{
    close_cat_window(user_data);
    return(FALSE);
}

static void cat_add_button_clicked(GtkButton *button, gpointer user_data)
{
    category_win_struct *catw = (category_win_struct *)user_data;
    gchar *entry_category;
    GdkColor color;

    entry_category = g_strdup(gtk_entry_get_text((GtkEntry *)catw->new_entry));
    g_strstrip(entry_category);
    gtk_color_button_get_color((GtkColorButton *)catw->new_color_button
            , &color);
    orage_category_write_entry(entry_category, &color);
    g_free(entry_category);
    refresh_categories(catw);
}

static void cat_color_button_changed(GtkColorButton *color_button
        , gpointer user_data)
{
    gchar *category;
    GdkColor color;

    category = g_object_get_data(G_OBJECT(color_button), "CATEGORY");
    gtk_color_button_get_color(color_button, &color);
    orage_category_write_entry(category, &color);
}

static void cat_del_button_clicked(GtkButton *button, gpointer user_data)
{
    category_win_struct *catw = (category_win_struct *)user_data;
    gchar *category;

    category = g_object_get_data(G_OBJECT(button), "CATEGORY");
    orage_category_remove_entry(category);
    refresh_categories(catw);
}

static void show_category(gpointer elem, gpointer user_data)
{
    orage_category_struct *cat = (orage_category_struct *)elem;
    category_win_struct *catw = (category_win_struct *)user_data;
    GtkWidget *label, *hbox, *button, *color_button;

    hbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new(cat->category);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 5);

    color_button = gtk_color_button_new_with_color(&cat->color);
    gtk_box_pack_start(GTK_BOX(hbox), color_button, FALSE, FALSE, 5);
    g_object_set_data_full(G_OBJECT(color_button), "CATEGORY"
            , g_strdup(cat->category), g_free);
    g_signal_connect((gpointer)color_button, "color-set"
            , G_CALLBACK(cat_color_button_changed), catw);

    button = gtk_button_new_from_stock("gtk-remove");
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 5);
    g_object_set_data_full(G_OBJECT(button), "CATEGORY"
            , g_strdup(cat->category), g_free);
    g_signal_connect((gpointer)button, "clicked"
            , G_CALLBACK(cat_del_button_clicked), catw);

    gtk_box_pack_start(GTK_BOX(catw->cur_frame_vbox), hbox, FALSE, FALSE, 5);
}

static void refresh_categories(category_win_struct *catw)
{
    GtkWidget *swin;

    gtk_widget_destroy(catw->cur_frame);

    catw->cur_frame_vbox = gtk_vbox_new(FALSE, 0);
    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin)
            , GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(swin)
            , catw->cur_frame_vbox);
    catw->cur_frame = orage_create_framebox_with_content(
            _("Current categories"), swin);
    gtk_box_pack_start(GTK_BOX(catw->vbox), catw->cur_frame, TRUE, TRUE, 5);

    orage_category_get_list();
    g_list_foreach(orage_category_list, show_category, catw);
    gtk_widget_show_all(catw->cur_frame);
}

static void create_cat_win(category_win_struct *catw)
{
    GtkWidget *label, *hbox, *vbox;

    /***** New category *****/
    vbox = gtk_vbox_new(FALSE, 0);
    catw->new_frame = orage_create_framebox_with_content(
            _("Add new category with color"), vbox);
    gtk_box_pack_start(GTK_BOX(catw->vbox), catw->new_frame, FALSE, FALSE, 5);

    hbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new(_("Category:"));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
    catw->new_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), catw->new_entry, TRUE, TRUE, 0);
    catw->new_color_button = gtk_color_button_new();
    gtk_box_pack_start(GTK_BOX(hbox), catw->new_color_button, FALSE, FALSE, 5);
    catw->new_add_button = gtk_button_new_from_stock("gtk-add");
    gtk_box_pack_start(GTK_BOX(hbox), catw->new_add_button, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);
    g_signal_connect((gpointer)catw->new_add_button, "clicked"
            , G_CALLBACK(cat_add_button_clicked), catw);

    /***** Current categories *****/
    /* refresh_categories always destroys frame first, so let's create
     * a dummy for it for the first time */
    vbox = gtk_vbox_new(FALSE, 0);
    catw->cur_frame = orage_create_framebox_with_content(("dummy"), vbox);
    refresh_categories(catw);
}

static void on_categories_button_clicked_cb(GtkWidget *button
        , gpointer *user_data)
{
    appt_win *apptw = (appt_win *)user_data;
    category_win_struct *catw;

    catw = g_new(category_win_struct, 1);
    catw->apptw = (gpointer)apptw; /* remember the caller */
    catw->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_modal(GTK_WINDOW(catw->window), TRUE);
    gtk_window_set_title(GTK_WINDOW(catw->window)
            , _("Colors of categories - Orage"));
    gtk_window_set_default_size(GTK_WINDOW(catw->window), 390, 360);

    catw->tooltips = gtk_tooltips_new();
    catw->accelgroup = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(catw->window), catw->accelgroup);

    catw->vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(catw->window), catw->vbox);

    create_cat_win(catw);

    g_signal_connect((gpointer)catw->window, "delete_event",
        G_CALLBACK(on_cat_window_delete_event), catw);
    gtk_widget_show_all(catw->window);
}

/**********************************************************/
/* categories end.                                        */
/**********************************************************/

static void fill_appt_window_general(appt_win *apptw, xfical_appt *appt
        , char *action)
{
    int i;

    /* type */
    if (appt->type == XFICAL_TYPE_EVENT)
        gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(apptw->Type_event_rb), TRUE);
    else if (appt->type == XFICAL_TYPE_TODO)
        gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(apptw->Type_todo_rb), TRUE);
    else if (appt->type == XFICAL_TYPE_JOURNAL)
        gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(apptw->Type_journal_rb), TRUE);
    else
        g_warning("fill_appt_window_general: Illegal value for type\n");

    /* appointment name */
    gtk_entry_set_text(GTK_ENTRY(apptw->Title_entry)
            , (appt->title ? appt->title : ""));

    if (strcmp(action, "COPY") == 0) {
        gtk_editable_set_position(GTK_EDITABLE(apptw->Title_entry), -1);
        i = gtk_editable_get_position(GTK_EDITABLE(apptw->Title_entry));
        gtk_editable_insert_text(GTK_EDITABLE(apptw->Title_entry)
                , _(" *** COPY ***"), strlen(_(" *** COPY ***")), &i);
    }

    /* location */
    gtk_entry_set_text(GTK_ENTRY(apptw->Location_entry)
            , (appt->location ? appt->location : ""));

    /* times */
    fill_appt_window_times(apptw, appt);

    /* availability */
    if (appt->availability != -1) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(apptw->Availability_cb)
               , appt->availability);
    }

    /* categories */
    fill_category_data(apptw, appt);

    /* priority */
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(apptw->Priority_spin)
            , (gdouble)appt->priority);

    /* note */
    gtk_text_buffer_set_text(apptw->Note_buffer
            , (appt->note ? (const gchar *) appt->note : ""), -1);
}

static void fill_appt_window_alarm(appt_win *apptw, xfical_appt *appt)
{
    int day, hours, minutes;
    char *tmp;

    /* alarmtime (comes in seconds) */
    day = appt->alarmtime/(24*60*60);
    hours = (appt->alarmtime-day*(24*60*60))/(60*60);
    minutes = (appt->alarmtime-day*(24*60*60)-hours*(60*60))/(60);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(apptw->Alarm_spin_dd)
                , (gdouble)day);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(apptw->Alarm_spin_hh)
                , (gdouble)hours);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(apptw->Alarm_spin_mm)
                , (gdouble)minutes);

    /* alarmrelation */
    /*
    char *when_array[4] = {
    _("Before Start"), _("Before End"), _("After Start"), _("After End")};
    */
    if (appt->alarm_before)
        if (appt->alarm_related_start)
            gtk_combo_box_set_active(GTK_COMBO_BOX(apptw->Alarm_when_cb), 0);
        else
            gtk_combo_box_set_active(GTK_COMBO_BOX(apptw->Alarm_when_cb), 1);
    else
        if (appt->alarm_related_start)
            gtk_combo_box_set_active(GTK_COMBO_BOX(apptw->Alarm_when_cb), 2);
        else
            gtk_combo_box_set_active(GTK_COMBO_BOX(apptw->Alarm_when_cb), 3);

    /* persistent */
    gtk_toggle_button_set_active(
            GTK_TOGGLE_BUTTON(apptw->Per_checkbutton), appt->alarm_persistent);

    /* sound */
    gtk_toggle_button_set_active(
            GTK_TOGGLE_BUTTON(apptw->Sound_checkbutton), appt->sound_alarm);
    gtk_entry_set_text(GTK_ENTRY(apptw->Sound_entry)
            , (appt->sound ? appt->sound : 
                PACKAGE_DATA_DIR "/orage/sounds/Spo.wav"));

    /* sound repeat */
    gtk_toggle_button_set_active(
            GTK_TOGGLE_BUTTON(apptw->SoundRepeat_checkbutton)
                    , appt->soundrepeat);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(apptw->SoundRepeat_spin_cnt)
            , (gdouble)appt->soundrepeat_cnt);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(apptw->SoundRepeat_spin_len)
            , (gdouble)appt->soundrepeat_len);

    /* display */
    gtk_toggle_button_set_active(
            GTK_TOGGLE_BUTTON(apptw->Display_checkbutton_orage)
                    , appt->display_alarm_orage);
    /* display:notify */
#ifdef HAVE_NOTIFY
    gtk_toggle_button_set_active(
            GTK_TOGGLE_BUTTON(apptw->Display_checkbutton_notify)
                    , appt->display_alarm_notify);
    if (!appt->display_alarm_notify || appt->display_notify_timeout == -1) { 
        /* no timeout */
        gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(apptw->Display_checkbutton_expire_notify)
                        , FALSE);
        gtk_spin_button_set_value(
                GTK_SPIN_BUTTON(apptw->Display_spin_expire_notify)
                , (gdouble)0);
    }
    else {
        gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(apptw->Display_checkbutton_expire_notify)
                        , TRUE);
        gtk_spin_button_set_value(
                GTK_SPIN_BUTTON(apptw->Display_spin_expire_notify)
                , (gdouble)appt->display_notify_timeout);
    }
#endif

    /* procedure */
    gtk_toggle_button_set_active(
            GTK_TOGGLE_BUTTON(apptw->Proc_checkbutton), appt->procedure_alarm);
    tmp = g_strconcat(appt->procedure_cmd, " ", appt->procedure_params, NULL);
    gtk_entry_set_text(GTK_ENTRY(apptw->Proc_entry), tmp ? tmp : "");
    g_free(tmp);
}

static void fill_appt_window_recurrence(appt_win *apptw, xfical_appt *appt)
{
    char *untildate_to_display, *text;
    GList *tmp;
    xfical_exception *recur_exception;
    struct tm tm_date;
    int i;

    gtk_combo_box_set_active(GTK_COMBO_BOX(apptw->Recur_freq_cb), appt->freq);
    switch(appt->recur_limit) {
        case 0: /* no limit */
            gtk_toggle_button_set_active(
                    GTK_TOGGLE_BUTTON(apptw->Recur_limit_rb), TRUE);
            gtk_spin_button_set_value(
                    GTK_SPIN_BUTTON(apptw->Recur_count_spin), (gdouble)1);
            untildate_to_display = orage_localdate_i18();
            gtk_button_set_label(GTK_BUTTON(apptw->Recur_until_button)
                    , (const gchar *)untildate_to_display);
            break;
        case 1: /* count */
            gtk_toggle_button_set_active(
                    GTK_TOGGLE_BUTTON(apptw->Recur_count_rb), TRUE);
            gtk_spin_button_set_value(
                    GTK_SPIN_BUTTON(apptw->Recur_count_spin)
                    , (gdouble)appt->recur_count);
            untildate_to_display = orage_localdate_i18();
            gtk_button_set_label(GTK_BUTTON(apptw->Recur_until_button)
                    , (const gchar *)untildate_to_display);
            break;
        case 2: /* until */
            gtk_toggle_button_set_active(
                    GTK_TOGGLE_BUTTON(apptw->Recur_until_rb), TRUE);
            gtk_spin_button_set_value(
                    GTK_SPIN_BUTTON(apptw->Recur_count_spin), (gdouble)1);
            tm_date = orage_icaltime_to_tm_time(appt->recur_until, TRUE);
            untildate_to_display = orage_tm_date_to_i18_date(&tm_date);
            gtk_button_set_label(GTK_BUTTON(apptw->Recur_until_button)
                    , (const gchar *)untildate_to_display);
            break;
        default: /* error */
            g_warning("fill_appt_window: Unsupported recur_limit %d",
                    appt->recur_limit);
    }

    /* weekdays */
    for (i=0; i <= 6; i++) {
        gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(apptw->Recur_byday_cb[i])
                        , appt->recur_byday[i]);
        /* which weekday of month / year */
        gtk_spin_button_set_value(
                GTK_SPIN_BUTTON(apptw->Recur_byday_spin[i])
                        , (gdouble)appt->recur_byday_cnt[i]);
    }

    /* interval */
    gtk_spin_button_set_value(
            GTK_SPIN_BUTTON(apptw->Recur_int_spin)
                    , (gdouble)appt->interval);

    if (appt->interval > 1
    || !appt->recur_byday[0] || !appt->recur_byday[1] || !appt->recur_byday[2] 
    || !appt->recur_byday[3] || !appt->recur_byday[4] || !appt->recur_byday[5] 
    || !appt->recur_byday[6]) {
        gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(apptw->Recur_feature_advanced_rb), TRUE);
    }
    else {
        gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(apptw->Recur_feature_normal_rb), TRUE);
    }

    /* todo base */
    if (appt->recur_todo_base_start)
        gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(apptw->Recur_todo_base_start_rb), TRUE);
    else
        gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(apptw->Recur_todo_base_done_rb), TRUE);

    /* exceptions */
    for (tmp = g_list_first(appt->recur_exceptions);
         tmp != NULL;
         tmp = g_list_next(tmp)) {
        recur_exception = (xfical_exception *)tmp->data;
        text = g_strdup(orage_icaltime_to_i18_time(recur_exception->time));
        add_recur_exception_row(text, recur_exception->type, apptw, TRUE);
        g_free(text);
    }
    /* note: include times is setup in the fill_appt_window_times */
}

/* Fill appointment window with data */
static void fill_appt_window(appt_win *apptw, char *action, char *par)
{
    xfical_appt *appt;
    struct tm *t;

    /********************* INIT *********************/
    orage_message(10, "%s appointment: %s", action, par);
    if ((appt = fill_appt_window_get_appt(apptw, action, par)) == NULL) {
        apptw->xf_appt = NULL;
        return;
    }
    apptw->xf_appt = appt;

    /* first flags */
    apptw->xf_uid = g_strdup(appt->uid);
    apptw->par = g_strdup((const gchar *)par);
    apptw->appointment_changed = FALSE;
    if (strcmp(action, "NEW") == 0) {
        apptw->appointment_add = TRUE;
        apptw->appointment_new = TRUE;
    }
    else if (strcmp(action, "UPDATE") == 0) {
        apptw->appointment_add = FALSE;
        apptw->appointment_new = FALSE;
    }
    else if (strcmp(action, "COPY") == 0) {
            /* COPY uses old uid as base and adds new, so
             * add == TRUE && new == FALSE */
        apptw->appointment_add = TRUE;
        apptw->appointment_new = FALSE;
    }
    else {
        g_error("fill_appt_window: unknown parameter\n");
        return;
    }
    if (!appt->completed) { /* some nice default */
        t = orage_localtime(); /* probably completed today? */
        g_sprintf(appt->completedtime, XFICAL_APPT_TIME_FORMAT
                , t->tm_year+1900, t->tm_mon+1 , t->tm_mday
                , t->tm_hour, t->tm_min, 0);
        appt->completed_tz_loc = g_strdup(appt->start_tz_loc);
    }
    /* we only want to enable duplication if we are working with an old
     * existing app (=not adding new) */
    gtk_widget_set_sensitive(apptw->Duplicate, !apptw->appointment_add);
    gtk_widget_set_sensitive(apptw->File_menu_duplicate
            , !apptw->appointment_add);

    /* window title */
    gtk_window_set_title(GTK_WINDOW(apptw->Window)
            , _("New appointment - Orage"));

    /********************* GENERAL tab *********************/
    fill_appt_window_general(apptw, appt, action);

    /********************* ALARM tab *********************/
    fill_appt_window_alarm(apptw, appt);

    /********************* RECURRENCE tab *********************/
    fill_appt_window_recurrence(apptw, appt);

    /********************* FINALIZE *********************/
    set_time_sensitivity(apptw);
    set_repeat_sensitivity(apptw);
    set_sound_sensitivity(apptw);
    set_notify_sensitivity(apptw);
    set_proc_sensitivity(apptw);
    mark_appointment_unchanged(apptw);
}

static void build_menu(appt_win *apptw)
{
    GtkWidget *menu_separator;

    /* Menu bar */
    apptw->Menubar = gtk_menu_bar_new();
    gtk_box_pack_start(GTK_BOX(apptw->Vbox), apptw->Menubar, FALSE, FALSE, 0);

    /* File menu stuff */
    apptw->File_menu = orage_menu_new(_("_File"), apptw->Menubar);

    apptw->File_menu_save = orage_image_menu_item_new_from_stock("gtk-save"
            , apptw->File_menu, apptw->accel_group);

    apptw->File_menu_saveclose = 
            orage_menu_item_new_with_mnemonic(_("Sav_e and close")
                    , apptw->File_menu);
    gtk_widget_add_accelerator(apptw->File_menu_saveclose
            , "activate", apptw->accel_group
            , GDK_w, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);

    menu_separator = orage_separator_menu_item_new(apptw->File_menu);

    apptw->File_menu_revert = 
            orage_image_menu_item_new_from_stock("gtk-revert-to-saved"
                    , apptw->File_menu, apptw->accel_group);

    apptw->File_menu_duplicate = 
            orage_menu_item_new_with_mnemonic(_("D_uplicate")
                    , apptw->File_menu);
    gtk_widget_add_accelerator(apptw->File_menu_duplicate
            , "activate", apptw->accel_group
            , GDK_d, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    menu_separator = orage_separator_menu_item_new(apptw->File_menu);

    apptw->File_menu_delete = orage_image_menu_item_new_from_stock("gtk-delete"
            , apptw->File_menu, apptw->accel_group);

    menu_separator = orage_separator_menu_item_new(apptw->File_menu);

    apptw->File_menu_close = orage_image_menu_item_new_from_stock("gtk-close"
            , apptw->File_menu, apptw->accel_group);

    g_signal_connect((gpointer)apptw->File_menu_save, "activate"
            , G_CALLBACK(on_appFileSave_menu_activate_cb), apptw);
    g_signal_connect((gpointer)apptw->File_menu_saveclose, "activate"
            , G_CALLBACK(on_appFileSaveClose_menu_activate_cb), apptw);
    g_signal_connect((gpointer)apptw->File_menu_duplicate, "activate"
            , G_CALLBACK(on_appFileDuplicate_menu_activate_cb), apptw);
    g_signal_connect((gpointer)apptw->File_menu_revert, "activate"
            , G_CALLBACK(on_appFileRevert_menu_activate_cb), apptw);
    g_signal_connect((gpointer)apptw->File_menu_delete, "activate"
            , G_CALLBACK(on_appFileDelete_menu_activate_cb), apptw);
    g_signal_connect((gpointer)apptw->File_menu_close, "activate"
            , G_CALLBACK(on_appFileClose_menu_activate_cb), apptw);
}

static OrageRc *orage_alarm_file_open(gboolean read_only)
{
    gchar *fpath;
    OrageRc *orc;

    fpath = orage_config_file_location(ORAGE_DEFAULT_ALARM_FILE);
    if (!read_only)  /* we need to empty it before each write */
        g_remove(fpath);
    if ((orc = (OrageRc *)orage_rc_file_open(fpath, read_only)) == NULL) {
        orage_message(150, "orage_alarm_file_open: default alarm file open failed.");
    }
    g_free(fpath);

    return(orc);
}

static void store_default_alarm(xfical_appt *appt)
{
    OrageRc *orc;

    orc = orage_alarm_file_open(FALSE);
    orage_rc_put_int(orc, "TIME", appt->alarmtime);
    orage_rc_put_bool(orc, "BEFORE", appt->alarm_before);
    orage_rc_put_bool(orc, "RELATED_START", appt->alarm_related_start);
    orage_rc_put_bool(orc, "PERSISTENT", appt->alarm_persistent);
    orage_rc_put_bool(orc, "SOUND_USE", appt->sound_alarm);
    orage_rc_put_str(orc, "SOUND", appt->sound);
    orage_rc_put_bool(orc, "SOUND_REPEAT_USE", appt->soundrepeat);
    orage_rc_put_int(orc, "SOUND_REPEAT_CNT", appt->soundrepeat_cnt);
    orage_rc_put_int(orc, "SOUND_REPEAT_LEN", appt->soundrepeat_len);
    orage_rc_put_bool(orc, "DISPLAY_ORAGE_USE", appt->display_alarm_orage);
    orage_rc_put_bool(orc, "DISPLAY_NOTIFY_USE", appt->display_alarm_notify);
    orage_rc_put_int(orc, "DISPLAY_NOTIFY_TIMEOUT"
            , appt->display_notify_timeout);
    orage_rc_put_bool(orc, "PROCEDURE_USE", appt->procedure_alarm);
    orage_rc_put_str(orc, "PROCEDURE_CMD", appt->procedure_cmd);
    orage_rc_put_str(orc, "PROCEDURE_PARAMS", appt->procedure_params);
    orage_rc_file_close(orc);
}

static void read_default_alarm(xfical_appt *appt)
{
    OrageRc *orc;

    orc = orage_alarm_file_open(TRUE);
    appt->alarmtime = orage_rc_get_int(orc, "TIME", 5*60); /* 5 mins */
    appt->alarm_before = orage_rc_get_bool(orc, "BEFORE", TRUE);
    appt->alarm_related_start = orage_rc_get_bool(orc, "RELATED_START", TRUE);
    appt->alarm_persistent = orage_rc_get_bool(orc, "PERSISTENT", FALSE);
    appt->sound_alarm = orage_rc_get_bool(orc, "SOUND_USE", TRUE);
    if (appt->sound)
        g_free(appt->sound);
    appt->sound = orage_rc_get_str(orc, "SOUND"
            , PACKAGE_DATA_DIR "/orage/sounds/Spo.wav");
    appt->soundrepeat = orage_rc_get_bool(orc, "SOUND_REPEAT_USE", FALSE);
    appt->soundrepeat_cnt = orage_rc_get_int(orc, "SOUND_REPEAT_CNT", 500);
    appt->soundrepeat_len = orage_rc_get_int(orc, "SOUND_REPEAT_LEN", 2);
    appt->display_alarm_orage = orage_rc_get_bool(orc, "DISPLAY_ORAGE_USE"
            , TRUE);
    appt->display_alarm_notify = orage_rc_get_bool(orc, "DISPLAY_NOTIFY_USE"
            , FALSE);
    appt->display_notify_timeout = orage_rc_get_int(orc
            , "DISPLAY_NOTIFY_TIMEOUT", 0);
    appt->procedure_alarm = orage_rc_get_bool(orc, "PROCEDURE_USE", FALSE);
    if (appt->procedure_cmd)
        g_free(appt->procedure_cmd);
    appt->procedure_cmd = orage_rc_get_str(orc, "PROCEDURE_CMD", "");
    if (appt->procedure_params)
        g_free(appt->procedure_params);
    appt->procedure_params = orage_rc_get_str(orc, "PROCEDURE_PARAMS", "");
    orage_rc_file_close(orc);
}

static void on_test_button_clicked_cb(GtkButton *button
        , gpointer user_data)
{
    appt_win *apptw = (appt_win *)user_data;
    xfical_appt *appt = (xfical_appt *)apptw->xf_appt;
    alarm_struct cur_alarm;

    fill_appt_from_apptw(appt, apptw);

    /* no need for alarm time as we are doing this now */
    cur_alarm.alarm_time = NULL;
    if (appt->uid)
        cur_alarm.uid = g_strdup(appt->uid);
    else
        cur_alarm.uid = NULL;
    cur_alarm.title = g_strdup(appt->title);
    cur_alarm.description = g_strdup(appt->note);
    cur_alarm.persistent = appt->alarm_persistent;
    cur_alarm.display_orage = appt->display_alarm_orage;
    cur_alarm.display_notify = appt->display_alarm_notify;
    cur_alarm.notify_refresh = TRUE; /* not needed ? */
    cur_alarm.notify_timeout = appt->display_notify_timeout;
    cur_alarm.audio = appt->sound_alarm;
    if (appt->sound)
        cur_alarm.sound = g_strdup(appt->sound);
    else 
        cur_alarm.sound = NULL;
    cur_alarm.repeat_cnt = appt->soundrepeat_cnt;
    cur_alarm.repeat_delay = appt->soundrepeat_len;
    cur_alarm.procedure = appt->procedure_alarm;
    if (appt->procedure_alarm)
        cur_alarm.cmd = g_strdup(appt->procedure_cmd);
    else
        cur_alarm.cmd = NULL;
    create_reminders(&cur_alarm);
    g_free(cur_alarm.uid);
    g_free(cur_alarm.title);
    g_free(cur_alarm.description);
    g_free(cur_alarm.sound);
    g_free(cur_alarm.cmd);
}

static void on_appDefault_save_button_clicked_cb(GtkButton *button
        , gpointer user_data)
{
    appt_win *apptw = (appt_win *)user_data;
    xfical_appt *appt = (xfical_appt *)apptw->xf_appt;

    fill_appt_from_apptw_alarm(appt, apptw);
    store_default_alarm(appt);
}

static void on_appDefault_read_button_clicked_cb(GtkButton *button
        , gpointer user_data)
{
    appt_win *apptw = (appt_win *)user_data;
    xfical_appt *appt = (xfical_appt *)apptw->xf_appt;

    read_default_alarm(appt);
    fill_appt_window_alarm(apptw, appt);
}

static void build_toolbar(appt_win *apptw)
{
    GtkWidget *toolbar_separator;
    gint i = 0;

    apptw->Toolbar = gtk_toolbar_new();
    gtk_box_pack_start(GTK_BOX(apptw->Vbox), apptw->Toolbar, FALSE, FALSE, 0);
    gtk_toolbar_set_tooltips(GTK_TOOLBAR(apptw->Toolbar), TRUE);

    /* Add buttons to the toolbar */
    apptw->Save = orage_toolbar_append_button(apptw->Toolbar
            , "gtk-save", apptw->Tooltips, _("Save"), i++);
    apptw->SaveClose = orage_toolbar_append_button(apptw->Toolbar
            , "gtk-close", apptw->Tooltips, _("Save and close"), i++);

    toolbar_separator = orage_toolbar_append_separator(apptw->Toolbar, i++);

    apptw->Revert = orage_toolbar_append_button(apptw->Toolbar
            , "gtk-revert-to-saved", apptw->Tooltips, _("Revert"), i++);
    apptw->Duplicate = orage_toolbar_append_button(apptw->Toolbar
            , "gtk-copy", apptw->Tooltips, _("Duplicate"), i++);

    toolbar_separator = orage_toolbar_append_separator(apptw->Toolbar, i++);

    apptw->Delete = orage_toolbar_append_button(apptw->Toolbar
            , "gtk-delete", apptw->Tooltips, _("Delete"), i++);

    g_signal_connect((gpointer)apptw->Save, "clicked"
            , G_CALLBACK(on_appSave_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->SaveClose, "clicked"
            , G_CALLBACK(on_appSaveClose_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->Revert, "clicked"
            , G_CALLBACK(on_appRevert_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->Duplicate, "clicked"
            , G_CALLBACK(on_appDuplicate_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->Delete, "clicked"
            , G_CALLBACK(on_appDelete_clicked_cb), apptw);
}

static void build_general_page(appt_win *apptw)
{
    gint row;
    GtkWidget *label, *event, *hbox;
    char *availability_array[2] = {_("Free"), _("Busy")};

    apptw->TableGeneral = orage_table_new(12, BORDER_SIZE);
    apptw->General_notebook_page = apptw->TableGeneral;
    apptw->General_tab_label = gtk_label_new(_("General"));

    gtk_notebook_append_page(GTK_NOTEBOOK(apptw->Notebook)
            , apptw->General_notebook_page, apptw->General_tab_label);

    /* type */
    apptw->Type_label = gtk_label_new(_("Type "));
    hbox =  gtk_hbox_new(FALSE, 0);
    apptw->Type_event_rb = gtk_radio_button_new_with_label(NULL, _("Event"));
    gtk_box_pack_start(GTK_BOX(hbox), apptw->Type_event_rb, FALSE, FALSE, 15);
    gtk_tooltips_set_tip(apptw->Tooltips, apptw->Type_event_rb
            , _("Event that will happen sometime. For example:\nMeeting or birthday or TV show."), NULL);

    apptw->Type_todo_rb = gtk_radio_button_new_with_mnemonic_from_widget(
            GTK_RADIO_BUTTON(apptw->Type_event_rb) , _("Todo"));
    gtk_box_pack_start(GTK_BOX(hbox), apptw->Type_todo_rb, FALSE, FALSE, 15);
    gtk_tooltips_set_tip(apptw->Tooltips, apptw->Type_todo_rb
            , _("Something that you should do sometime. For example:\nWash your car or test new version of Orage."), NULL);

    apptw->Type_journal_rb = gtk_radio_button_new_with_mnemonic_from_widget(
            GTK_RADIO_BUTTON(apptw->Type_event_rb) , _("Journal"));
    gtk_box_pack_start(GTK_BOX(hbox), apptw->Type_journal_rb
            , FALSE, FALSE, 15);
    gtk_tooltips_set_tip(apptw->Tooltips, apptw->Type_journal_rb
            , _("Make a note that something happened. For example:\nRemark that your mother called or first snow came."), NULL);

    orage_table_add_row(apptw->TableGeneral
            , apptw->Type_label, hbox
            , row = 0, (GTK_EXPAND | GTK_FILL), (0));

    /* title */
    apptw->Title_label = gtk_label_new(_("Title "));
    apptw->Title_entry = gtk_entry_new();
    orage_table_add_row(apptw->TableGeneral
            , apptw->Title_label, apptw->Title_entry
            , ++row, (GTK_EXPAND | GTK_FILL), (0));

    /* location */
    apptw->Location_label = gtk_label_new(_("Location"));
    apptw->Location_entry = gtk_entry_new();
    orage_table_add_row(apptw->TableGeneral
            , apptw->Location_label, apptw->Location_entry
            , ++row, (GTK_EXPAND | GTK_FILL), (0));

    /* All day */
    apptw->AllDay_checkbutton = 
            gtk_check_button_new_with_mnemonic(_("All day event"));
    orage_table_add_row(apptw->TableGeneral
            , NULL, apptw->AllDay_checkbutton
            , ++row, (GTK_EXPAND | GTK_FILL), (0));

    /* start time */
    apptw->Start_label = gtk_label_new(_("Start"));
    apptw->StartDate_button = gtk_button_new();
    apptw->StartTime_spin_hh = gtk_spin_button_new_with_range(0, 23, 1);
    apptw->StartTime_spin_mm = gtk_spin_button_new_with_range(0, 59, 1);
    apptw->StartTimezone_button = gtk_button_new();
    apptw->StartTime_hbox = datetime_hbox_new(
            apptw->StartDate_button, 
            apptw->StartTime_spin_hh, 
            apptw->StartTime_spin_mm, 
            apptw->StartTimezone_button);
    orage_table_add_row(apptw->TableGeneral
            , apptw->Start_label, apptw->StartTime_hbox
            , ++row, (GTK_SHRINK | GTK_FILL), (GTK_SHRINK | GTK_FILL));

    /* end time */
    apptw->End_label = gtk_label_new(_("End"));
    apptw->End_hbox = gtk_hbox_new(FALSE, 0);
    /* translators: leave some spaces after this so that it looks better on
     * the screen */
    apptw->End_checkbutton = 
            gtk_check_button_new_with_mnemonic(_("Set      "));
    gtk_box_pack_start(GTK_BOX(apptw->End_hbox)
            , apptw->End_checkbutton, FALSE, FALSE, 0);
    apptw->EndDate_button = gtk_button_new();
    apptw->EndTime_spin_hh = gtk_spin_button_new_with_range(0, 23, 1);
    apptw->EndTime_spin_mm = gtk_spin_button_new_with_range(0, 59, 1);
    apptw->EndTimezone_button = gtk_button_new();
    apptw->EndTime_hbox = datetime_hbox_new(
            apptw->EndDate_button, 
            apptw->EndTime_spin_hh, 
            apptw->EndTime_spin_mm, 
            apptw->EndTimezone_button);
    gtk_box_pack_end(GTK_BOX(apptw->End_hbox)
            , apptw->EndTime_hbox, TRUE, TRUE, 0);
    orage_table_add_row(apptw->TableGeneral
            , apptw->End_label, apptw->End_hbox
            , ++row, (GTK_SHRINK | GTK_FILL), (GTK_SHRINK | GTK_FILL));

    /* duration */
    apptw->Dur_hbox = gtk_hbox_new(FALSE, 0);
    apptw->Dur_checkbutton = 
            gtk_check_button_new_with_mnemonic(_("Duration"));
    gtk_box_pack_start(GTK_BOX(apptw->Dur_hbox), apptw->Dur_checkbutton
            , FALSE, FALSE, 0);
    apptw->Dur_spin_dd = gtk_spin_button_new_with_range(0, 1000, 1);
    apptw->Dur_spin_dd_label = gtk_label_new(_("days"));
    apptw->Dur_spin_hh = gtk_spin_button_new_with_range(0, 23, 1);
    apptw->Dur_spin_hh_label = gtk_label_new(_("hours"));
    apptw->Dur_spin_mm = gtk_spin_button_new_with_range(0, 59, 5);
    apptw->Dur_spin_mm_label = gtk_label_new(_("mins"));
    apptw->Dur_time_hbox = orage_period_hbox_new(TRUE, FALSE
            , apptw->Dur_spin_dd, apptw->Dur_spin_dd_label
            , apptw->Dur_spin_hh, apptw->Dur_spin_hh_label
            , apptw->Dur_spin_mm, apptw->Dur_spin_mm_label);
    gtk_box_pack_start(GTK_BOX(apptw->Dur_hbox), apptw->Dur_time_hbox
            , FALSE, FALSE, 0);
    orage_table_add_row(apptw->TableGeneral
            , NULL, apptw->Dur_hbox
            , ++row, (GTK_FILL), (GTK_FILL));
    
    /* Availability (only for EVENT) */
    apptw->Availability_label = gtk_label_new(_("Availability"));
    apptw->Availability_cb = orage_create_combo_box_with_content(
            availability_array, 2);
    orage_table_add_row(apptw->TableGeneral
            , apptw->Availability_label, apptw->Availability_cb
            , ++row, (GTK_FILL), (GTK_FILL));
    
    /* completed (only for TODO) */
    apptw->Completed_label = gtk_label_new(_("Completed"));
    apptw->Completed_hbox = gtk_hbox_new(FALSE, 0);
    apptw->Completed_checkbutton = 
            gtk_check_button_new_with_mnemonic(_("Done"));
    gtk_box_pack_start(GTK_BOX(apptw->Completed_hbox)
            , apptw->Completed_checkbutton, FALSE, FALSE, 0);
    label = gtk_label_new(" ");
    gtk_box_pack_start(GTK_BOX(apptw->Completed_hbox), label, FALSE, FALSE, 4);
    apptw->CompletedDate_button = gtk_button_new();
    apptw->CompletedTime_spin_hh = gtk_spin_button_new_with_range(0, 23, 1);
    apptw->CompletedTime_spin_mm = gtk_spin_button_new_with_range(0, 59, 1);
    apptw->CompletedTimezone_button = gtk_button_new();
    apptw->CompletedTime_hbox = datetime_hbox_new(
            apptw->CompletedDate_button, 
            apptw->CompletedTime_spin_hh, 
            apptw->CompletedTime_spin_mm, 
            apptw->CompletedTimezone_button);
    gtk_box_pack_end(GTK_BOX(apptw->Completed_hbox)
            , apptw->CompletedTime_hbox, TRUE, TRUE, 0);
    orage_table_add_row(apptw->TableGeneral
            , apptw->Completed_label, apptw->Completed_hbox
            , ++row, (GTK_FILL), (GTK_FILL));

    /* categories */
    apptw->Categories_label = gtk_label_new(_("Categories"));
    apptw->Categories_hbox = gtk_hbox_new(FALSE, 0);
    apptw->Categories_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(apptw->Categories_hbox), apptw->Categories_entry
            , TRUE, TRUE, 0);
    apptw->Categories_cb = gtk_combo_box_new_text();
    apptw->Categories_cb_event =  gtk_event_box_new(); /* needed for tooltips */
    gtk_container_add(GTK_CONTAINER(apptw->Categories_cb_event)
            , apptw->Categories_cb);
    gtk_box_pack_start(GTK_BOX(apptw->Categories_hbox)
            , apptw->Categories_cb_event, FALSE, FALSE, 4);
    gtk_tooltips_set_tip(apptw->Tooltips, apptw->Categories_cb_event
            , _("This is special category, which can be used to color this appointment in list views."), NULL);
    apptw->Categories_button =gtk_button_new_from_stock(GTK_STOCK_SELECT_COLOR);
    gtk_box_pack_start(GTK_BOX(apptw->Categories_hbox), apptw->Categories_button
            , FALSE, FALSE, 0);
    gtk_tooltips_set_tip(apptw->Tooltips, apptw->Categories_button
            , _("update colors for categories."), NULL);
    orage_table_add_row(apptw->TableGeneral
            , apptw->Categories_label, apptw->Categories_hbox
            , ++row, (GTK_EXPAND | GTK_FILL), (0));

    /* priority */
    apptw->Priority_label = gtk_label_new(_("Priority"));
    apptw->Priority_spin = gtk_spin_button_new_with_range(0, 9, 1);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(apptw->Priority_spin), TRUE);
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), apptw->Priority_spin, FALSE, FALSE, 0);
    orage_table_add_row(apptw->TableGeneral
            , apptw->Priority_label, hbox
            , ++row, (GTK_FILL), (GTK_FILL));

    /* note */
    apptw->Note = gtk_label_new(_("Note"));
    apptw->Note_Scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
    event =  gtk_event_box_new(); /* only needed for tooltips */
    gtk_container_add(GTK_CONTAINER(event), apptw->Note_Scrolledwindow);
    gtk_scrolled_window_set_policy(
            GTK_SCROLLED_WINDOW(apptw->Note_Scrolledwindow)
            , GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(
            GTK_SCROLLED_WINDOW(apptw->Note_Scrolledwindow), GTK_SHADOW_IN);
    apptw->Note_buffer = gtk_text_buffer_new(NULL);
    apptw->Note_textview = 
            gtk_text_view_new_with_buffer(GTK_TEXT_BUFFER(apptw->Note_buffer));
    gtk_container_add(GTK_CONTAINER(apptw->Note_Scrolledwindow)
            , apptw->Note_textview);
    orage_table_add_row(apptw->TableGeneral
            , apptw->Note, event
            , ++row, (GTK_EXPAND | GTK_FILL), (GTK_EXPAND | GTK_FILL));

    gtk_tooltips_set_tip(apptw->Tooltips, event
            , _("These shorthand commands take effect immediately:\n    <D> inserts current date in local date format\n    <T> inserts time and\n    <DT> inserts date and time.\n\nThese are converted only later when they are seen:\n    <&Ynnnn> is translated to current year minus nnnn.\n(This can be used for example in birthday reminders to tell how old the person will be.)"), NULL);

    /* Take care of the title entry to build the appointment window title */
    g_signal_connect((gpointer)apptw->Title_entry, "changed"
            , G_CALLBACK(on_appTitle_entry_changed_cb), apptw);
}

static void enable_general_page_signals(appt_win *apptw)
{
    g_signal_connect((gpointer)apptw->Type_event_rb, "clicked"
            , G_CALLBACK(app_type_checkbutton_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->Type_todo_rb, "clicked"
            , G_CALLBACK(app_type_checkbutton_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->Type_journal_rb, "clicked"
            , G_CALLBACK(app_type_checkbutton_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->AllDay_checkbutton, "clicked"
            , G_CALLBACK(app_time_checkbutton_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->StartDate_button, "clicked"
            , G_CALLBACK(on_Date_button_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->StartTime_spin_hh, "changed"
            , G_CALLBACK(on_app_spin_button_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->StartTime_spin_mm, "changed"
            , G_CALLBACK(on_app_spin_button_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->StartTimezone_button, "clicked"
            , G_CALLBACK(on_appStartTimezone_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->End_checkbutton, "clicked"
            , G_CALLBACK(app_time_checkbutton_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->EndDate_button, "clicked"
            , G_CALLBACK(on_Date_button_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->EndTime_spin_hh, "changed"
            , G_CALLBACK(on_app_spin_button_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->EndTime_spin_mm, "changed"
            , G_CALLBACK(on_app_spin_button_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->EndTimezone_button, "clicked"
            , G_CALLBACK(on_appEndTimezone_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->Dur_checkbutton, "clicked"
            , G_CALLBACK(app_time_checkbutton_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->Dur_spin_dd, "value-changed"
            , G_CALLBACK(on_app_spin_button_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->Dur_spin_hh, "value-changed"
            , G_CALLBACK(on_app_spin_button_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->Dur_spin_mm, "value-changed"
            , G_CALLBACK(on_app_spin_button_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->Completed_checkbutton, "clicked"
            , G_CALLBACK(app_time_checkbutton_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->CompletedDate_button, "clicked"
            , G_CALLBACK(on_Date_button_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->CompletedTime_spin_hh, "changed"
            , G_CALLBACK(on_app_spin_button_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->CompletedTime_spin_mm, "changed"
            , G_CALLBACK(on_app_spin_button_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->CompletedTimezone_button, "clicked"
            , G_CALLBACK(on_appCompletedTimezone_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->Location_entry, "changed"
            , G_CALLBACK(on_app_entry_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->Categories_entry, "changed"
            , G_CALLBACK(on_app_entry_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->Categories_cb, "changed"
            , G_CALLBACK(on_app_combobox_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->Categories_button, "clicked"
            , G_CALLBACK(on_categories_button_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->Availability_cb, "changed"
            , G_CALLBACK(on_app_combobox_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->Note_buffer, "changed"
            , G_CALLBACK(on_appNote_buffer_changed_cb), apptw);
}

static void build_alarm_page(appt_win *apptw)
{
    gint row;
    GtkWidget *label, *event, *vbox;
    char *when_array[4] = {_("Before Start"), _("Before End")
        , _("After Start"), _("After End")};

    /***** Header *****/
    vbox = gtk_vbox_new(FALSE, 0);
    apptw->TableAlarm = orage_table_new(7, BORDER_SIZE);
    gtk_box_pack_start(GTK_BOX(vbox), apptw->TableAlarm, FALSE, FALSE, 0);
    apptw->Alarm_notebook_page = vbox;
    apptw->Alarm_tab_label = gtk_label_new(_("Alarm"));

    gtk_notebook_append_page(GTK_NOTEBOOK(apptw->Notebook)
            , apptw->Alarm_notebook_page, apptw->Alarm_tab_label);

    /***** ALARM TIME *****/
    apptw->Alarm_label = gtk_label_new(_("Alarm time"));
    apptw->Alarm_hbox = gtk_hbox_new(FALSE, 0);
    apptw->Alarm_spin_dd = gtk_spin_button_new_with_range(0, 100, 1);
    apptw->Alarm_spin_dd_label = gtk_label_new(_("days"));
    apptw->Alarm_spin_hh = gtk_spin_button_new_with_range(0, 23, 1);
    apptw->Alarm_spin_hh_label = gtk_label_new(_("hours"));
    apptw->Alarm_spin_mm = gtk_spin_button_new_with_range(0, 59, 5);
    apptw->Alarm_spin_mm_label = gtk_label_new(_("mins"));
    apptw->Alarm_time_hbox = orage_period_hbox_new(FALSE, TRUE
            , apptw->Alarm_spin_dd, apptw->Alarm_spin_dd_label
            , apptw->Alarm_spin_hh, apptw->Alarm_spin_hh_label
            , apptw->Alarm_spin_mm, apptw->Alarm_spin_mm_label);
    gtk_box_pack_start(GTK_BOX(apptw->Alarm_hbox), apptw->Alarm_time_hbox
            , FALSE, FALSE, 0);
    apptw->Alarm_when_cb = orage_create_combo_box_with_content(when_array, 4);
    event =  gtk_event_box_new(); /* only needed for tooltips */
    gtk_container_add(GTK_CONTAINER(event), apptw->Alarm_when_cb);
    gtk_box_pack_start(GTK_BOX(apptw->Alarm_hbox)
            , event, FALSE, FALSE, 0);

    orage_table_add_row(apptw->TableAlarm
            , apptw->Alarm_label, apptw->Alarm_hbox
            , row = 0, (GTK_FILL), (GTK_FILL));
    gtk_tooltips_set_tip(apptw->Tooltips, event
            , _("Often you want to get alarm:\n 1) before Event start\n 2) before Todo end\n 3) after Todo start"), NULL);

    /***** Persistent Alarm *****/
    apptw->Per_hbox = gtk_hbox_new(FALSE, 6);
    apptw->Per_checkbutton = 
            gtk_check_button_new_with_mnemonic(_("Persistent alarm"));
    gtk_tooltips_set_tip(apptw->Tooltips, apptw->Per_checkbutton
            , _("Select this if you want Orage to remind you even if it has not been active when the alarm happened."), NULL);
    gtk_box_pack_start(GTK_BOX(apptw->Per_hbox), apptw->Per_checkbutton
            , FALSE, TRUE, 0);

    orage_table_add_row(apptw->TableAlarm
            , NULL, apptw->Per_hbox
            , ++row, (GTK_FILL), (GTK_FILL));

    /***** Audio Alarm *****/
    apptw->Sound_label = gtk_label_new(_("Sound"));

    apptw->Sound_hbox = gtk_hbox_new(FALSE, 6);
    apptw->Sound_checkbutton = 
            gtk_check_button_new_with_mnemonic(_("Use"));
    gtk_tooltips_set_tip(apptw->Tooltips, apptw->Sound_checkbutton
            , _("Select this if you want audible alarm"), NULL);
    gtk_box_pack_start(GTK_BOX(apptw->Sound_hbox), apptw->Sound_checkbutton
            , FALSE, TRUE, 0);

    apptw->Sound_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(apptw->Sound_hbox), apptw->Sound_entry
            , TRUE, TRUE, 0);
    apptw->Sound_button = gtk_button_new_from_stock("gtk-open");
    gtk_box_pack_start(GTK_BOX(apptw->Sound_hbox), apptw->Sound_button
            , FALSE, TRUE, 0);

    orage_table_add_row(apptw->TableAlarm
            , apptw->Sound_label, apptw->Sound_hbox
            , ++row, (GTK_FILL), (GTK_FILL));

    apptw->SoundRepeat_hbox = gtk_hbox_new(FALSE, 0);
    apptw->SoundRepeat_checkbutton = 
            gtk_check_button_new_with_mnemonic(_("Repeat alarm sound"));
    gtk_box_pack_start(GTK_BOX(apptw->SoundRepeat_hbox)
            , apptw->SoundRepeat_checkbutton
            , FALSE, FALSE, 0);

    label = gtk_label_new(" ");
    gtk_box_pack_start(GTK_BOX(apptw->SoundRepeat_hbox), label
            , FALSE, FALSE, 10);

    apptw->SoundRepeat_spin_cnt = gtk_spin_button_new_with_range(1, 999, 10);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(apptw->SoundRepeat_spin_cnt)
            , TRUE);
    gtk_box_pack_start(GTK_BOX(apptw->SoundRepeat_hbox)
            , apptw->SoundRepeat_spin_cnt
            , FALSE, FALSE, 0);

    apptw->SoundRepeat_spin_cnt_label = gtk_label_new(_("times"));
    gtk_box_pack_start(GTK_BOX(apptw->SoundRepeat_hbox)
            , apptw->SoundRepeat_spin_cnt_label
            , FALSE, FALSE, 5);
    
    label = gtk_label_new(" ");
    gtk_box_pack_start(GTK_BOX(apptw->SoundRepeat_hbox), label
            , FALSE, FALSE, 10);

    apptw->SoundRepeat_spin_len = gtk_spin_button_new_with_range(1, 250, 1);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(apptw->SoundRepeat_spin_len)
            , TRUE);
    gtk_box_pack_start(GTK_BOX(apptw->SoundRepeat_hbox)
            , apptw->SoundRepeat_spin_len
            , FALSE, FALSE, 0);

    apptw->SoundRepeat_spin_len_label = gtk_label_new(_("sec interval"));
    gtk_box_pack_start(GTK_BOX(apptw->SoundRepeat_hbox)
            , apptw->SoundRepeat_spin_len_label
            , FALSE, FALSE, 5);

    orage_table_add_row(apptw->TableAlarm
            , NULL, apptw->SoundRepeat_hbox
            , ++row, (GTK_EXPAND | GTK_FILL), (0));

    /***** Display Alarm *****/
    apptw->Display_label = gtk_label_new(_("Visual"));

    apptw->Display_hbox_orage = gtk_hbox_new(FALSE, 0);
    apptw->Display_checkbutton_orage = 
            gtk_check_button_new_with_mnemonic(_("Use Orage window"));
    gtk_tooltips_set_tip(apptw->Tooltips, apptw->Display_checkbutton_orage
            , _("Select this if you want Orage window alarm"), NULL);
    gtk_box_pack_start(GTK_BOX(apptw->Display_hbox_orage)
            , apptw->Display_checkbutton_orage
            , FALSE, TRUE, 0);

    orage_table_add_row(apptw->TableAlarm
            , apptw->Display_label, apptw->Display_hbox_orage
            , ++row, (GTK_EXPAND | GTK_FILL), (0));

#ifdef HAVE_NOTIFY
    apptw->Display_hbox_notify = gtk_hbox_new(FALSE, 0);
    apptw->Display_checkbutton_notify = 
            gtk_check_button_new_with_mnemonic(_("Use notification"));
    gtk_tooltips_set_tip(apptw->Tooltips, apptw->Display_checkbutton_notify
            , _("Select this if you want notification alarm"), NULL);
    gtk_box_pack_start(GTK_BOX(apptw->Display_hbox_notify)
            , apptw->Display_checkbutton_notify
            , FALSE, TRUE, 0);

    apptw->Display_checkbutton_expire_notify = 
            gtk_check_button_new_with_mnemonic(_("Set timeout"));
    gtk_tooltips_set_tip(apptw->Tooltips
            , apptw->Display_checkbutton_expire_notify
            , _("Select this if you want notification to expire automatically")
            , NULL);
    gtk_box_pack_start(GTK_BOX(apptw->Display_hbox_notify)
            , apptw->Display_checkbutton_expire_notify
            , FALSE, TRUE, 10);

    apptw->Display_spin_expire_notify = 
            gtk_spin_button_new_with_range(0, 999, 1);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(apptw->Display_spin_expire_notify)
            , TRUE);
    gtk_tooltips_set_tip(apptw->Tooltips, apptw->Display_spin_expire_notify
            , _("0 = system default expiration time"), NULL);
    gtk_box_pack_start(GTK_BOX(apptw->Display_hbox_notify)
            , apptw->Display_spin_expire_notify
            , FALSE, TRUE, 10);

    apptw->Display_spin_expire_notify_label = gtk_label_new(_("seconds"));
    gtk_box_pack_start(GTK_BOX(apptw->Display_hbox_notify)
            , apptw->Display_spin_expire_notify_label
            , FALSE, TRUE, 0);

    orage_table_add_row(apptw->TableAlarm
            , NULL, apptw->Display_hbox_notify
            , ++row, (GTK_EXPAND | GTK_FILL), (0));
#endif

    /***** Procedure Alarm *****/
    apptw->Proc_label = gtk_label_new(_("Procedure"));

    apptw->Proc_hbox = gtk_hbox_new(FALSE, 6);
    apptw->Proc_checkbutton = 
            gtk_check_button_new_with_mnemonic(_("Use"));
    gtk_tooltips_set_tip(apptw->Tooltips, apptw->Proc_checkbutton
            , _("Select this if you want procedure or script alarm"), NULL);
    gtk_box_pack_start(GTK_BOX(apptw->Proc_hbox), apptw->Proc_checkbutton
            , FALSE, TRUE, 0);

    apptw->Proc_entry = gtk_entry_new();
    gtk_tooltips_set_tip(apptw->Tooltips, apptw->Proc_entry
            , _("You must enter all escape etc characters yourself.\n This string is just given to shell to process"), NULL);
    gtk_box_pack_start(GTK_BOX(apptw->Proc_hbox), apptw->Proc_entry
            , TRUE, TRUE, 0);

    orage_table_add_row(apptw->TableAlarm
            , apptw->Proc_label, apptw->Proc_hbox
            , ++row, (GTK_FILL), (GTK_FILL));

    /***** Test Alarm *****/
    gtk_box_pack_start(GTK_BOX(vbox), gtk_hseparator_new(), FALSE, FALSE, 3);
    apptw->Test_button = gtk_button_new_from_stock("gtk-execute");
    gtk_tooltips_set_tip(apptw->Tooltips, apptw->Test_button
            , _("Test this alarm by raising it now"), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), apptw->Test_button, FALSE, FALSE, 0);

    /***** Default Alarm Settings *****/
    apptw->Default_hbox = gtk_hbox_new(FALSE, 6);
    apptw->Default_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(apptw->Default_label)
            , _("<b>Default alarm</b>"));
    gtk_box_pack_start(GTK_BOX(apptw->Default_hbox), apptw->Default_label
            , FALSE, FALSE, 0);

    apptw->Default_savebutton = gtk_button_new_from_stock("gtk-save");
    gtk_box_pack_start(GTK_BOX(apptw->Default_hbox), apptw->Default_savebutton
            , TRUE, TRUE, 0);
    gtk_tooltips_set_tip(apptw->Tooltips, apptw->Default_savebutton
            , _("Store current settings as default alarm"), NULL);
    apptw->Default_readbutton =gtk_button_new_from_stock("gtk-revert-to-saved");
    gtk_box_pack_start(GTK_BOX(apptw->Default_hbox), apptw->Default_readbutton
            , TRUE, TRUE, 0);
    gtk_tooltips_set_tip(apptw->Tooltips, apptw->Default_readbutton
            , _("Set current settings from default alarm"), NULL);

    gtk_box_pack_end(GTK_BOX(vbox), apptw->Default_hbox, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(vbox), gtk_hseparator_new(), FALSE, FALSE, 3);
}

static void enable_alarm_page_signals(appt_win *apptw)
{
    g_signal_connect((gpointer)apptw->Alarm_spin_dd, "value-changed"
            , G_CALLBACK(on_app_spin_button_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->Alarm_spin_hh, "value-changed"
            , G_CALLBACK(on_app_spin_button_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->Alarm_spin_mm, "value-changed"
            , G_CALLBACK(on_app_spin_button_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->Alarm_when_cb, "changed"
            , G_CALLBACK(on_app_combobox_changed_cb), apptw);

    g_signal_connect((gpointer)apptw->Sound_checkbutton, "clicked"
            , G_CALLBACK(app_sound_checkbutton_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->Sound_entry, "changed"
            , G_CALLBACK(on_app_entry_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->Sound_button, "clicked"
            , G_CALLBACK(on_appSound_button_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->SoundRepeat_checkbutton, "clicked"
            , G_CALLBACK(app_sound_checkbutton_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->SoundRepeat_spin_cnt, "value-changed"
            , G_CALLBACK(on_app_spin_button_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->SoundRepeat_spin_len, "value-changed"
            , G_CALLBACK(on_app_spin_button_changed_cb), apptw);

    g_signal_connect((gpointer)apptw->Display_checkbutton_orage, "clicked"
            , G_CALLBACK(app_checkbutton_clicked_cb), apptw);
#ifdef HAVE_NOTIFY
    g_signal_connect((gpointer)apptw->Display_checkbutton_notify, "clicked"
            , G_CALLBACK(app_notify_checkbutton_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->Display_checkbutton_expire_notify
            , "clicked"
            , G_CALLBACK(app_notify_checkbutton_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->Display_spin_expire_notify
            , "value-changed"
            , G_CALLBACK(on_app_spin_button_changed_cb), apptw);
#endif

    g_signal_connect((gpointer)apptw->Proc_checkbutton, "clicked"
            , G_CALLBACK(app_proc_checkbutton_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->Proc_entry, "changed"
            , G_CALLBACK(on_app_entry_changed_cb), apptw);

    g_signal_connect((gpointer)apptw->Test_button, "clicked"
            , G_CALLBACK(on_test_button_clicked_cb), apptw);

    g_signal_connect((gpointer)apptw->Default_savebutton, "clicked"
            , G_CALLBACK(on_appDefault_save_button_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->Default_readbutton, "clicked"
            , G_CALLBACK(on_appDefault_read_button_clicked_cb), apptw);
}

static void build_recurrence_page(appt_win *apptw)
{
    gint row, i;
    guint y, m;
    char *recur_freq_array[5] = {
        _("None"), _("Daily"), _("Weekly"), _("Monthly"), _("Yearly")};
    char *weekday_array[7] = {
        _("Mon"), _("Tue"), _("Wed"), _("Thu"), _("Fri"), _("Sat"), _("Sun")};
    GtkWidget *hbox;

    apptw->TableRecur = orage_table_new(9, BORDER_SIZE);
    apptw->Recur_notebook_page = apptw->TableRecur;
    apptw->Recur_tab_label = gtk_label_new(_("Recurrence"));

    gtk_notebook_append_page(GTK_NOTEBOOK(apptw->Notebook)
            , apptw->Recur_notebook_page, apptw->Recur_tab_label);

    /* complexity */
    apptw->Recur_feature_label = gtk_label_new(_("Complexity"));
    apptw->Recur_feature_hbox = gtk_hbox_new(FALSE, 0);
    apptw->Recur_feature_normal_rb = gtk_radio_button_new_with_label(NULL
            , _("Basic"));
    gtk_box_pack_start(GTK_BOX(apptw->Recur_feature_hbox)
            , apptw->Recur_feature_normal_rb, FALSE, FALSE, 0);
    apptw->Recur_feature_advanced_rb = 
            gtk_radio_button_new_with_mnemonic_from_widget(
                    GTK_RADIO_BUTTON(apptw->Recur_feature_normal_rb)
                    , _("Advanced"));
    gtk_box_pack_start(GTK_BOX(apptw->Recur_feature_hbox)
            , apptw->Recur_feature_advanced_rb, FALSE, FALSE, 20);
    gtk_tooltips_set_tip(apptw->Tooltips, apptw->Recur_feature_normal_rb
            , _("Use this if you want regular repeating event"), NULL);
    gtk_tooltips_set_tip(apptw->Tooltips, apptw->Recur_feature_advanced_rb
            , _("Use this if you need complex times like:\n Every Saturday and Sunday or \n First Tuesday every month")
            , NULL);
    orage_table_add_row(apptw->TableRecur
            , apptw->Recur_feature_label, apptw->Recur_feature_hbox
            , row = 0, (GTK_EXPAND | GTK_FILL), (0));

    /* frequency */
    apptw->Recur_freq_label = gtk_label_new(_("Frequency"));
    apptw->Recur_freq_hbox = gtk_hbox_new(FALSE, 0);
    apptw->Recur_freq_cb = orage_create_combo_box_with_content(
            recur_freq_array, 5);
    gtk_box_pack_start(GTK_BOX(apptw->Recur_freq_hbox)
            , apptw->Recur_freq_cb, FALSE, FALSE, 0);
    apptw->Recur_int_spin_label1 = gtk_label_new(_("Each"));
    gtk_box_pack_start(GTK_BOX(apptw->Recur_freq_hbox)
            , apptw->Recur_int_spin_label1, FALSE, FALSE, 5);
    apptw->Recur_int_spin = gtk_spin_button_new_with_range(1, 100, 1);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(apptw->Recur_int_spin), TRUE);
    gtk_box_pack_start(GTK_BOX(apptw->Recur_freq_hbox)
            , apptw->Recur_int_spin, FALSE, FALSE, 0);
    apptw->Recur_int_spin_label2 = gtk_label_new(_("occurrence"));
    gtk_box_pack_start(GTK_BOX(apptw->Recur_freq_hbox)
            , apptw->Recur_int_spin_label2, FALSE, FALSE, 5);
    gtk_tooltips_set_tip(apptw->Tooltips, apptw->Recur_int_spin
            , _("Limit frequency to certain interval.\n For example: Every third day:\n Frequency = Daily and Interval = 3")
            , NULL);
    orage_table_add_row(apptw->TableRecur
            , apptw->Recur_freq_label, apptw->Recur_freq_hbox
            , ++row, (GTK_EXPAND | GTK_FILL), (GTK_FILL));

    /*
    apptw->Recur_limit_label = gtk_label_new(_("Limit"));
    apptw->Recur_limit_rb = gtk_radio_button_new_with_label(NULL
            , _("Repeat forever"));
    orage_table_add_row(apptw->TableRecur
            , apptw->Recur_limit_label, apptw->Recur_limit_rb
            , ++row, (GTK_EXPAND | GTK_FILL), (0));

    apptw->Recur_count_hbox = gtk_hbox_new(FALSE, 0);
    apptw->Recur_count_rb = gtk_radio_button_new_with_mnemonic_from_widget(
            GTK_RADIO_BUTTON(apptw->Recur_limit_rb), _("Repeat "));
    gtk_box_pack_start(GTK_BOX(apptw->Recur_count_hbox)
            , apptw->Recur_count_rb, FALSE, FALSE, 0);
    apptw->Recur_count_spin = gtk_spin_button_new_with_range(1, 100, 1);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(apptw->Recur_count_spin)
            , TRUE);
    gtk_box_pack_start(GTK_BOX(apptw->Recur_count_hbox)
            , apptw->Recur_count_spin, FALSE, FALSE, 0);
    apptw->Recur_count_label = gtk_label_new(_("times"));
    gtk_box_pack_start(GTK_BOX(apptw->Recur_count_hbox)
            , apptw->Recur_count_label, FALSE, FALSE, 5);
    orage_table_add_row(apptw->TableRecur
            , NULL, apptw->Recur_count_hbox
            , ++row, (GTK_EXPAND | GTK_FILL), (0));

    apptw->Recur_until_hbox = gtk_hbox_new(FALSE, 0);
    apptw->Recur_until_rb = gtk_radio_button_new_with_mnemonic_from_widget(
            GTK_RADIO_BUTTON(apptw->Recur_limit_rb), _("Repeat until "));
    gtk_box_pack_start(GTK_BOX(apptw->Recur_until_hbox)
            , apptw->Recur_until_rb, FALSE, FALSE, 0);
    apptw->Recur_until_button = gtk_button_new();
    gtk_box_pack_start(GTK_BOX(apptw->Recur_until_hbox)
            , apptw->Recur_until_button, FALSE, FALSE, 0);
    orage_table_add_row(apptw->TableRecur
            , NULL, apptw->Recur_until_hbox
            , ++row, (GTK_EXPAND | GTK_FILL), (0));
            */

    /* limitation */
    hbox = gtk_hbox_new(FALSE, 0);
    apptw->Recur_limit_label = gtk_label_new(_("Limit"));
    apptw->Recur_limit_rb = gtk_radio_button_new_with_label(NULL
            , _("Repeat forever"));
    gtk_box_pack_start(GTK_BOX(hbox)
            , apptw->Recur_limit_rb, FALSE, FALSE, 0);

    apptw->Recur_count_hbox = gtk_hbox_new(FALSE, 0);
    apptw->Recur_count_rb = gtk_radio_button_new_with_mnemonic_from_widget(
            GTK_RADIO_BUTTON(apptw->Recur_limit_rb), _("Repeat "));
    gtk_box_pack_start(GTK_BOX(apptw->Recur_count_hbox)
            , apptw->Recur_count_rb, FALSE, FALSE, 0);
    apptw->Recur_count_spin = gtk_spin_button_new_with_range(1, 100, 1);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(apptw->Recur_count_spin)
            , TRUE);
    gtk_box_pack_start(GTK_BOX(apptw->Recur_count_hbox)
            , apptw->Recur_count_spin, FALSE, FALSE, 0);
    apptw->Recur_count_label = gtk_label_new(_("times"));
    gtk_box_pack_start(GTK_BOX(apptw->Recur_count_hbox)
            , apptw->Recur_count_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox)
            , apptw->Recur_count_hbox, FALSE, FALSE, 20);

    apptw->Recur_until_hbox = gtk_hbox_new(FALSE, 0);
    apptw->Recur_until_rb = gtk_radio_button_new_with_mnemonic_from_widget(
            GTK_RADIO_BUTTON(apptw->Recur_limit_rb), _("Repeat until "));
    gtk_box_pack_start(GTK_BOX(apptw->Recur_until_hbox)
            , apptw->Recur_until_rb, FALSE, FALSE, 0);
    apptw->Recur_until_button = gtk_button_new();
    gtk_box_pack_start(GTK_BOX(apptw->Recur_until_hbox)
            , apptw->Recur_until_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox)
            , apptw->Recur_until_hbox, FALSE, FALSE, 0);
    orage_table_add_row(apptw->TableRecur
            , apptw->Recur_limit_label, hbox
            , ++row, (GTK_EXPAND | GTK_FILL), (0));

    /* weekdays (only for complex settings) */
    apptw->Recur_byday_label = gtk_label_new(_("Weekdays"));
    apptw->Recur_byday_hbox = gtk_hbox_new(TRUE, 0);
    for (i=0; i <= 6; i++) {
        apptw->Recur_byday_cb[i] = 
                gtk_check_button_new_with_mnemonic(weekday_array[i]);
        gtk_box_pack_start(GTK_BOX(apptw->Recur_byday_hbox)
                , apptw->Recur_byday_cb[i], FALSE, FALSE, 0);
    }
    orage_table_add_row(apptw->TableRecur
            , apptw->Recur_byday_label, apptw->Recur_byday_hbox
            , ++row, (GTK_EXPAND | GTK_FILL), (0));

    apptw->Recur_byday_spin_label = gtk_label_new(_("Which day"));
    apptw->Recur_byday_spin_hbox = gtk_hbox_new(TRUE, 0);
    for (i=0; i <= 6; i++) {
        apptw->Recur_byday_spin[i] = 
                gtk_spin_button_new_with_range(-9, 9, 1);
        gtk_box_pack_start(GTK_BOX(apptw->Recur_byday_spin_hbox)
                , apptw->Recur_byday_spin[i], FALSE, FALSE, 0);
        gtk_tooltips_set_tip(apptw->Tooltips, apptw->Recur_byday_spin[i]
                , _("Specify which weekday for monthly and yearly events.\n For example:\n Second Wednesday each month:\n\tFrequency = Monthly,\n\tWeekdays = check only Wednesday,\n\tWhich day = select 2 from the number below Wednesday")
                , NULL);
    }
    orage_table_add_row(apptw->TableRecur
            , apptw->Recur_byday_spin_label, apptw->Recur_byday_spin_hbox
            , ++row ,(GTK_EXPAND | GTK_FILL), (0));

    /* TODO base (only for TODOs) */
    apptw->Recur_todo_base_label = gtk_label_new(_("TODO base"));
    apptw->Recur_todo_base_hbox = gtk_hbox_new(FALSE, 0);
    apptw->Recur_todo_base_start_rb = gtk_radio_button_new_with_label(NULL
            , _("Start"));
    gtk_box_pack_start(GTK_BOX(apptw->Recur_todo_base_hbox)
            , apptw->Recur_todo_base_start_rb, FALSE, FALSE, 15);
    apptw->Recur_todo_base_done_rb = 
            gtk_radio_button_new_with_mnemonic_from_widget(
                    GTK_RADIO_BUTTON(apptw->Recur_todo_base_start_rb)
                            , _("Completed"));
    gtk_box_pack_start(GTK_BOX(apptw->Recur_todo_base_hbox)
            , apptw->Recur_todo_base_done_rb, FALSE, FALSE, 15);
    gtk_tooltips_set_tip(apptw->Tooltips, apptw->Recur_todo_base_start_rb
            , _("TODO reoccurs regularly starting on start time and repeating after each interval no matter when it was last completed"), NULL);
    gtk_tooltips_set_tip(apptw->Tooltips, apptw->Recur_todo_base_done_rb
            , _("TODO reoccurrency is based on complete time and repeats after the interval counted from the last completed time.\n(Note that you can not tell anything about the history of the TODO since reoccurrence base changes after each completion.)")
            , NULL);
    orage_table_add_row(apptw->TableRecur
            , apptw->Recur_todo_base_label, apptw->Recur_todo_base_hbox
            , ++row ,(GTK_EXPAND | GTK_FILL), (0));

    /* exceptions */
    apptw->Recur_exception_label = gtk_label_new(_("Exceptions"));
    apptw->Recur_exception_hbox = gtk_hbox_new(FALSE, 0);
    apptw->Recur_exception_scroll_win = gtk_scrolled_window_new(NULL, NULL);
    gtk_box_pack_start(GTK_BOX(apptw->Recur_exception_hbox)
            , apptw->Recur_exception_scroll_win, TRUE, TRUE, 0);
    gtk_scrolled_window_set_policy(
            GTK_SCROLLED_WINDOW(apptw->Recur_exception_scroll_win)
            , GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    apptw->Recur_exception_rows_vbox = gtk_vbox_new(FALSE, 0);
    gtk_scrolled_window_add_with_viewport(
            GTK_SCROLLED_WINDOW(apptw->Recur_exception_scroll_win)
            , apptw->Recur_exception_rows_vbox);
    gtk_tooltips_set_tip(apptw->Tooltips, apptw->Recur_exception_scroll_win
            , _("Add more exception dates by clicking the calendar days below.\nException is either exclusion(-) or inclusion(+) depending on the selection.\nRemove by clicking the data."), NULL);
    apptw->Recur_exception_type_vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(apptw->Recur_exception_hbox)
            , apptw->Recur_exception_type_vbox, TRUE, TRUE, 5);
    apptw->Recur_exception_excl_rb = gtk_radio_button_new_with_label(NULL
            , _("Add excluded date (-)"));
    gtk_tooltips_set_tip(apptw->Tooltips, apptw->Recur_exception_excl_rb
            , _("Excluded days are full days where this appointment is not happening"), NULL);
    gtk_box_pack_start(GTK_BOX(apptw->Recur_exception_type_vbox)
            , apptw->Recur_exception_excl_rb, FALSE, FALSE, 0);
    apptw->Recur_exception_incl_rb = 
            gtk_radio_button_new_with_mnemonic_from_widget(
                    GTK_RADIO_BUTTON(apptw->Recur_exception_excl_rb)
                    , _("Add included time (+)"));
    gtk_tooltips_set_tip(apptw->Tooltips, apptw->Recur_exception_incl_rb
            , _("Included times have same timezone than start time, but they may have different time"), NULL);
    apptw->Recur_exception_incl_spin_hh = 
            gtk_spin_button_new_with_range(0, 23, 1);
    apptw->Recur_exception_incl_spin_mm = 
            gtk_spin_button_new_with_range(0, 59, 1);
    apptw->Recur_exception_incl_time_hbox = datetime_hbox_new(
            apptw->Recur_exception_incl_rb
            , apptw->Recur_exception_incl_spin_hh
            , apptw->Recur_exception_incl_spin_mm, NULL);
    gtk_box_pack_start(GTK_BOX(apptw->Recur_exception_type_vbox)
            , apptw->Recur_exception_incl_time_hbox, FALSE, FALSE, 0);

    orage_table_add_row(apptw->TableRecur
            , apptw->Recur_exception_label, apptw->Recur_exception_hbox
            , ++row ,(GTK_EXPAND | GTK_FILL), (0));

    /* calendars showing the action days */
    apptw->Recur_calendar_label = gtk_label_new(_("Action dates"));
    apptw->Recur_calendar_hbox = gtk_hbox_new(FALSE, 0);
    apptw->Recur_calendar1 = gtk_calendar_new();
    gtk_calendar_set_display_options(GTK_CALENDAR(apptw->Recur_calendar1)
            , GTK_CALENDAR_SHOW_HEADING | GTK_CALENDAR_SHOW_DAY_NAMES);
    gtk_calendar_get_date(GTK_CALENDAR(apptw->Recur_calendar1), &y, &m, NULL);
    gtk_calendar_select_day(GTK_CALENDAR(apptw->Recur_calendar1), 0);
    gtk_box_pack_start(GTK_BOX(apptw->Recur_calendar_hbox)
            , apptw->Recur_calendar1, FALSE, FALSE, 0);

    apptw->Recur_calendar2 = gtk_calendar_new();
    gtk_calendar_set_display_options(GTK_CALENDAR(apptw->Recur_calendar2)
            , GTK_CALENDAR_SHOW_HEADING | GTK_CALENDAR_SHOW_DAY_NAMES);
    if (++m>11) {
        m=0;
        y++;
    }
    gtk_calendar_select_month(GTK_CALENDAR(apptw->Recur_calendar2), m, y);
    gtk_calendar_select_day(GTK_CALENDAR(apptw->Recur_calendar2), 0);
    gtk_box_pack_start(GTK_BOX(apptw->Recur_calendar_hbox)
            , apptw->Recur_calendar2, FALSE, FALSE, 0);
    
    apptw->Recur_calendar3 = gtk_calendar_new();
    gtk_calendar_set_display_options(GTK_CALENDAR(apptw->Recur_calendar3)
            , GTK_CALENDAR_SHOW_HEADING | GTK_CALENDAR_SHOW_DAY_NAMES);
    if (++m>11) {
        m=0;
        y++;
    }
    gtk_calendar_select_month(GTK_CALENDAR(apptw->Recur_calendar3), m, y);
    gtk_calendar_select_day(GTK_CALENDAR(apptw->Recur_calendar3), 0);
    gtk_box_pack_start(GTK_BOX(apptw->Recur_calendar_hbox)
            , apptw->Recur_calendar3, FALSE, FALSE, 0);
    orage_table_add_row(apptw->TableRecur
            , apptw->Recur_calendar_label, apptw->Recur_calendar_hbox
            , ++row ,(GTK_EXPAND | GTK_FILL), (0));

}

static void enable_recurrence_page_signals(appt_win *apptw)
{
    gint i;

    g_signal_connect((gpointer)apptw->Recur_feature_normal_rb, "clicked"
            , G_CALLBACK(app_recur_feature_checkbutton_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->Recur_feature_advanced_rb, "clicked"
            , G_CALLBACK(app_recur_feature_checkbutton_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->Recur_freq_cb, "changed"
            , G_CALLBACK(on_freq_combobox_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->Recur_int_spin, "value-changed"
            , G_CALLBACK(on_recur_spin_button_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->Recur_limit_rb, "clicked"
            , G_CALLBACK(app_recur_checkbutton_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->Recur_count_rb, "clicked"
            , G_CALLBACK(app_recur_checkbutton_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->Recur_count_spin, "value-changed"
            , G_CALLBACK(on_recur_spin_button_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->Recur_until_rb, "clicked"
            , G_CALLBACK(app_recur_checkbutton_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->Recur_until_button, "clicked"
            , G_CALLBACK(on_recur_Date_button_clicked_cb), apptw);
    for (i=0; i <= 6; i++) {
        g_signal_connect((gpointer)apptw->Recur_byday_cb[i], "clicked"
                , G_CALLBACK(app_recur_checkbutton_clicked_cb), apptw);
        g_signal_connect((gpointer)apptw->Recur_byday_spin[i], "value-changed"
                , G_CALLBACK(on_recur_spin_button_changed_cb), apptw);
    }
    g_signal_connect((gpointer)apptw->Recur_todo_base_start_rb, "clicked"
            , G_CALLBACK(app_recur_checkbutton_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->Recur_todo_base_done_rb, "clicked"
            , G_CALLBACK(app_recur_checkbutton_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->Recur_calendar1, "month-changed"
            , G_CALLBACK(recur_month_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->Recur_calendar2, "month-changed"
            , G_CALLBACK(recur_month_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->Recur_calendar3, "month-changed"
            , G_CALLBACK(recur_month_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->Recur_calendar1
            , "day_selected_double_click"
            , G_CALLBACK(recur_day_selected_double_click_cb), apptw);
    g_signal_connect((gpointer)apptw->Recur_calendar2
            , "day_selected_double_click"
            , G_CALLBACK(recur_day_selected_double_click_cb), apptw);
    g_signal_connect((gpointer)apptw->Recur_calendar3
            , "day_selected_double_click"
            , G_CALLBACK(recur_day_selected_double_click_cb), apptw);
}

appt_win *create_appt_win(char *action, char *par)
{
    appt_win *apptw;
    GdkWindow *window;

    /*  initialisation + main window + base vbox */
    apptw = g_new(appt_win, 1);
    apptw->xf_uid = NULL;
    apptw->par = NULL;
    apptw->xf_appt = NULL;
    apptw->el = NULL;
    apptw->dw = NULL;
    apptw->appointment_changed = FALSE;
    apptw->Tooltips = gtk_tooltips_new();
    apptw->accel_group = gtk_accel_group_new();

    apptw->Window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    /*
    gtk_window_set_default_size(GTK_WINDOW(apptw->Window), 450, 325);
    */
    gtk_window_add_accel_group(GTK_WINDOW(apptw->Window), apptw->accel_group);

    apptw->Vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(apptw->Window), apptw->Vbox);

    build_menu(apptw);
    build_toolbar(apptw);

    /* ********** Here begins tabs ********** */
    apptw->Notebook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(apptw->Vbox), apptw->Notebook);
    gtk_container_set_border_width(GTK_CONTAINER (apptw->Notebook), 5);

    build_general_page(apptw);
    build_alarm_page(apptw);
    build_recurrence_page(apptw);

    g_signal_connect((gpointer)apptw->Window, "delete-event"
            , G_CALLBACK(on_appWindow_delete_event_cb), apptw);

    fill_appt_window(apptw, action, par);
    if (apptw->xf_appt) { /* all fine */
        enable_general_page_signals(apptw);
        enable_alarm_page_signals(apptw);
        enable_recurrence_page_signals(apptw);
        gtk_widget_show_all(apptw->Window);
        recur_hide_show(apptw);
        type_hide_show(apptw);
        g_signal_connect((gpointer)apptw->Notebook, "switch-page"
                , G_CALLBACK(on_notebook_page_switch), apptw);
        gtk_widget_grab_focus(apptw->Title_entry);
        window = GTK_WIDGET(apptw->Window)->window;
        gdk_x11_window_set_user_time(window, gdk_x11_get_server_time(window));
        gtk_window_present(GTK_WINDOW(apptw->Window));
    }
    else { /* failed to get data */
        app_free_memory(apptw);
        apptw = NULL;
    }

    return(apptw);
}
