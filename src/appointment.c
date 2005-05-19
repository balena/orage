/* appointment.c
 *
 * Copyright (C) 2004-2005 Mickaël Graf <korbinus at xfce.org>
 * Copyright (C) 2005 Juha Kautto <kautto.juha at kolumbus.fi>
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
#include <regex.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib/gprintf.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/netk-trayicon.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4mcs/mcs-client.h>

#include "mainbox.h"
#include "appointment.h"
#include "event-list.h"
#include "ical-code.h"

#define DATE_SEPARATOR "/"
#define MAX_APP_LENGTH 4096
#define RCDIR          "xfce4" G_DIR_SEPARATOR_S "xfcalendar"
#define APPOINTMENT_FILE "appointments.ics"
#define FILETYPE_SIZE 38

static GtkWidget *selDate_Window_dialog;

gboolean ical_to_year_month_day_hours_minutes(char *ical, int *year, int *month, int *day, int *hours, int *minutes){
    int i, j;
    char cyear[5], cmonth[3], cday[3], chours[3], cminutes[3];

    if(strlen(ical) >= 8){
       for (i = 0, j = 0; i < 4; i++){
            cyear[j++] = ical[i];
        }
        cyear[4] = '\0';

        cmonth[0] = ical[4];
        cmonth[1] = ical[5];
        cmonth[2] = '\0';

        cday[0] = ical[6];
        cday[1] = ical[7];
        cday[2] = '\0';

        *year = atoi(cyear);
        *month = atoi(cmonth);
        *day = atoi(cday);

        if(strlen(ical) >= 13){
            chours[0] = ical[9];
            chours[1] = ical[10];
            chours[2] = '\0';

            cminutes[0] = ical[11];
            cminutes[1] = ical[12];
            cminutes[2] = '\0';

            *hours = atoi(chours);
            *minutes = atoi(cminutes);

        }
        else{
            *hours = -1;
            *minutes = -1;
        }

        return TRUE;
    }
    else{
        g_warning("ical string uncomplete\n");
        return FALSE;
    }
}

void year_month_day_to_display(int year, int month, int day, char *string_to_display){
    char cyear[5], cmonth[3], cday[3];
    const char *date_format;
    int i;
    struct tm *d;

    d = (struct tm *)malloc(1*sizeof(struct tm));

    date_format = _("%m/%d/%Y");
    d->tm_mday = day;
    d->tm_mon = month - 1;
    d->tm_year = year - 1900;

    strftime(string_to_display, 11, date_format, (const struct tm *) d);

    free(d);
}

void
on_appTitle_entry_changed_cb(GtkEditable *entry, gpointer user_data)
{
    gchar *title, *application_name;
    const gchar *appointment_name;

    appt_win *apptw = (appt_win *)user_data;

    appointment_name = gtk_entry_get_text((GtkEntry *)apptw->appTitle_entry);
    application_name = _("Xfcalendar");

    if(strlen((char *)appointment_name) > 0)
        title = g_strdup_printf("%s - %s", gtk_entry_get_text((GtkEntry *)apptw->appTitle_entry), application_name);
    else
        title = g_strdup_printf("%s", application_name);

    gtk_window_set_title (GTK_WINDOW (apptw->appWindow), (const gchar *)title);

    g_free(title);
}

void
on_appSound_button_clicked_cb(GtkButton *button, gpointer user_data)
{
    GtkWidget *file_chooser;
	XfceFileFilter *filter;

    gchar *appSound_entry_filename;

    const gchar * filetype[FILETYPE_SIZE] = {"*.aiff", "*.al", "*.alsa", "*.au", "*.auto", "*.avr",
                                             "*.cdr", "*.cvs", "*.dat", "*.vms", "*.gsm", "*.hcom", 
                                             "*.la", "*.lu", "*.maud", "*.mp3", "*.nul", "*.ogg", "*.ossdsp",
                                             "*.prc", "*.raw", "*.sb", "*.sf", "*.sl", "*.smp", "*.sndt", 
                                             "*.sph", "*.8svx", "*.sw", "*.txw", "*.ub", "*.ul", "*.uw",
                                             "*.voc", "*.vorbis", "*.vox", "*.wav", "*.wve"};

    int i;

    appt_win *apptw = (appt_win *)user_data;
    appSound_entry_filename = g_strdup(gtk_entry_get_text((GtkEntry *)apptw->appSound_entry));

    file_chooser = xfce_file_chooser_new (_("Select a file..."),
                                            GTK_WINDOW (apptw->appWindow),
                                            XFCE_FILE_CHOOSER_ACTION_OPEN,
                                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                            GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                            NULL);

    filter = xfce_file_filter_new ();
	xfce_file_filter_set_name(filter, _("Sound Files"));
    for(i = 0; i<FILETYPE_SIZE; i++){
        xfce_file_filter_add_pattern(filter, filetype[i]);
    }
	xfce_file_chooser_add_filter(XFCE_FILE_CHOOSER(file_chooser), filter);
	filter = xfce_file_filter_new ();
	xfce_file_filter_set_name(filter, _("All Files"));
	xfce_file_filter_add_pattern(filter, "*");
	xfce_file_chooser_add_filter(XFCE_FILE_CHOOSER(file_chooser), filter);

    xfce_file_chooser_add_shortcut_folder(XFCE_FILE_CHOOSER(file_chooser), PACKAGE_DATA_DIR "/xfcalendar/sounds", NULL);

    if(strlen(appSound_entry_filename) > 0){
        g_warning("File: %s\n", appSound_entry_filename);
        xfce_file_chooser_set_filename(XFCE_FILE_CHOOSER(file_chooser), 
                                       (const gchar *) appSound_entry_filename);
    }
    else
        xfce_file_chooser_set_current_folder(XFCE_FILE_CHOOSER(file_chooser), PACKAGE_DATA_DIR "/xfcalendar/sounds");

	if(gtk_dialog_run(GTK_DIALOG(file_chooser)) == GTK_RESPONSE_ACCEPT) {
        gchar *sound_file;
		sound_file = xfce_file_chooser_get_filename(XFCE_FILE_CHOOSER(file_chooser));

        if(sound_file){
			gtk_entry_set_text(GTK_ENTRY(apptw->appSound_entry), sound_file);
			gtk_editable_set_position(GTK_EDITABLE(apptw->appSound_entry), -1);
        }
    }
    gtk_widget_show(file_chooser);

    gtk_widget_destroy(file_chooser);
    g_free(appSound_entry_filename);
}

void
on_appAllDay_clicked_cb(GtkCheckButton *checkbutton, gpointer user_data)
{
  gboolean check_status;
  appt_win *apptw = (appt_win *)user_data;

  check_status = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(apptw->appAllDay_checkbutton));

  if(check_status){
/*
    gtk_widget_hide(apptw->appStartTime_comboboxentry);
    gtk_widget_hide(apptw->appEndTime_comboboxentry);
*/
    gtk_widget_set_sensitive(apptw->appStartTime_comboboxentry, FALSE);
    gtk_widget_set_sensitive(apptw->appEndTime_comboboxentry, FALSE);
  } else {
/*
    gtk_widget_show(apptw->appStartTime_comboboxentry);
    gtk_widget_show(apptw->appEndTime_comboboxentry);
*/
    gtk_widget_set_sensitive(apptw->appStartTime_comboboxentry, TRUE);
    gtk_widget_set_sensitive(apptw->appEndTime_comboboxentry, TRUE);
  }
}

void
fill_appt(appt_type *appt, appt_win *apptw)
{
    GtkTextIter start, end;
    const char *date_format, *time_format;
    struct tm current_t;
    char *returned_by_strptime;
    int dummy1, dummy2;

    appt->title = (gchar *) gtk_entry_get_text((GtkEntry *)apptw->appTitle_entry);

    appt->location = (gchar *) gtk_entry_get_text((GtkEntry *)apptw->appLocation_entry);

    date_format = _("%m/%d/%Y");
    time_format = "%H:%M";

    current_t.tm_hour = 0;
    current_t.tm_min = 0;

    returned_by_strptime = strptime(gtk_button_get_label(GTK_BUTTON(apptw->appStartDate_button)), date_format, &current_t);
    returned_by_strptime = strptime(gtk_entry_get_text(GTK_ENTRY(GTK_BIN(apptw->appStartTime_comboboxentry)->child)), time_format, &current_t);

    g_sprintf(appt->starttime, XFICAL_APP_TIME_FORMAT
            , current_t.tm_year + 1900, current_t.tm_mon + 1, current_t.tm_mday
            , current_t.tm_hour, current_t.tm_min, 0);

    current_t.tm_hour = 0;
    current_t.tm_min = 0;

    returned_by_strptime = strptime(gtk_button_get_label(GTK_BUTTON(apptw->appEndDate_button)), date_format, &current_t);
    returned_by_strptime = strptime(gtk_entry_get_text(GTK_ENTRY(GTK_BIN(apptw->appEndTime_comboboxentry)->child)), time_format, &current_t);
    g_sprintf(appt->endtime, XFICAL_APP_TIME_FORMAT
            , current_t.tm_year + 1900, current_t.tm_mon + 1, current_t.tm_mday
            , current_t.tm_hour, current_t.tm_min, 0);

    appt->alarmtime = gtk_combo_box_get_active((GtkComboBox *)apptw->appAlarm_combobox);

    appt->freq = gtk_combo_box_get_active((GtkComboBox *)apptw->appRecurrency_cb);

    appt->availability = gtk_combo_box_get_active((GtkComboBox *)apptw->appAvailability_cb);

    gtk_text_buffer_get_bounds(gtk_text_view_get_buffer((GtkTextView *)apptw->appNote_textview), 
                                &start, 
                                &end);

    appt->note = gtk_text_iter_get_text(&start, &end);

    appt->allDay = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(apptw->appAllDay_checkbutton));

    appt->alarmrepeat = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(apptw->appSoundRepeat_checkbutton));

    appt->sound = (gchar *) gtk_entry_get_text((GtkEntry *)apptw->appSound_entry);

}

gboolean validate_time(gchar *str){
    /* I tried to use regexec here but it was such a hassle that 
     * I decided to go the hard way :(
     */
    char chh[3], cmm[3];
    int hh, mm;

    if (strlen((char *)str) == 0) {
        return TRUE;
    }

    if (strlen((char *)str) != 5) {
        g_warning ("Wrong length for %s", str);
        return FALSE;
    }

    if (str[2] != ':') {
        g_warning ("Wrong separator for %s", str);
        return FALSE;
    }

    chh[0] = str[0];
    chh[1] = str[1];
    chh[2] = '\0';

    cmm[0] = str[3];
    cmm[1] = str[4];
    cmm[2] = '\0';

    if (!isdigit(chh[0]) ||
        !isdigit(chh[1]) ||
        !isdigit(cmm[0]) ||
        !isdigit(cmm[1])) {
        g_warning("Incorrect numbers %s", str);
        return FALSE;
    }

    hh = atoi (chh);
    mm = atoi (cmm);

    if (!(hh >= 0 && hh < 24) ||
        !(mm >= 0 && mm < 60)) {
        g_warning("Time values out of range %d and %d", hh, mm);
        return FALSE;
    }

    return TRUE;

}

void
on_appSave_clicked_cb(GtkButton *button, gpointer user_data)
{
    gint result;
    gchar *starttime, *endtime;

    appt_win *apptw = (appt_win *)user_data;
    gboolean ok = FALSE;
    appt_type *appt = g_new(appt_type, 1); 
    gchar *new_uid;

    starttime = g_strdup(gtk_entry_get_text(GTK_ENTRY(GTK_BIN(apptw->appStartTime_comboboxentry)->child)));
    endtime = g_strdup(gtk_entry_get_text(GTK_ENTRY(GTK_BIN(apptw->appEndTime_comboboxentry)->child)));

    fill_appt(appt, apptw);

    if(g_ascii_strcasecmp(appt->endtime, appt->starttime) < 0){
        result = xfce_message_dialog(GTK_WINDOW(apptw->appWindow),
                                     _("Warning"),
                                     GTK_STOCK_DIALOG_WARNING,
                                     _("The end of this appointment is earlier than the beginning."),
                                     NULL,
                                     GTK_STOCK_OK,
                                     GTK_RESPONSE_ACCEPT,
                                     NULL);
    }
    else if(!validate_time(starttime) || !validate_time(endtime)){
        result = xfce_message_dialog(GTK_WINDOW(apptw->appWindow),
                                     _("Warning"),
                                     GTK_STOCK_DIALOG_WARNING,
                                     _("A time value is wrong."),
                                     _("Time values must be written 'hh:mm', for instance '09:36' or '15:23'."),
                                     GTK_STOCK_OK,
                                     GTK_RESPONSE_ACCEPT,
                                     NULL);
    }
    else{
        /* Here we try to save the event... */
        if (xfical_file_open()){
            if (apptw->add_appointment) {
                apptw->xf_uid = xfical_app_add(appt);
                apptw->add_appointment = FALSE;
                ok = TRUE;
                g_message("New ical uid: %s \n", apptw->xf_uid);
            }
            else {
                ok = xfical_app_mod(apptw->xf_uid, appt);
                g_message("Modified :%s \n", apptw->xf_uid);
            }
            xfical_file_close();
        }

        if (ok && apptw->wEventlist != NULL) {
            recreate_wEventlist(apptw->wEventlist);
        }
    }

    g_free(starttime);
    g_free(endtime);

}

void
on_appSaveClose_clicked_cb(GtkButton *button, gpointer user_data)
{
    gint result;
    gchar *starttime, *endtime;

    appt_win *apptw = (appt_win *)user_data;
    gboolean ok = FALSE;
    appt_type *appt = g_new(appt_type, 1); 
    gchar *new_uid;

    starttime = g_strdup(gtk_entry_get_text(GTK_ENTRY(GTK_BIN(apptw->appStartTime_comboboxentry)->child)));
    endtime = g_strdup(gtk_entry_get_text(GTK_ENTRY(GTK_BIN(apptw->appEndTime_comboboxentry)->child)));

    fill_appt(appt, apptw);

    if(g_ascii_strcasecmp(appt->endtime, appt->starttime) < 0){
        result = xfce_message_dialog(GTK_WINDOW(apptw->appWindow),
                                     _("Warning"),
                                     GTK_STOCK_DIALOG_WARNING,
                                     _("The end of this appointment is earlier than the beginning."),
                                     NULL,
                                     GTK_STOCK_OK,
                                     GTK_RESPONSE_ACCEPT,
                                     NULL);
    }
    else if(!validate_time(starttime) || !validate_time(endtime)){
        result = xfce_message_dialog(GTK_WINDOW(apptw->appWindow),
                                     _("Warning"),
                                     GTK_STOCK_DIALOG_WARNING,
                                     _("A time value is wrong."),
                                     _("Time values must be written 'hh:mm', for instance '09:36' or '15:23'."),
                                     GTK_STOCK_OK,
                                     GTK_RESPONSE_ACCEPT,
                                     NULL);
    }
    else{
        /* Here we try to save the event... */
        if (xfical_file_open()){
            if (apptw->add_appointment) {
                new_uid = xfical_app_add(appt);
                ok = TRUE;
                g_message("New ical uid: %s \n", new_uid);
            }
            else {
                ok = xfical_app_mod(apptw->xf_uid, appt);
                g_message("Modified :%s \n", apptw->xf_uid);
            }
            xfical_file_close();
        }
        if (ok && apptw->wEventlist != NULL) {
            recreate_wEventlist(apptw->wEventlist);
        }
        gtk_widget_destroy(apptw->appWindow);
    }

    g_free(starttime);
    g_free(endtime);

}

void
on_appDelete_clicked_cb(GtkButton *button, gpointer user_data)
{
    gint result;
    appt_win *apptw = (appt_win *)user_data;

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
                                 
    if (result == GTK_RESPONSE_ACCEPT){
        if (xfical_file_open()){
            xfical_app_del(apptw->xf_uid);
            xfical_file_close();
        }

        g_message("Removed ical uid: %s \n", apptw->xf_uid);

        if (apptw->wEventlist != NULL) {
            recreate_wEventlist(apptw->wEventlist);
        }

        gtk_widget_destroy(apptw->appWindow);
    }
}

void
on_appDuplicate_clicked_cb(GtkButton *button, gpointer user_data)
{
    appt_win *apptw = (appt_win *)user_data;
    appt_win *app;
    appt_type *appt = xfical_app_alloc();
    gchar *new_uid;
    gint x, y;

    fill_appt(appt, apptw);
    if (xfical_file_open()){
        new_uid = g_strdup(xfical_app_add(appt));
        g_message("Added ical uid: %s \n", new_uid);
        xfical_file_close();
    }

    if (apptw->wEventlist != NULL) {
        recreate_wEventlist(apptw->wEventlist);
    }
    app = create_appt_win("UPDATE", new_uid, apptw->wEventlist);
    gtk_window_get_position(GTK_WINDOW(apptw->appWindow), &x, &y);
    gtk_window_move(GTK_WINDOW(app->appWindow), x+20, y+20);
    gtk_widget_show(app->appWindow);
}

void
on_appStartEndDate_clicked_cb (GtkWidget *button, gpointer *user_data)
{
    GtkWidget *selDate_Window_dialog;
    GtkWidget *selDate_Calendar_calendar;
    gint result;
    guint year, month, day;
    const char *date_format;
    char *date_to_display; 
    struct tm *t;
    struct tm current_t;
    char *returned_by_strptime;
    time_t tt;

    appt_win *apptw = (appt_win *)user_data;

    date_to_display = (char *)malloc(11);
    if(!date_to_display){
        g_warning("Memory allocation failure!\n");
    }

    date_format = _("%m/%d/%Y");
    returned_by_strptime = strptime(gtk_button_get_label(GTK_BUTTON(button)), date_format, &current_t);

    selDate_Window_dialog = gtk_dialog_new_with_buttons(NULL, GTK_WINDOW(apptw->appWindow),
                                                        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                        _("Today"), 
                                                        1,
                                                        GTK_STOCK_OK,
                                                        GTK_RESPONSE_ACCEPT,
                                                        NULL);

    selDate_Calendar_calendar = gtk_calendar_new();
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(selDate_Window_dialog)->vbox), selDate_Calendar_calendar);
    gtk_calendar_select_month(GTK_CALENDAR(selDate_Calendar_calendar), current_t.tm_mon, current_t.tm_year + 1900);
    gtk_calendar_select_day(GTK_CALENDAR(selDate_Calendar_calendar), current_t.tm_mday);

    gtk_widget_show_all(selDate_Window_dialog);

    result = gtk_dialog_run(GTK_DIALOG(selDate_Window_dialog));
    switch(result){
        case GTK_RESPONSE_ACCEPT:
            gtk_calendar_get_date(GTK_CALENDAR(selDate_Calendar_calendar), &year, &month, &day);
            year_month_day_to_display(year, month + 1, day, date_to_display);
            break;
        case 1:
            tt=time(NULL);
            t=localtime(&tt);
            year_month_day_to_display(t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, date_to_display);
            break;
        case GTK_RESPONSE_DELETE_EVENT:
            date_to_display = (char *)g_strdup(gtk_button_get_label(GTK_BUTTON(button)));
            break;
    }
    gtk_button_set_label(GTK_BUTTON(button), (const gchar *)date_to_display);
    free(date_to_display);
    gtk_widget_destroy(selDate_Window_dialog);
}

void fill_appt_window(appt_win *appt_w, char *action, char *par)
{
  char start_hh[3], start_mi[3], 
    end_hh[3], end_mi[3];
    char start_date[11], end_date[11];
    char *startdate_to_display, *enddate_to_display,
         *starttime_to_display, *endtime_to_display;
    int year, month, day, hours, minutes;
    GtkTextBuffer *tb;
    appt_type *appt_data=NULL;
    struct tm *t;
    time_t tt;

    if (strcmp(action, "NEW") == 0) {
        appt_w->add_appointment = TRUE;
        appt_data = xfical_app_alloc();
  /* par contains XFICAL_APP_DATE_FORMAT (yyyymmdd) date for new appointment */
        tt=time(NULL);
        t=localtime(&tt);
        if(t->tm_min <= 30){
            g_sprintf(appt_data->starttime,"%sT%02d%02d00"
                        , par, t->tm_hour, 30);
            g_sprintf(appt_data->endtime,"%sT%02d%02d00"
                        , par, t->tm_hour + 1, 00);
        }
        else{
            g_sprintf(appt_data->starttime,"%sT%02d%02d00"
                        , par, t->tm_hour + 1, 00);
            g_sprintf(appt_data->endtime,"%sT%02d%02d00"
                        , par, t->tm_hour + 1, 30);
        }
        g_message("Building NEW ical uid\n");
        g_message("Starttime address: %d\n", &appt_data->starttime);
        g_message("Endtime address: %d\n", &appt_data->endtime);
    }
    else if (strcmp(action, "UPDATE") == 0) {
        g_message("Editing ical uid: %s \n", par);
        appt_w->add_appointment = FALSE;
    /* par contains ical uid */
        if (!xfical_file_open()) {
            g_message("ical file open failed\n");
            return;
        }
        if ((appt_data = xfical_app_get(par)) == NULL) {
            g_message("appointment not found\n");
            xfical_file_close();
            return;
        }
    }
    else
        g_error("unknown parameter\n");

	appt_w->xf_uid = g_strdup(appt_data->uid);

    gtk_window_set_title (GTK_WINDOW (appt_w->appWindow), _("New appointment - Xfcalendar"));
    if (appt_data->title)
        gtk_entry_set_text(GTK_ENTRY(appt_w->appTitle_entry), appt_data->title);
    if (appt_data->location)
        gtk_entry_set_text(GTK_ENTRY(appt_w->appLocation_entry), appt_data->location);
    if (strlen(appt_data->starttime) > 6 ) {
        g_message("starttime: %s\n", appt_data->starttime);
        ical_to_year_month_day_hours_minutes(appt_data->starttime, &year, &month, &day, &hours, &minutes);

        startdate_to_display = (char *)malloc(11);
        if(!startdate_to_display){
            g_warning("Memory allocation failure!\n");
        }
        year_month_day_to_display(year, month, day, startdate_to_display);
        gtk_button_set_label(GTK_BUTTON(appt_w->appStartDate_button), (const gchar *)startdate_to_display);
        free(startdate_to_display);

        starttime_to_display = (char *)malloc(6);
        if(!starttime_to_display){
            g_warning("Memory allocation failure!\n");
        }
        if(hours > -1 && minutes > -1){
            sprintf(starttime_to_display, "%02d:%02d", hours, minutes);
            gtk_entry_set_text(GTK_ENTRY(GTK_BIN(appt_w->appStartTime_comboboxentry)->child), (const gchar *)starttime_to_display);
        }
        free(starttime_to_display);
    }
    if (strlen( appt_data->endtime) > 6 ) {
        g_message("endtime: %s\n", appt_data->endtime);
        ical_to_year_month_day_hours_minutes(appt_data->endtime, &year, &month, &day, &hours, &minutes);

        enddate_to_display = (char *)malloc(11);
        if(!enddate_to_display){
            g_warning("Memory allocation failure!\n");
        }
        year_month_day_to_display(year, month, day, enddate_to_display);
        gtk_button_set_label(GTK_BUTTON(appt_w->appEndDate_button), (const gchar *)enddate_to_display);
        free(enddate_to_display);

        endtime_to_display = (char *)malloc(6);
        if(!endtime_to_display){
            g_warning("Memory allocation failure!\n");
        }
        if(hours > -1 && minutes > -1){
            sprintf(endtime_to_display, "%02d:%02d", hours, minutes);
            gtk_entry_set_text(GTK_ENTRY(GTK_BIN(appt_w->appEndTime_comboboxentry)->child), (const gchar *)endtime_to_display);
        }
        free(endtime_to_display);
    }
    if (appt_data->sound)
        gtk_entry_set_text(GTK_ENTRY(appt_w->appSound_entry), appt_data->sound);
    if (appt_data->alarmtime != -1){
      gtk_combo_box_set_active(GTK_COMBO_BOX(appt_w->appAlarm_combobox)
                   , appt_data->alarmtime);
    }
	gtk_combo_box_set_active(GTK_COMBO_BOX(appt_w->appRecurrency_cb)
				   , appt_data->freq);
	if (appt_data->availability != -1){
	  gtk_combo_box_set_active(GTK_COMBO_BOX(appt_w->appAvailability_cb)
				   , appt_data->availability);
	}
	if (appt_data->note){
	  tb = gtk_text_view_get_buffer((GtkTextView *)appt_w->appNote_textview);
	  gtk_text_buffer_set_text(tb, (const gchar *) appt_data->note, -1);
	}
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(appt_w->appAllDay_checkbutton), appt_data->allDay);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(appt_w->appSoundRepeat_checkbutton), appt_data->alarmrepeat);

    if (strcmp(action, "NEW") == 0) {
        g_free(appt_data);
	}
    else
        xfical_file_close();
}

GtkWidget *appt_win_toolbar_append_button(GtkWidget *toolbar, 
                                    const gchar *stock_id, const char *text, const char *tooltip_text){
    GtkWidget *tmp_toolbar_icon;
    GtkWidget *button;

    tmp_toolbar_icon = gtk_image_new_from_stock(stock_id, gtk_toolbar_get_icon_size(GTK_TOOLBAR(toolbar)));
    button = gtk_toolbar_append_element(GTK_TOOLBAR(toolbar),
                                GTK_TOOLBAR_CHILD_BUTTON, NULL,
                                text, tooltip_text, NULL,
                                tmp_toolbar_icon, NULL, NULL);
    gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(toolbar)->children)->data))->label), TRUE);
    gtk_widget_show(button);

    return button;
}

void appt_win_table_attach_label (GtkWidget *table, GtkWidget *label, 
                                  guint left_attach, guint right_attach, guint top_attach, guint bottom_attach){
    gtk_widget_show (label);
    gtk_table_attach (GTK_TABLE (table), label, left_attach, right_attach, top_attach, bottom_attach,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
}

appt_win 
*create_appt_win(char *action, char *par, GtkWidget *wEventlist)
{
    int i;
    char * hours[48];
    GtkWidget *tmp_toolbar_icon;
    appt_win *appt = g_new(appt_win, 1);

    for(i = 0; i < 48 ; i++){
        hours[i] = (char *)calloc(6, sizeof(gchar));
        sprintf(hours[i], "%02d:%02d", (int)(i/2), (i%2)*30);
    }

    appt->xf_uid = NULL;
    appt->wEventlist = wEventlist;

    appt->appWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(appt->appWindow), 450, 325);
    if (appt->wEventlist != NULL) {
      gtk_window_set_transient_for(GTK_WINDOW(appt->appWindow)
              , GTK_WINDOW(appt->wEventlist));
      gtk_window_set_destroy_with_parent(GTK_WINDOW(appt->appWindow), TRUE);
    }

    appt->appVBox1 = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (appt->appVBox1);
    gtk_container_add (GTK_CONTAINER (appt->appWindow), appt->appVBox1);

    appt->appHeader = xfce_create_header(NULL, _("Appointment "));
    gtk_widget_show (appt->appHeader);
    gtk_box_pack_start (GTK_BOX (appt->appVBox1), appt->appHeader, FALSE, TRUE, 0);

    appt->appHandleBox = gtk_handle_box_new();
    gtk_widget_show (appt->appHandleBox);
    gtk_box_pack_start (GTK_BOX (appt->appVBox1), appt->appHandleBox, FALSE, FALSE, 0);

    appt->appToolbar = gtk_toolbar_new();
    gtk_widget_show (appt->appToolbar);
    gtk_container_add (GTK_CONTAINER (appt->appHandleBox), appt->appToolbar);

    /* Add buttons to the toolbar */
    appt->appSave = appt_win_toolbar_append_button(appt->appToolbar, "gtk-save", _("_Save"), _("Save"));
    appt->appSaveClose = appt_win_toolbar_append_button(appt->appToolbar, "gtk-close", _("Save and _close"), _("Save and close"));
    gtk_toolbar_append_space(GTK_TOOLBAR(appt->appToolbar));
    appt->appDuplicate = appt_win_toolbar_append_button(appt->appToolbar, "gtk-copy", _("D_uplicate"), _("Duplicate"));
/*
    gtk_widget_add_accelerator(appt->appDuplicate, "clicked", accel_group, GDK_u, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
*/
    gtk_toolbar_append_space(GTK_TOOLBAR(appt->appToolbar));
    appt->appDelete = appt_win_toolbar_append_button(appt->appToolbar, "gtk-delete", _("_Delete"), _("Delete"));

    appt->appNotebook = gtk_notebook_new();
    gtk_widget_show (appt->appNotebook);
    gtk_container_add (GTK_CONTAINER (appt->appVBox1), appt->appNotebook);
    gtk_container_set_border_width(GTK_CONTAINER (appt->appNotebook), 5);

    /* Here begins the General tab */
    appt->appGeneral_notebook_page = xfce_framebox_new(NULL, FALSE);
    gtk_widget_show(appt->appGeneral_notebook_page);
    appt->appGeneral_tab_label = gtk_label_new(_("General"));
    gtk_widget_show(appt->appGeneral_tab_label);

    gtk_notebook_append_page(GTK_NOTEBOOK(appt->appNotebook)
                            , appt->appGeneral_notebook_page
                            , appt->appGeneral_tab_label);

    appt->appTableGeneral = gtk_table_new (8, 2, FALSE);
    gtk_widget_show (appt->appTableGeneral);
    xfce_framebox_add(XFCE_FRAMEBOX(appt->appGeneral_notebook_page), appt->appTableGeneral);
    gtk_container_set_border_width (GTK_CONTAINER (appt->appTableGeneral), 10);
    gtk_table_set_row_spacings (GTK_TABLE (appt->appTableGeneral), 6);
    gtk_table_set_col_spacings (GTK_TABLE (appt->appTableGeneral), 6);

    appt->appTitle_label = gtk_label_new (_("Title "));
    appt_win_table_attach_label(appt->appTableGeneral, appt->appTitle_label, 0, 1, 0, 1);

    appt->appLocation_label = gtk_label_new (_("Location"));
    appt_win_table_attach_label(appt->appTableGeneral, appt->appLocation_label, 0, 1, 1, 2);

    appt->appStart = gtk_label_new (_("Start"));
    appt_win_table_attach_label(appt->appTableGeneral, appt->appStart, 0, 1, 3, 4);

    appt->appEnd = gtk_label_new (_("End"));
    appt_win_table_attach_label(appt->appTableGeneral, appt->appEnd, 0, 1, 4, 5);

    /*
    appt->appPrivate = gtk_label_new (_("Private"));
    gtk_widget_show (appt->appPrivate);
    gtk_table_attach (GTK_TABLE (appt->appTableGeneral), appt->appPrivate, 0, 1, 5, 6,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (appt->appPrivate), 0, 0.5);
    */

    appt->appRecurrency = gtk_label_new (_("Recurrency"));
    appt_win_table_attach_label(appt->appTableGeneral, appt->appRecurrency, 0, 1, 5, 6);

    appt->appAvailability = gtk_label_new (_("Availability"));
    appt_win_table_attach_label(appt->appTableGeneral, appt->appAvailability, 0, 1, 6, 7);

    appt->appNote = gtk_label_new (_("Note"));
    appt_win_table_attach_label(appt->appTableGeneral, appt->appNote, 0, 1, 7, 8);

    appt->appTitle_entry = gtk_entry_new ();
    gtk_widget_show (appt->appTitle_entry);
    gtk_table_attach (GTK_TABLE (appt->appTableGeneral), appt->appTitle_entry, 1, 2, 0, 1,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);

    appt->appLocation_entry = gtk_entry_new ();
    gtk_widget_show (appt->appLocation_entry);
    gtk_table_attach (GTK_TABLE (appt->appTableGeneral), appt->appLocation_entry, 1, 2, 1, 2,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);

    appt->appAllDay_checkbutton = gtk_check_button_new_with_mnemonic (_("All day event"));
    gtk_widget_show (appt->appAllDay_checkbutton);
    gtk_table_attach (GTK_TABLE (appt->appTableGeneral), appt->appAllDay_checkbutton, 1, 2, 2, 3,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);

    appt->appStartTime_hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (appt->appStartTime_hbox);
    gtk_table_attach (GTK_TABLE (appt->appTableGeneral), appt->appStartTime_hbox, 1, 2, 3, 4,
                      (GtkAttachOptions) (GTK_SHRINK | GTK_FILL),
                      (GtkAttachOptions) (GTK_SHRINK | GTK_FILL), 0, 0);

    appt->appStartDate_button = gtk_button_new();
    gtk_widget_show(appt->appStartDate_button);
    gtk_box_pack_start (GTK_BOX (appt->appStartTime_hbox), appt->appStartDate_button, TRUE, TRUE, 0);

    appt->appStartSpace_label = gtk_label_new (_("  "));
    gtk_widget_show (appt->appStartSpace_label);
    gtk_box_pack_start (GTK_BOX (appt->appStartTime_hbox), appt->appStartSpace_label, FALSE, FALSE, 0);
    gtk_misc_set_alignment (GTK_MISC (appt->appStartSpace_label), 0.5, 0.43);

    appt->appStartTime_comboboxentry = gtk_combo_box_entry_new_text();
    for(i = 0; i < 48; i++){
        gtk_combo_box_append_text(GTK_COMBO_BOX(appt->appStartTime_comboboxentry),
                (const gchar *)hours[i]);
    }
    gtk_widget_show(appt->appStartTime_comboboxentry);
    gtk_box_pack_start (GTK_BOX (appt->appStartTime_hbox), appt->appStartTime_comboboxentry, FALSE, TRUE, 0);

    appt->appStartTime_fixed = gtk_fixed_new ();
    gtk_widget_show (appt->appStartTime_fixed);
    gtk_box_pack_start (GTK_BOX (appt->appStartTime_hbox), appt->appStartTime_fixed, TRUE, TRUE, 0);

    appt->appEndTime_hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (appt->appEndTime_hbox);
    gtk_table_attach (GTK_TABLE (appt->appTableGeneral), appt->appEndTime_hbox, 1, 2, 4, 5,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);

    appt->appEndDate_button = gtk_button_new();
    gtk_widget_show(appt->appEndDate_button);
    gtk_box_pack_start (GTK_BOX (appt->appEndTime_hbox), appt->appEndDate_button, TRUE, TRUE, 0);

    appt->appEndSpace_label = gtk_label_new (_("  "));
    gtk_widget_show (appt->appEndSpace_label);
    gtk_box_pack_start (GTK_BOX (appt->appEndTime_hbox), appt->appEndSpace_label, FALSE, FALSE, 0);
    gtk_misc_set_alignment (GTK_MISC (appt->appEndSpace_label), 0.5, 0.43);

    appt->appEndTime_comboboxentry = gtk_combo_box_entry_new_text();
    for(i = 0; i < 48; i++){
        gtk_combo_box_append_text(GTK_COMBO_BOX(appt->appEndTime_comboboxentry),
                (const gchar *)hours[i]);
    }
    gtk_widget_show(appt->appEndTime_comboboxentry);
    gtk_box_pack_start (GTK_BOX (appt->appEndTime_hbox), appt->appEndTime_comboboxentry, FALSE, TRUE, 0);

    appt->appEndTime_fixed = gtk_fixed_new ();
    gtk_widget_show (appt->appEndTime_fixed);
    gtk_box_pack_start (GTK_BOX (appt->appEndTime_hbox), appt->appEndTime_fixed, TRUE, TRUE, 0);

    /*
    appt->appPrivate_check = gtk_check_button_new_with_mnemonic ("");
    gtk_widget_show (appt->appPrivate_check);
    gtk_table_attach (GTK_TABLE (appt->appTableGeneral), appt->appPrivate_check, 1, 2, 5, 6,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    */

    appt->appRecurrency_cb = gtk_combo_box_new_text ();
    gtk_widget_show (appt->appRecurrency_cb);
    gtk_table_attach (GTK_TABLE (appt->appTableGeneral), appt->appRecurrency_cb, 1, 2, 5, 6,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);

    gtk_combo_box_append_text (GTK_COMBO_BOX (appt->appRecurrency_cb), _("None"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (appt->appRecurrency_cb), _("Daily"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (appt->appRecurrency_cb), _("Weekly"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (appt->appRecurrency_cb), _("Monthly"));

    appt->appAvailability_cb = gtk_combo_box_new_text ();
    gtk_widget_show (appt->appAvailability_cb);
    gtk_table_attach (GTK_TABLE (appt->appTableGeneral), appt->appAvailability_cb, 1, 2, 6, 7,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);

    gtk_combo_box_append_text (GTK_COMBO_BOX (appt->appAvailability_cb), _("Free"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (appt->appAvailability_cb), _("Busy"));

    appt->appNote_Scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_show (appt->appNote_Scrolledwindow);
    gtk_table_attach (GTK_TABLE (appt->appTableGeneral), appt->appNote_Scrolledwindow, 1, 2, 7, 8,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);

    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (appt->appNote_Scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (appt->appNote_Scrolledwindow), GTK_SHADOW_IN);

    appt->appNote_textview = gtk_text_view_new ();
    gtk_widget_show (appt->appNote_textview);
    gtk_container_add (GTK_CONTAINER (appt->appNote_Scrolledwindow), appt->appNote_textview);

    /* Here begins the Alarm tab */
    appt->appAlarm_notebook_page = xfce_framebox_new(NULL, FALSE);
    gtk_widget_show(appt->appAlarm_notebook_page);
    appt->appAlarm_tab_label = gtk_label_new(_("Alarm"));
    gtk_widget_show(appt->appAlarm_tab_label);

    gtk_notebook_append_page(GTK_NOTEBOOK(appt->appNotebook)
                            , appt->appAlarm_notebook_page
                            , appt->appAlarm_tab_label);

    appt->appTableAlarm = gtk_table_new (3, 2, FALSE);
    gtk_widget_show (appt->appTableAlarm);
    xfce_framebox_add(XFCE_FRAMEBOX(appt->appAlarm_notebook_page), appt->appTableAlarm);
    gtk_container_set_border_width (GTK_CONTAINER (appt->appTableAlarm), 10);
    gtk_table_set_row_spacings (GTK_TABLE (appt->appTableAlarm), 6);
    gtk_table_set_col_spacings (GTK_TABLE (appt->appTableAlarm), 6);

    appt->appAlarm = gtk_label_new (_("Alarm"));
    appt_win_table_attach_label(appt->appTableAlarm, appt->appAlarm, 0, 1, 0, 1);

    appt->appSound_label = gtk_label_new (_("Sound"));
    appt_win_table_attach_label(appt->appTableAlarm, appt->appSound_label, 0, 1, 1, 2);

    appt->appAlarm_combobox = gtk_combo_box_new_text ();
    gtk_widget_show (appt->appAlarm_combobox);
    gtk_table_attach (GTK_TABLE (appt->appTableAlarm), appt->appAlarm_combobox, 1, 2, 0, 1,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (GTK_FILL), 0, 0);
    gtk_combo_box_append_text (GTK_COMBO_BOX (appt->appAlarm_combobox), _("None"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (appt->appAlarm_combobox), _("5 minutes"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (appt->appAlarm_combobox), _("15 minutes"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (appt->appAlarm_combobox), _("30 minutes"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (appt->appAlarm_combobox), _("45 minutes"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (appt->appAlarm_combobox), _("1 hour"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (appt->appAlarm_combobox), _("2 hours"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (appt->appAlarm_combobox), _("4 hours"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (appt->appAlarm_combobox), _("8 hours"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (appt->appAlarm_combobox), _("1 day"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (appt->appAlarm_combobox), _("2 days"));

    appt->appSound_hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_set_spacing(GTK_BOX (appt->appSound_hbox), 6);
    gtk_widget_show (appt->appSound_hbox);
    gtk_table_attach (GTK_TABLE (appt->appTableAlarm), appt->appSound_hbox, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

    appt->appSound_entry = gtk_entry_new ();
    gtk_widget_show (appt->appSound_entry);
    gtk_box_pack_start (GTK_BOX (appt->appSound_hbox), appt->appSound_entry, TRUE, TRUE, 0);

    appt->appSound_button = gtk_button_new_from_stock("gtk-find");
    gtk_widget_show (appt->appSound_button);
    gtk_box_pack_start (GTK_BOX (appt->appSound_hbox), appt->appSound_button, FALSE, TRUE, 0);

    appt->appSoundRepeat_checkbutton = gtk_check_button_new_with_mnemonic (_("Repeat alarm sound"));
    gtk_widget_show (appt->appSoundRepeat_checkbutton);
    gtk_table_attach (GTK_TABLE (appt->appTableAlarm), appt->appSoundRepeat_checkbutton, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
    /* */

    g_signal_connect ((gpointer) appt->appAllDay_checkbutton, "clicked",
            G_CALLBACK (on_appAllDay_clicked_cb),
            appt);

    g_signal_connect ((gpointer) appt->appSound_button, "clicked",
            G_CALLBACK (on_appSound_button_clicked_cb),
            appt);

    g_signal_connect ((gpointer) appt->appSave, "clicked",
            G_CALLBACK (on_appSave_clicked_cb),
            appt);

    g_signal_connect ((gpointer) appt->appSaveClose, "clicked",
            G_CALLBACK (on_appSaveClose_clicked_cb),
            appt);

    g_signal_connect ((gpointer) appt->appDelete, "clicked",
            G_CALLBACK (on_appDelete_clicked_cb),
            appt);

    g_signal_connect ((gpointer) appt->appDuplicate, "clicked",
            G_CALLBACK (on_appDuplicate_clicked_cb),
            appt);

    g_signal_connect ((gpointer) appt->appStartDate_button, "clicked",
            G_CALLBACK (on_appStartEndDate_clicked_cb),
            appt);

    g_signal_connect ((gpointer) appt->appEndDate_button, "clicked",
            G_CALLBACK (on_appStartEndDate_clicked_cb),
            appt);

    /* Take care of the title entry to build the appointment window title 
     * Beware: we are not using appt->appTitle_entry as a GtkEntry here 
     * but as an interface GtkEditable instead.
     */
    g_signal_connect_after ((gpointer) appt->appTitle_entry, "changed",
            G_CALLBACK (on_appTitle_entry_changed_cb),
            appt);

    for( i = 0; i < 48; i++){
        free(hours[i]);
    }

    fill_appt_window(appt, action, par);

    return appt;
}
