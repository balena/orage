/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2006-2007 Juha Kautto  (juha at xfce.org)
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
#include <config.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4util/libxfce4util.h>

#include "functions.h"
#include "tray_icon.h"
#include "ical-code.h"
#include "parameters.h"


static gboolean is_running = FALSE;

typedef struct _Itf
{
    GtkWidget *orage_dialog;
    GtkWidget *dialog_vbox1;
    GtkWidget *notebook;

    /* Tabs */
    GtkWidget *setup_tab;
    GtkWidget *setup_tab_label;
    GtkWidget *setup_vbox;
    /* Choose the timezone for appointments */
    GtkWidget *timezone_frame;
    GtkWidget *timezone_button;
    /* Archive period */
    GtkWidget *archive_threshold_frame;
    GtkWidget *archive_threshold_spin;
    /* Choose the sound application for reminders */
    GtkWidget *sound_application_frame;
    GtkWidget *sound_application_entry;
    GtkWidget *sound_application_open_button;

    GtkWidget *display_tab;
    GtkWidget *display_tab_label;
    GtkWidget *display_vbox;
    /* Show  border, menu and set stick, ontop */
    GtkWidget *mode_frame;
    GtkWidget *show_borders_checkbutton;
    GtkWidget *show_menu_checkbutton;
    GtkWidget *set_stick_checkbutton;
    GtkWidget *set_ontop_checkbutton;
    /* Show in... taskbar pager systray */
    GtkWidget *show_taskbar_checkbutton;
    GtkWidget *show_pager_checkbutton;
    GtkWidget *show_systray_checkbutton;
    /* Start visibity show or hide */
    GtkWidget *visibility_frame;
    GSList    *visibility_radiobutton_group;
    GtkWidget *visibility_show_radiobutton;
    GtkWidget *visibility_hide_radiobutton;
    GtkWidget *visibility_minimized_radiobutton;

    GtkWidget *extra_tab;
    GtkWidget *extra_tab_label;
    GtkWidget *extra_vbox;
    /* select_always_today */
    GtkWidget *always_today_frame;
    GtkWidget *always_today_checkbutton;
    /* ical week start day (0 = Monday, 1 = Tuesday,... 6 = Sunday) */
    GtkWidget *ical_weekstartday_frame;
    GtkWidget *ical_weekstartday_combobox;
    /* icon size */
    GtkWidget *icon_size_frame;
    GtkWidget *icon_size_x_spin;
    GtkWidget *icon_size_y_spin;

    /* the rest */
    GtkWidget *close_button;
    GtkWidget *help_button;
    GtkWidget *dialog_action_area1;
} Itf;


static void dialog_response(GtkWidget *dialog, gint response_id)
{
    gchar *helpdoc;

    if (response_id == GTK_RESPONSE_HELP) {
        helpdoc = g_strconcat("xfbrowser4 ", PACKAGE_DATA_DIR
                , G_DIR_SEPARATOR_S, "orage"
                , G_DIR_SEPARATOR_S, "doc"
                , G_DIR_SEPARATOR_S, "C"
                , G_DIR_SEPARATOR_S, "orage.html#orage-preferences-window"
                , NULL);
        xfce_exec(helpdoc, FALSE, FALSE, NULL);
    }
    else {
        write_parameters();
        is_running = FALSE;
        gtk_widget_destroy(dialog);
    }
}

static void sound_application_changed(GtkWidget *dialog, gpointer user_data)
{
    Itf *itf = (Itf *)user_data;
    
    if (g_par.sound_application)
        g_free(g_par.sound_application);
    g_par.sound_application = g_strdup(gtk_entry_get_text(
            GTK_ENTRY(itf->sound_application_entry)));
}

static void set_border()
{
    gtk_window_set_decorated(GTK_WINDOW(g_par.xfcal->mWindow)
            , g_par.show_borders);
}

static void borders_changed(GtkWidget *dialog, gpointer user_data)
{
    Itf *itf = (Itf *)user_data;

    g_par.show_borders = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
            itf->show_borders_checkbutton));
    set_border();
}

static void set_menu()
{
    if (g_par.show_menu)
        gtk_widget_show(g_par.xfcal->mMenubar);
    else
        gtk_widget_hide(g_par.xfcal->mMenubar);
}

static void menu_changed(GtkWidget *dialog, gpointer user_data)
{
    Itf *itf = (Itf *)user_data;

    g_par.show_menu = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
            itf->show_menu_checkbutton));
    set_menu();
}

static void set_stick()
{
    if (g_par.set_stick)
        gtk_window_stick(GTK_WINDOW(g_par.xfcal->mWindow));
    else
        gtk_window_unstick(GTK_WINDOW(g_par.xfcal->mWindow));
}

static void stick_changed(GtkWidget *dialog, gpointer user_data)
{
    Itf *itf = (Itf *)user_data;

    g_par.set_stick = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
            itf->set_stick_checkbutton));
    set_menu();
}

static void set_ontop()
{
    gtk_window_set_keep_above(GTK_WINDOW(g_par.xfcal->mWindow)
            , g_par.set_ontop);
}

static void ontop_changed(GtkWidget *dialog, gpointer user_data)
{
    Itf *itf = (Itf *)user_data;

    g_par.set_ontop = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
            itf->set_ontop_checkbutton));
    set_menu();
}

static void set_taskbar()
{
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(g_par.xfcal->mWindow)
            , !g_par.show_taskbar);
}

static void taskbar_changed(GtkWidget *dialog, gpointer user_data)
{
    Itf *itf = (Itf *)user_data;

    g_par.show_taskbar = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
            itf->show_taskbar_checkbutton));
    set_taskbar();
}

static void set_pager()
{
    gtk_window_set_skip_pager_hint(GTK_WINDOW(g_par.xfcal->mWindow)
            , !g_par.show_pager);
}

static void pager_changed(GtkWidget *dialog, gpointer user_data)
{
    Itf *itf = (Itf *)user_data;

    g_par.show_pager = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
            itf->show_pager_checkbutton));
    set_pager();
}

static void set_systray()
{
    if (!(g_par.trayIcon && NETK_IS_TRAY_ICON(g_par.trayIcon->tray))) {
        g_par.trayIcon = create_TrayIcon(g_par.xfcal);
    }

    if (g_par.show_systray)
        xfce_tray_icon_connect(g_par.trayIcon);
    else
        xfce_tray_icon_disconnect(g_par.trayIcon);
}

static void systray_changed(GtkWidget *dialog, gpointer user_data)
{
    Itf *itf = (Itf *)user_data;

    g_par.show_systray = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
            itf->show_systray_checkbutton));
    set_systray();
}

static void start_changed(GtkWidget *dialog, gpointer user_data)
{
    Itf *itf = (Itf *)user_data;

    g_par.start_visible = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
            itf->visibility_show_radiobutton));
    g_par.start_minimized = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
            itf->visibility_minimized_radiobutton));
}

static void sound_application_open_button_clicked(GtkButton *button
        , gpointer user_data)
{
    Itf *itf = (Itf *)user_data;
    GtkWidget *file_chooser;
    gchar *s; /* to avoid timing problems when updating entry */

    /* Create file chooser */
    file_chooser = gtk_file_chooser_dialog_new(_("Select a file...")
            , GTK_WINDOW(itf->orage_dialog), GTK_FILE_CHOOSER_ACTION_OPEN
            , GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL
            , GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT
            , NULL);

    /* Set sound application search path */
    if (g_par.sound_application == NULL || strlen(g_par.sound_application) == 0
    ||  g_par.sound_application[0] != '/'
    || ! gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(file_chooser)
                , g_par.sound_application))
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(file_chooser)
                , "/");
	if (gtk_dialog_run(GTK_DIALOG(file_chooser)) == GTK_RESPONSE_ACCEPT) {
		g_par.sound_application = gtk_file_chooser_get_filename(
                GTK_FILE_CHOOSER(file_chooser));
        if (g_par.sound_application) {
            s = g_strdup(g_par.sound_application);
            gtk_entry_set_text(GTK_ENTRY(itf->sound_application_entry)
                    , (const gchar *)s);
            g_free(s);
        }
    }
    gtk_widget_destroy(file_chooser);
}

static void timezone_button_clicked(GtkButton *button, gpointer user_data)
{
    Itf *itf = (Itf *)user_data;

    if (g_par.local_timezone == NULL || strlen(g_par.local_timezone) == 0) {
        g_warning("timezone pressed: local timezone missing");
        g_par.local_timezone = g_strdup("floating");
    }
    if (xfical_timezone_button_clicked(button, GTK_WINDOW(itf->orage_dialog)
            , &g_par.local_timezone))
        xfical_set_local_timezone();
}

static void archive_threshold_spin_changed(GtkSpinButton *sb
        , gpointer user_data)
{
    g_par.archive_limit = gtk_spin_button_get_value(sb);
}

static void always_today_changed(GtkWidget *dialog, gpointer user_data)
{
    Itf *itf = (Itf *)user_data;

    g_par.select_always_today = gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(itf->always_today_checkbutton));
}

static void ical_weekstartday_changed(GtkWidget *dialog, gpointer user_data)
{
    Itf *itf = (Itf *)user_data;

    g_par.ical_weekstartday = gtk_combo_box_get_active(
            GTK_COMBO_BOX(itf->ical_weekstartday_combobox));
}

static void set_icon_size()
{
    if (g_par.trayIcon && NETK_IS_TRAY_ICON(g_par.trayIcon->tray)) {
        /* refresh date in tray icon */
        xfce_tray_icon_disconnect(g_par.trayIcon);
        destroy_TrayIcon(g_par.trayIcon);
        g_par.trayIcon = create_TrayIcon(g_par.xfcal);
        xfce_tray_icon_connect(g_par.trayIcon);
    }
}

static void icon_size_x_spin_changed(GtkSpinButton *sb, gpointer user_data)
{
    g_par.icon_size_x = gtk_spin_button_get_value(sb);
    set_icon_size();
}

static void icon_size_y_spin_changed(GtkSpinButton *sb, gpointer user_data)
{
    g_par.icon_size_y = gtk_spin_button_get_value(sb);
    set_icon_size();
}

static void create_parameter_dialog_main_setup_tab(Itf *dialog)
{
    GtkWidget *hbox, *vbox, *label;

    dialog->setup_vbox = gtk_vbox_new(FALSE, 0);
    dialog->setup_tab = 
            xfce_create_framebox_with_content(NULL, dialog->setup_vbox);
    dialog->setup_tab_label = gtk_label_new(_("Main setups"));
    gtk_notebook_append_page(GTK_NOTEBOOK(dialog->notebook)
          , dialog->setup_tab, dialog->setup_tab_label);

    /* Choose a timezone to be used in appointments */
    vbox = gtk_vbox_new(TRUE, 0);
    dialog->timezone_frame = xfce_create_framebox_with_content(_("Timezone")
            , vbox);
    gtk_box_pack_start(GTK_BOX(dialog->setup_vbox)
            , dialog->timezone_frame, FALSE, FALSE, 5);

    dialog->timezone_button = gtk_button_new();
    if (g_par.local_timezone) {
        gtk_button_set_label(GTK_BUTTON(dialog->timezone_button)
                , _(g_par.local_timezone));
    }
    else { /* we should never arrive here */
        g_warning("parameters: timezone not set.");
        g_par.local_timezone = g_strdup("floating");
        gtk_button_set_label(GTK_BUTTON(dialog->timezone_button)
                , _("floating"));
    }
    gtk_box_pack_start(GTK_BOX(vbox)
            , dialog->timezone_button, FALSE, FALSE, 5);
    g_signal_connect(G_OBJECT(dialog->timezone_button), "clicked"
            , G_CALLBACK(timezone_button_clicked), dialog);

    /* Choose archiving threshold */
    hbox = gtk_hbox_new(FALSE, 0);
    dialog->archive_threshold_frame = 
            xfce_create_framebox_with_content(_("Archive threshold (months)")
                    , hbox);
    gtk_box_pack_start(GTK_BOX(dialog->setup_vbox)
            , dialog->archive_threshold_frame, FALSE, FALSE, 5);

    dialog->archive_threshold_spin = gtk_spin_button_new_with_range(0, 12, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->archive_threshold_spin)
            , g_par.archive_limit);
    gtk_box_pack_start(GTK_BOX(hbox)
            , dialog->archive_threshold_spin, FALSE, FALSE, 5);
    label = gtk_label_new(_("(0 = no archiving)"));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
    g_signal_connect(G_OBJECT(dialog->archive_threshold_spin), "value-changed"
            , G_CALLBACK(archive_threshold_spin_changed), dialog);

    /* Choose a sound application for reminders */
    hbox = gtk_hbox_new(FALSE, 0);
    dialog->sound_application_frame = 
            xfce_create_framebox_with_content(_("Sound command"), hbox);
    gtk_box_pack_start(GTK_BOX(dialog->setup_vbox)
            , dialog->sound_application_frame, FALSE, FALSE, 5);

    dialog->sound_application_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox)
            , dialog->sound_application_entry, TRUE, TRUE, 5);
    gtk_entry_set_text(GTK_ENTRY(dialog->sound_application_entry)
            , (const gchar *)g_par.sound_application);
    dialog->sound_application_open_button = 
            gtk_button_new_from_stock("gtk-open");
    gtk_box_pack_start(GTK_BOX(hbox)
            , dialog->sound_application_open_button, FALSE, FALSE, 5);

    g_signal_connect(G_OBJECT(dialog->sound_application_open_button), "clicked"
            , G_CALLBACK(sound_application_open_button_clicked), dialog);
    g_signal_connect(G_OBJECT(dialog->sound_application_entry), "changed"
            , G_CALLBACK(sound_application_changed), dialog);
}

static void create_parameter_dialog_display_tab(Itf *dialog)
{
    GtkWidget *hbox, *vbox;

    dialog->display_vbox = gtk_vbox_new(FALSE, 0);
    dialog->display_tab = 
        xfce_create_framebox_with_content(NULL, dialog->display_vbox);
    dialog->display_tab_label = gtk_label_new(_("Display"));
    gtk_notebook_append_page(GTK_NOTEBOOK(dialog->notebook)
          , dialog->display_tab, dialog->display_tab_label);

    /* Display calendar borders and menu or not and set stick or ontop */
    vbox = gtk_vbox_new(TRUE, 0);
    dialog->mode_frame = 
            xfce_create_framebox_with_content(_("Calendar main window"), vbox);
    gtk_box_pack_start(GTK_BOX(dialog->display_vbox), dialog->mode_frame
            , FALSE, FALSE, 5);

    dialog->show_borders_checkbutton = 
            gtk_check_button_new_with_mnemonic(_("Show borders"));
    gtk_box_pack_start(GTK_BOX(vbox)
            , dialog->show_borders_checkbutton, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
            dialog->show_borders_checkbutton), g_par.show_borders);

    dialog->show_menu_checkbutton = 
            gtk_check_button_new_with_mnemonic(_("Show menu"));
    gtk_box_pack_start(GTK_BOX(vbox)
            , dialog->show_menu_checkbutton, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
            dialog->show_menu_checkbutton), g_par.show_menu);

    dialog->set_stick_checkbutton = 
            gtk_check_button_new_with_mnemonic(_("Set sticked"));
    gtk_box_pack_start(GTK_BOX(vbox)
            , dialog->set_stick_checkbutton, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
            dialog->set_stick_checkbutton), g_par.set_stick);

    dialog->set_ontop_checkbutton = 
            gtk_check_button_new_with_mnemonic(_("Set on top"));
    gtk_box_pack_start(GTK_BOX(vbox)
            , dialog->set_ontop_checkbutton, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
            dialog->set_ontop_checkbutton), g_par.set_ontop);

    g_signal_connect(G_OBJECT(dialog->show_borders_checkbutton), "toggled"
            , G_CALLBACK(borders_changed), dialog);
    g_signal_connect(G_OBJECT(dialog->show_menu_checkbutton), "toggled"
            , G_CALLBACK(menu_changed), dialog);
    g_signal_connect(G_OBJECT(dialog->set_stick_checkbutton), "toggled"
            , G_CALLBACK(stick_changed), dialog);
    g_signal_connect(G_OBJECT(dialog->set_ontop_checkbutton), "toggled"
            , G_CALLBACK(ontop_changed), dialog);
    
    /* Show in... taskbar pager systray */
    dialog->show_taskbar_checkbutton = gtk_check_button_new_with_mnemonic(
            _("Show in taskbar"));
    gtk_box_pack_start(GTK_BOX(vbox)
            , dialog->show_taskbar_checkbutton, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
            dialog->show_taskbar_checkbutton), g_par.show_taskbar);

    dialog->show_pager_checkbutton = gtk_check_button_new_with_mnemonic(
            _("Show in pager"));
    gtk_box_pack_start(GTK_BOX(vbox)
            , dialog->show_pager_checkbutton, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
            dialog->show_pager_checkbutton), g_par.show_pager);

    dialog->show_systray_checkbutton = gtk_check_button_new_with_mnemonic(
            _("Show in systray"));
    gtk_box_pack_start(GTK_BOX(vbox)
            , dialog->show_systray_checkbutton, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
            dialog->show_systray_checkbutton), g_par.show_systray);

    g_signal_connect(G_OBJECT(dialog->show_taskbar_checkbutton), "toggled"
            , G_CALLBACK(taskbar_changed), dialog);
    g_signal_connect(G_OBJECT(dialog->show_pager_checkbutton), "toggled"
            , G_CALLBACK(pager_changed), dialog);
    g_signal_connect(G_OBJECT(dialog->show_systray_checkbutton), "toggled"
            , G_CALLBACK(systray_changed), dialog);

    /* how to show when started (show/hide/minimize) */
    hbox = gtk_hbox_new(TRUE, 0);
    dialog->visibility_frame = xfce_create_framebox_with_content(
            _("Calendar start") , hbox);
    gtk_box_pack_start(GTK_BOX(dialog->display_vbox), dialog->visibility_frame
            , FALSE, FALSE, 5);

    dialog->visibility_show_radiobutton = gtk_radio_button_new_with_mnemonic(
            NULL, _("Show"));
    gtk_box_pack_start(GTK_BOX(hbox)
            , dialog->visibility_show_radiobutton, FALSE, FALSE, 0);
    gtk_radio_button_set_group(
            GTK_RADIO_BUTTON(dialog->visibility_show_radiobutton)
            , dialog->visibility_radiobutton_group);
    dialog->visibility_radiobutton_group = gtk_radio_button_get_group(
            GTK_RADIO_BUTTON(dialog->visibility_show_radiobutton));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
            dialog->visibility_show_radiobutton), g_par.start_visible);

    dialog->visibility_hide_radiobutton = gtk_radio_button_new_with_mnemonic(
            NULL, _("Hide"));
    gtk_box_pack_start(GTK_BOX(hbox)
            , dialog->visibility_hide_radiobutton, FALSE, FALSE, 0);
    gtk_radio_button_set_group(
            GTK_RADIO_BUTTON(dialog->visibility_hide_radiobutton)
            , dialog->visibility_radiobutton_group);
    dialog->visibility_radiobutton_group = gtk_radio_button_get_group(
            GTK_RADIO_BUTTON(dialog->visibility_hide_radiobutton));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
            dialog->visibility_hide_radiobutton), !g_par.start_visible);

    dialog->visibility_minimized_radiobutton = 
            gtk_radio_button_new_with_mnemonic(NULL, _("Minimized"));
    gtk_box_pack_start(GTK_BOX(hbox)
            , dialog->visibility_minimized_radiobutton, FALSE, FALSE, 0);
    gtk_radio_button_set_group(
            GTK_RADIO_BUTTON(dialog->visibility_minimized_radiobutton)
            , dialog->visibility_radiobutton_group);
    dialog->visibility_radiobutton_group = gtk_radio_button_get_group(
            GTK_RADIO_BUTTON(dialog->visibility_minimized_radiobutton));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
            dialog->visibility_minimized_radiobutton), g_par.start_minimized);

    g_signal_connect(G_OBJECT(dialog->visibility_show_radiobutton), "toggled"
            , G_CALLBACK(start_changed), dialog);
    g_signal_connect(G_OBJECT(dialog->visibility_minimized_radiobutton)
            , "toggled", G_CALLBACK(start_changed), dialog);
}

static void create_parameter_dialog_extra_setup_tab(Itf *dialog)
{
    GtkWidget *hbox, *vbox, *label;
    gint i;
    gchar *weekday_array[7] = {
            _("Monday"), _("Tuesday"), _("Wednesday"), _("Thursday")
          , _("Friday"), _("Saturday"), _("Sunday")};

    dialog->extra_vbox = gtk_vbox_new(FALSE, 0);
    dialog->extra_tab = 
            xfce_create_framebox_with_content(NULL, dialog->extra_vbox);
    dialog->extra_tab_label = gtk_label_new(_("Extra setups"));
    gtk_notebook_append_page(GTK_NOTEBOOK(dialog->notebook)
          , dialog->extra_tab, dialog->extra_tab_label);

    /****** select_always_today ******/
    hbox = gtk_hbox_new(FALSE, 0);
    dialog->always_today_frame = xfce_create_framebox_with_content(
            _("Select always today"), hbox);
    gtk_box_pack_start(GTK_BOX(dialog->extra_vbox)
            , dialog->always_today_frame, FALSE, FALSE, 5);

    dialog->always_today_checkbutton = 
            gtk_check_button_new_with_mnemonic(_("Select always today"));
    gtk_box_pack_start(GTK_BOX(hbox)
            , dialog->always_today_checkbutton, FALSE, FALSE, 5);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
            dialog->always_today_checkbutton), g_par.select_always_today);
    g_signal_connect(G_OBJECT(dialog->always_today_checkbutton), "toggled"
            , G_CALLBACK(always_today_changed), dialog);

    /***** ical week start day (0 = Monday, 1 = Tuesday,... 6 = Sunday) *****/
    hbox = gtk_hbox_new(FALSE, 0);
    dialog->ical_weekstartday_frame = xfce_create_framebox_with_content(
            _("Ical week start day"), hbox);
    gtk_box_pack_start(GTK_BOX(dialog->extra_vbox)
            , dialog->ical_weekstartday_frame, FALSE, FALSE, 5);

    dialog->ical_weekstartday_combobox = gtk_combo_box_new_text();
    for (i = 0; i < 7; i++) {
        gtk_combo_box_append_text(
                GTK_COMBO_BOX(dialog->ical_weekstartday_combobox)
                        , (const gchar *)weekday_array[i]);
    }
    gtk_box_pack_start(GTK_BOX(hbox)
            , dialog->ical_weekstartday_combobox, FALSE, FALSE, 5);
    gtk_combo_box_set_active(GTK_COMBO_BOX(dialog->ical_weekstartday_combobox)
            , g_par.ical_weekstartday);
    g_signal_connect(G_OBJECT(dialog->ical_weekstartday_combobox), "changed"
            , G_CALLBACK(ical_weekstartday_changed), dialog);

    /***** tray icon size  (0 = use static icon) *****/
    vbox = gtk_vbox_new(FALSE, 0);
    dialog->ical_weekstartday_frame = xfce_create_framebox_with_content(
            _("Dynamic icon size"), vbox);
    gtk_box_pack_start(GTK_BOX(dialog->extra_vbox)
            , dialog->ical_weekstartday_frame, FALSE, FALSE, 5);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);
    label = gtk_label_new("X:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
    dialog->icon_size_x_spin = gtk_spin_button_new_with_range(0, 128, 1);
    gtk_box_pack_start(GTK_BOX(hbox)
            , dialog->icon_size_x_spin, FALSE, FALSE, 5);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->icon_size_x_spin)
            , g_par.icon_size_x);
    label = gtk_label_new(_("(0 = use static icon)"));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
    g_signal_connect(G_OBJECT(dialog->icon_size_x_spin), "value-changed"
            , G_CALLBACK(icon_size_x_spin_changed), dialog);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);
    label = gtk_label_new("Y:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
    dialog->icon_size_y_spin = gtk_spin_button_new_with_range(0, 128, 1);
    gtk_box_pack_start(GTK_BOX(hbox)
            , dialog->icon_size_y_spin, FALSE, FALSE, 5);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->icon_size_y_spin)
            , g_par.icon_size_y);
    label = gtk_label_new(_("(0 = use static icon)"));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
    g_signal_connect(G_OBJECT(dialog->icon_size_y_spin), "value-changed"
            , G_CALLBACK(icon_size_y_spin_changed), dialog);
}

Itf *create_parameter_dialog()
{
    Itf *dialog;

    dialog = g_new(Itf, 1);

    dialog->visibility_radiobutton_group = NULL;

    dialog->orage_dialog = xfce_titled_dialog_new();
    gtk_window_set_default_size(GTK_WINDOW(dialog->orage_dialog), 300, 350);
    gtk_window_set_title(GTK_WINDOW(dialog->orage_dialog)
            , _("Orage Preferences"));
    gtk_window_set_position(GTK_WINDOW(dialog->orage_dialog)
            , GTK_WIN_POS_CENTER);
    gtk_window_set_modal(GTK_WINDOW(dialog->orage_dialog), FALSE);
    gtk_window_set_resizable(GTK_WINDOW(dialog->orage_dialog), TRUE);
    gtk_window_set_icon_name(GTK_WINDOW(dialog->orage_dialog), "xfcalendar");

    gtk_dialog_set_has_separator(GTK_DIALOG(dialog->orage_dialog), FALSE);

    dialog->dialog_vbox1 = GTK_DIALOG(dialog->orage_dialog)->vbox;

    dialog->notebook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(dialog->dialog_vbox1), dialog->notebook);
    gtk_container_set_border_width(GTK_CONTAINER(dialog->notebook), 5);

    create_parameter_dialog_main_setup_tab(dialog);
    create_parameter_dialog_display_tab(dialog);
    create_parameter_dialog_extra_setup_tab(dialog);

    /* the rest */
    dialog->help_button = gtk_button_new_from_stock("gtk-help");
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog->orage_dialog)
            , dialog->help_button, GTK_RESPONSE_HELP);
    dialog->close_button = gtk_button_new_from_stock("gtk-close");
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog->orage_dialog)
            , dialog->close_button, GTK_RESPONSE_CLOSE);
    GTK_WIDGET_SET_FLAGS(dialog->close_button, GTK_CAN_DEFAULT);

    g_signal_connect(G_OBJECT (dialog->orage_dialog), "response"
            , G_CALLBACK(dialog_response), NULL);

    xfce_gtk_window_center_on_monitor_with_pointer(
            GTK_WINDOW(dialog->orage_dialog));
    gdk_x11_window_set_user_time(GTK_WIDGET(dialog->orage_dialog)->window, 
            gdk_x11_get_server_time(GTK_WIDGET(dialog->orage_dialog)->window));
    gtk_widget_show_all(dialog->orage_dialog);

    return(dialog);
}

void write_parameters()
{
    gchar *fpath;
    XfceRc *rc;
    gint i;
    gchar f_par[50];

    fpath = xfce_resource_save_location(XFCE_RESOURCE_CONFIG
            , ORAGE_DIR PARFILE, TRUE);
    if ((rc = xfce_rc_simple_open(fpath, FALSE)) == NULL) {
        g_warning("Unable to open RC file.");
        return;
    }
    xfce_rc_write_entry(rc, "Timezone", g_par.local_timezone);
    xfce_rc_write_int_entry(rc, "Archive limit", g_par.archive_limit);
    xfce_rc_write_entry(rc, "Archive file", g_par.archive_file);
    xfce_rc_write_entry(rc, "Orage file", g_par.orage_file);
    xfce_rc_write_entry(rc, "Sound application", g_par.sound_application);
    gtk_window_get_position(GTK_WINDOW(g_par.xfcal->mWindow)
            , &g_par.pos_x, &g_par.pos_y);
    xfce_rc_write_int_entry(rc, "Main window X", g_par.pos_x);
    xfce_rc_write_int_entry(rc, "Main window Y", g_par.pos_y);
    xfce_rc_write_int_entry(rc, "Eventlist window X", g_par.el_size_x);
    xfce_rc_write_int_entry(rc, "Eventlist window Y", g_par.el_size_y);
    xfce_rc_write_bool_entry(rc, "Show Main Window Menu", g_par.show_menu);
    xfce_rc_write_bool_entry(rc, "Select Always Today"
            , g_par.select_always_today);
    xfce_rc_write_bool_entry(rc, "Show borders", g_par.show_borders);
    xfce_rc_write_bool_entry(rc, "Show in pager", g_par.show_pager);
    xfce_rc_write_bool_entry(rc, "Show in systray", g_par.show_systray);
    xfce_rc_write_bool_entry(rc, "Show in taskbar", g_par.show_taskbar);
    xfce_rc_write_bool_entry(rc, "Start visible", g_par.start_visible);
    xfce_rc_write_bool_entry(rc, "Start minimized", g_par.start_minimized);
    xfce_rc_write_bool_entry(rc, "Set sticked", g_par.set_stick);
    xfce_rc_write_bool_entry(rc, "Set ontop", g_par.set_ontop);
    xfce_rc_write_int_entry(rc, "Dynamic icon X", g_par.icon_size_x);
    xfce_rc_write_int_entry(rc, "Dynamic icon Y", g_par.icon_size_y);
    xfce_rc_write_int_entry(rc, "Ical week start day", g_par.ical_weekstartday);
    xfce_rc_write_int_entry(rc, "Foreign file count", g_par.foreign_count);
    for (i = 0; i < g_par.foreign_count;  i++) {
        g_sprintf(f_par, "Foreign file %02d name", i);
        xfce_rc_write_entry(rc, f_par, g_par.foreign_data[i].file);
        g_sprintf(f_par, "Foreign file %02d read-only", i);
        xfce_rc_write_bool_entry(rc, f_par, g_par.foreign_data[i].read_only);
    }
    for (i = g_par.foreign_count; i < 10;  i++) {
        g_sprintf(f_par, "Foreign file %02d name", i);
        if (!xfce_rc_has_entry(rc, f_par))
            break; /* it is in order, so we know that the rest are missing */
        xfce_rc_delete_entry(rc, f_par, TRUE);
        g_sprintf(f_par, "Foreign file %02d read-only", i);
        xfce_rc_delete_entry(rc, f_par, TRUE);
    }

    g_free(fpath);
    xfce_rc_close(rc);
}

void read_parameters(void)
{
    gchar *fpath;
    XfceRc *rc;
    gint i;
    gchar f_par[100];

    fpath = xfce_resource_save_location(XFCE_RESOURCE_CONFIG
            , ORAGE_DIR PARFILE, TRUE);

    if ((rc = xfce_rc_simple_open(fpath, TRUE)) == NULL) {
        g_warning("Unable to open (read) RC file.");
        /* let's try to build it */
        if ((rc = xfce_rc_simple_open(fpath, FALSE)) == NULL) {
            /* still failed, can't do more */
            g_warning("Unable to open (write) RC file.");
            return;
        }
    }
    g_par.local_timezone = 
            g_strdup(xfce_rc_read_entry(rc, "Timezone", "floating"));
    g_par.archive_limit = xfce_rc_read_int_entry(rc, "Archive limit", 0);
    g_par.archive_file = g_strdup(xfce_rc_read_entry(rc, "Archive file"
            , xfce_resource_save_location(XFCE_RESOURCE_DATA, ORAGE_DIR ARCFILE
                    , TRUE)));
    g_par.orage_file = g_strdup(xfce_rc_read_entry(rc, "Orage file"
            , xfce_resource_save_location(XFCE_RESOURCE_DATA, ORAGE_DIR APPFILE
                    , TRUE)));
    g_par.sound_application = 
            g_strdup(xfce_rc_read_entry(rc, "Sound application", "play"));
    g_par.pos_x = xfce_rc_read_int_entry(rc, "Main window X", 0);
    g_par.pos_y = xfce_rc_read_int_entry(rc, "Main window Y", 0);
    g_par.el_size_x = xfce_rc_read_int_entry(rc, "Eventlist window X", 500);
    g_par.el_size_y = xfce_rc_read_int_entry(rc, "Eventlist window Y", 350);
    g_par.show_menu = 
            xfce_rc_read_bool_entry(rc, "Show Main Window Menu", TRUE);
    g_par.select_always_today = 
            xfce_rc_read_bool_entry(rc, "Select Always Today", FALSE);
    g_par.show_borders = xfce_rc_read_bool_entry(rc, "Show borders", TRUE);
    g_par.show_pager = xfce_rc_read_bool_entry(rc, "Show in pager", TRUE);
    g_par.show_systray = xfce_rc_read_bool_entry(rc, "Show in systray", TRUE);
    g_par.show_taskbar = xfce_rc_read_bool_entry(rc, "Show in taskbar", TRUE);
    g_par.start_visible = xfce_rc_read_bool_entry(rc, "Start visible", TRUE);
    g_par.start_minimized = 
            xfce_rc_read_bool_entry(rc, "Start minimized", FALSE);
    g_par.set_stick = xfce_rc_read_bool_entry(rc, "Set sticked", TRUE);
    g_par.set_ontop = xfce_rc_read_bool_entry(rc, "Set ontop", FALSE);
    g_par.icon_size_x = xfce_rc_read_int_entry(rc, "Dynamic icon X", 42);
    g_par.icon_size_y = xfce_rc_read_int_entry(rc, "Dynamic icon Y", 32);
    g_par.ical_weekstartday = 
            xfce_rc_read_int_entry(rc, "Ical week start day", 0); /* monday */
    g_par.foreign_count = 
            xfce_rc_read_int_entry(rc, "Foreign file count", 0);
    for (i = 0; i < g_par.foreign_count; i++) {
        g_sprintf(f_par, "Foreign file %02d name", i);
        g_par.foreign_data[i].file = 
                g_strdup(xfce_rc_read_entry(rc, f_par, NULL));
        g_sprintf(f_par, "Foreign file %02d read-only", i);
        g_par.foreign_data[i].read_only = 
                xfce_rc_read_bool_entry(rc, f_par, TRUE);
    }

    g_free(fpath);
    xfce_rc_close(rc);
}

void show_parameters()
{
    static Itf *dialog = NULL;

    if (is_running && dialog && dialog->orage_dialog) {
        gtk_window_present(GTK_WINDOW(dialog->orage_dialog));
        return;
    }
    is_running = TRUE;
    dialog = create_parameter_dialog();
}

void set_parameters()
{
    static Itf *dialog = NULL;

    set_menu();
    set_border();
    set_taskbar();
    set_pager();
    /*
    set_systray();
    */
    set_stick();
    set_ontop();
    xfical_set_local_timezone();
}
