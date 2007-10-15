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
#  include <config.h>
#endif

#define _XOPEN_SOURCE /* glibc2 needs this */

#include <sys/types.h>
#include <sys/stat.h>
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
#include <glib/gprintf.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/netk-trayicon.h>
#include <libxfcegui4/libxfcegui4.h>

#include "functions.h"
#include "mainbox.h"
#include "ical-code.h"
#include "event-list.h"
#include "appointment.h"
#include "parameters.h"

#define AVAILABILITY_ARRAY_DIM 2
#define RECUR_ARRAY_DIM 5
#define BORDER_SIZE 20
#define FILETYPE_SIZE 38


static void fill_appt_window(appt_win *apptw, char *action, char *par);


static void combo_box_append_array(GtkWidget *combo_box, char *text[], int size)
{
    register int i;

    for (i = 0; i < size; i++) {
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo_box)
                , (const gchar *)text[i]);
    }
}

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
    gtk_widget_set_size_request(spin_hh, 40, -1);
    gtk_box_pack_start(GTK_BOX(hbox), spin_hh, FALSE, FALSE, 0);

    space_label = gtk_label_new(":");
    gtk_box_pack_start(GTK_BOX(hbox), space_label, FALSE, FALSE, 0);

    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(spin_mm), TRUE);
    gtk_widget_set_size_request(spin_mm, 40, -1);
    gtk_box_pack_start(GTK_BOX(hbox), spin_mm, FALSE, FALSE, 0);

    space_label = gtk_label_new("  ");
    gtk_box_pack_start(GTK_BOX(hbox), space_label, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(hbox), timezone_button, TRUE, TRUE, 0);

    return hbox;
}

static GtkWidget *period_hbox_new(gboolean head_space, gboolean tail_space
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
    gtk_widget_set_size_request(spin_hh, 40, -1);
    gtk_box_pack_start(GTK_BOX(hbox), spin_hh, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(hbox), hh_label, FALSE, FALSE, 5);
    
    space_label = gtk_label_new("   ");
    gtk_box_pack_start(GTK_BOX(hbox), space_label, FALSE, FALSE, 0);

    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(spin_mm), TRUE);
    gtk_widget_set_size_request(spin_mm, 40, -1);
    gtk_box_pack_start(GTK_BOX(hbox), spin_mm, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(hbox), mm_label, FALSE, FALSE, 5);

    if (tail_space) {
        space_label = gtk_label_new("   ");
        gtk_box_pack_start(GTK_BOX(hbox), space_label, FALSE, FALSE, 0);
    }

    return hbox;
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
    gboolean dur_act, allDay_act, completed_act;

    dur_act = gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(apptw->Dur_checkbutton));
    allDay_act = gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(apptw->AllDay_checkbutton));
    completed_act = gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(apptw->Completed_checkbutton));

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
        gtk_widget_set_sensitive(apptw->CompletedTime_spin_hh, FALSE);
        gtk_widget_set_sensitive(apptw->CompletedTime_spin_mm, FALSE);
        gtk_widget_set_sensitive(apptw->CompletedTimezone_button, FALSE);
        if (dur_act) {
            gtk_widget_set_sensitive(apptw->EndDate_button, FALSE);
            gtk_widget_set_sensitive(apptw->Dur_spin_dd, TRUE);
            gtk_widget_set_sensitive(apptw->Dur_spin_dd_label, TRUE);
        }
        else {
            gtk_widget_set_sensitive(apptw->EndDate_button, TRUE);
            gtk_widget_set_sensitive(apptw->Dur_spin_dd, FALSE);
            gtk_widget_set_sensitive(apptw->Dur_spin_dd_label, FALSE);
        }
        if (completed_act) {
            gtk_widget_set_sensitive(apptw->CompletedDate_button, TRUE);
        }
        else {
            gtk_widget_set_sensitive(apptw->CompletedDate_button, FALSE);
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
            gtk_widget_set_sensitive(apptw->Dur_spin_dd, TRUE);
            gtk_widget_set_sensitive(apptw->Dur_spin_dd_label, TRUE);
            gtk_widget_set_sensitive(apptw->Dur_spin_hh, TRUE);
            gtk_widget_set_sensitive(apptw->Dur_spin_hh_label, TRUE);
            gtk_widget_set_sensitive(apptw->Dur_spin_mm, TRUE);
            gtk_widget_set_sensitive(apptw->Dur_spin_mm_label, TRUE);
        }
        else {
            gtk_widget_set_sensitive(apptw->EndDate_button, TRUE);
            gtk_widget_set_sensitive(apptw->EndTime_spin_hh, TRUE);
            gtk_widget_set_sensitive(apptw->EndTime_spin_mm, TRUE);
            gtk_widget_set_sensitive(apptw->EndTimezone_button, TRUE);
            gtk_widget_set_sensitive(apptw->Dur_spin_dd, FALSE);
            gtk_widget_set_sensitive(apptw->Dur_spin_dd_label, FALSE);
            gtk_widget_set_sensitive(apptw->Dur_spin_hh, FALSE);
            gtk_widget_set_sensitive(apptw->Dur_spin_hh_label, FALSE);
            gtk_widget_set_sensitive(apptw->Dur_spin_mm, FALSE);
            gtk_widget_set_sensitive(apptw->Dur_spin_mm_label, FALSE);
        }
        if (completed_act) {
            gtk_widget_set_sensitive(apptw->CompletedDate_button, TRUE);
            gtk_widget_set_sensitive(apptw->CompletedTime_spin_hh, TRUE);
            gtk_widget_set_sensitive(apptw->CompletedTime_spin_mm, TRUE);
            gtk_widget_set_sensitive(apptw->CompletedTimezone_button, TRUE);
        }
        else {
            gtk_widget_set_sensitive(apptw->CompletedDate_button, FALSE);
            gtk_widget_set_sensitive(apptw->CompletedTime_spin_hh, FALSE);
            gtk_widget_set_sensitive(apptw->CompletedTime_spin_mm, FALSE);
            gtk_widget_set_sensitive(apptw->CompletedTimezone_button, FALSE);
        }
    }
}

static void set_repeat_sensitivity(appt_win *apptw)
{
    gint freq, i;
    
    freq = gtk_combo_box_get_active((GtkComboBox *)apptw->Recur_freq_cb);
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
    }
    else if (gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(apptw->Type_todo_rb))) {
        gtk_label_set_text(GTK_LABEL(apptw->End_label), _("Due"));
        gtk_widget_hide(apptw->Availability_label);
        gtk_widget_hide(apptw->Availability_cb);
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
    }
    else /* if (gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(apptw->Type_journal_rb))) */ {
        gtk_label_set_text(GTK_LABEL(apptw->End_label), _("End"));
        gtk_widget_hide(apptw->Availability_label);
        gtk_widget_hide(apptw->Availability_cb);
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
    }
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

static void appDur_checkbutton_clicked_cb(GtkCheckButton *cb
        , gpointer user_data)
{
    set_time_sensitivity((appt_win *)user_data);
    mark_appointment_changed((appt_win *)user_data);
}

static void appCompleted_checkbutton_clicked_cb(GtkCheckButton *cb
        , gpointer user_data)
{
    set_time_sensitivity((appt_win *)user_data);
    mark_appointment_changed((appt_win *)user_data);
}

static void app_recur_checkbutton_clicked_cb(GtkCheckButton *checkbutton
        , gpointer user_data)
{
    mark_appointment_changed((appt_win *)user_data);
    set_repeat_sensitivity((appt_win *)user_data);
}

static void app_recur_feature_checkbutton_clicked_cb(GtkCheckButton *cb
        , gpointer user_data)
{
    mark_appointment_changed((appt_win *)user_data);
    recur_hide_show((appt_win *)user_data);
}

static void app_sound_checkbutton_clicked_cb(GtkCheckButton *cb
        , gpointer user_data)
{
    mark_appointment_changed((appt_win *)user_data);
    set_sound_sensitivity((appt_win *)user_data);
}

#ifdef HAVE_NOTIFY
static void app_notify_checkbutton_clicked_cb(GtkCheckButton *cb
        , gpointer user_data)
{
    mark_appointment_changed((appt_win *)user_data);
    set_notify_sensitivity((appt_win *)user_data);
}
#endif

static void app_proc_checkbutton_clicked_cb(GtkCheckButton *cb
        , gpointer user_data)
{
    mark_appointment_changed((appt_win *)user_data);
    set_proc_sensitivity((appt_win *)user_data);
}

static void app_checkbutton_clicked_cb(GtkCheckButton *cb, gpointer user_data)
{
    mark_appointment_changed((appt_win *)user_data);
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
}

static void on_app_combobox_changed_cb(GtkComboBox *cb, gpointer user_data)
{
    mark_appointment_changed((appt_win *)user_data);
}

static void on_app_spin_button_changed_cb(GtkSpinButton *sb
        , gpointer user_data)
{
    mark_appointment_changed((appt_win *)user_data);
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

static void on_appAllDay_clicked_cb(GtkCheckButton *cb, gpointer user_data)
{
    set_time_sensitivity((appt_win *)user_data);
    mark_appointment_changed((appt_win *)user_data);
}

static void app_free_memory(appt_win *apptw)
{
    gtk_widget_destroy(apptw->Window);
    gtk_object_destroy(GTK_OBJECT(apptw->Tooltips));
    g_free(apptw->xf_uid);
    g_free(apptw->par);
    xfical_appt_free(apptw->appt);
    g_free(apptw);
}

static gboolean appWindow_check_and_close(appt_win *apptw)
{
    gint result;

    if (apptw->appointment_changed == TRUE) {
        result = xfce_message_dialog(GTK_WINDOW(apptw->Window),
             _("Warning"),
             GTK_STOCK_DIALOG_WARNING,
             _("The appointment information has been modified."),
             _("Do you want to continue?"),
             GTK_STOCK_YES, GTK_RESPONSE_ACCEPT,
             GTK_STOCK_NO, GTK_RESPONSE_CANCEL,
             NULL);

        if (result == GTK_RESPONSE_ACCEPT) {
            app_free_memory(apptw);
            /*
            gtk_widget_destroy(apptw->Window);
            gtk_object_destroy(GTK_OBJECT(apptw->Tooltips));
            g_free(apptw);
            */
        }
    }
    else {
        app_free_memory(apptw);
        /*
        gtk_widget_destroy(apptw->Window);
        gtk_object_destroy(GTK_OBJECT(apptw->Tooltips));
        xfical_appt_free(apptw->appt);
        g_free(apptw);
        */
    }
    return TRUE;
}

static gboolean on_appWindow_delete_event_cb(GtkWidget *widget, GdkEvent *event
    , gpointer user_data)
{
    return appWindow_check_and_close((appt_win *)user_data);
}

static gboolean orage_validate_datetime(appt_win *apptw, xfical_appt *appt)
{
    gint result;

    if (xfical_compare_times(appt) > 0) {
        result = xfce_message_dialog(GTK_WINDOW(apptw->Window),
                _("Warning"),
                GTK_STOCK_DIALOG_WARNING,
                _("The end of this appointment is earlier than the beginning."),
                NULL,
                GTK_STOCK_OK,
                GTK_RESPONSE_ACCEPT,
                NULL);
        return FALSE;
    }
    else {
        return TRUE;
    }
}

/*
 * fill_appt_from_apptw
 * This function fills an appointment with the contents of an appointment 
 * window
 */
static gboolean fill_appt_from_apptw(xfical_appt *appt, appt_win *apptw)
{
    GtkTextIter start, end;
    const char *time_format="%H:%M";
    struct tm current_t;
    gchar starttime[6], endtime[6], completedtime[6];
    gint i, j, k;
    gchar *tmp;

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
    appt->duration = gtk_spin_button_get_value_as_int(
            GTK_SPIN_BUTTON(apptw->Dur_spin_dd)) * 24*60*60
                    + gtk_spin_button_get_value_as_int(
            GTK_SPIN_BUTTON(apptw->Dur_spin_hh)) *    60*60
                    + gtk_spin_button_get_value_as_int(
            GTK_SPIN_BUTTON(apptw->Dur_spin_mm)) *       60;

    /* Check that end time is after start time.
     * Journal does not have end time so no need to check */
    if (appt->type != XFICAL_TYPE_JOURNAL 
    && !orage_validate_datetime(apptw, appt))
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

    /* notes */
    gtk_text_buffer_get_bounds(apptw->Note_buffer, &start, &end);
            /*
            gtk_text_view_get_buffer((GtkTextView *)apptw->Note_textview)
            */
    appt->note = gtk_text_iter_get_text(&start, &end);

            /*********** ALARM TAB ***********/
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

    /* Do we use audio alarm */
    appt->sound_alarm = gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(apptw->Sound_checkbutton));

    /* Which sound file will be played */
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
    appt->procedure_alarm = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(apptw->Proc_checkbutton));

    /* The actual command. 
     * Note that we need to split this into cmd and the parameters 
     * since that is how libical stores it */
    appt->procedure_cmd =  NULL;
    appt->procedure_params =  NULL;
    tmp = (char*)gtk_entry_get_text(GTK_ENTRY(apptw->Proc_entry));
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

    return(TRUE);
}

static void on_appFileClose_menu_activate_cb(GtkMenuItem *mi
        , gpointer user_data)
{
    appWindow_check_and_close((appt_win *)user_data);
}

static gboolean save_xfical_from_appt_win(appt_win *apptw)
{
    gboolean ok = FALSE;
    xfical_appt *appt = apptw->appt;

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
                orage_message("Added: %s", apptw->xf_uid);
            }
            else
                g_warning("Addition failed: %s", apptw->xf_uid);
            }
        else {
            ok = xfical_appt_mod(apptw->xf_uid, appt);
            if (ok)
                orage_message("Modified: %s", apptw->xf_uid);
            else
                g_warning("Modification failed: %s", apptw->xf_uid);
        }
        xfical_file_close(TRUE);
        if (ok) {
            apptw->appointment_new = FALSE;
            mark_appointment_unchanged(apptw);
        /* FIXME: This fails if event_list window has been removed.
         * We should check that it really still exists, before calling this.
            if (apptw->el != NULL)
                refresh_el_win((el_win *)apptw->el);
        */
            orage_mark_appointments();
        }
    }
    return (ok);
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
        /*
        gtk_widget_destroy(apptw->Window);
        gtk_object_destroy(GTK_OBJECT(apptw->Tooltips));
        xfical_appt_free(apptw->appt);
        g_free(apptw);
        */
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

    result = xfce_message_dialog(GTK_WINDOW(apptw->Window)
            , _("Warning")
            , GTK_STOCK_DIALOG_WARNING
            , _("This appointment will be permanently removed.")
            , _("Do you want to continue?")
            , GTK_STOCK_YES, GTK_RESPONSE_ACCEPT
            , GTK_STOCK_NO, GTK_RESPONSE_REJECT
            , NULL);
                                 
    if (result == GTK_RESPONSE_ACCEPT) {
        if (!apptw->appointment_add) {
            if (!xfical_file_open(TRUE))
                    return;
            result = xfical_appt_del(apptw->xf_uid);
            if (result)
                orage_message("Removed: %s", apptw->xf_uid);
            else
                g_warning("Removal failed: %s", apptw->xf_uid);
            xfical_file_close(TRUE);
        }

        /* FIXME: This fails if event_list window has been removed.
         * We should check that it really still exists, before calling this.
        if (apptw->el != NULL)
            refresh_el_win((el_win *)apptw->el);
        */
        orage_mark_appointments();

        app_free_memory(apptw);
        /*
        gtk_widget_destroy(apptw->Window);
        gtk_object_destroy(GTK_OBJECT(apptw->Tooltips));
        g_free(apptw->xf_uid);
        g_free(apptw->par);
        xfical_appt_free(apptw->appt);
        g_free(apptw);
        */
    }
}

static void on_appDelete_clicked_cb(GtkButton *b, gpointer user_data)
{
    delete_xfical_from_appt_win((appt_win *)user_data);
}

static void on_appFileDelete_menu_activate_cb(GtkMenuItem *mi
        , gpointer user_data)
{
    delete_xfical_from_appt_win((appt_win *) user_data);
}

static void duplicate_xfical_from_appt_win(appt_win *apptw)
{
    gint x, y;
    appt_win *apptw2;

    apptw2 = create_appt_win("COPY", apptw->xf_uid, apptw->el);
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

static void on_appFileRevert_menu_activate_cb(GtkMenuItem *mi, gpointer user_data)
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

static void on_appStartTimezone_clicked_cb(GtkButton *button
        , gpointer *user_data)
{
    appt_win *apptw = (appt_win *)user_data;
    xfical_appt *appt;

    appt = apptw->appt;
    if (xfical_timezone_button_clicked(button, GTK_WINDOW(apptw->Window)
            , &appt->start_tz_loc))
        mark_appointment_changed(apptw);
}

static void on_appEndTimezone_clicked_cb(GtkButton *button
        , gpointer *user_data)
{
    appt_win *apptw = (appt_win *)user_data;
    xfical_appt *appt;

    appt = apptw->appt;
    if (xfical_timezone_button_clicked(button, GTK_WINDOW(apptw->Window)
            , &appt->end_tz_loc))
        mark_appointment_changed(apptw);
}

static void on_appCompletedTimezone_clicked_cb(GtkButton *button
        , gpointer *user_data)
{
    appt_win *apptw = (appt_win *)user_data;
    xfical_appt *appt;

    appt = apptw->appt;
    if (xfical_timezone_button_clicked(button, GTK_WINDOW(apptw->Window)
            , &appt->completed_tz_loc))
        mark_appointment_changed(apptw);
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
                GTK_SPIN_BUTTON(apptw->StartTime_spin_mm)
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
        gtk_spin_button_set_value(
                GTK_SPIN_BUTTON(apptw->CompletedTime_spin_hh)
                        , (gdouble)tm_date.tm_hour);
        gtk_spin_button_set_value(
                GTK_SPIN_BUTTON(apptw->CompletedTime_spin_mm)
                        , (gdouble)tm_date.tm_hour);
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

static xfical_appt *fill_appt_window_get_appt(char *action, char *par)
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
        appt->duration = 30;
        g_sprintf(appt->completedtime,"%sT%02d%02d00"
                    , today, t->tm_hour, t->tm_min);
        appt->completed_tz_loc = g_strdup(appt->start_tz_loc);

        /* default alarm time is 5 mins before event (Bug 3425) */
        appt->alarmtime = 5*60;

        /* default alarm time is 500 cnt & 2 secs each */
        appt->soundrepeat_cnt = 500;
        appt->soundrepeat_len = 2;

        /* default alarm type is orage window */
        appt->display_alarm_orage = TRUE;
    }
    else if ((strcmp(action, "UPDATE") == 0) 
          || (strcmp(action, "COPY") == 0)) {
        /* par contains ical uid */
        if (!xfical_file_open(TRUE))
            return(NULL);
        if ((appt = xfical_appt_get(par)) == NULL) {
            orage_message("appointment not found");
            xfical_file_close(TRUE);
            return(NULL);
        }
        xfical_file_close(TRUE);
    }
    else {
        g_error("unknown parameter\n");
        return(NULL);
    }

    return(appt);
}

static void fill_appt_window(appt_win *apptw, char *action, char *par)
{
    int year, month, day, hours, minutes;
    xfical_appt *appt;
    struct tm *t, tm_date;
    char *untildate_to_display, *tmp;
    int i;

    orage_message("%s appointment: %s", action, par);
    if ((appt = fill_appt_window_get_appt(action, par)) == NULL) {
        apptw->appt = NULL;
        return;
    }
    apptw->appt = appt;

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
        g_warning("fill_appt_window: Illegal value for type\n");

    /* appointment name */
    gtk_entry_set_text(GTK_ENTRY(apptw->Title_entry)
            , (appt->title ? appt->title : ""));

    if (strcmp(action, "COPY") == 0) {
            gtk_editable_set_position(GTK_EDITABLE(apptw->Title_entry), -1);
            i = gtk_editable_get_position(GTK_EDITABLE(apptw->Title_entry));
            gtk_editable_insert_text(GTK_EDITABLE(apptw->Title_entry)
                    , _(" *** COPY ***"), strlen(_(" *** COPY ***")), &i);
    }
    /*
        gtk_entry_append_text(GTK_ENTRY(apptw->Title_entry)
                , _(" *** COPY ***"));
                */

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

    /* note */
    gtk_text_buffer_set_text(apptw->Note_buffer
            , (appt->note ? (const gchar *) appt->note : ""), -1);

    /********************* ALARM tab *********************/
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

    /* sound */
    gtk_toggle_button_set_active(
            GTK_TOGGLE_BUTTON(apptw->Sound_checkbutton)
                    , appt->sound_alarm);
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
    if (!appt->display_alarm_notify 
            || appt->display_notify_timeout == -1) { /* no timeout */
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
            GTK_TOGGLE_BUTTON(apptw->Proc_checkbutton)
                    , appt->procedure_alarm);
    tmp = g_strconcat(appt->procedure_cmd, " ", appt->procedure_params, NULL);
    gtk_entry_set_text(GTK_ENTRY(apptw->Proc_entry), tmp ? tmp : "");
    g_free(tmp);

    /********************* RECURRENCE tab *********************/
    /* recurrence */
    gtk_combo_box_set_active(GTK_COMBO_BOX(apptw->Recur_freq_cb)
            , appt->freq);
    switch(appt->recur_limit) {
        case 0: /* no limit */
            gtk_toggle_button_set_active(
                    GTK_TOGGLE_BUTTON(apptw->Recur_limit_rb), TRUE);
            gtk_spin_button_set_value(
                    GTK_SPIN_BUTTON(apptw->Recur_count_spin), (gdouble)1);
            /*
            t = orage_localtime();
            untildate_to_display = orage_tm_date_to_i18_date(t);
            */
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
            /*
            t = orage_localtime();
            untildate_to_display = orage_tm_date_to_i18_date(t);
            */
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
    /*
    xfical_appt_free(appt);
    */

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

/*
static void oc_set_height_changed(GtkSpinButton *sb, appt_win *apptw)
{
        gint test;

        test = gtk_spin_button_get_value_as_int(sb);
        g_print("oc_set_height_changed: %d\n", test);
}
*/


static void build_general_page(appt_win *apptw)
{
    gint row;
    GtkWidget *label, *event, *hbox;
    char *availability_array[AVAILABILITY_ARRAY_DIM] = {
            _("Free"), _("Busy")};

    apptw->TableGeneral = orage_table_new(10, BORDER_SIZE);
    apptw->General_notebook_page = apptw->TableGeneral;
    apptw->General_tab_label = gtk_label_new(_("General"));

    gtk_notebook_append_page(GTK_NOTEBOOK(apptw->Notebook)
            , apptw->General_notebook_page, apptw->General_tab_label);

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

    apptw->Title_label = gtk_label_new(_("Title "));
    apptw->Title_entry = gtk_entry_new();
    orage_table_add_row(apptw->TableGeneral
            , apptw->Title_label, apptw->Title_entry
            , ++row, (GTK_EXPAND | GTK_FILL), (0));

    apptw->Location_label = gtk_label_new(_("Location"));
    apptw->Location_entry = gtk_entry_new();
    orage_table_add_row(apptw->TableGeneral
            , apptw->Location_label, apptw->Location_entry
            , ++row, (GTK_EXPAND | GTK_FILL), (0));

    apptw->AllDay_checkbutton = 
            gtk_check_button_new_with_mnemonic(_("All day event"));
    orage_table_add_row(apptw->TableGeneral
            , NULL, apptw->AllDay_checkbutton
            , ++row, (GTK_EXPAND | GTK_FILL), (0));

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
    /*
g_signal_connect((gpointer) apptw->StartTime_spin_hh, "value-changed",
            G_CALLBACK(oc_set_height_changed), NULL);
*/

    apptw->End_label = gtk_label_new(_("End"));
    apptw->EndDate_button = gtk_button_new();
    apptw->EndTime_spin_hh = gtk_spin_button_new_with_range(0, 23, 1);
    apptw->EndTime_spin_mm = gtk_spin_button_new_with_range(0, 59, 1);
    apptw->EndTimezone_button = gtk_button_new();
    apptw->EndTime_hbox = datetime_hbox_new(
            apptw->EndDate_button, 
            apptw->EndTime_spin_hh, 
            apptw->EndTime_spin_mm, 
            apptw->EndTimezone_button);
    orage_table_add_row(apptw->TableGeneral
            , apptw->End_label, apptw->EndTime_hbox
            , ++row, (GTK_SHRINK | GTK_FILL), (GTK_SHRINK | GTK_FILL));

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
    apptw->Dur_time_hbox = period_hbox_new(TRUE, FALSE
            , apptw->Dur_spin_dd, apptw->Dur_spin_dd_label
            , apptw->Dur_spin_hh, apptw->Dur_spin_hh_label
            , apptw->Dur_spin_mm, apptw->Dur_spin_mm_label);
    gtk_box_pack_start(GTK_BOX(apptw->Dur_hbox), apptw->Dur_time_hbox
            , FALSE, FALSE, 0);
    orage_table_add_row(apptw->TableGeneral
            , NULL, apptw->Dur_hbox
            , ++row, (GTK_FILL), (GTK_FILL));
    
    apptw->Availability_label = gtk_label_new(_("Availability"));
    apptw->Availability_cb = gtk_combo_box_new_text();
    combo_box_append_array(apptw->Availability_cb
            , availability_array, AVAILABILITY_ARRAY_DIM);
    orage_table_add_row(apptw->TableGeneral
            , apptw->Availability_label, apptw->Availability_cb
            , ++row, (GTK_FILL), (GTK_FILL));
    
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
            , _("<D> inserts current date in local date format.\n<T> inserts time and\n<DT> inserts date and time."), NULL);

    g_signal_connect((gpointer)apptw->Type_event_rb, "clicked"
            , G_CALLBACK(app_type_checkbutton_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->Type_todo_rb, "clicked"
            , G_CALLBACK(app_type_checkbutton_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->Type_journal_rb, "clicked"
            , G_CALLBACK(app_type_checkbutton_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->AllDay_checkbutton, "clicked"
            , G_CALLBACK(on_appAllDay_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->StartDate_button, "clicked"
            , G_CALLBACK(on_Date_button_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->StartTime_spin_hh, "changed"
            , G_CALLBACK(on_app_spin_button_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->StartTime_spin_mm, "changed"
            , G_CALLBACK(on_app_spin_button_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->StartTimezone_button, "clicked"
            , G_CALLBACK(on_appStartTimezone_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->EndDate_button, "clicked"
            , G_CALLBACK(on_Date_button_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->EndTime_spin_hh, "changed"
            , G_CALLBACK(on_app_spin_button_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->EndTime_spin_mm, "changed"
            , G_CALLBACK(on_app_spin_button_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->EndTimezone_button, "clicked"
            , G_CALLBACK(on_appEndTimezone_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->Dur_checkbutton, "clicked"
            , G_CALLBACK(appDur_checkbutton_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->Dur_spin_dd, "value-changed"
            , G_CALLBACK(on_app_spin_button_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->Dur_spin_hh, "value-changed"
            , G_CALLBACK(on_app_spin_button_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->Dur_spin_mm, "value-changed"
            , G_CALLBACK(on_app_spin_button_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->Completed_checkbutton, "clicked"
            , G_CALLBACK(appCompleted_checkbutton_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->CompletedDate_button, "clicked"
            , G_CALLBACK(on_Date_button_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->CompletedTime_spin_hh, "changed"
            , G_CALLBACK(on_app_spin_button_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->CompletedTime_spin_mm, "changed"
            , G_CALLBACK(on_app_spin_button_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->CompletedTimezone_button, "clicked"
            , G_CALLBACK(on_appCompletedTimezone_clicked_cb), apptw);
    /* Take care of the title entry to build the appointment window title 
     * Beware: we are not using apptw->Title_entry as a GtkEntry here 
     * but as an interface GtkEditable instead.
     */
    g_signal_connect((gpointer)apptw->Title_entry, "changed"
            , G_CALLBACK(on_appTitle_entry_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->Location_entry, "changed"
            , G_CALLBACK(on_app_entry_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->Availability_cb, "changed"
            , G_CALLBACK(on_app_combobox_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->Note_buffer, "changed"
            , G_CALLBACK(on_appNote_buffer_changed_cb), apptw);
}

static void build_alarm_page(appt_win *apptw)
{
    gint row;
    GtkWidget *label, *event;
    char *when_array[4] = {_("Before Start"), _("Before End")
        , _("After Start"), _("After End")};

    /***** Header *****/
    apptw->TableAlarm = orage_table_new(8, BORDER_SIZE);
    apptw->Alarm_notebook_page = apptw->TableAlarm;
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
    apptw->Alarm_time_hbox = period_hbox_new(FALSE, TRUE
            , apptw->Alarm_spin_dd, apptw->Alarm_spin_dd_label
            , apptw->Alarm_spin_hh, apptw->Alarm_spin_hh_label
            , apptw->Alarm_spin_mm, apptw->Alarm_spin_mm_label);
    gtk_box_pack_start(GTK_BOX(apptw->Alarm_hbox), apptw->Alarm_time_hbox
            , FALSE, FALSE, 0);
    apptw->Alarm_when_cb = gtk_combo_box_new_text();
    combo_box_append_array(apptw->Alarm_when_cb
            , when_array, 4);
    event =  gtk_event_box_new(); /* only needed for tooltips */
    gtk_container_add(GTK_CONTAINER(event), apptw->Alarm_when_cb);
    gtk_box_pack_start(GTK_BOX(apptw->Alarm_hbox)
            , event, FALSE, FALSE, 0);
    orage_table_add_row(apptw->TableAlarm
            , apptw->Alarm_label, apptw->Alarm_hbox
            , row = 0, (GTK_FILL), (GTK_FILL));
    gtk_tooltips_set_tip(apptw->Tooltips, event
            , _("Often you want to get alarm:\n 1) before Event start\n 2) before Todo end\n 3) after Todo start"), NULL);

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
}

static void build_recurrence_page(appt_win *apptw)
{
    gint row, i;
    char *recur_freq_array[RECUR_ARRAY_DIM] = {
        _("None"), _("Daily"), _("Weekly"), _("Monthly"), _("Yearly")};
    char *weekday_array[7] = {
        _("Mon"), _("Tue"), _("Wed"), _("Thu"), _("Fri"), _("Sat"), _("Sun")};

    apptw->TableRecur = orage_table_new(8, BORDER_SIZE);
    apptw->Recur_notebook_page = apptw->TableRecur;
    apptw->Recur_tab_label = gtk_label_new(_("Recurrence"));

    gtk_notebook_append_page(GTK_NOTEBOOK(apptw->Notebook)
            , apptw->Recur_notebook_page, apptw->Recur_tab_label);

    apptw->Recur_feature_label = gtk_label_new(_("Complexity"));
    apptw->Recur_feature_hbox = gtk_hbox_new(FALSE, 0);
    apptw->Recur_feature_normal_rb = gtk_radio_button_new_with_label(NULL
            , _("Basic"));
    gtk_box_pack_start(GTK_BOX(apptw->Recur_feature_hbox)
            , apptw->Recur_feature_normal_rb, FALSE, FALSE, 15);
    apptw->Recur_feature_advanced_rb = 
            gtk_radio_button_new_with_mnemonic_from_widget(
                    GTK_RADIO_BUTTON(apptw->Recur_feature_normal_rb)
            , _("Advanced"));
    gtk_box_pack_start(GTK_BOX(apptw->Recur_feature_hbox)
            , apptw->Recur_feature_advanced_rb, FALSE, FALSE, 15);
    orage_table_add_row(apptw->TableRecur
            , apptw->Recur_feature_label, apptw->Recur_feature_hbox
            , row = 0, (GTK_EXPAND | GTK_FILL), (0));
    gtk_tooltips_set_tip(apptw->Tooltips, apptw->Recur_feature_normal_rb
            , _("Use this if you want regular repeating event"), NULL);
    gtk_tooltips_set_tip(apptw->Tooltips, apptw->Recur_feature_advanced_rb
            , _("Use this if you need complex times like:\n Every Saturday and Sunday or \n First Tuesday every month")
            , NULL);

    apptw->Recur_freq_label = gtk_label_new(_("Frequency"));
    apptw->Recur_freq_hbox = gtk_hbox_new(FALSE, 0);
    apptw->Recur_freq_cb = gtk_combo_box_new_text();
    combo_box_append_array(apptw->Recur_freq_cb
            , recur_freq_array, RECUR_ARRAY_DIM);
    gtk_box_pack_start(GTK_BOX(apptw->Recur_freq_hbox)
            , apptw->Recur_freq_cb, FALSE, FALSE, 15);
    apptw->Recur_int_spin_label1 = gtk_label_new(_("Each"));
    gtk_box_pack_start(GTK_BOX(apptw->Recur_freq_hbox)
            , apptw->Recur_int_spin_label1, FALSE, FALSE, 0);
    apptw->Recur_int_spin = gtk_spin_button_new_with_range(1, 100, 1);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(apptw->Recur_int_spin), TRUE);
    gtk_box_pack_start(GTK_BOX(apptw->Recur_freq_hbox)
            , apptw->Recur_int_spin, FALSE, FALSE, 5);
    apptw->Recur_int_spin_label2 = gtk_label_new(_("occurrence"));
    gtk_box_pack_start(GTK_BOX(apptw->Recur_freq_hbox)
            , apptw->Recur_int_spin_label2, FALSE, FALSE, 0);
    gtk_tooltips_set_tip(apptw->Tooltips, apptw->Recur_int_spin
            , _("Limit frequency to certain interval.\n For example: Every third day:\n Frequency = Daily and Interval = 3")
            , NULL);
    orage_table_add_row(apptw->TableRecur
            , apptw->Recur_freq_label, apptw->Recur_freq_hbox
            , ++row, (GTK_EXPAND | GTK_FILL), (GTK_FILL));

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

    g_signal_connect((gpointer)apptw->Recur_feature_normal_rb, "clicked"
            , G_CALLBACK(app_recur_feature_checkbutton_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->Recur_feature_advanced_rb, "clicked"
            , G_CALLBACK(app_recur_feature_checkbutton_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->Recur_freq_cb, "changed"
            , G_CALLBACK(on_freq_combobox_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->Recur_int_spin, "value-changed"
            , G_CALLBACK(on_app_spin_button_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->Recur_limit_rb, "clicked"
            , G_CALLBACK(app_recur_checkbutton_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->Recur_count_rb, "clicked"
            , G_CALLBACK(app_recur_checkbutton_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->Recur_count_spin, "value-changed"
            , G_CALLBACK(on_app_spin_button_changed_cb), apptw);
    g_signal_connect((gpointer)apptw->Recur_until_rb, "clicked"
            , G_CALLBACK(app_recur_checkbutton_clicked_cb), apptw);
    g_signal_connect((gpointer)apptw->Recur_until_button, "clicked"
            , G_CALLBACK(on_Date_button_clicked_cb), apptw);
    for (i=0; i <= 6; i++) {
        g_signal_connect((gpointer)apptw->Recur_byday_cb[i], "clicked"
                , G_CALLBACK(app_checkbutton_clicked_cb), apptw);
        g_signal_connect((gpointer)apptw->Recur_byday_spin[i], "value-changed"
                , G_CALLBACK(on_app_spin_button_changed_cb), apptw);
    }
}

appt_win *create_appt_win(char *action, char *par, el_win *event_list)
{
    appt_win *apptw;

    /*  initialisation + main window + base vbox */
    apptw = g_new(appt_win, 1);
    apptw->xf_uid = NULL;
    apptw->par = NULL;
    apptw->appt = NULL;
    apptw->el = event_list;    /* Keep track of the parent, if any */
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
    if (apptw->appt) { /* all fine */
        gtk_widget_show_all(apptw->Window);
        recur_hide_show(apptw);
        type_hide_show(apptw);
        gtk_widget_grab_focus(apptw->Title_entry);
    }
    else { /* failed to get data */
        app_free_memory(apptw);
        /*
        gtk_widget_destroy(apptw->Window);
        gtk_object_destroy(GTK_OBJECT(apptw->Tooltips));
        g_free(apptw);
        */
    }

    return apptw;
}
