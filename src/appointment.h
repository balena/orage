/* appointment.h
 *
 * Copyright (c) 2004-2006 Mickaël Graf <korbinus@xfce.org>
 * Copyright (c) 2005-2006 Juha Kautto <juha@xfce.org>
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

#ifndef __APPOINTMENT_H__
#define __APPOINTMENT_H__

#define XFICAL_APPT_TIME_FORMAT "%04d%02d%02dT%02d%02d%02d"
#define XFICAL_APPT_DATE_FORMAT "%04d%02d%02d"

typedef enum 
{
    XFICAL_FREQ_NONE = 0
    ,XFICAL_FREQ_DAILY
    ,XFICAL_FREQ_WEEKLY
    ,XFICAL_FREQ_MONTHLY
    ,XFICAL_FREQ_YEARLY
} xfical_freq;

typedef struct
{
    gchar *uid;

    gchar *title;
    gchar *location;

    gboolean allDay;

        /* time format must be:
         * yyyymmdd[Thhmiss[Z]] = %04d%02d%02dT%02d%02d%02d
         * T means it has also time part
         * Z means it is in UTC format
         */
    gchar  starttime[17];
    gchar *start_tz_loc;
    gchar  endtime[17];
    gchar *end_tz_loc;
    gboolean use_duration;
    gint   duration; 

    gint availability;
    gchar *note;

        /* alarm */
    gint alarmtime;
    gchar *sound;
    gboolean alarmrepeat;

        /* for repeating events cur times show current repeating event.
         * normal times are always the real (=first) start and end times 
         */
    gchar  starttimecur[17]; 
    gchar  endtimecur[17];
    xfical_freq freq;
    gint   recur_limit; /* 0 = no limit  1 = count  2 = until */
    gint   recur_count;
    gchar  recur_until[17];
    gboolean recur_byday[7]; /* 0=Mo, 1=Tu, 2=We, 3=Th, 4=Fr, 5=Sa, 6=Su */
    gint    recur_byday_cnt[7]; /* monthly/early: 1=first -1=last 2=second... */
    gint   interval;    /* 1=every day/week..., 2=every second day/week,... */
} appt_data;

typedef struct
{
    GtkWidget *appWindow;
    GtkAccelGroup *appAccelgroup;
    GtkWidget *appVBox1;
    GtkWidget *appMenubar;
    GtkWidget *appFile_menu;
    GtkWidget *appFileSave_menuitem;
    GtkWidget *appFileSaveClose_menuitem;
    GtkWidget *appFileRevert_menuitem;
    GtkWidget *appFileDuplicate_menuitem;
    GtkWidget *appFileDelete_menuitem;
    GtkWidget *appFileClose_menuitem;
    GtkWidget *appToolbar;
    GtkTooltips *appTooltips;
    GtkWidget *appNotebook;
    GtkWidget *appGeneral_notebook_page;
    GtkWidget *appGeneral_tab_label;
    GtkWidget *appAlarm_notebook_page;
    GtkWidget *appAlarm_tab_label;
    GtkWidget *appRecur_notebook_page;
    GtkWidget *appRecur_tab_label;
    GtkWidget *appTableGeneral;
    GtkWidget *appTableAlarm;
    GtkWidget *appTableRecur;
    GtkWidget *appTitle_label;
    GtkWidget *appLocation_label;
    GtkWidget *appStart;
    GtkWidget *appEnd;
    GtkWidget *appPrivate;
    GtkWidget *appAlarm;
    GtkWidget *appAlarm_hbox;
    GtkWidget *appAlarm_spin_dd;
    GtkWidget *appAlarm_spin_hh;
    GtkWidget *appAlarm_spin_mm;
    GtkWidget *appNote;
    GtkWidget *appAvailability;
    GtkWidget *appTitle_entry;
    GtkWidget *appLocation_entry;
    GtkWidget *appPrivate_check;
    GtkWidget *appNote_Scrolledwindow;
    GtkWidget *appNote_textview;
    GtkTextBuffer *appNote_buffer;
    GtkWidget *appRecur_feature_label;
    GtkWidget *appRecur_feature_hbox;
    GtkWidget *appRecur_feature_normal_rb;
    GtkWidget *appRecur_feature_advanced_rb;
    GtkWidget *appRecur_freq_label;
    GtkWidget *appRecur_freq_cb;
    GtkWidget *appRecur_limit_rb;
    GtkWidget *appRecur_limit_label;
    GtkWidget *appRecur_count_hbox;
    GtkWidget *appRecur_count_rb;
    GtkWidget *appRecur_count_spin;
    GtkWidget *appRecur_count_label;
    GtkWidget *appRecur_until_hbox;
    GtkWidget *appRecur_until_rb;
    GtkWidget *appRecur_until_button;
    GtkWidget *appRecur_byday_label;
    GtkWidget *appRecur_byday_hbox;
    GtkWidget *appRecur_byday_cb[7];    /* 0=Mo, 1=Tu ... 6=Su */
    GtkWidget *appRecur_byday_spin_label;
    GtkWidget *appRecur_byday_spin_hbox;
    GtkWidget *appRecur_byday_spin[7];  /* 0=Mo, 1=Tu ... 6=Su */
    GtkWidget *appRecur_int_label;
    GtkWidget *appRecur_int_hbox;
    GtkWidget *appRecur_int_spin;
    GtkWidget *appRecur_int_spin_label1;
    GtkWidget *appRecur_int_spin_label2;
    GtkWidget *appAvailability_cb;
    GtkWidget *appAllDay_checkbutton;
    GtkWidget *appStartDate_button;
    GtkWidget *appStartTime_hbox;
    GtkWidget *appStartTime_spin_hh;
    GtkWidget *appStartTime_spin_mm;
    GtkWidget *appStartTimezone_button;
    GtkWidget *appEndDate_button;
    GtkWidget *appEndTime_hbox;
    GtkWidget *appEndTime_spin_hh;
    GtkWidget *appEndTime_spin_mm;
    GtkWidget *appEndTimezone_button;
    GtkWidget *appDur_hbox;
    GtkWidget *appDur_checkbutton;
    GtkWidget *appDur_spin_dd;
    GtkWidget *appDur_spin_dd_label;
    GtkWidget *appDur_spin_hh;
    GtkWidget *appDur_spin_hh_label;
    GtkWidget *appDur_spin_mm;
    GtkWidget *appDur_spin_mm_label;
    GtkWidget *appSound_label;
    GtkWidget *appSound_hbox;
    GtkWidget *appSound_entry;
    GtkWidget *appSound_button;
    GtkWidget *appSoundRepeat_checkbutton;
    GtkWidget *appHBox1;
    GtkWidget *appRevert;
    GtkWidget *appDelete;
    GtkWidget *appDuplicate;
    GtkWidget *appSave;
    GtkWidget *appSaveClose;

    gchar *xf_uid;
    gchar *par;
    eventlist_win *eventlist; 
    gboolean appointment_add;       /* are we adding app */
    gboolean appointment_changed;   /* has this app been modified now */
    gboolean appointment_new;       /* is this new = no uid yet */
    /* COPY uses old uid as base and adds new, so 
     * add == TRUE && new == FALSE */
} appt_win;

appt_win 
*create_appt_win(char *action, char *par, eventlist_win *el);

#endif /* !__APPOINTMENT_H__ */
