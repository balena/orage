/* appointment.h
 *
 * Copyright (C) 2004 Mickaël Graf <korbinus@lunar-linux.org>
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


typedef struct
{
    gchar *title,
        *location;

    gint alarmtime,
        availability;

    gboolean allDay;

    gchar *note;
    gchar *sound;
    gchar *uid;

        /* time format must be:
         * yyyymmdd[Thhmiss[Z]] = %04d%02d%02dT%02d%02d%02d
         * T means it has also time part
         * Z means it is in UTC format
         */
    gchar 
        starttime[17],
        endtime[17];

} appt_type;

#define XF_APP_TIME_FORMAT "%04d%02d%02dT%02d%02d%02d"
#define XF_APP_DATE_FORMAT "%04d%02d%02d"

typedef struct
{
    GtkWidget *appWindow;
    GtkWidget *appVBox1;
    GtkWidget *appTable;
    GtkWidget *appTitle_label;
    GtkWidget *appLocation_label;
    GtkWidget *appStart;
    GtkWidget *appEnd;
    GtkWidget *appPrivate;
    GtkWidget *appAlarm;
    GtkWidget *appRecurrence;
    GtkWidget *appNote;
    GtkWidget *appAvailability;
    GtkWidget *appTitle_entry;
    GtkWidget *appLocation_entry;
    GtkWidget *appPrivate_check;
    GtkWidget *appNote_Scrolledwindow;
    GtkWidget *appNote_textview;
    GtkWidget *appAvailability_cb;
    GtkWidget *appAlarm_combobox;
    GtkWidget *appAllDay_checkbutton;
    GtkObject *appStartYear_spinbutton_adj;
    GtkWidget *appStartYear_spinbutton;
    GtkWidget *appStartSlash1_label;
    GtkObject *appStartMonth_spinbutton_adj;
    GtkWidget *appStartMonth_spinbutton;
    GtkWidget *appStartSlash2_label;
    GtkObject *appStartDay_spinbutton_adj;
    GtkWidget *appStartDay_spinbutton;
    GtkWidget *appStartTime_hbox;
    GtkObject *appStartHour_spinbutton_adj;
    GtkWidget *appStartHour_spinbutton;
    GtkWidget *appStartColumn_label;
    GtkObject *appStartMinutes_spinbutton_adj;
    GtkWidget *appStartMinutes_spinbutton;
    GtkWidget *appStartTime_fixed;
    GtkObject *appEndYear_spinbutton_adj;
    GtkWidget *appEndYear_spinbutton;
    GtkWidget *appEndSlash1_label;
    GtkObject *appEndMonth_spinbutton_adj;
    GtkWidget *appEndMonth_spinbutton;
    GtkWidget *appEndSlash2_label;
    GtkObject *appEndDay_spinbutton_adj;
    GtkWidget *appEndDay_spinbutton;
    GtkWidget *appEndTime_hbox;
    GtkObject *appEndHour_spinbutton_adj;
    GtkWidget *appEndHour_spinbutton;
    GtkWidget *appEndColumn_label;
    GtkObject *appEndMinutes_spinbutton_adj;
    GtkWidget *appEndMinutes_spinbutton;
    GtkWidget *appEndTime_fixed;
    GtkWidget *appSound_chooserbutton;
    GtkWidget *appHBox1;
    GtkWidget *appRemove;
    GtkWidget *appBottom_fixed;
    GtkWidget *appClose;

    gchar *xf_uid;
    GtkWidget *wAppointment; /* event-list window */

} appt_win;

void fill_appt_window(appt_win *appt, char *action, char *par);

appt_win *create_appt_win(char *action, char *par, GtkWidget *wAppointment);



