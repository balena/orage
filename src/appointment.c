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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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

#define DATE_SEPARATOR "-"
#define MAX_APP_LENGTH 4096
#define RCDIR          "xfce4" G_DIR_SEPARATOR_S "xfcalendar"
#define APPOINTMENT_FILE "appointments.ics"
#define FILETYPE_SIZE 38

static gchar *last_sound_dir;

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

    const gchar * filetype[FILETYPE_SIZE] = {"*.aiff", "*.al", "*.alsa", "*.au", "*.auto", "*.avr",
                                             "*.cdr", "*.cvs", "*.dat", "*.vms", "*.gsm", "*.hcom", 
                                             "*.la", "*.lu", "*.maud", "*.mp3", "*.nul", "*.ogg", "*.ossdsp",
                                             "*.prc", "*.raw", "*.sb", "*.sf", "*.sl", "*.smp", "*.sndt", 
                                             "*.sph", "*.8svx", "*.sw", "*.txw", "*.ub", "*.ul", "*.uw",
                                             "*.voc", "*.vorbis", "*.vox", "*.wav", "*.wve"};

    appt_win *apptw = (appt_win *)user_data;

    file_chooser = xfce_file_chooser_new (_("Select a file..."),
                                            GTK_WINDOW (apptw->appWindow),
                                            XFCE_FILE_CHOOSER_ACTION_OPEN,
                                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                            GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                            NULL);

    filter = xfce_file_filter_new ();
	xfce_file_filter_set_name(filter, _("Sound Files"));
    int i;
    for(i = 0; i<FILETYPE_SIZE; i++){
        xfce_file_filter_add_pattern(filter, filetype[i]);
    }
	xfce_file_chooser_add_filter(XFCE_FILE_CHOOSER(file_chooser), filter);
	filter = xfce_file_filter_new ();
	xfce_file_filter_set_name(filter, _("All Files"));
	xfce_file_filter_add_pattern(filter, "*");
	xfce_file_chooser_add_filter(XFCE_FILE_CHOOSER(file_chooser), filter);

    xfce_file_chooser_add_shortcut_folder(XFCE_FILE_CHOOSER(file_chooser), PACKAGE_DATA_DIR "/xfcalendar/sounds", NULL);

    /* FIXME: file_chooser never find the file given in appSound_entry
    if(apptw->appSound_entry){
        g_warning("File: %s\n", (gchar *) gtk_entry_get_text((GtkEntry *)apptw->appSound_entry));
        xfce_file_chooser_set_current_name(XFCE_FILE_CHOOSER(file_chooser), 
                                          (const gchar *) gtk_entry_get_text((GtkEntry *)apptw->appSound_entry));
    }
    else
    */
        if(last_sound_dir)
            xfce_file_chooser_set_current_folder(XFCE_FILE_CHOOSER(file_chooser), last_sound_dir);
        else
            xfce_file_chooser_set_current_folder(XFCE_FILE_CHOOSER(file_chooser), PACKAGE_DATA_DIR "/xfcalendar/sounds");

	if(gtk_dialog_run(GTK_DIALOG(file_chooser)) == GTK_RESPONSE_ACCEPT) {
        gchar *sound_file;
		sound_file = xfce_file_chooser_get_filename(XFCE_FILE_CHOOSER(file_chooser));
        last_sound_dir = xfce_file_chooser_get_current_folder(XFCE_FILE_CHOOSER(file_chooser));

        if(sound_file){
			gtk_entry_set_text(GTK_ENTRY(apptw->appSound_entry), sound_file);
			gtk_editable_set_position(GTK_EDITABLE(apptw->appSound_entry), -1);
        }
    }
    gtk_widget_show(file_chooser);

    gtk_widget_destroy(file_chooser);
}

void
on_appAllDay_clicked_cb(GtkCheckButton *checkbutton, gpointer user_data)
{
  gboolean check_status;
  appt_win *apptw = (appt_win *)user_data;

  check_status = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(apptw->appAllDay_checkbutton));

  if(check_status){
    gtk_widget_set_sensitive(apptw->appStartYear_spinbutton, FALSE);
    gtk_widget_set_sensitive(apptw->appStartMonth_spinbutton, FALSE);
    gtk_widget_set_sensitive(apptw->appStartDay_spinbutton, FALSE);
    gtk_widget_set_sensitive(apptw->appStartHour_spinbutton, FALSE);
    gtk_widget_set_sensitive(apptw->appStartMinutes_spinbutton, FALSE);
    gtk_widget_set_sensitive(apptw->appEndYear_spinbutton, FALSE);
    gtk_widget_set_sensitive(apptw->appEndMonth_spinbutton, FALSE);
    gtk_widget_set_sensitive(apptw->appEndDay_spinbutton, FALSE);
    gtk_widget_set_sensitive(apptw->appEndHour_spinbutton, FALSE);
    gtk_widget_set_sensitive(apptw->appEndMinutes_spinbutton, FALSE);
/*
    gtk_widget_set_sensitive(apptw->appAlarm_spinbutton, FALSE);
    gtk_widget_set_sensitive(apptw->appAlarmTimeType_combobox, FALSE);
*/
    gtk_widget_set_sensitive(apptw->appAlarm_combobox, FALSE);
  } else {
    gtk_widget_set_sensitive(apptw->appStartYear_spinbutton, TRUE);
    gtk_widget_set_sensitive(apptw->appStartMonth_spinbutton, TRUE);
    gtk_widget_set_sensitive(apptw->appStartDay_spinbutton, TRUE);
    gtk_widget_set_sensitive(apptw->appStartHour_spinbutton, TRUE);
    gtk_widget_set_sensitive(apptw->appStartMinutes_spinbutton, TRUE);
    gtk_widget_set_sensitive(apptw->appEndYear_spinbutton, TRUE);
    gtk_widget_set_sensitive(apptw->appEndMonth_spinbutton, TRUE);
    gtk_widget_set_sensitive(apptw->appEndDay_spinbutton, TRUE);
    gtk_widget_set_sensitive(apptw->appEndHour_spinbutton, TRUE);
    gtk_widget_set_sensitive(apptw->appEndMinutes_spinbutton, TRUE);
/*
    gtk_widget_set_sensitive(apptw->appAlarm_spinbutton, TRUE);
    gtk_widget_set_sensitive(apptw->appAlarmTimeType_combobox, TRUE);
*/
    gtk_widget_set_sensitive(apptw->appAlarm_combobox, TRUE);
  }
}

void
fill_appt(appt_type *appt, appt_win *apptw)
{
    gint StartYear_value,
      StartMonth_value,
      StartDay_value,
      StartHour_value,
      StartMinutes_value,
      EndYear_value,
      EndMonth_value,
      EndDay_value,
      EndHour_value,
      EndMinutes_value;
    GtkTextIter start, end;

    appt->title = (gchar *) gtk_entry_get_text((GtkEntry *)apptw->appTitle_entry);

    appt->location = (gchar *) gtk_entry_get_text((GtkEntry *)apptw->appLocation_entry);

    StartYear_value = gtk_spin_button_get_value_as_int((GtkSpinButton *)apptw->appStartYear_spinbutton);
    StartMonth_value = gtk_spin_button_get_value_as_int((GtkSpinButton *)apptw->appStartMonth_spinbutton);
    StartDay_value = gtk_spin_button_get_value_as_int((GtkSpinButton *)apptw->appStartDay_spinbutton);
    StartHour_value = gtk_spin_button_get_value_as_int((GtkSpinButton *)apptw->appStartHour_spinbutton);
    StartMinutes_value = gtk_spin_button_get_value_as_int((GtkSpinButton *)apptw->appStartMinutes_spinbutton);
    g_sprintf(appt->starttime, XFICAL_APP_TIME_FORMAT
            , StartYear_value, StartMonth_value, StartDay_value
            , StartHour_value, StartMinutes_value, 0);

    EndYear_value = gtk_spin_button_get_value_as_int((GtkSpinButton *)apptw->appEndYear_spinbutton);
    EndMonth_value = gtk_spin_button_get_value_as_int((GtkSpinButton *)apptw->appEndMonth_spinbutton);
    EndDay_value = gtk_spin_button_get_value_as_int((GtkSpinButton *)apptw->appEndDay_spinbutton);
    EndHour_value = gtk_spin_button_get_value_as_int((GtkSpinButton *)apptw->appEndHour_spinbutton);
    EndMinutes_value = gtk_spin_button_get_value_as_int((GtkSpinButton *)apptw->appEndMinutes_spinbutton);
    g_sprintf(appt->endtime, XFICAL_APP_TIME_FORMAT
            , EndYear_value, EndMonth_value, EndDay_value
            , EndHour_value, EndMinutes_value, 0);

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

void
on_appClose_clicked_cb(GtkButton *button, gpointer user_data)
{

    appt_win *apptw = (appt_win *)user_data;

    appt_type *appt = g_new(appt_type, 1); 
    gchar *new_uid;

    fill_appt(appt, apptw);

  /* Here we try to save the event... */
  if (xfical_file_open()){
     if (apptw->add_appointment) {
        new_uid = xfical_app_add(appt);
        g_message("New ical uid: %s \n", new_uid);
     }
     else {
        xfical_app_mod(apptw->xf_uid, appt);
        g_message("Modified :%s \n", apptw->xf_uid);
     }
     xfical_file_close();
  }

  if (apptw->wAppointment != NULL) {
     recreate_wAppointment(apptw->wAppointment);
  }

  gtk_widget_destroy(apptw->appWindow);

}

void
on_appRemove_clicked_cb(GtkButton *button, gpointer user_data)
{

  appt_win *apptw = (appt_win *)user_data;

  if (xfical_file_open()){
     xfical_app_del(apptw->xf_uid);
     xfical_file_close();
  }

  if (apptw->wAppointment != NULL) {
    recreate_wAppointment(apptw->wAppointment);
  }

  gtk_widget_destroy(apptw->appWindow);

}

void
on_appCopy_clicked_cb(GtkButton *button, gpointer user_data)
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

    if (apptw->wAppointment != NULL) {
        recreate_wAppointment(apptw->wAppointment);
    }
    app = create_appt_win("UPDATE", new_uid, apptw->wAppointment);
    gtk_window_get_position(GTK_WINDOW(apptw->appWindow), &x, &y);
    gtk_window_move(GTK_WINDOW(app->appWindow), x+20, y+20);
    gtk_widget_show(app->appWindow);
}

void ical_to_title(char *ical, char *title)
{
    gint i, j;

    for (i = 0, j = 0; i <= 9; i++) { /* yyyymmdd -> yyyy-mm-dd */
        if ((i == 4) || (i == 7))
            title[i] = '-';
        else
            title[i] = ical[j++];
    }
    title[10] = '\0';
}

void fill_appt_window(appt_win *appt_w, char *action, char *par)
{
  char start_yy[5], start_mm[3], start_dd[3], start_hh[3], start_mi[3], 
    end_yy[5], end_mm[3], end_dd[3], end_hh[3], end_mi[3];
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
        g_sprintf(appt_data->starttime,"%sT%02d%02d00"
                    , par, t->tm_hour, t->tm_min);
        strcpy(appt_data->endtime, appt_data->starttime);
        g_message("Building new ical uid: %s \n", appt_data->uid);
    }
    else if (strcmp(action, "UPDATE") == 0) {
        appt_w->add_appointment = FALSE;
    /* par contains ical uid */
        if (!xfical_file_open()) {
            g_message("ical file open failed\n");
            return;
        }
        if ((appt_data = xfical_app_get(par)) == NULL) {
            g_message("appointment not found\n");
            return;
        }
        g_message("Editing ical uid: %s \n", appt_data->uid);
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
        start_yy[0]= appt_data->starttime[0];
        start_yy[1]= appt_data->starttime[1];
        start_yy[2]= appt_data->starttime[2];
        start_yy[3]= appt_data->starttime[3];
        start_yy[4]= '\0';
        start_mm[0]= appt_data->starttime[4];
        start_mm[1]= appt_data->starttime[5];
        start_mm[2]= '\0';
        start_dd[0]= appt_data->starttime[6];
        start_dd[1]= appt_data->starttime[7];
        start_dd[2]= '\0';
        start_hh[0]= appt_data->starttime[9];
        start_hh[1]= appt_data->starttime[10];
        start_hh[2]= '\0';
        start_mi[0]= appt_data->starttime[11];
        start_mi[1]= appt_data->starttime[12];
        start_mi[2]= '\0';
        gtk_spin_button_set_value(
                  GTK_SPIN_BUTTON(appt_w->appStartYear_spinbutton)
                , (gdouble) atoi(start_yy));
        gtk_spin_button_set_value(
                  GTK_SPIN_BUTTON(appt_w->appStartMonth_spinbutton)
                , (gdouble) atoi(start_mm));
        gtk_spin_button_set_value(
                  GTK_SPIN_BUTTON(appt_w->appStartDay_spinbutton)
                , (gdouble) atoi(start_dd));
        gtk_spin_button_set_value(
                  GTK_SPIN_BUTTON(appt_w->appStartHour_spinbutton)
                , (gdouble) atoi(start_hh));
        gtk_spin_button_set_value(
                  GTK_SPIN_BUTTON(appt_w->appStartMinutes_spinbutton)
                , (gdouble) atoi(start_mi));
    }
    if (strlen( appt_data->endtime) > 6 ) {
        g_message("endtime: %s\n", appt_data->endtime);
        end_yy[0]= appt_data->endtime[0];
        end_yy[1]= appt_data->endtime[1];
        end_yy[2]= appt_data->endtime[2];
        end_yy[3]= appt_data->endtime[3];
        end_yy[4]= '\0';
        end_mm[0]= appt_data->endtime[4];
        end_mm[1]= appt_data->endtime[5];
        end_mm[2]= '\0';
        end_dd[0]= appt_data->endtime[6];
        end_dd[1]= appt_data->endtime[7];
        end_dd[2]= '\0';
        end_hh[0]= appt_data->endtime[9];
        end_hh[1]= appt_data->endtime[10];
        end_hh[2]= '\0';
        end_mi[0]= appt_data->endtime[11];
        end_mi[1]= appt_data->endtime[12];
        end_mi[2]= '\0';
        gtk_spin_button_set_value(
                  GTK_SPIN_BUTTON(appt_w->appEndYear_spinbutton)
                , (gdouble) atoi(end_yy));
        gtk_spin_button_set_value(
                  GTK_SPIN_BUTTON(appt_w->appEndMonth_spinbutton)
                , (gdouble) atoi(end_mm));
        gtk_spin_button_set_value(
                  GTK_SPIN_BUTTON(appt_w->appEndDay_spinbutton)
                , (gdouble) atoi(end_dd));
        gtk_spin_button_set_value(
                  GTK_SPIN_BUTTON(appt_w->appEndHour_spinbutton)
                , (gdouble) atoi(end_hh));
        gtk_spin_button_set_value(
                  GTK_SPIN_BUTTON(appt_w->appEndMinutes_spinbutton)
                , (gdouble) atoi(end_mi));
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

appt_win *create_appt_win(char *action, char *par, GtkWidget *wAppointment)
{
  appt_win *appt = g_new(appt_win, 1);

  appt->xf_uid = NULL;
  appt->wAppointment = wAppointment;

  appt->appWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(appt->appWindow), 450, 325);
  if (appt->wAppointment != NULL) {
    gtk_window_set_transient_for(GTK_WINDOW(appt->appWindow)
            , GTK_WINDOW(appt->wAppointment));
    gtk_window_set_destroy_with_parent(GTK_WINDOW(appt->appWindow), TRUE);
  }

  appt->appVBox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (appt->appVBox1);
  gtk_container_add (GTK_CONTAINER (appt->appWindow), appt->appVBox1);

    appt->appHeader = xfce_create_header(NULL, _("Appointment "));
    gtk_widget_show (appt->appHeader);
    gtk_box_pack_start (GTK_BOX (appt->appVBox1), appt->appHeader, TRUE, TRUE, 0);

  appt->appTable = gtk_table_new (13, 2, FALSE); 
  gtk_widget_show (appt->appTable);
  gtk_box_pack_start (GTK_BOX (appt->appVBox1), appt->appTable, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (appt->appTable), 10);
  gtk_table_set_row_spacings (GTK_TABLE (appt->appTable), 5);
  gtk_table_set_col_spacings (GTK_TABLE (appt->appTable), 5);

  appt->appTitle_label = gtk_label_new (_("Title "));
  gtk_widget_show (appt->appTitle_label);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appTitle_label, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (appt->appTitle_label), 0, 0.5);

  appt->appLocation_label = gtk_label_new (_("Location"));
  gtk_widget_show (appt->appLocation_label);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appLocation_label, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (appt->appLocation_label), 0, 0.5);

  appt->appStart = gtk_label_new (_("Start"));
  gtk_widget_show (appt->appStart);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appStart, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (appt->appStart), 0, 0.5);

  appt->appEnd = gtk_label_new (_("End"));
  gtk_widget_show (appt->appEnd);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appEnd, 0, 1, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (appt->appEnd), 0, 0.5);

  /*
  appt->appPrivate = gtk_label_new (_("Private"));
  gtk_widget_show (appt->appPrivate);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appPrivate, 0, 1, 5, 6,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (appt->appPrivate), 0, 0.5);
  */

  appt->appAlarm = gtk_label_new (_("Alarm"));
  gtk_widget_show (appt->appAlarm);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appAlarm, 0, 1, 6, 7,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (appt->appAlarm), 0, 0.5);

  /*
  appt->appRecurrence = gtk_label_new (_("Recurrence"));
  gtk_widget_show (appt->appRecurrence);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appRecurrence, 0, 1, 7, 8,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (appt->appRecurrence), 0, 0.5);
  */

    appt->appSound_label = gtk_label_new (_("Sound"));
    gtk_widget_show (appt->appSound_label);
    gtk_table_attach (GTK_TABLE (appt->appTable), appt->appSound_label, 0, 1, 8, 9,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (appt->appSound_label), 0, 0.5);

  appt->appRecurrency = gtk_label_new (_("Recurrency"));
  gtk_widget_show (appt->appRecurrency);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appRecurrency, 0, 1, 10, 11,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (appt->appRecurrency), 0, 0.5);

  appt->appAvailability = gtk_label_new (_("Availability"));
  gtk_widget_show (appt->appAvailability);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appAvailability, 0, 1, 11, 12,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (appt->appAvailability), 0, 0.5);

  appt->appNote = gtk_label_new (_("Note"));
  gtk_widget_show (appt->appNote);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appNote, 0, 1, 12, 13,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (appt->appNote), 0, 0.5);

  appt->appTitle_entry = gtk_entry_new ();
  gtk_widget_show (appt->appTitle_entry);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appTitle_entry, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  appt->appLocation_entry = gtk_entry_new ();
  gtk_widget_show (appt->appLocation_entry);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appLocation_entry, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  appt->appAllDay_checkbutton = gtk_check_button_new_with_mnemonic (_("All day event"));
  gtk_widget_show (appt->appAllDay_checkbutton);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appAllDay_checkbutton, 1, 2, 2, 3,
		    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		    (GtkAttachOptions) (0), 0, 0);

  appt->appStartTime_hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (appt->appStartTime_hbox);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appStartTime_hbox, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_SHRINK | GTK_FILL), 0, 0);

    appt->appStartYear_spinbutton_adj = gtk_adjustment_new(2005, 1971, 2150, 1, 10, 10);
    appt->appStartYear_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (appt->appStartYear_spinbutton_adj), 1, 0);
    gtk_widget_show (appt->appStartYear_spinbutton);
    gtk_box_pack_start (GTK_BOX (appt->appStartTime_hbox), appt->appStartYear_spinbutton, FALSE, TRUE, 0);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (appt->appStartYear_spinbutton), TRUE);

    appt->appStartSlash1_label = gtk_label_new (_(" / "));
    gtk_widget_show (appt->appStartSlash1_label);
    gtk_box_pack_start (GTK_BOX (appt->appStartTime_hbox), appt->appStartSlash1_label, FALSE, FALSE, 0);
    gtk_misc_set_alignment (GTK_MISC (appt->appStartSlash1_label), 0.5, 0.43);

    appt->appStartMonth_spinbutton_adj = gtk_adjustment_new(1, 1, 12, 1, 10, 10);
    appt->appStartMonth_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (appt->appStartMonth_spinbutton_adj), 1, 0);
    gtk_widget_show (appt->appStartMonth_spinbutton);
    gtk_box_pack_start (GTK_BOX (appt->appStartTime_hbox), appt->appStartMonth_spinbutton, FALSE, TRUE, 0);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (appt->appStartMonth_spinbutton), TRUE);

    appt->appStartSlash2_label = gtk_label_new (_(" / "));
    gtk_widget_show (appt->appStartSlash2_label);
    gtk_box_pack_start (GTK_BOX (appt->appStartTime_hbox), appt->appStartSlash2_label, FALSE, FALSE, 0);
    gtk_misc_set_alignment (GTK_MISC (appt->appStartSlash2_label), 0.5, 0.43);

    appt->appStartDay_spinbutton_adj = gtk_adjustment_new(1, 1, 31, 1, 10, 10);
    appt->appStartDay_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (appt->appStartDay_spinbutton_adj), 1, 0);
    gtk_widget_show (appt->appStartDay_spinbutton);
    gtk_box_pack_start (GTK_BOX (appt->appStartTime_hbox), appt->appStartDay_spinbutton, FALSE, TRUE, 0);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (appt->appStartDay_spinbutton), TRUE);

  appt->appStartHour_spinbutton_adj = gtk_adjustment_new (0, 0, 23, 1, 10, 10);
  appt->appStartHour_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (appt->appStartHour_spinbutton_adj), 1, 0);
  gtk_widget_show (appt->appStartHour_spinbutton);
  gtk_box_pack_start (GTK_BOX (appt->appStartTime_hbox), appt->appStartHour_spinbutton, FALSE, TRUE, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (appt->appStartHour_spinbutton), TRUE);

  appt->appStartColumn_label = gtk_label_new (_(" : "));
  gtk_widget_show (appt->appStartColumn_label);
  gtk_box_pack_start (GTK_BOX (appt->appStartTime_hbox), appt->appStartColumn_label, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (appt->appStartColumn_label), 0.5, 0.43);

  appt->appStartMinutes_spinbutton_adj = gtk_adjustment_new (0, 0, 59, 15, 10, 10);
  appt->appStartMinutes_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (appt->appStartMinutes_spinbutton_adj), 1, 0);
  gtk_widget_show (appt->appStartMinutes_spinbutton);
  gtk_box_pack_start (GTK_BOX (appt->appStartTime_hbox), appt->appStartMinutes_spinbutton, FALSE, TRUE, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (appt->appStartMinutes_spinbutton), TRUE);

  appt->appStartTime_fixed = gtk_fixed_new ();
  gtk_widget_show (appt->appStartTime_fixed);
  gtk_box_pack_start (GTK_BOX (appt->appStartTime_hbox), appt->appStartTime_fixed, TRUE, TRUE, 0);

  appt->appEndTime_hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (appt->appEndTime_hbox);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appEndTime_hbox, 1, 2, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

    appt->appEndYear_spinbutton_adj = gtk_adjustment_new(2005, 1971, 2150, 1, 10, 10);
    appt->appEndYear_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (appt->appEndYear_spinbutton_adj), 1, 0);
    gtk_widget_show (appt->appEndYear_spinbutton);
    gtk_box_pack_start (GTK_BOX (appt->appEndTime_hbox), appt->appEndYear_spinbutton, FALSE, TRUE, 0);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (appt->appEndYear_spinbutton), TRUE);

    appt->appEndSlash1_label = gtk_label_new (_(" / "));
    gtk_widget_show (appt->appEndSlash1_label);
    gtk_box_pack_start (GTK_BOX (appt->appEndTime_hbox), appt->appEndSlash1_label, FALSE, FALSE, 0);
    gtk_misc_set_alignment (GTK_MISC (appt->appEndSlash1_label), 0.5, 0.43);

    appt->appEndMonth_spinbutton_adj = gtk_adjustment_new(1, 1, 12, 1, 10, 10);
    appt->appEndMonth_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (appt->appEndMonth_spinbutton_adj), 1, 0);
    gtk_widget_show (appt->appEndMonth_spinbutton);
    gtk_box_pack_start (GTK_BOX (appt->appEndTime_hbox), appt->appEndMonth_spinbutton, FALSE, TRUE, 0);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (appt->appEndMonth_spinbutton), TRUE);

    appt->appEndSlash2_label = gtk_label_new (_(" / "));
    gtk_widget_show (appt->appEndSlash2_label);
    gtk_box_pack_start (GTK_BOX (appt->appEndTime_hbox), appt->appEndSlash2_label, FALSE, FALSE, 0);
    gtk_misc_set_alignment (GTK_MISC (appt->appEndSlash2_label), 0.5, 0.43);

    appt->appEndDay_spinbutton_adj = gtk_adjustment_new(1, 1, 31, 1, 10, 10);
    appt->appEndDay_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (appt->appEndDay_spinbutton_adj), 1, 0);
    gtk_widget_show (appt->appEndDay_spinbutton);
    gtk_box_pack_start (GTK_BOX (appt->appEndTime_hbox), appt->appEndDay_spinbutton, FALSE, TRUE, 0);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (appt->appEndDay_spinbutton), TRUE);

  appt->appEndHour_spinbutton_adj = gtk_adjustment_new (0, 0, 23, 1, 10, 10);
  appt->appEndHour_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (appt->appEndHour_spinbutton_adj), 1, 0);
  gtk_widget_show (appt->appEndHour_spinbutton);
  gtk_box_pack_start (GTK_BOX (appt->appEndTime_hbox), appt->appEndHour_spinbutton, FALSE, TRUE, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (appt->appEndHour_spinbutton), TRUE);

  appt->appEndColumn_label = gtk_label_new (_(" : "));
  gtk_widget_show (appt->appEndColumn_label);
  gtk_box_pack_start (GTK_BOX (appt->appEndTime_hbox), appt->appEndColumn_label, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (appt->appEndColumn_label), 0.5, 0.43);

  appt->appEndMinutes_spinbutton_adj = gtk_adjustment_new (0, 0, 59, 15, 10, 10);
  appt->appEndMinutes_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (appt->appEndMinutes_spinbutton_adj), 1, 0);
  gtk_widget_show (appt->appEndMinutes_spinbutton);
  gtk_box_pack_start (GTK_BOX (appt->appEndTime_hbox), appt->appEndMinutes_spinbutton, FALSE, TRUE, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (appt->appEndMinutes_spinbutton), TRUE);

  appt->appEndTime_fixed = gtk_fixed_new ();
  gtk_widget_show (appt->appEndTime_fixed);
  gtk_box_pack_start (GTK_BOX (appt->appEndTime_hbox), appt->appEndTime_fixed, TRUE, TRUE, 0);

  /*
  appt->appPrivate_check = gtk_check_button_new_with_mnemonic ("");
  gtk_widget_show (appt->appPrivate_check);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appPrivate_check, 1, 2, 5, 6,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  */

    appt->appAlarm_combobox = gtk_combo_box_new_text ();
    gtk_widget_show (appt->appAlarm_combobox);
    gtk_table_attach (GTK_TABLE (appt->appTable), appt->appAlarm_combobox, 1, 2, 6, 7,
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
    gtk_table_attach (GTK_TABLE (appt->appTable), appt->appSound_hbox, 1, 2, 8, 9,
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
    gtk_table_attach (GTK_TABLE (appt->appTable), appt->appSoundRepeat_checkbutton, 1, 2, 9, 10,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  appt->appRecurrency_cb = gtk_combo_box_new_text ();
  gtk_widget_show (appt->appRecurrency_cb);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appRecurrency_cb, 1, 2, 10, 11,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_combo_box_append_text (GTK_COMBO_BOX (appt->appRecurrency_cb), _("None"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (appt->appRecurrency_cb), _("Daily"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (appt->appRecurrency_cb), _("Weekly"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (appt->appRecurrency_cb), _("Monthly"));

  appt->appAvailability_cb = gtk_combo_box_new_text ();
  gtk_widget_show (appt->appAvailability_cb);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appAvailability_cb, 1, 2, 11, 12,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_combo_box_append_text (GTK_COMBO_BOX (appt->appAvailability_cb), _("Free"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (appt->appAvailability_cb), _("Busy"));

  appt->appNote_Scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (appt->appNote_Scrolledwindow);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appNote_Scrolledwindow, 1, 2, 12, 13,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (appt->appNote_Scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (appt->appNote_Scrolledwindow), GTK_SHADOW_IN);

  appt->appNote_textview = gtk_text_view_new ();
  gtk_widget_show (appt->appNote_textview);
  gtk_container_add (GTK_CONTAINER (appt->appNote_Scrolledwindow), appt->appNote_textview);

  appt->appHBox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (appt->appHBox1);
  gtk_box_pack_start (GTK_BOX (appt->appVBox1), appt->appHBox1, FALSE, TRUE, 0);

  appt->appRemove = gtk_button_new_from_stock ("gtk-remove");
  gtk_widget_show (appt->appRemove);
  gtk_box_pack_start (GTK_BOX (appt->appHBox1), appt->appRemove, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (appt->appRemove), 10);

  appt->appAdd = gtk_button_new_from_stock ("gtk-copy");
  gtk_widget_show (appt->appAdd);
  gtk_box_pack_start (GTK_BOX (appt->appHBox1), appt->appAdd, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (appt->appAdd), 10);

  appt->appBottom_fixed = gtk_fixed_new ();
  gtk_widget_show (appt->appBottom_fixed);
  gtk_box_pack_start (GTK_BOX (appt->appHBox1), appt->appBottom_fixed, TRUE, TRUE, 0);

  appt->appClose = gtk_button_new_from_stock ("gtk-close");
  gtk_widget_show (appt->appClose);
  gtk_box_pack_start (GTK_BOX (appt->appHBox1), appt->appClose, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (appt->appClose), 10);
  GTK_WIDGET_SET_FLAGS (appt->appClose, GTK_CAN_DEFAULT);

  gtk_widget_grab_default (appt->appClose);

    g_signal_connect ((gpointer) appt->appAllDay_checkbutton, "clicked",
            G_CALLBACK (on_appAllDay_clicked_cb),
            appt);

    g_signal_connect ((gpointer) appt->appSound_button, "clicked",
            G_CALLBACK (on_appSound_button_clicked_cb),
            appt);

    g_signal_connect ((gpointer) appt->appClose, "clicked",
            G_CALLBACK (on_appClose_clicked_cb),
            appt);

    g_signal_connect ((gpointer) appt->appRemove, "clicked",
            G_CALLBACK (on_appRemove_clicked_cb),
            appt);

    g_signal_connect ((gpointer) appt->appAdd, "clicked",
            G_CALLBACK (on_appCopy_clicked_cb),
            appt);

    /* Take care of the title entry to build the appointment window title 
     * Beware: we are not using appt->appTitle_entry as a GtkEntry here 
     * but as an interface GtkEditable instead.
     */
    g_signal_connect_after ((gpointer) appt->appTitle_entry, "changed",
            G_CALLBACK (on_appTitle_entry_changed_cb),
            appt);

  fill_appt_window(appt, action, par);

  return appt;
}
