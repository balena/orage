/* appointment.c
 *
 * Copyright (C) 2004-2005 Mickaël Graf <korbinus at xfce.org>
 * Copyright (C) 2005 Juha Kautto <juha at xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
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
#include <libxfce4mcs/mcs-client.h>

#include "mainbox.h"
#include "event-list.h"
#include "appointment.h"
#include "ical-code.h"
#include "functions.h"

#define AVAILABILITY_ARRAY_DIM 2
#define RECUR_ARRAY_DIM 5
#define ALARM_ARRAY_DIM 11
#define FILETYPE_SIZE 38

extern char *local_icaltimezone_location;
extern gboolean local_icaltimezone_utc;

enum {
    LOCATION,
    LOCATION_ENG,
    N_COLUMNS
};

gboolean 
ical_to_year_month_day_hour_minute(char *ical
    , int *year, int *month, int *day, int *hour, int *minute)
{
    int i;

    i = sscanf(ical, "%04d%02d%02dT%02d%02d", year, month, day, hour, minute);
    switch (i) {
        case 3: /* date */
            *hour = -1;
            *minute = -1;
            break;
        case 5: /* time */
            break;
        default: /* error */
            g_warning("ical_to_year_month_day_hour_minute: ical error %s %d"
                    , ical, i);
            return FALSE;
            break;
    }
    return TRUE;
}

void 
year_month_day_to_display(int year, int month, int day , char *display)
{
    const char *date_format;
    struct tm d = {0,0,0,0,0,0,0,0,0};

    date_format = _("%m/%d/%Y");
    d.tm_mday = day;
    d.tm_mon = month - 1;
    d.tm_year = year - 1900;

    strftime(display, 11, date_format, &d);
}

void
mark_appointment_changed(appt_win *apptw)
{
    if (!apptw->appointment_changed) {
        apptw->appointment_changed = TRUE;
        gtk_widget_set_sensitive(apptw->appRevert, TRUE);
    }
}

void
mark_appointment_unchanged(appt_win *apptw)
{
    if (apptw->appointment_changed) {
        apptw->appointment_changed = FALSE;
        gtk_widget_set_sensitive(apptw->appRevert, FALSE);
    }
}

void
set_time_sensitivity(appt_win *apptw)
{
    gboolean dur_act;
    gboolean allDay_act;

    dur_act = gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(apptw->appDur_checkbutton));
    allDay_act = gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(apptw->appAllDay_checkbutton));

    if (allDay_act) {
        gtk_widget_set_sensitive(apptw->appStartTime_spin_hh, FALSE);
        gtk_widget_set_sensitive(apptw->appStartTime_spin_mm, FALSE);
        gtk_widget_set_sensitive(apptw->appStartTimezone_button, FALSE);
        gtk_widget_set_sensitive(apptw->appEndTime_spin_hh, FALSE);
        gtk_widget_set_sensitive(apptw->appEndTime_spin_mm, FALSE);
        gtk_widget_set_sensitive(apptw->appEndTimezone_button, FALSE);
        gtk_widget_set_sensitive(apptw->appDur_spin_hh, FALSE);
        gtk_widget_set_sensitive(apptw->appDur_spin_mm, FALSE);
        if (dur_act) {
            gtk_widget_set_sensitive(apptw->appEndDate_button, FALSE);
            gtk_widget_set_sensitive(apptw->appDur_spin_dd, TRUE);
        }
        else {
            gtk_widget_set_sensitive(apptw->appEndDate_button, TRUE);
            gtk_widget_set_sensitive(apptw->appDur_spin_dd, FALSE);
        }
    }
    else {
        gtk_widget_set_sensitive(apptw->appStartTime_spin_hh, TRUE);
        gtk_widget_set_sensitive(apptw->appStartTime_spin_mm, TRUE);
        gtk_widget_set_sensitive(apptw->appStartTimezone_button, TRUE);
        if (dur_act) {
            gtk_widget_set_sensitive(apptw->appEndDate_button, FALSE);
            gtk_widget_set_sensitive(apptw->appEndTime_spin_hh, FALSE);
            gtk_widget_set_sensitive(apptw->appEndTime_spin_mm, FALSE);
            gtk_widget_set_sensitive(apptw->appEndTimezone_button, FALSE);
            gtk_widget_set_sensitive(apptw->appDur_spin_dd, TRUE);
            gtk_widget_set_sensitive(apptw->appDur_spin_hh, TRUE);
            gtk_widget_set_sensitive(apptw->appDur_spin_mm, TRUE);
        }
        else {
            gtk_widget_set_sensitive(apptw->appEndDate_button, TRUE);
            gtk_widget_set_sensitive(apptw->appEndTime_spin_hh, TRUE);
            gtk_widget_set_sensitive(apptw->appEndTime_spin_mm, TRUE);
            gtk_widget_set_sensitive(apptw->appEndTimezone_button, TRUE);
            gtk_widget_set_sensitive(apptw->appDur_spin_dd, FALSE);
            gtk_widget_set_sensitive(apptw->appDur_spin_hh, FALSE);
            gtk_widget_set_sensitive(apptw->appDur_spin_mm, FALSE);
        }
    }
}

void
set_repeat_sensitivity(appt_win *apptw)
{
    gint freq;
    
    freq = gtk_combo_box_get_active((GtkComboBox *)apptw->appRecur_cb);
    if (freq == XFICAL_FREQ_NONE) {
        gtk_widget_set_sensitive(apptw->appRecur_repeat_rb, FALSE);
        gtk_widget_set_sensitive(apptw->appRecur_count_rb, FALSE);
        gtk_widget_set_sensitive(apptw->appRecur_count_spin, FALSE);
        gtk_widget_set_sensitive(apptw->appRecur_until_rb, FALSE);
        gtk_widget_set_sensitive(apptw->appRecur_until_button, FALSE);
    }
    else {
        gtk_widget_set_sensitive(apptw->appRecur_repeat_rb, TRUE);
        gtk_widget_set_sensitive(apptw->appRecur_count_rb, TRUE);
        gtk_widget_set_sensitive(apptw->appRecur_count_spin, TRUE);
        gtk_widget_set_sensitive(apptw->appRecur_until_rb, TRUE);
        gtk_widget_set_sensitive(apptw->appRecur_until_button, TRUE);
    }
}

void
on_appTitle_entry_changed_cb(GtkEditable *entry, gpointer user_data)
{
    gchar *title, *application_name;
    const gchar *appointment_name;
    appt_win *apptw = (appt_win *)user_data;

    appointment_name = gtk_entry_get_text((GtkEntry *)apptw->appTitle_entry);
    application_name = _("Orage");

    if(strlen((char *)appointment_name) > 0)
        title = g_strdup_printf("%s - %s", appointment_name, application_name);
    else
        title = g_strdup_printf("%s", application_name);

    gtk_window_set_title(GTK_WINDOW(apptw->appWindow), (const gchar *)title);

    g_free(title);
    mark_appointment_changed((appt_win *)user_data);
}

void
appDur_checkbutton_clicked_cb(GtkCheckButton *cb, gpointer user_data)
{
    set_time_sensitivity((appt_win *)user_data);
    mark_appointment_changed((appt_win *)user_data);
}

void
app_checkbutton_clicked_cb(GtkCheckButton *checkbutton, gpointer user_data)
{
    mark_appointment_changed((appt_win *)user_data);
}

void
on_appNote_buffer_changed_cb(GtkTextBuffer *buffer, gpointer user_data)
{
    mark_appointment_changed((appt_win *)user_data);
}

void
on_app_entry_changed_cb(GtkEditable *entry, gpointer user_data)
{
    mark_appointment_changed((appt_win *)user_data);
}

void
on_app_combobox_changed_cb(GtkComboBox *cb, gpointer user_data)
{
    set_repeat_sensitivity((appt_win *)user_data);
    mark_appointment_changed((appt_win *)user_data);
}

void
on_app_spin_button_changed_cb(GtkSpinButton *cb, gpointer user_data)
{
    mark_appointment_changed((appt_win *)user_data);
}

void
on_appSound_button_clicked_cb(GtkButton *button, gpointer user_data)
{
    appt_win *apptw = (appt_win *)user_data;
    GtkWidget *file_chooser;
    XfceFileFilter *filter;

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

    appSound_entry_filename = g_strdup(gtk_entry_get_text((GtkEntry *)apptw->appSound_entry));

    file_chooser = xfce_file_chooser_new(_("Select a file..."),
                                        GTK_WINDOW (apptw->appWindow),
                                        XFCE_FILE_CHOOSER_ACTION_OPEN,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                        NULL);

    filter = xfce_file_filter_new();
    xfce_file_filter_set_name(filter, _("Sound Files"));
    for (i = 0; i < FILETYPE_SIZE; i++){
        xfce_file_filter_add_pattern(filter, filetype[i]);
    }
    xfce_file_chooser_add_filter(XFCE_FILE_CHOOSER(file_chooser), filter);
    filter = xfce_file_filter_new();
    xfce_file_filter_set_name(filter, _("All Files"));
    xfce_file_filter_add_pattern(filter, "*");
    xfce_file_chooser_add_filter(XFCE_FILE_CHOOSER(file_chooser), filter);

    xfce_file_chooser_add_shortcut_folder(XFCE_FILE_CHOOSER(file_chooser)
            , PACKAGE_DATA_DIR "/orage/sounds", NULL);

    if(strlen(appSound_entry_filename) > 0){
        xfce_file_chooser_set_filename(XFCE_FILE_CHOOSER(file_chooser), 
                                   (const gchar *) appSound_entry_filename);
    }
    else
        xfce_file_chooser_set_current_folder(XFCE_FILE_CHOOSER(file_chooser)
            , PACKAGE_DATA_DIR "/orage/sounds");

    if(gtk_dialog_run(GTK_DIALOG(file_chooser)) == GTK_RESPONSE_ACCEPT) {
        sound_file = xfce_file_chooser_get_filename(XFCE_FILE_CHOOSER(file_chooser));
        if(sound_file){
            gtk_entry_set_text(GTK_ENTRY(apptw->appSound_entry), sound_file);
            gtk_editable_set_position(GTK_EDITABLE(apptw->appSound_entry), -1);
        }
    }

    gtk_widget_destroy(file_chooser);
    g_free(appSound_entry_filename);
}

void
on_appAllDay_clicked_cb(GtkCheckButton *checkbutton, gpointer user_data)
{
    set_time_sensitivity((appt_win *)user_data);
    mark_appointment_changed((appt_win *)user_data);
}

gboolean
appWindow_check_and_close(appt_win *apptw)
{
    gint result;

    if(apptw->appointment_changed == TRUE){
        result = xfce_message_dialog(GTK_WINDOW(apptw->appWindow),
                 _("Warning"),
                 GTK_STOCK_DIALOG_WARNING,
                 _("The appointment information has been modified."),
                 _("Do you want to continue?"),
                 GTK_STOCK_YES, GTK_RESPONSE_ACCEPT,
                 GTK_STOCK_NO, GTK_RESPONSE_CANCEL,
                 NULL);

        if(result == GTK_RESPONSE_ACCEPT){
            gtk_widget_destroy(apptw->appWindow);
            g_free(apptw);
        }
    }
    else{
        gtk_widget_destroy(apptw->appWindow);
        gtk_object_destroy(GTK_OBJECT(apptw->appTooltips));
        g_free(apptw);
    }
    return TRUE;
}

gboolean
on_appWindow_delete_event_cb(GtkWidget *widget, GdkEvent *event
    , gpointer user_data)
{
    return appWindow_check_and_close((appt_win *) user_data);
}

gboolean 
orage_validate_datetime(appt_win *apptw, appt_data *appt)
{
    gint result;

    if (xfical_compare_times(appt) > 0) {
        result = xfce_message_dialog(GTK_WINDOW(apptw->appWindow),
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
        fill_appt_window_times(apptw, appt);
        return TRUE;
    }
}

/*
 * fill_appt
 * This function fills an appointment with the content of an appointment window
 */
gboolean
fill_appt(appt_data *appt, appt_win *apptw)
{
    GtkTextIter start, end;
    const char *date_format, *time_format;
    struct tm current_t;
    char *returned_by_strptime;
    gchar starttime[6], endtime[6];

    /*Get the title */
    appt->title = (gchar *) gtk_entry_get_text((GtkEntry *)apptw->appTitle_entry);
    /* Get the location */
    appt->location = (gchar *) gtk_entry_get_text((GtkEntry *)apptw->appLocation_entry);

    /* Get if the appointment is for the all day */
    appt->allDay = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(apptw->appAllDay_checkbutton));

    /* Get the start date and time and timezone */
    /* FIXME: use year_month_day_to_display() instead of doing it here */
    date_format = _("%m/%d/%Y");
    time_format = "%H:%M";

    current_t.tm_hour = 0;
    current_t.tm_min = 0;

    returned_by_strptime = strptime(gtk_button_get_label(GTK_BUTTON(apptw->appStartDate_button)), date_format, &current_t);
    g_sprintf(starttime, "%02d:%02d"
            , gtk_spin_button_get_value_as_int(
                    GTK_SPIN_BUTTON(apptw->appStartTime_spin_hh))
            , gtk_spin_button_get_value_as_int(
                    GTK_SPIN_BUTTON(apptw->appStartTime_spin_mm)));
    returned_by_strptime = strptime(starttime, time_format, &current_t);
    g_sprintf(appt->starttime, XFICAL_APPT_TIME_FORMAT
            , current_t.tm_year + 1900, current_t.tm_mon + 1, current_t.tm_mday
            , current_t.tm_hour, current_t.tm_min, 0);
    appt->start_tz_loc = g_object_get_data(G_OBJECT(apptw->appStartTimezone_button), "LOCATION_ENG");

    /* Get the end date and time and timezone */
    current_t.tm_hour = 0;
    current_t.tm_min = 0;

    returned_by_strptime = strptime(gtk_button_get_label(GTK_BUTTON(apptw->appEndDate_button)), date_format, &current_t);
    g_sprintf(endtime, "%02d:%02d"
            , gtk_spin_button_get_value_as_int(
                    GTK_SPIN_BUTTON(apptw->appEndTime_spin_hh))
            , gtk_spin_button_get_value_as_int(
                    GTK_SPIN_BUTTON(apptw->appEndTime_spin_mm)));
    returned_by_strptime = strptime(endtime, time_format, &current_t);
    g_sprintf(appt->endtime, XFICAL_APPT_TIME_FORMAT
            , current_t.tm_year + 1900, current_t.tm_mon + 1, current_t.tm_mday
            , current_t.tm_hour, current_t.tm_min, 0);
    appt->end_tz_loc = g_object_get_data(G_OBJECT(apptw->appEndTimezone_button), "LOCATION_ENG");

    /* Get the duration */
     appt->use_duration = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(apptw->appDur_checkbutton));
    appt->duration = gtk_spin_button_get_value_as_int(
            GTK_SPIN_BUTTON(apptw->appDur_spin_dd)) * 24*60*60
                    + gtk_spin_button_get_value_as_int(
            GTK_SPIN_BUTTON(apptw->appDur_spin_hh)) *    60*60
                    + gtk_spin_button_get_value_as_int(
            GTK_SPIN_BUTTON(apptw->appDur_spin_mm)) *       60
                    ;

    if(!orage_validate_datetime(apptw, appt))
        return(FALSE);

    /* Get the availability */
    appt->availability = gtk_combo_box_get_active((GtkComboBox *)apptw->appAvailability_cb);

    /* Get the notes */
    gtk_text_buffer_get_bounds(gtk_text_view_get_buffer((GtkTextView *)apptw->appNote_textview), 
                                &start, &end);
    appt->note = gtk_text_iter_get_text(&start, &end);

    /* Get when the reminder will show up */
    appt->alarmtime = gtk_spin_button_get_value_as_int(
            GTK_SPIN_BUTTON(apptw->appAlarm_spin_dd)) * 24*60*60
                    + gtk_spin_button_get_value_as_int(
            GTK_SPIN_BUTTON(apptw->appAlarm_spin_hh)) *    60*60
                    + gtk_spin_button_get_value_as_int(
            GTK_SPIN_BUTTON(apptw->appAlarm_spin_mm)) *       60
                    ;

    /* Which sound file will be played */
    appt->sound = (gchar *) gtk_entry_get_text((GtkEntry *)apptw->appSound_entry);

    /* Get if the alarm will repeat until someone shuts it off */
    appt->alarmrepeat = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(apptw->appSoundRepeat_checkbutton));

    /* Get the recurrence */
    appt->freq = gtk_combo_box_get_active((GtkComboBox *)apptw->appRecur_cb);

    /* Get the recurrence ending */
    if (gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(apptw->appRecur_repeat_rb))) {
        appt->recur_limit = 0;    /* no limit */
        appt->recur_count = 0;    /* special: means no repeat count limit */
        appt->recur_until[0] = 0; /* special: means no until time limit */
    }
    else if (gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(apptw->appRecur_count_rb))) {
        appt->recur_limit = 1;    /* count limit */
        appt->recur_count = gtk_spin_button_get_value_as_int(
                    GTK_SPIN_BUTTON(apptw->appRecur_count_spin));
        appt->recur_until[0] = 0; /* special: means no until time limit */
    }
    else if (gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(apptw->appRecur_until_rb))) {
        appt->recur_limit = 2;    /* until limit */
        appt->recur_count = 0;    /* special: means no repeat count limit */

        returned_by_strptime = strptime(gtk_button_get_label(GTK_BUTTON(apptw->appRecur_until_button)), date_format, &current_t);
        returned_by_strptime = strptime(starttime, time_format, &current_t);
        g_sprintf(appt->recur_until, XFICAL_APPT_TIME_FORMAT
            , current_t.tm_year + 1900, current_t.tm_mon + 1, current_t.tm_mday
            , 0, 0, 0);
    }
    else
        g_warning("fill_appt: coding error, illegal recurrence");

    return(TRUE);
}

void
on_appFileClose_menu_activate_cb(GtkMenuItem *menuitem, gpointer user_data)
{
    appWindow_check_and_close((appt_win *) user_data);
}

gboolean 
save_xfical_from_appt_win(appt_win *apptw)
{
    gboolean ok = FALSE;
    appt_data *appt = xfical_appt_alloc();

    if (fill_appt(appt, apptw)) {
        /* Here we try to save the event... */
        if (xfical_file_open()) {
            if (apptw->add_appointment) {
                apptw->xf_uid = g_strdup(xfical_appt_add(appt));
                ok = (apptw->xf_uid ? TRUE : FALSE);
                if (ok) {
                    apptw->add_appointment = FALSE;
                    gtk_widget_set_sensitive(apptw->appDuplicate, TRUE);
                    g_message("Added: %s", apptw->xf_uid);
                }
                else
                    g_warning("Addition failed: %s", apptw->xf_uid);
            }
            else {
                ok = xfical_appt_mod(apptw->xf_uid, appt);
                if (ok)
                    g_message("Modified: %s", apptw->xf_uid);
                else
                    g_warning("Modification failed: %s", apptw->xf_uid);
            }
            xfical_file_close();
        }

        if (ok) {
            apptw->appointment_new = FALSE;
            mark_appointment_unchanged(apptw);
            if (apptw->eventlist != NULL)
                recreate_eventlist_win((eventlist_win *)apptw->eventlist);
        }
    }

    g_free(appt->note);
    g_free(appt);
    return (ok);
}

void
on_appFileSave_menu_activate_cb(GtkMenuItem *menuitem, gpointer user_data)
{
    save_xfical_from_appt_win((appt_win *)user_data);
}

void
on_appSave_clicked_cb(GtkButton *button, gpointer user_data)
{
    save_xfical_from_appt_win((appt_win *)user_data);
}

void
save_xfical_from_appt_win_and_close(appt_win *apptw)
{
    if (save_xfical_from_appt_win(apptw)) {
        gtk_widget_destroy(apptw->appWindow);
        g_free(apptw);
    }
}

void 
on_appFileSaveClose_menu_activate_cb(GtkMenuItem *menuitem, gpointer user_data)
{
    save_xfical_from_appt_win_and_close((appt_win *)user_data);
}

void
on_appSaveClose_clicked_cb(GtkButton *button, gpointer user_data)
{
    save_xfical_from_appt_win_and_close((appt_win *)user_data);
}

void
delete_xfical_from_appt_win(appt_win *apptw)
{
    gint result;

    result = xfce_message_dialog(GTK_WINDOW(apptw->appWindow),
             _("Warning"),
             GTK_STOCK_DIALOG_WARNING,
             _("This appointment will be permanently removed."),
             _("Do you want to continue?"),
             GTK_STOCK_YES,
             GTK_RESPONSE_ACCEPT,
             GTK_STOCK_NO,
             GTK_RESPONSE_REJECT,
             NULL);
                                 
    if (result == GTK_RESPONSE_ACCEPT) {
        if (!apptw->add_appointment)
            if (xfical_file_open()) {
                result = xfical_appt_del(apptw->xf_uid);
                xfical_file_close();
                if (result)
                    g_message("Removed: %s", apptw->xf_uid);
                else
                    g_warning("Removal failed: %s", apptw->xf_uid);
            }

        if (apptw->eventlist != NULL)
            recreate_eventlist_win((eventlist_win *)apptw->eventlist);

        gtk_widget_destroy(apptw->appWindow);
        g_free(apptw);
    }
}

void
on_appDelete_clicked_cb(GtkButton *button, gpointer user_data)
{
    delete_xfical_from_appt_win((appt_win *)user_data);
}

void
on_appFileDelete_menu_activate_cb(GtkMenuItem *menuitem, gpointer user_data)
{
    delete_xfical_from_appt_win((appt_win *) user_data);
}

void 
duplicate_xfical_from_appt_win(appt_win *apptw)
{
    gint x, y;
    appt_win *apptw2;

    apptw2 = create_appt_win("COPY", apptw->xf_uid, apptw->eventlist);
    gtk_window_get_position(GTK_WINDOW(apptw->appWindow), &x, &y);
    gtk_window_move(GTK_WINDOW(apptw2->appWindow), x+20, y+20);
}

void
on_appFileDuplicate_menu_activate_cb(GtkMenuItem *menuitem, gpointer user_data)
{
    duplicate_xfical_from_appt_win((appt_win *)user_data);
}

void
on_appDuplicate_clicked_cb(GtkButton *button, gpointer user_data)
{
    duplicate_xfical_from_appt_win((appt_win *)user_data);
}

void
revert_xfical_to_last_saved(appt_win *apptw)
{
    if(!apptw->appointment_new){
        fill_appt_window(apptw, "UPDATE", apptw->xf_uid);
    }
    else{
        fill_appt_window(apptw, "NEW", apptw->chosen_date);
    }
}

void
on_appFileRevert_menu_activate_cb(GtkMenuItem *menuitem, gpointer user_data)
{
    revert_xfical_to_last_saved((appt_win *) user_data);
}

void
on_appRevert_clicked_cb(GtkWidget *button, gpointer *user_data)
{
    revert_xfical_to_last_saved((appt_win *) user_data);
}

void
on_appStartEndDate_clicked_cb(GtkWidget *button, gpointer *user_data)
{
    appt_win *apptw = (appt_win *)user_data;
    GtkWidget *selDate_Window_dialog;
    GtkWidget *selDate_Calendar_calendar;
    gint result;
    guint year, month, day;
    const char *date_format;
    char date_to_display[11]; 
    struct tm *t;
    struct tm current_t;
    char *returned_by_strptime;
    time_t tt;

    date_format = _("%m/%d/%Y");
    returned_by_strptime = strptime(gtk_button_get_label(GTK_BUTTON(button))
                , date_format, &current_t);

    selDate_Window_dialog = gtk_dialog_new_with_buttons(
                            _("Pick the date"), GTK_WINDOW(apptw->appWindow),
                            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                            _("Today"), 
                            1,
                            GTK_STOCK_OK,
                            GTK_RESPONSE_ACCEPT,
                            NULL);

    selDate_Calendar_calendar = gtk_calendar_new();
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(selDate_Window_dialog)->vbox)
                , selDate_Calendar_calendar);
    xfcalendar_select_date (GTK_CALENDAR (selDate_Calendar_calendar)
                , current_t.tm_year+1900
                , current_t.tm_mon, current_t.tm_mday);
    gtk_widget_show_all(selDate_Window_dialog);

    result = gtk_dialog_run(GTK_DIALOG(selDate_Window_dialog));
    switch(result){
        case GTK_RESPONSE_ACCEPT:
            gtk_calendar_get_date(GTK_CALENDAR(selDate_Calendar_calendar)
                    , &year, &month, &day);
            year_month_day_to_display(year, month + 1, day, date_to_display);
            break;
        case 1:
            tt=time(NULL);
            t=localtime(&tt);
            year_month_day_to_display(t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, date_to_display);
            break;
        case GTK_RESPONSE_DELETE_EVENT:
            g_strlcpy(date_to_display, gtk_button_get_label(GTK_BUTTON(button))
                    ,11);
            break;
    }
    if (g_ascii_strcasecmp((gchar *)date_to_display
            , (gchar *)gtk_button_get_label(GTK_BUTTON(button))) != 0) {
        mark_appointment_changed(apptw);
    }
    gtk_button_set_label(GTK_BUTTON(button), (const gchar *)date_to_display);
    gtk_widget_destroy(selDate_Window_dialog);
}

void
on_appStartEndTimezone_clicked_cb(GtkWidget *button, gpointer *user_data)
{
#define MAX_AREA_LENGTH 20
    appt_win *apptw = (appt_win *)user_data;
    GtkTreeStore *store;
    GtkTreeIter iter1, iter2;
    GtkWidget *tree;
    GtkCellRenderer *rend;
    GtkTreeViewColumn *col;
    GtkWidget *window;
    GtkWidget *sw;
    xfical_timezone_array tz;
    int i, j, result;
    char area_old[MAX_AREA_LENGTH], *loc, *loc_eng, *loc_int;
    GtkTreeSelection *sel;
    GtkTreeModel     *model;
    GtkTreeIter       iter;

    /* enter data */
    store = gtk_tree_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);
    strcpy(area_old, "S T a R T");
    tz = xfical_get_timezones();
    for (i=0; i < tz.count-2; i++) {
        /* first area */
        if (! g_str_has_prefix(tz.city[i], area_old)) {
            for (j=0; tz.city[i][j] != '/' && j < MAX_AREA_LENGTH; j++) {
                area_old[j] = tz.city[i][j];
            }
            if (j < MAX_AREA_LENGTH)
                area_old[j] = 0;
            else
                g_warning("on_appStartEndTimezone_clicked_cb: too long line in zones.tab %s", tz.city[i]);

            gtk_tree_store_append(store, &iter1, NULL);
            gtk_tree_store_set(store, &iter1
                    , LOCATION, _(area_old)
                    , LOCATION_ENG, area_old
                    , -1);
        }
        /* then city translated and in base form used internally */
        /*
        g_print("TREE store: %s %s\n", _(tz.city[i]), tz.city[i]);
        */
        gtk_tree_store_append(store, &iter2, &iter1);
        gtk_tree_store_set(store, &iter2
                , LOCATION, _(tz.city[i]) 
                , LOCATION_ENG, tz.city[i] 
                , -1);
    }
         
    /* create view */
    tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    rend = gtk_cell_renderer_text_new();
    col  = gtk_tree_view_column_new_with_attributes(_("Location")
                , rend, "text", LOCATION, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);

    rend = gtk_cell_renderer_text_new();
    col  = gtk_tree_view_column_new_with_attributes(_("Location")
                , rend, "text", LOCATION_ENG, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
    gtk_tree_view_column_set_visible(col, FALSE);

    /* show it */
    window =  gtk_dialog_new_with_buttons(_("Pick timezone")
            , GTK_WINDOW(apptw->appWindow)
            , GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT
            , _("UTC"), 1
            , _("floating"), 2
            , GTK_STOCK_OK, GTK_RESPONSE_ACCEPT
            , NULL);
    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(sw), tree);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), sw, TRUE, TRUE, 0);
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 500);

    gtk_widget_show_all(window);
    do {
        result = gtk_dialog_run(GTK_DIALOG(window));
        switch (result) {
            case GTK_RESPONSE_ACCEPT:
                sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
                if (gtk_tree_selection_get_selected(sel, &model, &iter))
                    if (gtk_tree_model_iter_has_child(model, &iter))
                        result = 0;
                    else {
                        gtk_tree_model_get(model, &iter, LOCATION, &loc, -1);
                        gtk_tree_model_get(model, &iter, LOCATION_ENG, &loc_eng, -1);
                    }
                else {
                    loc = g_strdup(gtk_button_get_label(GTK_BUTTON(button)));
                    loc_eng = g_object_get_data(G_OBJECT(button), "LOCATION_ENG");
                }
                break;
            case 1:
                loc = g_strdup(_("UTC"));
                loc_eng = g_strdup("UTC");
                break;
            case 2:
                loc = g_strdup(_("floating"));
                loc_eng = g_strdup("floating");
                break;
            default:
                loc = g_strdup(gtk_button_get_label(GTK_BUTTON(button)));
                loc_eng = g_object_get_data(G_OBJECT(button), "LOCATION_ENG");
                break;
        }
    } while (result == 0) ;
    if (g_ascii_strcasecmp((gchar *)loc
            , (gchar *)gtk_button_get_label(GTK_BUTTON(button))) != 0) {
        mark_appointment_changed(apptw);
    }
    gtk_button_set_label(GTK_BUTTON(button), loc);
    if (loc_int = g_object_get_data(G_OBJECT(button), "LOCATION_ENG"))
        g_free(loc_int);
    loc_int = g_strdup(loc_eng);
    g_object_set_data(G_OBJECT(button), "LOCATION_ENG", loc_int);
    g_free(loc);
    g_free(loc_eng);
    gtk_widget_destroy(window);
}

void
fill_appt_window_times(appt_win *apptw, appt_data *appt)
{
    char startdate_to_display[11], enddate_to_display[11];
    int year, month, day, hours, minutes;
    gchar *s_tz, *e_tz, *s_tze, *e_tze;

    /* remember that appt->start_tz_loc points to gtl internal button label
     * so we need to take care it does not get corrupted when we change it.
     * and end may actually point to the same place. 
     */
    if (appt->start_tz_loc) {
        s_tze = g_strdup(appt->start_tz_loc);
        s_tz  = g_strdup(_(appt->start_tz_loc));
    }
    else { /* null = local = no timezone = floating */
        s_tze = g_strdup("floating");
        s_tz  = g_strdup(_("floating"));
    }
    if (appt->end_tz_loc) {
        e_tze = g_strdup(appt->end_tz_loc);
        e_tz  = g_strdup(_(appt->end_tz_loc));
    }
    else { /* null = local = no timezone = floating */
        e_tze = g_strdup("floating");
        e_tz  = g_strdup(_("floating"));
    }

    /* all day ? */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(apptw->appAllDay_checkbutton), appt->allDay);

    /* start time */
    if (strlen(appt->starttime) > 6 ) {
        ical_to_year_month_day_hour_minute(appt->starttime, &year, &month, &day, &hours, &minutes);
        year_month_day_to_display(year, month, day, startdate_to_display);
        gtk_button_set_label(GTK_BUTTON(apptw->appStartDate_button), (const gchar *)startdate_to_display);

        if(hours > -1 && minutes > -1){
            gtk_spin_button_set_value(
                GTK_SPIN_BUTTON(apptw->appStartTime_spin_hh), (gdouble)hours);
            gtk_spin_button_set_value(
                GTK_SPIN_BUTTON(apptw->appStartTime_spin_mm), (gdouble)minutes);
        }
        if (s_tz) {
            gtk_button_set_label(GTK_BUTTON(apptw->appStartTimezone_button), s_tz);
            g_object_set_data(G_OBJECT(apptw->appStartTimezone_button), "LOCATION_ENG", s_tze);
            /* FIXED Memory error
            g_free(appt->start_tz_loc);
            */
            appt->start_tz_loc = s_tze;
        }
        else /* we should never get here */
            g_warning("fill_appt_window_times: s_tz is null");
    }
    else
        g_warning("fill_appt_window_times: starttime wrong %s", appt->uid);

    /* end time */
    if (strlen(appt->endtime) > 6 ) {
        ical_to_year_month_day_hour_minute(appt->endtime, &year, &month, &day, &hours, &minutes);

        year_month_day_to_display(year, month, day, enddate_to_display);
        gtk_button_set_label(GTK_BUTTON(apptw->appEndDate_button), (const gchar *)enddate_to_display);

        if(hours > -1 && minutes > -1){
            gtk_spin_button_set_value(
                GTK_SPIN_BUTTON(apptw->appEndTime_spin_hh), (gdouble)hours);
            gtk_spin_button_set_value(
                GTK_SPIN_BUTTON(apptw->appEndTime_spin_mm), (gdouble)minutes);
        }
        if (e_tz) {
            gtk_button_set_label(GTK_BUTTON(apptw->appEndTimezone_button), e_tz);
            g_object_set_data(G_OBJECT(apptw->appEndTimezone_button), "LOCATION_ENG", e_tze);
            /* FIXED Memory error
            g_free(appt->end_tz_loc);
            */
            appt->end_tz_loc = e_tze;
        }
        else /* we should never get here */
            g_warning("fill_appt_window_times: e_tz is null");
    }
    else
        g_warning("fill_appt_window_times: endtime wrong %s", appt->uid);

    g_free(s_tz);
    g_free(e_tz);

    /* duration */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(apptw->appDur_checkbutton), appt->use_duration);
    day = appt->duration/(24*60*60);
    hours = (appt->duration-day*(24*60*60))/(60*60);
    minutes = (appt->duration-day*(24*60*60)-hours*(60*60))/(60);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(apptw->appDur_spin_dd)
                , (gdouble)day);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(apptw->appDur_spin_hh)
                , (gdouble)hours);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(apptw->appDur_spin_mm)
                , (gdouble)minutes);
}

void
fill_appt_window(appt_win *apptw, char *action, char *par)
{
    int year, month, day, hours, minutes;
    GtkTextBuffer *tb;
    appt_data *appt=NULL;
    struct tm *t;
    time_t tt;
    gchar today[9];
    char untildate_to_display[11];

    g_message("%s appointment: %s", action, par);
    if (strcmp(action, "NEW") == 0) {
  /* par contains XFICAL_APPT_DATE_FORMAT (yyyymmdd) date for NEW appointment */
        apptw->add_appointment = TRUE;
        apptw->appointment_new = TRUE;
        apptw->chosen_date = g_strdup((const gchar *)par);
        appt = xfical_appt_alloc();
        tt=time(NULL);
        t=localtime(&tt);
        g_sprintf(today, "%04d%02d%02d", t->tm_year+1900, t->tm_mon+1
                , t->tm_mday);
        /* If we're today, we propose an appointment the next half-hour */
        /* JK: hour 24 is wrong, we use 00 */
        if (strcmp(apptw->chosen_date, today) == 0 && t->tm_hour < 23) { 
            if(t->tm_min <= 30){
                g_sprintf(appt->starttime,"%sT%02d%02d00"
                            , par, t->tm_hour, 30);
                g_sprintf(appt->endtime,"%sT%02d%02d00"
                            , par, t->tm_hour + 1, 00);
            }
            else{
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
        if (local_icaltimezone_utc) {
            appt->start_tz_loc = "UTC";
            appt->end_tz_loc = "UTC";
        }
        else if (local_icaltimezone_location) {
            appt->start_tz_loc = local_icaltimezone_location;
            appt->end_tz_loc = local_icaltimezone_location;
        }
        appt->duration = 30;
        gtk_widget_set_sensitive(apptw->appDuplicate, FALSE);
    }
    else if ((strcmp(action, "UPDATE") == 0) || (strcmp(action, "COPY") == 0)){
        apptw->appointment_new = FALSE;
        /* If we're making a copy, apptw->add_appointment must be TRUE
         * but the button for duplication must be inactivated in the 
           new appointment */
        apptw->add_appointment = (strcmp(action, "COPY") == 0);
        gtk_widget_set_sensitive(apptw->appDuplicate, !apptw->add_appointment);
        /* par contains ical uid */
        if (!xfical_file_open()) {
            g_error("ical file open failed\n");
            return;
        }
        if ((appt = xfical_appt_get(par)) == NULL) {
            g_message("appointment not found");
            xfical_file_close();
            return;
        }
    }
    else
        g_error("unknown parameter\n");

    apptw->xf_uid = g_strdup(appt->uid);

    /* window title */
    gtk_window_set_title(GTK_WINDOW(apptw->appWindow), _("New appointment - Orage"));

    /* appointment name */
    gtk_entry_set_text(GTK_ENTRY(apptw->appTitle_entry), (appt->title ? appt->title : ""));

    /* location */
    gtk_entry_set_text(GTK_ENTRY(apptw->appLocation_entry), (appt->location ? appt->location : ""));

    fill_appt_window_times(apptw, appt);

    /* availability */
    if (appt->availability != -1){
      gtk_combo_box_set_active(GTK_COMBO_BOX(apptw->appAvailability_cb)
                   , appt->availability);
    }

    /* note */
    tb = gtk_text_view_get_buffer((GtkTextView *)apptw->appNote_textview);
    gtk_text_buffer_set_text(tb, (appt->note ? (const gchar *) appt->note : ""), -1);

    /* ALARM */
    day = appt->alarmtime/(24*60*60);
    hours = (appt->alarmtime-day*(24*60*60))/(60*60);
    minutes = (appt->alarmtime-day*(24*60*60)-hours*(60*60))/(60);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(apptw->appAlarm_spin_dd)
                , (gdouble)day);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(apptw->appAlarm_spin_hh)
                , (gdouble)hours);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(apptw->appAlarm_spin_mm)
                , (gdouble)minutes);

    /* alarm sound */
    gtk_entry_set_text(GTK_ENTRY(apptw->appSound_entry), (appt->sound ? appt->sound : ""));

    /* alarm repeat */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(apptw->appSoundRepeat_checkbutton), appt->alarmrepeat);

    /* recurrence */
    gtk_combo_box_set_active(GTK_COMBO_BOX(apptw->appRecur_cb), appt->freq);
    switch(appt->recur_limit) {
        case 0: /* no limit */
            gtk_toggle_button_set_active(
                    GTK_TOGGLE_BUTTON(apptw->appRecur_repeat_rb), TRUE);
            gtk_spin_button_set_value(
                    GTK_SPIN_BUTTON(apptw->appRecur_count_spin), (gdouble)1);
            tt = time(NULL);
            t = localtime(&tt);
            year_month_day_to_display(t->tm_year+1900, t->tm_mon+1, t->tm_mday
                    , untildate_to_display);
            gtk_button_set_label(GTK_BUTTON(apptw->appRecur_until_button)
                    , (const gchar *)untildate_to_display);
            break;
        case 1: /* count */
            gtk_toggle_button_set_active(
                    GTK_TOGGLE_BUTTON(apptw->appRecur_count_rb), TRUE);
            gtk_spin_button_set_value(
                    GTK_SPIN_BUTTON(apptw->appRecur_count_spin)
                    , (gdouble)appt->recur_count);
            tt = time(NULL);
            t = localtime(&tt);
            year_month_day_to_display(t->tm_year+1900, t->tm_mon+1, t->tm_mday
                    , untildate_to_display);
            gtk_button_set_label(GTK_BUTTON(apptw->appRecur_until_button)
                    , (const gchar *)untildate_to_display);
            break;
        case 2: /* until */
            gtk_toggle_button_set_active(
                    GTK_TOGGLE_BUTTON(apptw->appRecur_until_rb), TRUE);
            gtk_spin_button_set_value(
                    GTK_SPIN_BUTTON(apptw->appRecur_count_spin), (gdouble)1);
            ical_to_year_month_day_hour_minute(appt->recur_until
                    , &year, &month, &day, &hours, &minutes);
            year_month_day_to_display(year, month, day, untildate_to_display);
            gtk_button_set_label(GTK_BUTTON(apptw->appRecur_until_button)
                    , (const gchar *)untildate_to_display);
            break;
        default: /* error */
            g_warning("fill_appt_window: Unsupported recur_limit %d",
                    appt->recur_limit);
    }

    if (strcmp(action, "NEW") == 0) {
        g_free(appt);
    }
    else
        xfical_file_close();

    set_time_sensitivity(apptw);
    set_repeat_sensitivity(apptw);
    mark_appointment_unchanged(apptw);
}

appt_win 
*create_appt_win(char *action, char *par, eventlist_win *event_list)
{
    register int i = 0;
    GtkWidget *menu_separator, *toolbar_separator, *label;

    char *availability_array[AVAILABILITY_ARRAY_DIM] = {
            _("Free"), _("Busy")};
    char *recur_array[RECUR_ARRAY_DIM] = {
            _("None"), _("Daily"), _("Weekly"), _("Monthly"), _("Yearly")};

    appt_win *apptw = g_new(appt_win, 1);

    apptw->xf_uid = NULL;
    apptw->eventlist = event_list;    /* Keep memory of the parent, if any */
    apptw->appointment_changed = FALSE;

    apptw->appWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(apptw->appWindow), 450, 325);

    apptw->appTooltips = gtk_tooltips_new();
    apptw->appAccelgroup = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(apptw->appWindow), apptw->appAccelgroup);

    apptw->appVBox1 = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(apptw->appWindow), apptw->appVBox1);

    /* Menu bar */
    apptw->appMenubar = gtk_menu_bar_new();
    gtk_box_pack_start(GTK_BOX (apptw->appVBox1),
                        apptw->appMenubar,
                        FALSE,
                        FALSE,
                        0);

    /* File menu stuff */
    apptw->appFile_menu = xfcalendar_menu_new(_("_File"), apptw->appMenubar);

    apptw->appFileSave_menuitem = xfcalendar_image_menu_item_new_from_stock("gtk-save", apptw->appFile_menu, apptw->appAccelgroup);

    apptw->appFileSaveClose_menuitem = xfcalendar_menu_item_new_with_mnemonic(_("Sav_e and close"), apptw->appFile_menu);
    gtk_widget_add_accelerator(apptw->appFileSaveClose_menuitem, "activate", apptw->appAccelgroup, GDK_w, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);

    menu_separator = xfcalendar_separator_menu_item_new(apptw->appFile_menu);

    apptw->appFileRevert_menuitem = xfcalendar_image_menu_item_new_from_stock("gtk-revert-to-saved", apptw->appFile_menu, apptw->appAccelgroup);

    apptw->appFileDuplicate_menuitem = xfcalendar_menu_item_new_with_mnemonic(_("D_uplicate"), apptw->appFile_menu);
    gtk_widget_add_accelerator(apptw->appFileDuplicate_menuitem, "activate", apptw->appAccelgroup, GDK_d, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    menu_separator = xfcalendar_separator_menu_item_new(apptw->appFile_menu);

    apptw->appFileDelete_menuitem = xfcalendar_image_menu_item_new_from_stock("gtk-delete", apptw->appFile_menu, apptw->appAccelgroup);

    menu_separator = xfcalendar_separator_menu_item_new(apptw->appFile_menu);

    apptw->appFileClose_menuitem = xfcalendar_image_menu_item_new_from_stock("gtk-close", apptw->appFile_menu, apptw->appAccelgroup);

    /* Handle box and toolbar */
    apptw->appHandleBox = gtk_handle_box_new();
    gtk_box_pack_start(GTK_BOX (apptw->appVBox1), apptw->appHandleBox, FALSE, FALSE, 0);

    apptw->appToolbar = gtk_toolbar_new();
    gtk_container_add(GTK_CONTAINER (apptw->appHandleBox), apptw->appToolbar);
    gtk_toolbar_set_tooltips(GTK_TOOLBAR(apptw->appToolbar), TRUE);

    /* Add buttons to the toolbar */
    apptw->appSave = xfcalendar_toolbar_append_button(apptw->appToolbar, "gtk-save", apptw->appTooltips, _("Save"), i++);

    apptw->appSaveClose = xfcalendar_toolbar_append_button(apptw->appToolbar, "gtk-close", apptw->appTooltips, _("Save and close"), i++);
    gtk_tool_button_set_label(GTK_TOOL_BUTTON (apptw->appSaveClose), _("Save and close"));
    gtk_tool_item_set_is_important(GTK_TOOL_ITEM(apptw->appSaveClose), TRUE);

    toolbar_separator = xfcalendar_toolbar_append_separator(apptw->appToolbar, i++);

    apptw->appRevert = xfcalendar_toolbar_append_button (apptw->appToolbar, "gtk-revert-to-saved", apptw->appTooltips, _("Revert"), i++);

    apptw->appDuplicate = xfcalendar_toolbar_append_button(apptw->appToolbar, "gtk-copy", apptw->appTooltips, _("Duplicate"), i++);
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(apptw->appDuplicate), _("Duplicate"));

    toolbar_separator = xfcalendar_toolbar_append_separator(apptw->appToolbar, i++);

    apptw->appDelete = xfcalendar_toolbar_append_button(apptw->appToolbar, "gtk-delete", apptw->appTooltips, _("Delete"), i++);

    apptw->appNotebook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(apptw->appVBox1), apptw->appNotebook);
    gtk_container_set_border_width(GTK_CONTAINER (apptw->appNotebook), 5);

    /* ********** Here begins the General tab ********** */
    apptw->appGeneral_notebook_page = xfce_framebox_new(NULL, FALSE);
    apptw->appGeneral_tab_label = gtk_label_new(_("General"));

    gtk_notebook_append_page(GTK_NOTEBOOK(apptw->appNotebook)
                            , apptw->appGeneral_notebook_page
                            , apptw->appGeneral_tab_label);

    apptw->appTableGeneral = xfcalendar_table_new(8, 2);
    xfce_framebox_add(XFCE_FRAMEBOX(apptw->appGeneral_notebook_page), apptw->appTableGeneral);

    apptw->appTitle_label = gtk_label_new(_("Title "));
    apptw->appTitle_entry = gtk_entry_new();
    xfcalendar_table_add_row(apptw->appTableGeneral, apptw->appTitle_label, apptw->appTitle_entry, 0,
                   (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions) (0));

    apptw->appLocation_label = gtk_label_new(_("Location"));
    apptw->appLocation_entry = gtk_entry_new();
    xfcalendar_table_add_row(apptw->appTableGeneral, apptw->appLocation_label, apptw->appLocation_entry, 1,
                   (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions) (0));

    apptw->appAllDay_checkbutton = gtk_check_button_new_with_mnemonic(_("All day event"));
    xfcalendar_table_add_row(apptw->appTableGeneral, NULL, apptw->appAllDay_checkbutton, 2,
                   (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions) (0));

    apptw->appStart = gtk_label_new(_("Start"));
    apptw->appStartDate_button = gtk_button_new();
    apptw->appStartTime_spin_hh = gtk_spin_button_new_with_range(0, 23, 1);
    apptw->appStartTime_spin_mm = gtk_spin_button_new_with_range(0, 59, 1);
    /*
    gtk_widget_set_size_request(apptw->appStartTime_comboboxentry, 70, -1);
    */
    apptw->appStartTimezone_button = gtk_button_new();
    apptw->appStartTime_hbox = xfcalendar_datetime_hbox_new(
            apptw->appStartDate_button, 
            apptw->appStartTime_spin_hh, 
            apptw->appStartTime_spin_mm, 
            apptw->appStartTimezone_button);
    xfcalendar_table_add_row(apptw->appTableGeneral, apptw->appStart, apptw->appStartTime_hbox, 3,
                      (GtkAttachOptions) (GTK_SHRINK | GTK_FILL),
                      (GtkAttachOptions) (GTK_SHRINK | GTK_FILL));

    apptw->appEnd = gtk_label_new(_("End"));
    apptw->appEndDate_button = gtk_button_new();
    apptw->appEndTime_spin_hh = gtk_spin_button_new_with_range(0, 23, 1);
    apptw->appEndTime_spin_mm = gtk_spin_button_new_with_range(0, 59, 1);
    apptw->appEndTimezone_button = gtk_button_new();
    apptw->appEndTime_hbox = xfcalendar_datetime_hbox_new(
            apptw->appEndDate_button, 
            apptw->appEndTime_spin_hh, 
            apptw->appEndTime_spin_mm, 
            apptw->appEndTimezone_button);
    xfcalendar_table_add_row(apptw->appTableGeneral, apptw->appEnd, apptw->appEndTime_hbox, 4,
                      (GtkAttachOptions) (GTK_SHRINK | GTK_FILL),
                      (GtkAttachOptions) (GTK_SHRINK | GTK_FILL));

    apptw->appDur_hbox = gtk_hbox_new(FALSE, 0);
    apptw->appDur_checkbutton = gtk_check_button_new_with_mnemonic(_("Duration"));
    gtk_box_pack_start(GTK_BOX(apptw->appDur_hbox), apptw->appDur_checkbutton,
                    FALSE, FALSE, 0);
    apptw->appDur_spin_dd = gtk_spin_button_new_with_range(0, 1000, 1);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(apptw->appDur_spin_dd), TRUE);
    gtk_box_pack_start(GTK_BOX (apptw->appDur_hbox), apptw->appDur_spin_dd,
                    FALSE, FALSE, 5);
    label = gtk_label_new(_("days"));
    gtk_box_pack_start(GTK_BOX (apptw->appDur_hbox), label, FALSE, FALSE, 0);
    label = gtk_label_new(" ");
    gtk_box_pack_start(GTK_BOX (apptw->appDur_hbox), label, FALSE, FALSE, 5);
    apptw->appDur_spin_hh = gtk_spin_button_new_with_range(0, 23, 1);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(apptw->appDur_spin_hh), TRUE);
    gtk_box_pack_start(GTK_BOX (apptw->appDur_hbox), apptw->appDur_spin_hh,
                    FALSE, FALSE, 5);
    label = gtk_label_new(_("hours"));
    gtk_box_pack_start(GTK_BOX (apptw->appDur_hbox), label, FALSE, FALSE, 0);
    label = gtk_label_new(" ");
    gtk_box_pack_start(GTK_BOX (apptw->appDur_hbox), label, FALSE, FALSE, 5);
    apptw->appDur_spin_mm = gtk_spin_button_new_with_range(0, 59, 5);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(apptw->appDur_spin_mm), TRUE);
    gtk_box_pack_start(GTK_BOX (apptw->appDur_hbox), apptw->appDur_spin_mm,
                    FALSE, FALSE, 5);
    label = gtk_label_new(_("mins"));
    gtk_box_pack_start(GTK_BOX (apptw->appDur_hbox), label, FALSE, FALSE, 0);
    xfcalendar_table_add_row(apptw->appTableGeneral, NULL, apptw->appDur_hbox, 5,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL));
    
    apptw->appAvailability = gtk_label_new(_("Availability"));
    apptw->appAvailability_cb = gtk_combo_box_new_text();
    xfcalendar_combo_box_append_array(apptw->appAvailability_cb, availability_array, AVAILABILITY_ARRAY_DIM);
    xfcalendar_table_add_row(apptw->appTableGeneral, apptw->appAvailability, apptw->appAvailability_cb, 6,
                   (GtkAttachOptions) (GTK_FILL),
                   (GtkAttachOptions) (GTK_FILL));

    apptw->appNote = gtk_label_new(_("Note"));
    apptw->appNote_Scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(apptw->appNote_Scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW (apptw->appNote_Scrolledwindow), GTK_SHADOW_IN);

    apptw->appNote_buffer = gtk_text_buffer_new(NULL);
    apptw->appNote_textview = gtk_text_view_new_with_buffer(GTK_TEXT_BUFFER(apptw->appNote_buffer));
    gtk_container_add(GTK_CONTAINER(apptw->appNote_Scrolledwindow), apptw->appNote_textview);
    xfcalendar_table_add_row(apptw->appTableGeneral, apptw->appNote, apptw->appNote_Scrolledwindow, 7,
                   (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions) (GTK_EXPAND | GTK_FILL));

    /*
    apptw->appPrivate = gtk_label_new (_("Private"));
    gtk_widget_show (apptw->appPrivate);
    gtk_table_attach (GTK_TABLE (apptw->appTableGeneral), apptw->appPrivate, 0, 1, 5, 6,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (apptw->appPrivate), 0, 0.5);

    apptw->appPrivate_check = gtk_check_button_new_with_mnemonic ("");
    gtk_widget_show (apptw->appPrivate_check);
    gtk_table_attach (GTK_TABLE (apptw->appTableGeneral), apptw->appPrivate_check, 1, 2, 5, 6,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    */

    /* ********** Here begins the Alarm tab ********** */
    apptw->appAlarm_notebook_page = xfce_framebox_new(NULL, FALSE);
    apptw->appAlarm_tab_label = gtk_label_new(_("Alarm"));

    gtk_notebook_append_page(GTK_NOTEBOOK(apptw->appNotebook)
                            , apptw->appAlarm_notebook_page
                            , apptw->appAlarm_tab_label);

    apptw->appTableAlarm = xfcalendar_table_new(3, 2);
    xfce_framebox_add(XFCE_FRAMEBOX(apptw->appAlarm_notebook_page), apptw->appTableAlarm);

    apptw->appAlarm = gtk_label_new(_("Alarm"));
    apptw->appAlarm_hbox = gtk_hbox_new(FALSE, 0);
    apptw->appAlarm_spin_dd = gtk_spin_button_new_with_range(0, 100, 1);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(apptw->appAlarm_spin_dd), TRUE);
    gtk_box_pack_start(GTK_BOX (apptw->appAlarm_hbox), apptw->appAlarm_spin_dd, FALSE, FALSE, 0);
    label = gtk_label_new(_("days"));
    gtk_box_pack_start(GTK_BOX (apptw->appAlarm_hbox), label, FALSE, FALSE, 5);
    label = gtk_label_new(" ");
    gtk_box_pack_start(GTK_BOX (apptw->appAlarm_hbox), label, FALSE, FALSE, 10);

    apptw->appAlarm_spin_hh = gtk_spin_button_new_with_range(0, 23, 1);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(apptw->appAlarm_spin_hh), TRUE);
    gtk_box_pack_start(GTK_BOX (apptw->appAlarm_hbox), apptw->appAlarm_spin_hh, FALSE, FALSE, 0);
    label = gtk_label_new(_("hours"));
    gtk_box_pack_start(GTK_BOX (apptw->appAlarm_hbox), label, FALSE, FALSE, 5);
    label = gtk_label_new(" ");
    gtk_box_pack_start(GTK_BOX (apptw->appAlarm_hbox), label, FALSE, FALSE, 10);
    apptw->appAlarm_spin_mm = gtk_spin_button_new_with_range(0, 59, 5);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(apptw->appAlarm_spin_mm), TRUE);
    gtk_box_pack_start(GTK_BOX (apptw->appAlarm_hbox), apptw->appAlarm_spin_mm, FALSE, FALSE, 0);
    label = gtk_label_new(_("mins"));
    gtk_box_pack_start(GTK_BOX (apptw->appAlarm_hbox), label, FALSE, FALSE, 5);
    xfcalendar_table_add_row(apptw->appTableAlarm, apptw->appAlarm, apptw->appAlarm_hbox, 1,
                   (GtkAttachOptions) (GTK_FILL),
                   (GtkAttachOptions) (GTK_FILL));


    apptw->appSound_label = gtk_label_new(_("Sound"));
    apptw->appSound_hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_set_spacing(GTK_BOX (apptw->appSound_hbox), 6);
    apptw->appSound_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX (apptw->appSound_hbox), apptw->appSound_entry, TRUE, TRUE, 0);
    apptw->appSound_button = gtk_button_new_from_stock("gtk-find");
    gtk_box_pack_start(GTK_BOX(apptw->appSound_hbox), apptw->appSound_button, FALSE, TRUE, 0);
    xfcalendar_table_add_row(apptw->appTableAlarm, apptw->appSound_label, apptw->appSound_hbox, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL));

    apptw->appSoundRepeat_checkbutton = gtk_check_button_new_with_mnemonic(_("Repeat alarm sound"));
    xfcalendar_table_add_row(apptw->appTableAlarm, NULL, apptw->appSoundRepeat_checkbutton, 3,
                   (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions) (0));

    /* ********** Here begins the Recurrence tab ********** */
    apptw->appRecur_notebook_page = xfce_framebox_new(NULL, FALSE);
    apptw->appRecur_tab_label = gtk_label_new(_("Recurrence"));

    gtk_notebook_append_page(GTK_NOTEBOOK(apptw->appNotebook)
                            , apptw->appRecur_notebook_page
                            , apptw->appRecur_tab_label);

    apptw->appTableRecur = xfcalendar_table_new(4, 2);
    xfce_framebox_add(XFCE_FRAMEBOX(apptw->appRecur_notebook_page), apptw->appTableRecur);

    apptw->appRecur = gtk_label_new(_("Recurrence"));
    apptw->appRecur_cb = gtk_combo_box_new_text();
    xfcalendar_combo_box_append_array(apptw->appRecur_cb, recur_array, RECUR_ARRAY_DIM);
    xfcalendar_table_add_row(apptw->appTableRecur, apptw->appRecur, apptw->appRecur_cb, 1,
                   (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions) (GTK_FILL));

    apptw->appRecur_repeat_rb = gtk_radio_button_new_with_label(
            NULL, _("Repeat forever"));
    xfcalendar_table_add_row(apptw->appTableRecur, NULL, apptw->appRecur_repeat_rb, 2,
                   (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions) (0));

    apptw->appRecur_count_hbox = gtk_hbox_new(FALSE, 0);
    apptw->appRecur_count_rb = gtk_radio_button_new_with_mnemonic_from_widget(
            GTK_RADIO_BUTTON(apptw->appRecur_repeat_rb), 
            _("Repeat "));
    gtk_box_pack_start(GTK_BOX(apptw->appRecur_count_hbox), apptw->appRecur_count_rb, FALSE, FALSE, 0);
    apptw->appRecur_count_spin = gtk_spin_button_new_with_range(1, 100, 1);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(apptw->appRecur_count_spin), TRUE);
    gtk_box_pack_start(GTK_BOX(apptw->appRecur_count_hbox), apptw->appRecur_count_spin, FALSE, FALSE, 0);
    label = gtk_label_new(_("times"));
    gtk_box_pack_start(GTK_BOX(apptw->appRecur_count_hbox), label, FALSE, FALSE, 5);
    xfcalendar_table_add_row(apptw->appTableRecur, NULL, apptw->appRecur_count_hbox, 3,
                   (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions) (0));

    apptw->appRecur_until_hbox = gtk_hbox_new(FALSE, 0);
    apptw->appRecur_until_rb = gtk_radio_button_new_with_mnemonic_from_widget(
            GTK_RADIO_BUTTON(apptw->appRecur_repeat_rb), 
            _("Repeat until "));
    gtk_box_pack_start(GTK_BOX(apptw->appRecur_until_hbox), apptw->appRecur_until_rb, FALSE, FALSE, 0);
    apptw->appRecur_until_button = gtk_button_new();
    gtk_box_pack_start(GTK_BOX(apptw->appRecur_until_hbox), apptw->appRecur_until_button, FALSE, FALSE, 0);
    xfcalendar_table_add_row(apptw->appTableRecur, NULL, apptw->appRecur_until_hbox, 4,
                   (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions) (0));


    /* */

    g_signal_connect((gpointer) apptw->appFileSave_menuitem, "activate",
            G_CALLBACK(on_appFileSave_menu_activate_cb), apptw);

    g_signal_connect((gpointer) apptw->appFileSaveClose_menuitem, "activate",
            G_CALLBACK(on_appFileSaveClose_menu_activate_cb), apptw);

    g_signal_connect((gpointer) apptw->appFileDuplicate_menuitem, "activate",
            G_CALLBACK(on_appFileDuplicate_menu_activate_cb), apptw);

    g_signal_connect((gpointer) apptw->appFileRevert_menuitem, "activate",
            G_CALLBACK(on_appFileRevert_menu_activate_cb), apptw);

    g_signal_connect((gpointer) apptw->appFileDelete_menuitem, "activate",
            G_CALLBACK(on_appFileDelete_menu_activate_cb), apptw);

    g_signal_connect((gpointer) apptw->appFileClose_menuitem, "activate",
            G_CALLBACK(on_appFileClose_menu_activate_cb), apptw);

    g_signal_connect((gpointer) apptw->appAllDay_checkbutton, "clicked",
            G_CALLBACK(on_appAllDay_clicked_cb), apptw);

    g_signal_connect((gpointer) apptw->appSound_button, "clicked",
            G_CALLBACK(on_appSound_button_clicked_cb), apptw);

    g_signal_connect((gpointer) apptw->appSave, "clicked",
            G_CALLBACK(on_appSave_clicked_cb), apptw);

    g_signal_connect((gpointer) apptw->appSaveClose, "clicked",
            G_CALLBACK(on_appSaveClose_clicked_cb), apptw);

    g_signal_connect((gpointer) apptw->appDelete, "clicked",
            G_CALLBACK(on_appDelete_clicked_cb), apptw);

    g_signal_connect((gpointer) apptw->appDuplicate, "clicked",
            G_CALLBACK(on_appDuplicate_clicked_cb), apptw);

    g_signal_connect((gpointer) apptw->appRevert, "clicked",
            G_CALLBACK(on_appRevert_clicked_cb), apptw);

    g_signal_connect((gpointer) apptw->appStartDate_button, "clicked",
            G_CALLBACK(on_appStartEndDate_clicked_cb), apptw);

    g_signal_connect((gpointer) apptw->appEndDate_button, "clicked",
            G_CALLBACK(on_appStartEndDate_clicked_cb), apptw);

    g_signal_connect((gpointer) apptw->appStartTime_spin_hh, "changed",
            G_CALLBACK(on_app_spin_button_changed_cb), apptw);

    g_signal_connect((gpointer) apptw->appStartTime_spin_mm, "changed",
            G_CALLBACK(on_app_spin_button_changed_cb), apptw);

    g_signal_connect((gpointer) apptw->appEndTime_spin_hh, "changed",
            G_CALLBACK(on_app_spin_button_changed_cb), apptw);

    g_signal_connect((gpointer) apptw->appEndTime_spin_mm, "changed",
            G_CALLBACK(on_app_spin_button_changed_cb), apptw);

    g_signal_connect((gpointer) apptw->appStartTimezone_button, "clicked",
            G_CALLBACK(on_appStartEndTimezone_clicked_cb), apptw);

    g_signal_connect((gpointer) apptw->appEndTimezone_button, "clicked",
            G_CALLBACK(on_appStartEndTimezone_clicked_cb), apptw);

    g_signal_connect((gpointer) apptw->appDur_checkbutton, "clicked",
            G_CALLBACK(appDur_checkbutton_clicked_cb), apptw);

    g_signal_connect((gpointer) apptw->appDur_spin_dd, "value-changed",
            G_CALLBACK(on_app_spin_button_changed_cb), apptw);

    g_signal_connect((gpointer) apptw->appDur_spin_hh, "value-changed",
            G_CALLBACK(on_app_spin_button_changed_cb), apptw);

    g_signal_connect((gpointer) apptw->appDur_spin_mm, "value-changed",
            G_CALLBACK(on_app_spin_button_changed_cb), apptw);

    g_signal_connect((gpointer) apptw->appWindow, "delete-event",
            G_CALLBACK(on_appWindow_delete_event_cb), apptw);
    /* Take care of the title entry to build the appointment window title 
     * Beware: we are not using apptw->appTitle_entry as a GtkEntry here 
     * but as an interface GtkEditable instead.
     */
    g_signal_connect((gpointer) apptw->appTitle_entry, "changed",
            G_CALLBACK(on_appTitle_entry_changed_cb), apptw);

    g_signal_connect((gpointer) apptw->appLocation_entry, "changed",
            G_CALLBACK(on_app_entry_changed_cb), apptw);

    g_signal_connect((gpointer) apptw->appSoundRepeat_checkbutton, "clicked",
            G_CALLBACK(app_checkbutton_clicked_cb), apptw);

    g_signal_connect((gpointer) apptw->appRecur_cb, "changed",
            G_CALLBACK(on_app_combobox_changed_cb), apptw);

    g_signal_connect((gpointer) apptw->appRecur_repeat_rb, "clicked",
            G_CALLBACK(app_checkbutton_clicked_cb), apptw);

    g_signal_connect((gpointer) apptw->appRecur_count_rb, "clicked",
            G_CALLBACK(app_checkbutton_clicked_cb), apptw);

    g_signal_connect((gpointer) apptw->appRecur_count_spin, "value-changed",
            G_CALLBACK(on_app_spin_button_changed_cb), apptw);

    g_signal_connect((gpointer) apptw->appRecur_count_rb, "clicked",
            G_CALLBACK(app_checkbutton_clicked_cb), apptw);

    g_signal_connect((gpointer) apptw->appRecur_until_button, "clicked",
            G_CALLBACK(on_appStartEndDate_clicked_cb), apptw);

    g_signal_connect((gpointer) apptw->appAvailability_cb, "changed",
            G_CALLBACK(on_app_combobox_changed_cb), apptw);

    g_signal_connect((gpointer) apptw->appAlarm_spin_dd, "value-changed",
            G_CALLBACK(on_app_spin_button_changed_cb), apptw);

    g_signal_connect((gpointer) apptw->appAlarm_spin_hh, "value-changed",
            G_CALLBACK(on_app_spin_button_changed_cb), apptw);

    g_signal_connect((gpointer) apptw->appAlarm_spin_mm, "value-changed",
            G_CALLBACK(on_app_spin_button_changed_cb), apptw);

    g_signal_connect((gpointer) apptw->appNote_buffer, "changed",
            G_CALLBACK(on_appNote_buffer_changed_cb), apptw);

    g_signal_connect((gpointer) apptw->appSound_entry, "changed",
            G_CALLBACK(on_app_entry_changed_cb), apptw);

    fill_appt_window(apptw, action, par);
    gtk_widget_show_all(apptw->appWindow);

    return apptw;
}
