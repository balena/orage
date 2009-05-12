/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2005-2008 Juha Kautto  (juha at xfce.org)
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

typedef struct _appt_win
{
    GtkAccelGroup *accel_group;
    GtkTooltips *Tooltips;

    GtkWidget *Window;
    GtkWidget *Vbox;

    GtkWidget *Menubar;
    GtkWidget *File_menu;
    GtkWidget *File_menu_save;
    GtkWidget *File_menu_saveclose;
    GtkWidget *File_menu_revert;
    GtkWidget *File_menu_duplicate;
    GtkWidget *File_menu_delete;
    GtkWidget *File_menu_close;

    GtkWidget *Toolbar;
    GtkWidget *Revert;
    GtkWidget *Delete;
    GtkWidget *Duplicate;
    GtkWidget *Save;
    GtkWidget *SaveClose;

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
    GtkWidget *StartTime_hbox;
    GtkWidget *StartDate_button;
    GtkWidget *StartTime_spin_hh;
    GtkWidget *StartTime_spin_mm;
    GtkWidget *StartTimezone_button;
    GtkWidget *End_label;
    GtkWidget *End_hbox;
    GtkWidget *End_checkbutton;
    GtkWidget *EndTime_hbox;
    GtkWidget *EndDate_button;
    GtkWidget *EndTime_spin_hh;
    GtkWidget *EndTime_spin_mm;
    GtkWidget *EndTimezone_button;
    GtkWidget *Dur_hbox;
    GtkWidget *Dur_checkbutton;
    GtkWidget *Dur_time_hbox;
    GtkWidget *Dur_spin_dd;
    GtkWidget *Dur_spin_dd_label;
    GtkWidget *Dur_spin_hh;
    GtkWidget *Dur_spin_hh_label;
    GtkWidget *Dur_spin_mm;
    GtkWidget *Dur_spin_mm_label;
    GtkWidget *Completed_label;
    GtkWidget *Completed_hbox;
    GtkWidget *Completed_checkbutton;
    GtkWidget *CompletedTime_hbox;
    GtkWidget *CompletedDate_button;
    GtkWidget *CompletedTime_spin_hh;
    GtkWidget *CompletedTime_spin_mm;
    GtkWidget *CompletedTimezone_button;
    GtkWidget *Availability_label;
    GtkWidget *Availability_cb;
    GtkWidget *Categories_label;
    GtkWidget *Categories_hbox;
    GtkWidget *Categories_entry;
    GtkWidget *Categories_cb;
    GtkWidget *Categories_cb_event;
    GtkWidget *Categories_button;
    GtkWidget *Priority_label;
    GtkWidget *Priority_spin;
    GtkWidget *Note;
    GtkWidget *Note_Scrolledwindow;
    GtkWidget *Note_textview;
    GtkTextBuffer *Note_buffer;

    GtkWidget *Alarm_notebook_page;
    GtkWidget *Alarm_tab_label;
    GtkWidget *TableAlarm;
    GtkWidget *Alarm_label;
    GtkWidget *Alarm_hbox;
    GtkWidget *Alarm_time_hbox;
    GtkWidget *Alarm_spin_dd;
    GtkWidget *Alarm_spin_dd_label;
    GtkWidget *Alarm_spin_hh;
    GtkWidget *Alarm_spin_hh_label;
    GtkWidget *Alarm_spin_mm;
    GtkWidget *Alarm_spin_mm_label;
    GtkWidget *Alarm_when_cb;
    GtkWidget *Per_hbox;
    GtkWidget *Per_checkbutton;
    GtkWidget *Sound_label;
    GtkWidget *Sound_hbox;
    GtkWidget *Sound_checkbutton;
    GtkWidget *Sound_entry;
    GtkWidget *Sound_button;
    GtkWidget *SoundRepeat_hbox;
    GtkWidget *SoundRepeat_checkbutton;
    GtkWidget *SoundRepeat_spin_cnt;
    GtkWidget *SoundRepeat_spin_cnt_label;
    GtkWidget *SoundRepeat_spin_len;
    GtkWidget *SoundRepeat_spin_len_label;
    GtkWidget *Display_label;
    GtkWidget *Display_hbox_orage;
    GtkWidget *Display_checkbutton_orage;
#ifdef HAVE_NOTIFY
    GtkWidget *Display_hbox_notify;
    GtkWidget *Display_checkbutton_notify;
    GtkWidget *Display_checkbutton_expire_notify;
    GtkWidget *Display_spin_expire_notify;
    GtkWidget *Display_spin_expire_notify_label;
#endif
    GtkWidget *Proc_label;
    GtkWidget *Proc_hbox;
    GtkWidget *Proc_checkbutton;
    GtkWidget *Proc_entry;
    GtkWidget *Default_label;
    GtkWidget *Default_hbox;
    GtkWidget *Default_savebutton;
    GtkWidget *Default_readbutton;

    GtkWidget *Recur_notebook_page;
    GtkWidget *Recur_tab_label;
    GtkWidget *TableRecur;
    GtkWidget *Recur_feature_label;
    GtkWidget *Recur_feature_hbox;
    GtkWidget *Recur_feature_normal_rb;
    GtkWidget *Recur_feature_advanced_rb;
    GtkWidget *Recur_freq_label;
    GtkWidget *Recur_freq_hbox;
    GtkWidget *Recur_freq_cb;
    GtkWidget *Recur_int_spin;
    GtkWidget *Recur_int_spin_label1;
    GtkWidget *Recur_int_spin_label2;
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
    GtkWidget *Recur_todo_base_label;
    GtkWidget *Recur_todo_base_hbox;
    GtkWidget *Recur_todo_base_start_rb;
    GtkWidget *Recur_todo_base_done_rb;
    GtkWidget *Recur_exception_label;
    GtkWidget *Recur_exception_hbox;
    GtkWidget *Recur_exception_scroll_win;
    GtkWidget *Recur_exception_rows_vbox;
    GtkWidget *Recur_exception_type_vbox;
    GtkWidget *Recur_exception_excl_rb;
    GtkWidget *Recur_exception_incl_rb;
    GtkWidget *Recur_exception_incl_time_hbox;;
    GtkWidget *Recur_exception_incl_spin_hh;
    GtkWidget *Recur_exception_incl_spin_mm;
    GtkWidget *Recur_calendar_label;
    GtkWidget *Recur_calendar_hbox;
    GtkWidget *Recur_calendar1;
    GtkWidget *Recur_calendar2;
    GtkWidget *Recur_calendar3;

    void *xf_appt; /* this is xfical_appt * */
    gchar *xf_uid;
    gchar *par;
    void  *el;          /* used to refresh calling event list */
    void  *dw;          /* used to refresh calling day list */
    gboolean appointment_add;       /* are we adding app */
    gboolean appointment_changed;   /* has this app been modified now */
    gboolean appointment_new;       /* is this new = no uid yet */
    /* COPY uses old uid as base and adds new, so 
     * add == TRUE && new == FALSE */
} appt_win;

appt_win *create_appt_win(char *action, char *par);

GdkColor *orage_category_list_contains(char *categories);
void orage_category_get_list();

#endif /* !__APPOINTMENT_H__ */
