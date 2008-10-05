/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2006-2008 Juha Kautto  (juha at xfce.org)
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

#include <stdio.h>
#include <locale.h>

#ifdef HAVE__NL_TIME_FIRST_WEEKDAY
#include <langinfo.h>
#endif

#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <libxfcegui4/libxfcegui4.h>

#include "functions.h"
#include "tray_icon.h"
#include "ical-code.h"
#include "parameters.h"
#include "mainbox.h"


static gboolean is_running = FALSE;

typedef struct _Itf
{
    GtkTooltips *Tooltips;

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
#ifdef HAVE_ARCHIVE
    GtkWidget *archive_threshold_frame;
    GtkWidget *archive_threshold_spin;
#endif
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
    GtkWidget *show_todos_checkbutton;
    GtkWidget *show_events_checkbutton;
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

/* code removed. relying in get_first_weekday_from_locale now
/ * ical week start day (0 = Monday, 1 = Tuesday,... 6 = Sunday) * /
GtkWidget *ical_weekstartday_frame;
GtkWidget *ical_weekstartday_combobox;
*/

    /* icon size */
    GtkWidget *icon_size_frame;
    GtkWidget *icon_size_x_spin;
    GtkWidget *icon_size_y_spin;
    /* show event/days window from main calendar */
    GtkWidget *click_to_show_frame;
    GSList    *click_to_show_radiobutton_group;
    GtkWidget *click_to_show_days_radiobutton;
    GtkWidget *click_to_show_events_radiobutton;

    /* the rest */
    GtkWidget *close_button;
    GtkWidget *help_button;
    GtkWidget *dialog_action_area1;
} Itf;

/* Return the first day of the week, where 0=monday, 6=sunday.
 *     Borrowed from GTK+:s Calendar Widget, but modified
 *     to return 0..6 mon..sun, which is what libical uses */
int get_first_weekday_from_locale()
{
#ifdef HAVE__NL_TIME_FIRST_WEEKDAY
    union { unsigned int word; char *string; } langinfo;
    int week_1stday = 0;
    int first_weekday = 1;
    unsigned int week_origin;

    setlocale(LC_TIME, "");
    langinfo.string = nl_langinfo(_NL_TIME_FIRST_WEEKDAY);
    first_weekday = langinfo.string[0];
    langinfo.string = nl_langinfo(_NL_TIME_WEEK_1STDAY);
    week_origin = langinfo.word;
    if (week_origin == 19971130) /* Sunday */
        week_1stday = 0;
    else if (week_origin == 19971201) /* Monday */
        week_1stday = 1;
    else
        orage_message(150, "get_first_weekday: unknown value of _NL_TIME_WEEK_1STDAY.");

    return((week_1stday + first_weekday - 2 + 7) % 7);
#else
    orage_message(150, "get_first_weekday: Can not find first weekday. Using default: Monday=0. If this is wrong guess. please set undocumented parameter: Ical week start day (Sunday=6)");
    return(0);
#endif
}

static void dialog_response(GtkWidget *dialog, gint response_id
        , gpointer user_data)
{
    Itf *itf = (Itf *)user_data;
    gchar *helpdoc;

    if (response_id == GTK_RESPONSE_HELP) {
        /* Needs to be in " to keep # */
        helpdoc = g_strconcat("xfbrowser4 \"", PACKAGE_DATA_DIR
                , G_DIR_SEPARATOR_S, "orage"
                , G_DIR_SEPARATOR_S, "doc"
                , G_DIR_SEPARATOR_S, "C"
                , G_DIR_SEPARATOR_S, "orage.html#orage-preferences-window\""
                , NULL);
        orage_exec(helpdoc, NULL, NULL);
    }
    else { /* delete signal or close response */
        write_parameters();
        is_running = FALSE;
        gtk_widget_destroy(dialog);
        gtk_object_destroy(GTK_OBJECT(itf->Tooltips));
        g_free(itf);
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
    gtk_window_set_decorated(GTK_WINDOW(((CalWin *)g_par.xfcal)->mWindow)
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
        gtk_widget_show(((CalWin *)g_par.xfcal)->mMenubar);
    else
        gtk_widget_hide(((CalWin *)g_par.xfcal)->mMenubar);
}

static void menu_changed(GtkWidget *dialog, gpointer user_data)
{
    Itf *itf = (Itf *)user_data;

    g_par.show_menu = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
            itf->show_menu_checkbutton));
    set_menu();
}

static void set_todos()
{
    if (g_par.show_todos)
        gtk_widget_show_all(((CalWin *)g_par.xfcal)->mTodo_vbox);
    else
        gtk_widget_hide_all(((CalWin *)g_par.xfcal)->mTodo_vbox);
}

static void todos_changed(GtkWidget *dialog, gpointer user_data)
{
    Itf *itf = (Itf *)user_data;

    g_par.show_todos = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
            itf->show_todos_checkbutton));
    set_todos();
}

static void set_events()
{
    if (g_par.show_events)
        gtk_widget_show_all(((CalWin *)g_par.xfcal)->mEvent_vbox);
    else
        gtk_widget_hide_all(((CalWin *)g_par.xfcal)->mEvent_vbox);
}

static void events_changed(GtkWidget *dialog, gpointer user_data)
{
    Itf *itf = (Itf *)user_data;

    g_par.show_events = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
            itf->show_events_checkbutton));
    set_events();
}

static void set_stick()
{
    if (g_par.set_stick)
        gtk_window_stick(GTK_WINDOW(((CalWin *)g_par.xfcal)->mWindow));
    else
        gtk_window_unstick(GTK_WINDOW(((CalWin *)g_par.xfcal)->mWindow));
}

static void stick_changed(GtkWidget *dialog, gpointer user_data)
{
    Itf *itf = (Itf *)user_data;

    g_par.set_stick = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
            itf->set_stick_checkbutton));
    set_stick();
}

static void set_ontop()
{
    gtk_window_set_keep_above(GTK_WINDOW(((CalWin *)g_par.xfcal)->mWindow)
            , g_par.set_ontop);
}

static void ontop_changed(GtkWidget *dialog, gpointer user_data)
{
    Itf *itf = (Itf *)user_data;

    g_par.set_ontop = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
            itf->set_ontop_checkbutton));
    set_ontop();
}

static void set_taskbar()
{
    gtk_window_set_skip_taskbar_hint(
            GTK_WINDOW(((CalWin *)g_par.xfcal)->mWindow), !g_par.show_taskbar);
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
    gtk_window_set_skip_pager_hint(GTK_WINDOW(((CalWin *)g_par.xfcal)->mWindow)
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
#if GTK_CHECK_VERSION(2,10,0)
    if (!(g_par.trayIcon 
    && gtk_status_icon_is_embedded((GtkStatusIcon *)g_par.trayIcon))) {
        g_par.trayIcon = create_TrayIcon();
    }

    if (g_par.show_systray)
        gtk_status_icon_set_visible((GtkStatusIcon *)g_par.trayIcon, TRUE);
    else
        gtk_status_icon_set_visible((GtkStatusIcon *)g_par.trayIcon, FALSE);
#else
    if (!(g_par.trayIcon 
    && NETK_IS_TRAY_ICON(((XfceTrayIcon *)g_par.trayIcon)->tray))) {
        g_par.trayIcon = create_TrayIcon();
    }

    if (g_par.show_systray)
        xfce_tray_icon_connect((XfceTrayIcon *)g_par.trayIcon);
    else
        xfce_tray_icon_disconnect((XfceTrayIcon *)g_par.trayIcon);
#endif
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

static void show_changed(GtkWidget *dialog, gpointer user_data)
{
    Itf *itf = (Itf *)user_data;

    g_par.show_days = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
            itf->click_to_show_days_radiobutton));
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

    if (!ORAGE_STR_EXISTS(g_par.local_timezone)) {
        g_warning("timezone pressed: local timezone missing");
        g_par.local_timezone = g_strdup("floating");
    }
    if (xfical_timezone_button_clicked(button, GTK_WINDOW(itf->orage_dialog)
            , &g_par.local_timezone))
        xfical_set_local_timezone();
}

#ifdef HAVE_ARCHIVE
static void archive_threshold_spin_changed(GtkSpinButton *sb
        , gpointer user_data)
{
    g_par.archive_limit = gtk_spin_button_get_value(sb);
}
#endif

static void always_today_changed(GtkWidget *dialog, gpointer user_data)
{
    Itf *itf = (Itf *)user_data;

    g_par.select_always_today = gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(itf->always_today_checkbutton));
}

/* code removed. relying in get_first_weekday_from_locale now
static void ical_weekstartday_changed(GtkWidget *dialog, gpointer user_data)
{
    Itf *itf = (Itf *)user_data;

    g_par.ical_weekstartday = gtk_combo_box_get_active(
            GTK_COMBO_BOX(itf->ical_weekstartday_combobox));
}
*/

static void set_icon_size()
{
    refresh_TrayIcon();
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
    gtk_tooltips_set_tip(dialog->Tooltips, dialog->timezone_button
            , _("You should always define your local timezone.")
            , NULL);
    g_signal_connect(G_OBJECT(dialog->timezone_button), "clicked"
            , G_CALLBACK(timezone_button_clicked), dialog);

#ifdef HAVE_ARCHIVE
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
    gtk_tooltips_set_tip(dialog->Tooltips, dialog->archive_threshold_spin
            , _("Archiving is used to save time and space when handling events.")
            , NULL);
    g_signal_connect(G_OBJECT(dialog->archive_threshold_spin), "value-changed"
            , G_CALLBACK(archive_threshold_spin_changed), dialog);
#endif

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

    gtk_tooltips_set_tip(dialog->Tooltips, dialog->sound_application_entry
            , _("This command is given to shell to make sound in alarms.")
            , NULL);
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

    dialog->show_borders_checkbutton = gtk_check_button_new_with_mnemonic(
            _("Show borders"));
    gtk_box_pack_start(GTK_BOX(vbox)
            , dialog->show_borders_checkbutton, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
            dialog->show_borders_checkbutton), g_par.show_borders);

    dialog->show_menu_checkbutton = gtk_check_button_new_with_mnemonic(
            _("Show menu"));
    gtk_box_pack_start(GTK_BOX(vbox)
            , dialog->show_menu_checkbutton, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
            dialog->show_menu_checkbutton), g_par.show_menu);

    dialog->show_todos_checkbutton = gtk_check_button_new_with_mnemonic(
            _("Show todo list"));
    gtk_box_pack_start(GTK_BOX(vbox)
            , dialog->show_todos_checkbutton, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
            dialog->show_todos_checkbutton), g_par.show_todos);

    dialog->show_events_checkbutton = gtk_check_button_new_with_mnemonic(
            _("Show event list"));
    gtk_box_pack_start(GTK_BOX(vbox)
            , dialog->show_events_checkbutton, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
            dialog->show_events_checkbutton), g_par.show_events);

    dialog->set_stick_checkbutton = gtk_check_button_new_with_mnemonic(
            _("Set sticked"));
    gtk_box_pack_start(GTK_BOX(vbox)
            , dialog->set_stick_checkbutton, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
            dialog->set_stick_checkbutton), g_par.set_stick);

    dialog->set_ontop_checkbutton = gtk_check_button_new_with_mnemonic(
            _("Set on top"));
    gtk_box_pack_start(GTK_BOX(vbox)
            , dialog->set_ontop_checkbutton, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
            dialog->set_ontop_checkbutton), g_par.set_ontop);

    g_signal_connect(G_OBJECT(dialog->show_borders_checkbutton), "toggled"
            , G_CALLBACK(borders_changed), dialog);
    g_signal_connect(G_OBJECT(dialog->show_menu_checkbutton), "toggled"
            , G_CALLBACK(menu_changed), dialog);
    g_signal_connect(G_OBJECT(dialog->show_todos_checkbutton), "toggled"
            , G_CALLBACK(todos_changed), dialog);
    g_signal_connect(G_OBJECT(dialog->show_events_checkbutton), "toggled"
            , G_CALLBACK(events_changed), dialog);
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
    dialog->visibility_radiobutton_group = NULL;
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
    /* code removed. relying in get_first_weekday_from_locale now
    GtkWidget *event;
    gchar *weekday_array[7] = {
            _("Monday"), _("Tuesday"), _("Wednesday"), _("Thursday")
          , _("Friday"), _("Saturday"), _("Sunday")};
     */

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
    gtk_tooltips_set_tip(dialog->Tooltips, dialog->always_today_checkbutton
            , _("When showing main calendar, set pointer to either previously selected day or always to current day.")
            , NULL);
    g_signal_connect(G_OBJECT(dialog->always_today_checkbutton), "toggled"
            , G_CALLBACK(always_today_changed), dialog);

    /* code removed. relying in get_first_weekday_from_locale now
    / ***** ical week start day (0 = Monday, 1 = Tuesday,... 6 = Sunday) ***** /
    hbox = gtk_hbox_new(FALSE, 0);
    dialog->ical_weekstartday_frame = xfce_create_framebox_with_content(
            _("Ical week start day"), hbox);
    gtk_box_pack_start(GTK_BOX(dialog->extra_vbox)
            , dialog->ical_weekstartday_frame, FALSE, FALSE, 5);

    dialog->ical_weekstartday_combobox = orage_create_combo_box_with_content(
            weekday_array, 7);
    event =  gtk_event_box_new(); / * only needed for tooltips * /
    gtk_container_add(GTK_CONTAINER(event), dialog->ical_weekstartday_combobox);
    gtk_box_pack_start(GTK_BOX(hbox)
            , event, FALSE, FALSE, 5);
    gtk_combo_box_set_active(GTK_COMBO_BOX(dialog->ical_weekstartday_combobox)
            , g_par.ical_weekstartday);
    gtk_tooltips_set_tip(dialog->Tooltips, event
            , _("This is used in some weekly repeating appointment rules. Just set it to the start day of the week for your country and forget it.")
            , NULL);
    g_signal_connect(G_OBJECT(dialog->ical_weekstartday_combobox), "changed"
            , G_CALLBACK(ical_weekstartday_changed), dialog);
    */

    /***** tray icon size  (0 = use static icon) *****/
    vbox = gtk_vbox_new(FALSE, 0);
    dialog->icon_size_frame = xfce_create_framebox_with_content(
            _("Dynamic icon size"), vbox);
    gtk_box_pack_start(GTK_BOX(dialog->extra_vbox)
            , dialog->icon_size_frame, FALSE, FALSE, 5);

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
    gtk_tooltips_set_tip(dialog->Tooltips, dialog->icon_size_x_spin
            , _("Dynamic icon shows current day and month. It is visible only in tray. If tray is too small for dynamic icon size, Orage switches automatically back to static icon.")
            , NULL);
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

    /***** Start event or day window from main calendar *****/
    dialog->click_to_show_radiobutton_group = NULL;
    hbox = gtk_vbox_new(FALSE, 0);
    dialog->click_to_show_frame = xfce_create_framebox_with_content(
            _("Main Calendar double click shows"), hbox);
    gtk_box_pack_start(GTK_BOX(dialog->extra_vbox)
            , dialog->click_to_show_frame, FALSE, FALSE, 5);

    dialog->click_to_show_days_radiobutton =
            gtk_radio_button_new_with_mnemonic(NULL, _("Days view"));
    gtk_box_pack_start(GTK_BOX(hbox)
            , dialog->click_to_show_days_radiobutton, FALSE, FALSE, 0);
    gtk_radio_button_set_group(
            GTK_RADIO_BUTTON(dialog->click_to_show_days_radiobutton)
            , dialog->click_to_show_radiobutton_group);
    dialog->click_to_show_radiobutton_group = gtk_radio_button_get_group(
            GTK_RADIO_BUTTON(dialog->click_to_show_days_radiobutton));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
            dialog->click_to_show_days_radiobutton), g_par.show_days);

    dialog->click_to_show_events_radiobutton =
            gtk_radio_button_new_with_mnemonic(NULL, _("Event list"));
    gtk_box_pack_start(GTK_BOX(hbox)
            , dialog->click_to_show_events_radiobutton, FALSE, FALSE, 0);
    gtk_radio_button_set_group(
            GTK_RADIO_BUTTON(dialog->click_to_show_events_radiobutton)
            , dialog->click_to_show_radiobutton_group);
    dialog->click_to_show_radiobutton_group = gtk_radio_button_get_group(
            GTK_RADIO_BUTTON(dialog->click_to_show_events_radiobutton));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
            dialog->click_to_show_events_radiobutton), !g_par.show_days);

    g_signal_connect(G_OBJECT(dialog->click_to_show_days_radiobutton), "toggled"
            , G_CALLBACK(show_changed), dialog);
}

Itf *create_parameter_dialog()
{
    Itf *dialog;

    dialog = g_new(Itf, 1);
    dialog->Tooltips = gtk_tooltips_new();

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

    g_signal_connect(G_OBJECT(dialog->orage_dialog), "response"
            , G_CALLBACK(dialog_response), dialog);

    gtk_widget_show_all(dialog->orage_dialog);
    /*
    gdk_x11_window_set_user_time(GTK_WIDGET(dialog->orage_dialog)->window, 
            gdk_x11_get_server_time(GTK_WIDGET(dialog->orage_dialog)->window));
            */

    return(dialog);
}

OrageRc *orage_parameters_file_open(gboolean read_only)
{
    gchar *fpath;
    OrageRc *orc;

    fpath = orage_config_file_location(ORAGE_PAR_FILE);
    if ((orc = (OrageRc *)orage_rc_file_open(fpath, read_only)) == NULL) {
        orage_message(150, "orage_category_file_open: Parameter file open failed.");
    }
    g_free(fpath);

    return(orc);
}

void write_parameters()
{
    OrageRc *orc;
    gint i;
    gchar f_par[50];

    orc = orage_parameters_file_open(FALSE);

    orage_rc_put_str(orc, "Timezone", g_par.local_timezone);
#ifdef HAVE_ARCHIVE
    orage_rc_put_int(orc, "Archive limit", g_par.archive_limit);
    orage_rc_put_str(orc, "Archive file", g_par.archive_file);
#endif
    orage_rc_put_str(orc, "Orage file", g_par.orage_file);
    orage_rc_put_str(orc, "Sound application", g_par.sound_application);
    gtk_window_get_position(GTK_WINDOW(((CalWin *)g_par.xfcal)->mWindow)
            , &g_par.pos_x, &g_par.pos_y);
    orage_rc_put_int(orc, "Main window X", g_par.pos_x);
    orage_rc_put_int(orc, "Main window Y", g_par.pos_y);
    orage_rc_put_int(orc, "Eventlist window X", g_par.el_size_x);
    orage_rc_put_int(orc, "Eventlist window Y", g_par.el_size_y);
    orage_rc_put_bool(orc, "Show Main Window Menu", g_par.show_menu);
    orage_rc_put_bool(orc, "Select Always Today"
            , g_par.select_always_today);
    orage_rc_put_bool(orc, "Show borders", g_par.show_borders);
    orage_rc_put_bool(orc, "Show todos", g_par.show_todos);
    orage_rc_put_bool(orc, "Show events", g_par.show_events);
    orage_rc_put_bool(orc, "Show in pager", g_par.show_pager);
    orage_rc_put_bool(orc, "Show in systray", g_par.show_systray);
    orage_rc_put_bool(orc, "Show in taskbar", g_par.show_taskbar);
    orage_rc_put_bool(orc, "Start visible", g_par.start_visible);
    orage_rc_put_bool(orc, "Start minimized", g_par.start_minimized);
    orage_rc_put_bool(orc, "Set sticked", g_par.set_stick);
    orage_rc_put_bool(orc, "Set ontop", g_par.set_ontop);
    orage_rc_put_int(orc, "Dynamic icon X", g_par.icon_size_x);
    orage_rc_put_int(orc, "Dynamic icon Y", g_par.icon_size_y);
    /* we write this with X so that we do not read it back unless
     * it is manually changed. It should need changes really seldom. */
    orage_rc_put_int(orc, "XIcal week start day"
            , g_par.ical_weekstartday);
    orage_rc_put_bool(orc, "Show days", g_par.show_days);
    orage_rc_put_int(orc, "Foreign file count", g_par.foreign_count);
    /* add what we have and remove the rest */
    for (i = 0; i < g_par.foreign_count;  i++) {
        g_sprintf(f_par, "Foreign file %02d name", i);
        orage_rc_put_str(orc, f_par, g_par.foreign_data[i].file);
        g_sprintf(f_par, "Foreign file %02d read-only", i);
        orage_rc_put_bool(orc, f_par, g_par.foreign_data[i].read_only);
    }
    for (i = g_par.foreign_count; i < 10;  i++) {
        g_sprintf(f_par, "Foreign file %02d name", i);
        if (!orage_rc_exists_item(orc, f_par))
            break; /* it is in order, so we know that the rest are missing */
        orage_rc_del_item(orc, f_par);
        g_sprintf(f_par, "Foreign file %02d read-only", i);
        orage_rc_del_item(orc, f_par);
    }
    orage_rc_put_int(orc, "Logging level", g_par.log_level);

    orage_rc_file_close(orc);
}

/* let's try to find the timezone name by comparing this file to
 * timezone files from the default location. This does not work
 * always, but is the best trial we can do */
void init_dtz_check_dir(gchar *tz_dirname, gchar *tz_local, gint len)
{
    gint tz_offset = strlen("/usr/share/zoneinfo/");
    gsize tz_len;         /* file lengths */
    GDir *tz_dir;         /* location of timezone data /usr/share/zoneinfo */
    const gchar *tz_file; /* filename containing timezone data */
    gchar *tz_fullfile;   /* full filename */
    gchar *tz_data;       /* contents of the timezone data file */
    GError *error = NULL;

    if ((tz_dir = g_dir_open(tz_dirname, 0, NULL))) {
        while ((tz_file = g_dir_read_name(tz_dir)) 
                && g_par.local_timezone == NULL) {
            tz_fullfile = g_strconcat(tz_dirname, "/", tz_file, NULL);
            if (g_file_test(tz_fullfile, G_FILE_TEST_IS_SYMLINK))
                ; /* we do not accept these, just continue searching */
            else if (g_file_test(tz_fullfile, G_FILE_TEST_IS_DIR)) {
                init_dtz_check_dir(tz_fullfile, tz_local, len);
            }
            else if (g_file_get_contents(tz_fullfile, &tz_data, &tz_len
                    , &error)) {
                if (len == tz_len && !memcmp(tz_local, tz_data, len)) {
                    /* this is a match (length is tested first since that 
                     * test is quick and it eliminates most) */
                    g_par.local_timezone = g_strdup(tz_fullfile+tz_offset);
                }
                g_free(tz_data);
            }
            else { /* we should never come here */
                g_warning("init_default_timezone: can not read (%s) %s"
                        , tz_fullfile, error->message);
                g_error_free(error);
                error = NULL;
            }
            /* if we found a candidate, test that libical knows it */
            if (g_par.local_timezone && !xfical_set_local_timezone()) { 
                /* candidate is not known by libical, remove it */
                g_warning("init_default_timezone: found match (%s) but libical can not use that, skipping it."
                        , g_par.local_timezone);
                g_free(g_par.local_timezone);
                g_par.local_timezone = NULL;
            }
            g_free(tz_fullfile);
        } /* while */
        g_dir_close(tz_dir);
    }
}

void init_default_timezone()
{
    gsize len;            /* file lengths */
    gchar *tz_local;      /* local timezone data */

    g_free(g_par.local_timezone);
    g_par.local_timezone = NULL;
    /* debian, ubuntu stores the timezone name into /etc/timezone */
    if (g_file_get_contents("/etd/timezone", &g_par.local_timezone
                , &len, NULL)) {
        /* success! let's check it */
        if (len > 2) /* get rid of the \n at the end */
            g_par.local_timezone[len-1] = 0;
        if (!xfical_set_local_timezone()) { /* test that libical knows it */
            g_free(g_par.local_timezone);
            g_par.local_timezone = NULL;
        }
    }
    /* redhat, gentoo, etc. stores the timezone data into /etc/localtime */
    else if (g_file_get_contents("/etc/localtime", &tz_local, &len, NULL)) {
        init_dtz_check_dir("/usr/share/zoneinfo", tz_local, len);
        g_free(tz_local);
    }
    if (!ORAGE_STR_EXISTS(g_par.local_timezone))
        g_par.local_timezone = g_strdup("floating");
}

void read_parameters(void)
{
    gchar *fpath;
    OrageRc *orc;
    gint i;
    gchar f_par[100];

    orc = orage_parameters_file_open(TRUE);

    g_par.local_timezone = orage_rc_get_str(orc, "Timezone", "not found");
    if (!strcmp(g_par.local_timezone, "not found")) { 
        init_default_timezone();
    }
#ifdef HAVE_ARCHIVE
    g_par.archive_limit = orage_rc_get_int(orc, "Archive limit", 0);
    fpath = orage_data_file_location(ORAGE_ARC_FILE);
    g_par.archive_file = orage_rc_get_str(orc, "Archive file", fpath);
    g_free(fpath);
#endif
    fpath = orage_data_file_location(ORAGE_APP_FILE);
    g_par.orage_file = orage_rc_get_str(orc, "Orage file", fpath);
    g_free(fpath);
    g_par.sound_application=orage_rc_get_str(orc, "Sound application", "play");
    g_par.pos_x = orage_rc_get_int(orc, "Main window X", 0);
    g_par.pos_y = orage_rc_get_int(orc, "Main window Y", 0);
    g_par.el_size_x = orage_rc_get_int(orc, "Eventlist window X", 500);
    g_par.el_size_y = orage_rc_get_int(orc, "Eventlist window Y", 350);
    g_par.show_menu = orage_rc_get_bool(orc, "Show Main Window Menu", TRUE);
    g_par.select_always_today = 
            orage_rc_get_bool(orc, "Select Always Today", FALSE);
    g_par.show_borders = orage_rc_get_bool(orc, "Show borders", TRUE);
    g_par.show_todos = orage_rc_get_bool(orc, "Show todos", TRUE);
    g_par.show_events = orage_rc_get_bool(orc, "Show events", TRUE);
    g_par.show_pager = orage_rc_get_bool(orc, "Show in pager", TRUE);
    g_par.show_systray = orage_rc_get_bool(orc, "Show in systray", TRUE);
    g_par.show_taskbar = orage_rc_get_bool(orc, "Show in taskbar", TRUE);
    g_par.start_visible = orage_rc_get_bool(orc, "Start visible", TRUE);
    g_par.start_minimized = orage_rc_get_bool(orc, "Start minimized", FALSE);
    g_par.set_stick = orage_rc_get_bool(orc, "Set sticked", TRUE);
    g_par.set_ontop = orage_rc_get_bool(orc, "Set ontop", FALSE);
    g_par.icon_size_x = orage_rc_get_int(orc, "Dynamic icon X", 42);
    g_par.icon_size_y = orage_rc_get_int(orc, "Dynamic icon Y", 32);
    /* 0 = monday, ..., 6 = sunday */
    g_par.ical_weekstartday = orage_rc_get_int(orc, "Ical week start day"
            , get_first_weekday_from_locale());
    g_par.show_days = orage_rc_get_bool(orc, "Show days", FALSE);
    g_par.foreign_count = orage_rc_get_int(orc, "Foreign file count", 0);
    for (i = 0; i < g_par.foreign_count; i++) {
        g_sprintf(f_par, "Foreign file %02d name", i);
        g_par.foreign_data[i].file = orage_rc_get_str(orc, f_par, NULL);
        g_sprintf(f_par, "Foreign file %02d read-only", i);
        g_par.foreign_data[i].read_only = orage_rc_get_bool(orc, f_par, TRUE);
    }
    g_par.log_level = orage_rc_get_int(orc, "Logging level", 0);

    orage_rc_file_close(orc);
}

void show_parameters()
{
    static Itf *dialog = NULL;

    if (is_running) {
        gtk_window_present(GTK_WINDOW(dialog->orage_dialog));
    }
    else {
        is_running = TRUE;
        dialog = create_parameter_dialog();
    }
}

void set_parameters()
{
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
