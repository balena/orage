/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2005-2007 Juha Kautto  (juha at xfce.org)
 * Copyright (c) 2004-2006 Mickael Graf (korbinus at xfce.org)
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

#ifndef __APPOINTMENT_H__
#define __APPOINTMENT_H__

#define XFICAL_APPT_TIME_FORMAT "%04d%02d%02dT%02d%02d%02d"
#define XFICAL_APPT_DATE_FORMAT "%04d%02d%02d"

typedef struct
{
    gchar *uid;

    xfical_type type;
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
    gboolean completed;
    gchar  completedtime[17];
    gchar *completed_tz_loc;

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

typedef struct _appt_win
{
    GtkWidget *Window;
    GtkWidget *Vbox;

    GtkWidget *Menubar;
    GtkWidget *Revert;
    GtkWidget *Delete;
    GtkWidget *Duplicate;
    GtkWidget *Save;
    GtkWidget *SaveClose;

    GtkWidget *File_menu;
    GtkWidget *File_menu_save;
    GtkWidget *File_menu_saveclose;
    GtkWidget *File_menu_revert;
    GtkWidget *File_menu_duplicate;
    GtkWidget *File_menu_delete;
    GtkWidget *File_menu_close;

    GtkWidget *Toolbar;

    GtkWidget *Notebook;
    GtkWidget *General_notebook_page;
    GtkWidget *General_tab_label;
    GtkWidget *TableGeneral;
    GtkWidget *Type_label;
    GtkWidget *Type_event_rb;
    GtkWidget *Type_todo_rb;
    GtkWidget *Type_journal_rb;
    GtkWidget *Title_label;
    GtkWidget *Title_entry;
    GtkWidget *Location_label;
    GtkWidget *Location_entry;
    GtkWidget *AllDay_checkbutton;
    GtkWidget *Start_label;
    GtkWidget *StartDate_button;
    GtkWidget *StartTime_hbox;
    GtkWidget *StartTime_spin_hh;
    GtkWidget *StartTime_spin_mm;
    GtkWidget *StartTimezone_button;
    GtkWidget *End_label;
    GtkWidget *EndDate_button;
    GtkWidget *EndTime_hbox;
    GtkWidget *EndTime_spin_hh;
    GtkWidget *EndTime_spin_mm;
    GtkWidget *EndTimezone_button;
    GtkWidget *Dur_hbox;
    GtkWidget *Dur_checkbutton;
    GtkWidget *Dur_spin_dd;
    GtkWidget *Dur_spin_dd_label;
    GtkWidget *Dur_spin_hh;
    GtkWidget *Dur_spin_hh_label;
    GtkWidget *Dur_spin_mm;
    GtkWidget *Dur_spin_mm_label;
    GtkWidget *Completed_label;
    GtkWidget *Completed_hbox;
    GtkWidget *Completed_checkbutton;
    GtkWidget *CompletedDate_button;
    GtkWidget *CompletedTime_hbox;
    GtkWidget *CompletedTime_spin_hh;
    GtkWidget *CompletedTime_spin_mm;
    GtkWidget *CompletedTimezone_button;
    GtkWidget *Availability_label;
    GtkWidget *Availability_cb;
    GtkWidget *Note;
    GtkWidget *Note_Scrolledwindow;
    GtkWidget *Note_textview;
    GtkTextBuffer *Note_buffer;

    GtkWidget *Alarm_notebook_page;
    GtkWidget *Alarm_tab_label;
    GtkWidget *TableAlarm;
    GtkWidget *Alarm;
    GtkWidget *Alarm_hbox;
    GtkWidget *Alarm_spin_dd;
    GtkWidget *Alarm_spin_hh;
    GtkWidget *Alarm_spin_mm;

    GtkWidget *Recur_notebook_page;
    GtkWidget *Recur_tab_label;
    GtkWidget *TableRecur;
    GtkWidget *Recur_feature_label;
    GtkWidget *Recur_feature_hbox;
    GtkWidget *Recur_feature_normal_rb;
    GtkWidget *Recur_feature_advanced_rb;
    GtkWidget *Recur_freq_label;
    GtkWidget *Recur_freq_cb;
    GtkWidget *Recur_limit_rb;
    GtkWidget *Recur_limit_label;
    GtkWidget *Recur_count_hbox;
    GtkWidget *Recur_count_rb;
    GtkWidget *Recur_count_spin;
    GtkWidget *Recur_count_label;
    GtkWidget *Recur_until_hbox;
    GtkWidget *Recur_until_rb;
    GtkWidget *Recur_until_button;
    GtkWidget *Recur_byday_label;
    GtkWidget *Recur_byday_hbox;
    GtkWidget *Recur_byday_cb[7];    /* 0=Mo, 1=Tu ... 6=Su */
    GtkWidget *Recur_byday_spin_label;
    GtkWidget *Recur_byday_spin_hbox;
    GtkWidget *Recur_byday_spin[7];  /* 0=Mo, 1=Tu ... 6=Su */
    GtkWidget *Recur_int_label;
    GtkWidget *Recur_int_hbox;
    GtkWidget *Recur_int_spin;
    GtkWidget *Recur_int_spin_label1;
    GtkWidget *Recur_int_spin_label2;
    GtkWidget *Sound_label;
    GtkWidget *Sound_hbox;
    GtkWidget *Sound_entry;
    GtkWidget *Sound_button;
    GtkWidget *SoundRepeat_checkbutton;

    GtkAccelGroup *accel_group;
    GtkTooltips *Tooltips;
    appt_data *appt;
    gchar *xf_uid;
    gchar *par;
    el_win *el; 
    gboolean appointment_add;       /* are we adding app */
    gboolean appointment_changed;   /* has this app been modified now */
    gboolean appointment_new;       /* is this new = no uid yet */
    /* COPY uses old uid as base and adds new, so 
     * add == TRUE && new == FALSE */
} appt_win;

appt_win *create_appt_win(char *action, char *par, el_win *el);

#endif /* !__APPOINTMENT_H__ */
