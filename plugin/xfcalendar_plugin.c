/*
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; You may only use version 2 of the License,
	you have no option to use any other version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

        xfcalendar mcs plugin    - (c) 2003-2005 Mickael Graf <korbinus at xfce.org>
        Parts of the code below  - (C) 2005 Juha Kautto <kautto.juha at kolumbus.fi>

 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <libxfce4mcs/mcs-common.h>
#include <libxfce4mcs/mcs-manager.h>
#include <libxfcegui4/libxfcegui4.h>
#include <xfce-mcs-manager/manager-plugin.h>

#define BORDER 5

#define RCDIR    "xfce4" G_DIR_SEPARATOR_S "mcs_settings"
#define OLDRCDIR "xfcalendar"
#define CHANNEL  "xfcalendar"
#define RCFILE   "xfcalendar.xml"
#define PLUGIN_NAME "orage"

static void create_channel(McsPlugin * mcs_plugin);
static gboolean write_options(McsPlugin * mcs_plugin);
static void run_dialog(McsPlugin * mcs_plugin);

static gboolean is_running = FALSE;
static gboolean normalmode = TRUE;
static gboolean showtaskbar = TRUE;
static gboolean showpager = TRUE;
static gboolean showsystray = TRUE;
static gboolean showstart = TRUE;
static gboolean hidestart = FALSE;
static gboolean ministart = FALSE;
static gchar *sound_application;
static gchar *archive_path;
static int archive_threshold;

typedef struct _Itf Itf;
struct _Itf
{
    McsPlugin *mcs_plugin;

    GtkWidget *xfcalendar_dialog;
    GtkWidget *dialog_header;
    GtkWidget *dialog_vbox1;
    GtkWidget *vbox1;
    GtkWidget *notebook;
    /* Tabs */
    GtkWidget *display_tab;
    GtkWidget *display_tab_label;
    GtkWidget *display_tab_table;
    GtkWidget *display_vbox;
    GtkWidget *archives_tab;
    GtkWidget *archives_tab_label;
    GtkWidget *sound_tab;
    GtkWidget *sound_tab_label;
    GtkWidget *sound_tab_table;
    /* Mode normal or compact */
    GSList    *mode_radiobutton_group;
    GtkWidget *mode_hbox;
    GtkWidget *mode_frame;
    GtkWidget *mode_label;
    GtkWidget *compact_mode_radiobutton;
    GtkWidget *normal_mode_radiobutton;
    /* Show in... taskbar pager systray */
    GtkWidget *show_taskbar_checkbutton;
    GtkWidget *show_pager_checkbutton;
    GtkWidget *show_systray_checkbutton;
    GtkWidget *show_vbox;
    GtkWidget *show_frame;
    GtkWidget *show_label;
    /* Start visibity show or hide */
    GSList    *visibility_radiobutton_group;
    GtkWidget *visibility_hbox;
    GtkWidget *visibility_frame;
    GtkWidget *visibility_label;
    GtkWidget *visibility_show_radiobutton;
    GtkWidget *visibility_hide_radiobutton;
    GtkWidget *visibility_minimized_radiobutton;
    /* Archive file and periodicity */
    GtkWidget *archive_vbox;
    GtkWidget *archive_file_frame;
    GtkWidget *archive_file_table;
    GtkWidget *archive_threshold_label;
    GtkWidget *archive_file_entry;
    GtkWidget *archive_open_file_button;
    GtkWidget *archive_threshold_frame;
    GtkWidget *archive_threshold_table;
    GtkWidget *archive_threshold_combobox;
    /* Choose the sound application for reminders */
    GtkWidget *sound_application_label;
    GtkWidget *sound_application_entry;
    /* */
    GtkWidget *close_button;
    GtkWidget *dialog_action_area1;
};

static void cb_dialog_response(GtkWidget * dialog, gint response_id)
{
    if(response_id == GTK_RESPONSE_HELP)
    {
        g_message("HELP: TBD");
    }
    else
    {
        is_running = FALSE;
        gtk_widget_destroy(dialog);
    }
}

static void cb_sound_application_changed(GtkWidget * dialog, gpointer user_data)
{
    Itf *itf = (Itf *) user_data;
    McsPlugin *mcs_plugin = itf->mcs_plugin;
    
    sound_application = g_strdup(gtk_entry_get_text(GTK_ENTRY(itf->sound_application_entry)));
    mcs_manager_set_string(mcs_plugin->manager, "XFCalendar/SoundApplication", CHANNEL, sound_application);
    mcs_manager_notify(mcs_plugin->manager, CHANNEL);
    write_options(mcs_plugin);
}

void static cb_archive_file_entry_changed (GtkWidget * dialog, gpointer user_data)
{
    Itf *itf = (Itf *) user_data;
    McsPlugin *mcs_plugin = itf->mcs_plugin;

    archive_path = g_strdup (gtk_entry_get_text (GTK_ENTRY (itf->archive_file_entry)));
    mcs_manager_set_string (mcs_plugin->manager, "XFCalendar/ArchiveFile", CHANNEL, archive_path);
    mcs_manager_notify (mcs_plugin->manager, CHANNEL);
    write_options (mcs_plugin);
}

static void cb_mode_changed(GtkWidget * dialog, gpointer user_data)
{
    Itf *itf = (Itf *) user_data;
    McsPlugin *mcs_plugin = itf->mcs_plugin;

    normalmode = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(itf->normal_mode_radiobutton));

    mcs_manager_set_int(mcs_plugin->manager, "XFCalendar/NormalMode", CHANNEL, normalmode ? 1 : 0);
    mcs_manager_notify(mcs_plugin->manager, CHANNEL);
    write_options(mcs_plugin);
}

static void cb_taskbar_changed(GtkWidget * dialog, gpointer user_data)
{
    Itf *itf = (Itf *) user_data;
    McsPlugin *mcs_plugin = itf->mcs_plugin;

    showtaskbar = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(itf->show_taskbar_checkbutton));

    mcs_manager_set_int(mcs_plugin->manager, "XFCalendar/TaskBar", CHANNEL, showtaskbar ? 1 : 0);
    mcs_manager_notify(mcs_plugin->manager, CHANNEL);
    write_options(mcs_plugin);
}

static void cb_pager_changed(GtkWidget * dialog, gpointer user_data)
{
    Itf *itf = (Itf *) user_data;
    McsPlugin *mcs_plugin = itf->mcs_plugin;

    showpager = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(itf->show_pager_checkbutton));

    mcs_manager_set_int(mcs_plugin->manager, "XFCalendar/Pager", CHANNEL, showpager ? 1 : 0);
    mcs_manager_notify(mcs_plugin->manager, CHANNEL);
    write_options(mcs_plugin);
}

static void cb_systray_changed(GtkWidget * dialog, gpointer user_data)
{
    Itf *itf = (Itf *) user_data;
    McsPlugin *mcs_plugin = itf->mcs_plugin;

    showsystray = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(itf->show_systray_checkbutton));

    mcs_manager_set_int(mcs_plugin->manager, "XFCalendar/Systray", CHANNEL, showsystray ? 1 : 0);
    mcs_manager_notify(mcs_plugin->manager, CHANNEL);
    write_options(mcs_plugin);
}

static void cb_start_changed(GtkWidget * dialog, gpointer user_data)
{
    Itf *itf = (Itf *) user_data;
    McsPlugin *mcs_plugin = itf->mcs_plugin;

    showstart = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(itf->visibility_show_radiobutton));
    hidestart = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(itf->visibility_hide_radiobutton));
    ministart = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(itf->visibility_minimized_radiobutton));

    mcs_manager_set_int(mcs_plugin->manager, "XFCalendar/ShowStart", CHANNEL, showstart ? 1 : (hidestart ? 0 : 2));
    mcs_manager_notify(mcs_plugin->manager, CHANNEL);
    write_options(mcs_plugin);
}

static void cb_archive_open_file_button_clicked (GtkButton *button, gpointer user_data)
{
    Itf *itf = (Itf *) user_data;
    GtkWidget *file_chooser;
	XfceFileFilter *filter;
    gchar *archive_path;

    /* Create file chooser */
    file_chooser = xfce_file_chooser_new (_("Select a file..."),
                                            GTK_WINDOW (itf->xfcalendar_dialog),
                                            XFCE_FILE_CHOOSER_ACTION_SAVE,
                                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                            GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                            NULL);
    /* Add filters */
    filter = xfce_file_filter_new ();
	xfce_file_filter_set_name(filter, _("Calendar files"));
	xfce_file_filter_add_pattern(filter, "*.ics");
	xfce_file_chooser_add_filter(XFCE_FILE_CHOOSER(file_chooser), filter);
	xfce_file_filter_set_name(filter, _("All Files"));
	xfce_file_filter_add_pattern(filter, "*");
	xfce_file_chooser_add_filter(XFCE_FILE_CHOOSER(file_chooser), filter);

	if(gtk_dialog_run(GTK_DIALOG(file_chooser)) == GTK_RESPONSE_ACCEPT) {
		archive_path = xfce_file_chooser_get_filename(XFCE_FILE_CHOOSER(file_chooser));

        if(archive_path){
            gtk_entry_set_text (GTK_ENTRY (itf->archive_file_entry), (const gchar*) archive_path);
        }
    }

    gtk_widget_destroy (file_chooser);
}

static void cb_archive_threshold_combobox_changed (GtkComboBox *cb, gpointer user_data)
{

    Itf *itf = (Itf *) user_data;
    McsPlugin *mcs_plugin = itf->mcs_plugin;
    int index;

    index = gtk_combo_box_get_active (GTK_COMBO_BOX (itf->archive_threshold_combobox));
    switch (index) {
        case 0: archive_threshold = 3;
                break;
        case 1: archive_threshold = 6;
                break;
        case 2: archive_threshold = 12;
                break;
    }

    mcs_manager_set_int (mcs_plugin->manager, "XFCalendar/Lookback", CHANNEL, archive_threshold);
    mcs_manager_notify (mcs_plugin->manager, CHANNEL);
    write_options (mcs_plugin);
}

Itf *create_xfcalendar_dialog(McsPlugin * mcs_plugin)
{
    Itf *dialog;

    dialog = g_new(Itf, 1);

    dialog->mcs_plugin = mcs_plugin;
    dialog->mode_radiobutton_group = NULL;
    dialog->visibility_radiobutton_group = NULL;

    dialog->xfcalendar_dialog = gtk_dialog_new();
    gtk_window_set_default_size(GTK_WINDOW(dialog->xfcalendar_dialog), 300, 350);
    gtk_window_set_title(GTK_WINDOW(dialog->xfcalendar_dialog), _("Orage Preferences"));
    gtk_window_set_position(GTK_WINDOW(dialog->xfcalendar_dialog), GTK_WIN_POS_CENTER);
    gtk_window_set_modal(GTK_WINDOW(dialog->xfcalendar_dialog), FALSE);
    gtk_window_set_resizable(GTK_WINDOW(dialog->xfcalendar_dialog), FALSE);
    gtk_window_set_icon(GTK_WINDOW(dialog->xfcalendar_dialog), mcs_plugin->icon);

    gtk_dialog_set_has_separator(GTK_DIALOG(dialog->xfcalendar_dialog), FALSE);

    dialog->dialog_vbox1 = GTK_DIALOG(dialog->xfcalendar_dialog)->vbox;
    gtk_widget_show(dialog->dialog_vbox1);

    dialog->dialog_header = xfce_create_header(mcs_plugin->icon, _("Orage Preferences"));
    gtk_widget_show(dialog->dialog_header);
    gtk_box_pack_start(GTK_BOX(dialog->dialog_vbox1), dialog->dialog_header, FALSE, TRUE, 0);

    dialog->vbox1 = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(dialog->vbox1), BORDER);
    gtk_widget_show(dialog->vbox1);
    gtk_box_pack_start(GTK_BOX(dialog->dialog_vbox1), dialog->vbox1, TRUE, TRUE, 0);

    dialog->notebook = gtk_notebook_new ();
    gtk_widget_show (dialog->notebook);
    gtk_container_add (GTK_CONTAINER (dialog->vbox1), dialog->notebook);
    gtk_container_set_border_width (GTK_CONTAINER (dialog->notebook), 5);

    /* Here begins display tab */
    dialog->display_tab = xfce_framebox_new (NULL, FALSE);
    dialog->display_tab_label = gtk_label_new (_("Display"));
    gtk_notebook_append_page (GTK_NOTEBOOK (dialog->notebook)
                              , dialog->display_tab
                              , dialog->display_tab_label);
    gtk_widget_show (dialog->display_tab);
    gtk_widget_show (dialog->display_tab_label);

    dialog->display_vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (dialog->display_vbox);
    xfce_framebox_add (XFCE_FRAMEBOX (dialog->display_tab)
                       , dialog->display_vbox);

    /* Display calendar borders or not */
    dialog->mode_frame = xfce_framebox_new (_("Calendar borders"), TRUE);
    gtk_widget_show (dialog->mode_frame);
    gtk_box_pack_start(GTK_BOX(dialog->display_vbox), dialog->mode_frame, TRUE, TRUE, 0);
    dialog->mode_hbox = gtk_hbox_new(TRUE, 0);
    gtk_widget_show(dialog->mode_hbox);
    xfce_framebox_add(XFCE_FRAMEBOX(dialog->mode_frame), dialog->mode_hbox);

    dialog->normal_mode_radiobutton = gtk_radio_button_new_with_mnemonic(NULL, _("Displayed"));
    gtk_widget_show(dialog->normal_mode_radiobutton);
    gtk_box_pack_start(GTK_BOX(dialog->mode_hbox), dialog->normal_mode_radiobutton, FALSE, FALSE, 0);
    gtk_radio_button_set_group(GTK_RADIO_BUTTON(dialog->normal_mode_radiobutton), dialog->mode_radiobutton_group);
    dialog->mode_radiobutton_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON (dialog->normal_mode_radiobutton));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->normal_mode_radiobutton), normalmode);

    dialog->compact_mode_radiobutton = gtk_radio_button_new_with_mnemonic(NULL, _("Hidden"));
    gtk_widget_show(dialog->compact_mode_radiobutton);
    gtk_box_pack_start(GTK_BOX(dialog->mode_hbox), dialog->compact_mode_radiobutton, FALSE, FALSE, 0);
    gtk_radio_button_set_group(GTK_RADIO_BUTTON(dialog->compact_mode_radiobutton), dialog->mode_radiobutton_group);
    dialog->mode_radiobutton_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON (dialog->compact_mode_radiobutton));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->compact_mode_radiobutton), !normalmode);
    
    /* Show in... taskbar pager systray */
    dialog->show_frame = xfce_framebox_new(_("Calendar window"), TRUE);
    gtk_widget_show(dialog->show_frame);
    gtk_box_pack_start(GTK_BOX(dialog->display_vbox), dialog->show_frame, TRUE, TRUE, 5);
     
    dialog->show_vbox = gtk_vbox_new(TRUE, 0);
    gtk_widget_show(dialog->show_vbox);
    xfce_framebox_add(XFCE_FRAMEBOX(dialog->show_frame), dialog->show_vbox);

    dialog->show_taskbar_checkbutton = gtk_check_button_new_with_mnemonic(_("Show button in taskbar"));
    gtk_widget_show(dialog->show_taskbar_checkbutton);
    gtk_box_pack_start(GTK_BOX(dialog->show_vbox), dialog->show_taskbar_checkbutton, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->show_taskbar_checkbutton), showtaskbar);

    dialog->show_pager_checkbutton = gtk_check_button_new_with_mnemonic(_("Show in pager"));
    gtk_widget_show(dialog->show_pager_checkbutton);
    gtk_box_pack_start(GTK_BOX(dialog->show_vbox), dialog->show_pager_checkbutton, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (dialog->show_pager_checkbutton), showpager);

    dialog->show_systray_checkbutton = gtk_check_button_new_with_mnemonic(_("Show icon in systray"));
    gtk_widget_show(dialog->show_systray_checkbutton);
    gtk_box_pack_start(GTK_BOX(dialog->show_vbox), dialog->show_systray_checkbutton, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->show_systray_checkbutton), showsystray);

    /* */
    dialog->visibility_frame = xfce_framebox_new(_("Calendar start"), TRUE);
    gtk_widget_show(dialog->visibility_frame);
    gtk_box_pack_start(GTK_BOX(dialog->display_vbox), dialog->visibility_frame, TRUE, TRUE, 5);
    dialog->visibility_hbox = gtk_hbox_new(TRUE, 0);
    gtk_widget_show(dialog->visibility_hbox);
    xfce_framebox_add(XFCE_FRAMEBOX(dialog->visibility_frame), dialog->visibility_hbox);

    dialog->visibility_show_radiobutton = gtk_radio_button_new_with_mnemonic(NULL, _("Show"));
    gtk_widget_show(dialog->visibility_show_radiobutton);
    gtk_box_pack_start(GTK_BOX(dialog->visibility_hbox), dialog->visibility_show_radiobutton, FALSE, FALSE, 0);
    gtk_radio_button_set_group(GTK_RADIO_BUTTON(dialog->visibility_show_radiobutton), dialog->visibility_radiobutton_group);
    dialog->visibility_radiobutton_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(dialog->visibility_show_radiobutton));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->visibility_show_radiobutton), showstart);

    dialog->visibility_hide_radiobutton = gtk_radio_button_new_with_mnemonic(NULL, _("Hide"));
    gtk_widget_show(dialog->visibility_hide_radiobutton);
    gtk_box_pack_start(GTK_BOX(dialog->visibility_hbox), dialog->visibility_hide_radiobutton, FALSE, FALSE, 0);
    gtk_radio_button_set_group(GTK_RADIO_BUTTON(dialog->visibility_hide_radiobutton), dialog->visibility_radiobutton_group);
    dialog->visibility_radiobutton_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(dialog->visibility_hide_radiobutton));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->visibility_hide_radiobutton), hidestart);

    dialog->visibility_minimized_radiobutton = gtk_radio_button_new_with_mnemonic(NULL, _("Minimized"));
    gtk_widget_show(dialog->visibility_minimized_radiobutton);
    gtk_box_pack_start(GTK_BOX(dialog->visibility_hbox), dialog->visibility_minimized_radiobutton, FALSE, FALSE, 0);
    gtk_radio_button_set_group(GTK_RADIO_BUTTON(dialog->visibility_minimized_radiobutton), dialog->visibility_radiobutton_group);
    dialog->visibility_radiobutton_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(dialog->visibility_minimized_radiobutton));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->visibility_minimized_radiobutton), ministart);

    /* Here begins archives tab */
    dialog->archives_tab = xfce_framebox_new (NULL, FALSE);
    dialog->archives_tab_label = gtk_label_new (_("Archives"));
    gtk_notebook_append_page (GTK_NOTEBOOK (dialog->notebook)
                              , dialog->archives_tab
                              , dialog->archives_tab_label);
    gtk_widget_show (dialog->archives_tab);
    gtk_widget_show (dialog->archives_tab_label);

    dialog->archive_vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (dialog->archive_vbox);
    xfce_framebox_add (XFCE_FRAMEBOX (dialog->archives_tab)
                       , dialog->archive_vbox);
    /* Archive file and periodicity */
    dialog->archive_file_frame = xfce_framebox_new (_("Archive file"), TRUE);
    gtk_widget_show (dialog->archive_file_frame);
    gtk_box_pack_start (GTK_BOX (dialog->archive_vbox), dialog->archive_file_frame, TRUE, TRUE, 5);

    dialog->archive_file_table = gtk_table_new (1, 2, FALSE);
    gtk_widget_show (dialog->archive_file_table);
    gtk_container_set_border_width (GTK_CONTAINER (dialog->archive_file_table), 10);
    gtk_table_set_row_spacings (GTK_TABLE (dialog->archive_file_table), 6);
    gtk_table_set_col_spacings (GTK_TABLE (dialog->archive_file_table), 6);
    xfce_framebox_add (XFCE_FRAMEBOX (dialog->archive_file_frame)
                       , dialog->archive_file_table);

    dialog->archive_file_entry = gtk_entry_new ();
    gtk_widget_show (dialog->archive_file_entry);
    gtk_table_attach (GTK_TABLE (dialog->archive_file_table), dialog->archive_file_entry, 0, 1, 0, 1,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);
    gtk_entry_set_text(GTK_ENTRY(dialog->archive_file_entry), (const gchar *) archive_path);

    dialog->archive_open_file_button = gtk_button_new_from_stock("gtk-open");
    gtk_widget_show (dialog->archive_open_file_button);
    gtk_table_attach (GTK_TABLE (dialog->archive_file_table), dialog->archive_open_file_button, 1, 2, 0, 1,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);

    dialog->archive_threshold_frame = xfce_framebox_new (_("Archive threshold"), TRUE);
    gtk_widget_show (dialog->archive_threshold_frame);
    gtk_box_pack_start (GTK_BOX (dialog->archive_vbox), dialog->archive_threshold_frame, TRUE, TRUE, 5);

    dialog->archive_threshold_table = gtk_table_new (1, 1, FALSE);
    gtk_widget_show (dialog->archive_threshold_table);
    gtk_container_set_border_width (GTK_CONTAINER (dialog->archive_threshold_table), 10);
    gtk_table_set_row_spacings (GTK_TABLE (dialog->archive_threshold_table), 6);
    gtk_table_set_col_spacings (GTK_TABLE (dialog->archive_threshold_table), 6);
    xfce_framebox_add (XFCE_FRAMEBOX (dialog->archive_threshold_frame)
                       , dialog->archive_threshold_table);

    dialog->archive_threshold_combobox = gtk_combo_box_new_text ();
    gtk_combo_box_append_text (GTK_COMBO_BOX (dialog->archive_threshold_combobox), _("3 months"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (dialog->archive_threshold_combobox), _("6 months"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (dialog->archive_threshold_combobox), _("1 year"));
    switch (archive_threshold) {
            case 6: gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->archive_threshold_combobox), 1);
                    break;
            case 12:gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->archive_threshold_combobox), 2);
                    break;
            default:gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->archive_threshold_combobox), 0);
    }
    gtk_widget_show (dialog->archive_threshold_combobox);

    gtk_table_attach (GTK_TABLE (dialog->archive_threshold_table), dialog->archive_threshold_combobox, 0, 1, 0, 1,
                        (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);

    /* Here begins the sound tab */
    dialog->sound_tab = xfce_framebox_new (NULL, FALSE);
    dialog->sound_tab_label = gtk_label_new (_("Sound"));
    gtk_notebook_append_page (GTK_NOTEBOOK (dialog->notebook)
                              , dialog->sound_tab
                              , dialog->sound_tab_label);
    gtk_widget_show (dialog->sound_tab);
    gtk_widget_show (dialog->sound_tab_label);

    /* Choose a sound application for reminders */
    dialog->sound_tab_table = gtk_table_new (1, 2, FALSE); 
    gtk_container_set_border_width (GTK_CONTAINER (dialog->sound_tab_table), 10);
    gtk_table_set_row_spacings (GTK_TABLE (dialog->sound_tab_table), 6);
    gtk_table_set_col_spacings (GTK_TABLE (dialog->sound_tab_table), 6);
    gtk_widget_show (dialog->sound_tab_table);
    xfce_framebox_add (XFCE_FRAMEBOX (dialog->sound_tab)
                       , dialog->sound_tab_table);

    dialog->sound_application_label = gtk_label_new (_("Application:"));
    gtk_widget_show (dialog->sound_application_label);
    gtk_table_attach (GTK_TABLE (dialog->sound_tab_table)
                      , dialog->sound_application_label, 0, 1, 0, 1
                      , (GtkAttachOptions) (GTK_FILL)
                      , (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (dialog->sound_application_label), 0, 0.5);

    dialog->sound_application_entry = gtk_entry_new();
    gtk_widget_show(dialog->sound_application_entry);
    gtk_table_attach (GTK_TABLE (dialog->sound_tab_table)
                      , dialog->sound_application_entry, 1, 2, 0, 1
                      , (GtkAttachOptions) (GTK_EXPAND | GTK_FILL)
                      , (GtkAttachOptions) (0), 0, 0);

    g_warning("Sound application to be displayed: %s\n", sound_application);
    gtk_entry_set_text(GTK_ENTRY(dialog->sound_application_entry), (const gchar *)sound_application);
    /* */
    dialog->close_button = gtk_button_new_from_stock ("gtk-close");
    gtk_widget_show (dialog->close_button);
    gtk_dialog_add_action_widget (GTK_DIALOG (dialog->xfcalendar_dialog), dialog->close_button, GTK_RESPONSE_CLOSE);
    GTK_WIDGET_SET_FLAGS (dialog->close_button, GTK_CAN_DEFAULT);

    return dialog;
}

static void setup_dialog (Itf * itf)
{
    g_signal_connect (G_OBJECT (itf->xfcalendar_dialog), "response", G_CALLBACK (cb_dialog_response), itf->mcs_plugin);

    g_signal_connect (G_OBJECT (itf->normal_mode_radiobutton), "toggled", G_CALLBACK (cb_mode_changed), itf);
    g_signal_connect (G_OBJECT (itf->show_taskbar_checkbutton), "toggled", G_CALLBACK (cb_taskbar_changed), itf);
    g_signal_connect (G_OBJECT (itf->show_pager_checkbutton), "toggled", G_CALLBACK (cb_pager_changed), itf);
    g_signal_connect (G_OBJECT (itf->show_systray_checkbutton), "toggled", G_CALLBACK (cb_systray_changed), itf);
    g_signal_connect (G_OBJECT (itf->visibility_show_radiobutton), "toggled", G_CALLBACK (cb_start_changed), itf);
    g_signal_connect (G_OBJECT (itf->visibility_minimized_radiobutton), "toggled", G_CALLBACK (cb_start_changed), itf);
    g_signal_connect (G_OBJECT (itf->sound_application_entry), "changed", G_CALLBACK (cb_sound_application_changed), itf);
    g_signal_connect (G_OBJECT (itf->archive_open_file_button), "clicked", G_CALLBACK (cb_archive_open_file_button_clicked), itf);
    g_signal_connect (G_OBJECT (itf->archive_file_entry), "changed", G_CALLBACK (cb_archive_file_entry_changed), itf);
    g_signal_connect (G_OBJECT (itf->archive_threshold_combobox), "changed", G_CALLBACK (cb_archive_threshold_combobox_changed), itf);

    xfce_gtk_window_center_on_monitor_with_pointer (GTK_WINDOW (itf->xfcalendar_dialog));
    gtk_widget_show (itf->xfcalendar_dialog);
}

McsPluginInitResult mcs_plugin_init(McsPlugin * mcs_plugin)
{
    /* This is required for UTF-8 at least - Please don't remove it */
    xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    create_channel(mcs_plugin);
    mcs_plugin->plugin_name = g_strdup(PLUGIN_NAME);
    mcs_plugin->caption = g_strdup(_("Orage"));
    mcs_plugin->run_dialog = run_dialog;
    mcs_plugin->icon = xfce_themed_icon_load ("xfcalendar", 48);
    mcs_manager_notify(mcs_plugin->manager, CHANNEL);

    return (MCS_PLUGIN_INIT_OK);
}

static void create_channel(McsPlugin * mcs_plugin)
{
    McsSetting *setting;
    gchar *rcfile;
    
    rcfile = xfce_resource_lookup (XFCE_RESOURCE_CONFIG, 
                                   RCDIR G_DIR_SEPARATOR_S RCFILE);

    if (!rcfile)
        rcfile = xfce_get_userfile(OLDRCDIR, RCFILE, NULL);

    if (g_file_test (rcfile, G_FILE_TEST_EXISTS))
        mcs_manager_add_channel_from_file(mcs_plugin->manager, CHANNEL, rcfile);
    else
        mcs_manager_add_channel (mcs_plugin->manager, CHANNEL);
    
    g_free(rcfile);

    setting = mcs_manager_setting_lookup(mcs_plugin->manager, "XFCalendar/NormalMode", CHANNEL);
    if(setting)
    {
        normalmode = setting->data.v_int ? TRUE: FALSE;
    }
    else
    {
        normalmode = TRUE;
        mcs_manager_set_int(mcs_plugin->manager, "XFCalendar/NormalMode", CHANNEL, normalmode ? 1 : 0);
    }
    
    setting = mcs_manager_setting_lookup(mcs_plugin->manager, "XFCalendar/TaskBar", CHANNEL);
    if(setting)
    {
      showtaskbar = setting->data.v_int ? TRUE: FALSE;
    }
    else
    {
      showtaskbar = TRUE;
      mcs_manager_set_int(mcs_plugin->manager, "XFCalendar/TaskBar", CHANNEL, showtaskbar ? 1 : 0);
    }
    setting = mcs_manager_setting_lookup(mcs_plugin->manager, "XFCalendar/Pager", CHANNEL);
    if(setting)
    {
      showpager = setting->data.v_int ? TRUE: FALSE;
    }
    else
    {
      showpager = TRUE;
      mcs_manager_set_int(mcs_plugin->manager, "XFCalendar/Pager", CHANNEL, showpager ? 1 : 0);
    }
    setting = mcs_manager_setting_lookup(mcs_plugin->manager, "XFCalendar/Systray", CHANNEL);
    if(setting)
    {
      showsystray = setting->data.v_int ? TRUE: FALSE;
    }
    else
    {
      showsystray = TRUE;
      mcs_manager_set_int(mcs_plugin->manager, "XFCalendar/Systray", CHANNEL, showsystray ? 1 : 0);
    }

    setting = mcs_manager_setting_lookup(mcs_plugin->manager, "XFCalendar/ShowStart", CHANNEL);
    if(setting)
    {
        showstart = FALSE;
        hidestart = FALSE;
        ministart = FALSE;
        switch (setting->data.v_int) {
            case 0: hidestart = TRUE; break;
            case 1: showstart = TRUE; break;
            case 2: ministart = TRUE; break;
            default: showstart = TRUE;
        }
        showstart = setting->data.v_int ? TRUE: FALSE;
    }
    else
    {
        showstart = TRUE;
        mcs_manager_set_int(mcs_plugin->manager, "XFCalendar/ShowStart", CHANNEL, 1);
    }

    setting = mcs_manager_setting_lookup (mcs_plugin->manager, "XFCalendar/ArchiveFile", CHANNEL);
    if (setting)
    {
        archive_path = (gchar *)malloc(255);
        archive_path = NULL;
        if (archive_path = setting->data.v_string) {
        	g_warning("Archive file: %s\n", archive_path);
        }
    }

    setting = mcs_manager_setting_lookup (mcs_plugin->manager, "XFCalendar/Lookback", CHANNEL);
    if (setting)
    {
        switch (setting->data.v_int) {
            case 6: archive_threshold = 6;
                    break;
            case 12: archive_threshold = 12;
                    break;
            default:archive_threshold = 3;
        }      
    }

    setting = mcs_manager_setting_lookup(mcs_plugin->manager, "XFCalendar/SoundApplication", CHANNEL);
    if(setting)
    {
        sound_application = (gchar*)malloc(255);
        if(!(sound_application = setting->data.v_string)){
            sound_application = "play";
        }
    	g_warning("Sound application read from file: %s\n", sound_application);
    }
    write_options (mcs_plugin);
}

static gboolean write_options(McsPlugin * mcs_plugin)
{
    gchar *rcfile;
    gboolean result;

    
    rcfile = xfce_resource_save_location (XFCE_RESOURCE_CONFIG,
                                          RCDIR G_DIR_SEPARATOR_S RCFILE,
                                          TRUE);

    result = mcs_manager_save_channel_to_file(mcs_plugin->manager, CHANNEL, rcfile);
    g_free(rcfile);

    return result;
}

static void run_dialog(McsPlugin * mcs_plugin)
{
    Itf *dialog;

    if(is_running)
        return;

    is_running = TRUE;

    xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    dialog = create_xfcalendar_dialog(mcs_plugin);
    setup_dialog(dialog);
}

/* macro defined in manager-plugin.h */
MCS_PLUGIN_CHECK_INIT
