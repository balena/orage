/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2006-2013 Juha Kautto  (juha at xfce.org)
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

#ifndef __ORAGE_PARAMETERS_INTERNAL_H__
#define __ORAGE_PARAMETERS_INTERNAL_H__

typedef struct _Itf
{
    GtkWidget *orage_dialog;
    GtkWidget *dialog_vbox1;
    GtkWidget *notebook;

    /* Tabs */
    /***** Main Tab *****/
    GtkWidget *setup_tab;
    GtkWidget *setup_tab_label;
    GtkWidget *setup_vbox;
    /* Choose the timezone for appointments */
    GtkWidget *timezone_frame;
    GtkWidget *timezone_button;
    /* Archive period */
#ifdef HAVE_ARCHIVE
    GtkWidget *archive_threshold_frame;
    GtkWidget *archive_threshold_spin;
#endif
    /* Choose the sound application for reminders */
    GtkWidget *sound_application_frame;
    GtkWidget *sound_application_entry;
    GtkWidget *sound_application_open_button;

    /***** Calendar Window Tab *****/
    GtkWidget *calendar_tab;
    GtkWidget *calendar_tab_label;
    GtkWidget *calendar_vbox;
    /* Show  border, menu and set stick, ontop */
    GtkWidget *mode_frame;
    GtkWidget *show_borders_checkbutton;
    GtkWidget *show_menu_checkbutton;
    GtkWidget *show_heading_checkbutton;
    GtkWidget *show_day_names_checkbutton;
    GtkWidget *show_weeks_checkbutton;
    /* Show in... taskbar pager systray */
    GtkWidget *appearance_frame;
    GtkWidget *set_stick_checkbutton;
    GtkWidget *set_ontop_checkbutton;
    GtkWidget *show_taskbar_checkbutton;
    GtkWidget *show_pager_checkbutton;
    GtkWidget *show_systray_checkbutton;
    /* info boxes */
    GtkWidget *info_frame;
    GtkWidget *show_todos_checkbutton;
    GtkWidget *show_events_spin;
    /* Start visibity show or hide */
    GtkWidget *visibility_frame;
    GSList    *visibility_radiobutton_group;
    GtkWidget *visibility_show_radiobutton;
    GtkWidget *visibility_hide_radiobutton;
    GtkWidget *visibility_minimized_radiobutton;
    /* select_always_today */
    GtkWidget *select_day_frame;
    GSList    *select_day_radiobutton_group;
    GtkWidget *select_day_today_radiobutton;
    GtkWidget *select_day_old_radiobutton;
    /* show eventlist/days window from main calendar */
    GtkWidget *click_to_show_frame;
    GSList    *click_to_show_radiobutton_group;
    GtkWidget *click_to_show_days_radiobutton;
    GtkWidget *click_to_show_events_radiobutton;

    /***** Extra Tab *****/
    GtkWidget *extra_tab;
    GtkWidget *extra_tab_label;
    GtkWidget *extra_vbox;
    /* number of extra days to show */
    GtkWidget *el_extra_days_frame;
    GtkWidget *el_extra_days_spin;
    /* day view week mode */
    GtkWidget *dw_week_mode_frame;
    GSList    *dw_week_mode_radiobutton_group;
    GtkWidget *dw_week_mode_week_radiobutton;
    GtkWidget *dw_week_mode_day_radiobutton;
    /* Dynamic icon */
    GtkWidget *use_dynamic_icon_frame;
    GtkWidget *use_dynamic_icon_checkbutton;
    /* Use wakeup timer for suspend */
    GtkWidget *use_wakeup_timer_frame;
    GtkWidget *use_wakeup_timer_checkbutton;
    /* default foreign file display alarm */
    GtkWidget *foreign_alarm_frame;
    GSList    *foreign_alarm_radiobutton_group;
    GtkWidget *foreign_alarm_orage_radiobutton;
    GtkWidget *foreign_alarm_notification_radiobutton;

    /***** the rest in all tabs *****/
    GtkWidget *close_button;
    GtkWidget *help_button;
} Itf;


#endif /* !__ORAGE_PARAMETERS_INTERNAL_H__ */
