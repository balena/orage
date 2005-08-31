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

        xfccalendar mcs plugin   - (c) 2003-2005 Mickael Graf <korbinus at xfce.org>
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
#define PLUGIN_NAME "xfcalendar"

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
static gchar *soundAppl;
static gchar *archivePath;
static int archiveLookback;

typedef struct _Itf Itf;
struct _Itf
{
    McsPlugin *mcs_plugin;

    GtkWidget *xfcalendar_dialog;
    GtkWidget *dialog_header;
    GtkWidget *dialog_vbox1;
    GtkWidget *hbox1;
    GtkWidget *vbox1;
    /* Mode normal or compact */
    GSList    *mode_radiobutton_group;
    GtkWidget *hboxMode;
    GtkWidget *frameMode;
    GtkWidget *CompactMode_radiobutton;
    GtkWidget *NormalMode_radiobutton;
    /* Show in... taskbar pager systray */
    GtkWidget *show_taskbar_checkbutton;
    GtkWidget *show_pager_checkbutton;
    GtkWidget *show_systray_checkbutton;
    GtkWidget *hboxShow;
    GtkWidget *frameShow;
    /* Start visibity show or hide */
    GSList    *start_radiobutton_group;
    GtkWidget *hboxStart;
    GtkWidget *frameStart;
    GtkWidget *ShowStart_radiobutton;
    GtkWidget *HideStart_radiobutton;
    GtkWidget *MiniStart_radiobutton;
    /* Archive file and periodicity */
    GtkWidget *labelTopArchive;
    GtkWidget *labelBottomArchive;
    GtkWidget *frameArchive;
    GtkWidget *tableArchive;
    GtkWidget *hboxArchive;
    GtkWidget *hboxFileArchive;
    GtkWidget *hboxPeriodicityArchive;
    GtkWidget *leftVboxArchive;
    GtkWidget *rightVboxArchive;
    GtkWidget *entryArchive;
    GtkWidget *buttonArchive;
    GtkWidget *comboboxArchive;
    /* Choose the sound application for reminders */
    GtkWidget *hboxSoundApplication;
    GtkWidget *frameSoundApplication;
    GtkWidget *labelSoundApplication;
    GtkWidget *entrySoundApplication;
    /* */
    GtkWidget *closebutton;
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

static void cb_SoundApplication_changed(GtkWidget * dialog, gpointer user_data)
{
    Itf *itf = (Itf *) user_data;
    McsPlugin *mcs_plugin = itf->mcs_plugin;
    
    soundAppl = g_strdup(gtk_entry_get_text(GTK_ENTRY(itf->entrySoundApplication)));
    mcs_manager_set_string(mcs_plugin->manager, "XFCalendar/SoundApplication", CHANNEL, soundAppl);
    mcs_manager_notify(mcs_plugin->manager, CHANNEL);
    write_options(mcs_plugin);
}

void static cb_entryArchive_changed (GtkWidget * dialog, gpointer user_data)
{
    Itf *itf = (Itf *) user_data;
    McsPlugin *mcs_plugin = itf->mcs_plugin;

    archivePath = g_strdup (gtk_entry_get_text (GTK_ENTRY (itf->entryArchive)));
    mcs_manager_set_string (mcs_plugin->manager, "XFCalendar/ArchiveFile", CHANNEL, archivePath);
    mcs_manager_notify (mcs_plugin->manager, CHANNEL);
    write_options (mcs_plugin);
}

static void cb_mode_changed(GtkWidget * dialog, gpointer user_data)
{
    Itf *itf = (Itf *) user_data;
    McsPlugin *mcs_plugin = itf->mcs_plugin;

    normalmode = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(itf->NormalMode_radiobutton));

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

    showstart = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(itf->ShowStart_radiobutton));
    hidestart = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(itf->HideStart_radiobutton));
    ministart = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(itf->MiniStart_radiobutton));

    mcs_manager_set_int(mcs_plugin->manager, "XFCalendar/ShowStart", CHANNEL, showstart ? 1 : (hidestart ? 0 : 2));
    mcs_manager_notify(mcs_plugin->manager, CHANNEL);
    write_options(mcs_plugin);
}

static void cb_buttonArchive_clicked (GtkButton *button, gpointer user_data)
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
            gtk_entry_set_text (GTK_ENTRY (itf->entryArchive), (const gchar*) archive_path);
        }
    }

    gtk_widget_destroy (file_chooser);
}

static void cb_comboboxArchive_changed (GtkComboBox *cb, gpointer user_data)
{

    Itf *itf = (Itf *) user_data;
    McsPlugin *mcs_plugin = itf->mcs_plugin;
    int index;

    index = gtk_combo_box_get_active (GTK_COMBO_BOX (itf->comboboxArchive));
    switch (index) {
        case 0: archiveLookback = 3;
                break;
        case 1: archiveLookback = 6;
                break;
        case 2: archiveLookback = 12;
                break;
    }
/*
    if (index == 0)
        archiveLookback = 3;
    if (index == 1)
        archiveLookback = 6;
    if (index == 2)
        archiveLookback = 12;
*/
    mcs_manager_set_int (mcs_plugin->manager, "XFCalendar/Lookback", CHANNEL, archiveLookback);
    mcs_manager_notify (mcs_plugin->manager, CHANNEL);
    write_options (mcs_plugin);
}

Itf *create_xfcalendar_dialog(McsPlugin * mcs_plugin)
{
    Itf *dialog;

    dialog = g_new(Itf, 1);

    dialog->mcs_plugin = mcs_plugin;
    dialog->mode_radiobutton_group = NULL;
    dialog->start_radiobutton_group = NULL;

    dialog->xfcalendar_dialog = gtk_dialog_new();
    gtk_window_set_default_size(GTK_WINDOW(dialog->xfcalendar_dialog), 400, 350);
    gtk_window_set_title(GTK_WINDOW(dialog->xfcalendar_dialog), _("Xfcalendar Preferences"));
    gtk_window_set_position(GTK_WINDOW(dialog->xfcalendar_dialog), GTK_WIN_POS_CENTER);
    gtk_window_set_modal(GTK_WINDOW(dialog->xfcalendar_dialog), FALSE);
    gtk_window_set_resizable(GTK_WINDOW(dialog->xfcalendar_dialog), FALSE);
    gtk_window_set_icon(GTK_WINDOW(dialog->xfcalendar_dialog), mcs_plugin->icon);

    gtk_dialog_set_has_separator(GTK_DIALOG(dialog->xfcalendar_dialog), FALSE);

    dialog->dialog_vbox1 = GTK_DIALOG(dialog->xfcalendar_dialog)->vbox;
    gtk_widget_show(dialog->dialog_vbox1);

    dialog->dialog_header = xfce_create_header(mcs_plugin->icon, _("Xfcalendar Preferences"));
    gtk_widget_show(dialog->dialog_header);
    gtk_box_pack_start(GTK_BOX(dialog->dialog_vbox1), dialog->dialog_header, FALSE, TRUE, 0);

    dialog->hbox1 = gtk_hbox_new(TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER (dialog->hbox1), BORDER + 1);
    gtk_widget_show(dialog->hbox1);
    gtk_box_pack_start(GTK_BOX(dialog->dialog_vbox1), dialog->hbox1, TRUE, TRUE, 0);
    
    dialog->vbox1 = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(dialog->vbox1), BORDER);
    gtk_widget_show(dialog->vbox1);
    gtk_box_pack_start(GTK_BOX(dialog->hbox1), dialog->vbox1, TRUE, TRUE, 0);

    /* Mode normal or compact */
    dialog->frameMode = xfce_framebox_new(_("Mode"), TRUE);
    gtk_widget_show(dialog->frameMode);
    gtk_box_pack_start(GTK_BOX(dialog->vbox1), dialog->frameMode, TRUE, TRUE, 0);
    
    dialog->hboxMode = gtk_hbox_new(TRUE, 0);
    gtk_widget_show(dialog->hboxMode);
    xfce_framebox_add(XFCE_FRAMEBOX(dialog->frameMode), dialog->hboxMode);

    dialog->NormalMode_radiobutton = gtk_radio_button_new_with_mnemonic(NULL, _("Normal"));
    gtk_widget_show(dialog->NormalMode_radiobutton);
    gtk_box_pack_start(GTK_BOX(dialog->hboxMode), dialog->NormalMode_radiobutton, FALSE, FALSE, 0);
    gtk_radio_button_set_group(GTK_RADIO_BUTTON(dialog->NormalMode_radiobutton), dialog->mode_radiobutton_group);
    dialog->mode_radiobutton_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON (dialog->NormalMode_radiobutton));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->NormalMode_radiobutton), normalmode);

    dialog->CompactMode_radiobutton = gtk_radio_button_new_with_mnemonic(NULL, _("Compact"));
    gtk_widget_show(dialog->CompactMode_radiobutton);
    gtk_box_pack_start(GTK_BOX(dialog->hboxMode), dialog->CompactMode_radiobutton, FALSE, FALSE, 0);
    gtk_radio_button_set_group(GTK_RADIO_BUTTON(dialog->CompactMode_radiobutton), dialog->mode_radiobutton_group);
    dialog->mode_radiobutton_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON (dialog->CompactMode_radiobutton));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->CompactMode_radiobutton), !normalmode);
    
    /* Show in... taskbar pager systray */
    dialog->frameShow = xfce_framebox_new(_("Show the calendar window in..."), TRUE);
    gtk_widget_show(dialog->frameShow);
    gtk_frame_set_shadow_type(GTK_FRAME(dialog->frameShow), GTK_SHADOW_ETCHED_IN);
    gtk_box_pack_start(GTK_BOX(dialog->vbox1), dialog->frameShow, TRUE, TRUE, 5);
    
    dialog->hboxShow = gtk_hbox_new(TRUE, 0);
    gtk_widget_show(dialog->hboxShow);
    xfce_framebox_add(XFCE_FRAMEBOX(dialog->frameShow), dialog->hboxShow);

    dialog->show_taskbar_checkbutton = gtk_check_button_new_with_mnemonic(_("Taskbar"));
    gtk_widget_show(dialog->show_taskbar_checkbutton);
    gtk_box_pack_start(GTK_BOX(dialog->hboxShow), dialog->show_taskbar_checkbutton, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->show_taskbar_checkbutton), showtaskbar);

    dialog->show_pager_checkbutton = gtk_check_button_new_with_mnemonic(_("Pager"));
    gtk_widget_show(dialog->show_pager_checkbutton);
    gtk_box_pack_start(GTK_BOX(dialog->hboxShow), dialog->show_pager_checkbutton, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (dialog->show_pager_checkbutton), showpager);

    dialog->show_systray_checkbutton = gtk_check_button_new_with_mnemonic(_("Systray"));
    gtk_widget_show(dialog->show_systray_checkbutton);
    gtk_box_pack_start(GTK_BOX(dialog->hboxShow), dialog->show_systray_checkbutton, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->show_systray_checkbutton), showsystray);

    /* */
    dialog->frameStart = xfce_framebox_new(_("Visibility when starting..."), TRUE);
    gtk_widget_show(dialog->frameStart);
    gtk_frame_set_shadow_type(GTK_FRAME(dialog->frameStart), GTK_SHADOW_ETCHED_IN);
    gtk_box_pack_start(GTK_BOX(dialog->vbox1), dialog->frameStart, TRUE, TRUE, 5);
    
    dialog->hboxStart = gtk_hbox_new(TRUE, 0);
    gtk_widget_show(dialog->hboxStart);
    xfce_framebox_add(XFCE_FRAMEBOX(dialog->frameStart), dialog->hboxStart);

    dialog->ShowStart_radiobutton = gtk_radio_button_new_with_mnemonic(NULL, _("Show"));
    gtk_widget_show(dialog->ShowStart_radiobutton);
    gtk_box_pack_start(GTK_BOX(dialog->hboxStart), dialog->ShowStart_radiobutton, FALSE, FALSE, 0);
    gtk_radio_button_set_group(GTK_RADIO_BUTTON(dialog->ShowStart_radiobutton), dialog->start_radiobutton_group);
    dialog->start_radiobutton_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(dialog->ShowStart_radiobutton));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->ShowStart_radiobutton), showstart);

    dialog->HideStart_radiobutton = gtk_radio_button_new_with_mnemonic(NULL, _("Hide"));
    gtk_widget_show(dialog->HideStart_radiobutton);
    gtk_box_pack_start(GTK_BOX(dialog->hboxStart), dialog->HideStart_radiobutton, FALSE, FALSE, 0);
    gtk_radio_button_set_group(GTK_RADIO_BUTTON(dialog->HideStart_radiobutton), dialog->start_radiobutton_group);
    dialog->start_radiobutton_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(dialog->HideStart_radiobutton));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->HideStart_radiobutton), hidestart);

    dialog->MiniStart_radiobutton = gtk_radio_button_new_with_mnemonic(NULL, _("Minimized"));
    gtk_widget_show(dialog->MiniStart_radiobutton);
    gtk_box_pack_start(GTK_BOX(dialog->hboxStart), dialog->MiniStart_radiobutton, FALSE, FALSE, 0);
    gtk_radio_button_set_group(GTK_RADIO_BUTTON(dialog->MiniStart_radiobutton), dialog->start_radiobutton_group);
    dialog->start_radiobutton_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(dialog->MiniStart_radiobutton));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->MiniStart_radiobutton), ministart);

    /* Archive file and periodicity */
    dialog->frameArchive = xfce_framebox_new (_("Archiving..."), TRUE);
    gtk_widget_show (dialog->frameArchive);
    gtk_frame_set_shadow_type (GTK_FRAME (dialog->frameArchive), GTK_SHADOW_ETCHED_IN);
    gtk_box_pack_start (GTK_BOX (dialog->vbox1), dialog->frameArchive, TRUE, TRUE, 5);

    dialog->tableArchive = gtk_table_new (2, 3, FALSE);
    gtk_widget_show (dialog->tableArchive);
    gtk_container_set_border_width (GTK_CONTAINER (dialog->tableArchive), 10);
    gtk_table_set_row_spacings (GTK_TABLE (dialog->tableArchive), 6);
    gtk_table_set_col_spacings (GTK_TABLE (dialog->tableArchive), 6);
    xfce_framebox_add (XFCE_FRAMEBOX (dialog->frameArchive), dialog->tableArchive);

    dialog->labelTopArchive = gtk_label_new (_("Archive file"));
    gtk_widget_show (dialog->labelTopArchive);
    gtk_table_attach (GTK_TABLE (dialog->tableArchive), dialog->labelTopArchive, 0, 1, 0, 1,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (dialog->labelTopArchive), 0, 0.5);

    dialog->entryArchive = gtk_entry_new ();
    gtk_widget_show (dialog->entryArchive);
    gtk_table_attach (GTK_TABLE (dialog->tableArchive), dialog->entryArchive, 1, 2, 0, 1,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);
    gtk_entry_set_text(GTK_ENTRY(dialog->entryArchive), (const gchar *) archivePath);

    dialog->buttonArchive = gtk_button_new_from_stock("gtk-open");
    gtk_widget_show (dialog->buttonArchive);
    gtk_table_attach (GTK_TABLE (dialog->tableArchive), dialog->buttonArchive, 2, 3, 0, 1,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);

    dialog->labelBottomArchive = gtk_label_new (_("Archive items older than"));
    gtk_widget_show (dialog->labelBottomArchive);
    gtk_table_attach (GTK_TABLE (dialog->tableArchive), dialog->labelBottomArchive, 0, 1, 1, 2,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (dialog->labelBottomArchive), 0, 0.5);

    dialog->comboboxArchive = gtk_combo_box_new_text ();
    gtk_combo_box_append_text (GTK_COMBO_BOX (dialog->comboboxArchive), _("3 months"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (dialog->comboboxArchive), _("6 months"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (dialog->comboboxArchive), _("1 year"));
    switch (archiveLookback) {
            case 6: gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->comboboxArchive), 1);
                    break;
            case 12:gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->comboboxArchive), 2);
                    break;
            default:gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->comboboxArchive), 0);
    }
    gtk_widget_show (dialog->comboboxArchive);

    gtk_table_attach (GTK_TABLE (dialog->tableArchive), dialog->comboboxArchive, 1, 2, 1, 2,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);

    /* Choose a sound application for reminders */
    dialog->frameSoundApplication = xfce_framebox_new(_("Play sounds with..."), TRUE);
    gtk_widget_show(dialog->frameSoundApplication);
    gtk_frame_set_shadow_type(GTK_FRAME(dialog->frameSoundApplication), GTK_SHADOW_ETCHED_IN);
    gtk_box_pack_start(GTK_BOX(dialog->vbox1), dialog->frameSoundApplication, TRUE, TRUE, 5);
    
    dialog->hboxSoundApplication = gtk_hbox_new(TRUE, 0);
    gtk_widget_show(dialog->hboxSoundApplication);
    xfce_framebox_add(XFCE_FRAMEBOX(dialog->frameSoundApplication), dialog->hboxSoundApplication);
    
    dialog->labelSoundApplication = gtk_label_new(_("Application"));
    gtk_widget_show(dialog->labelSoundApplication);
    gtk_box_pack_start(GTK_BOX(dialog->hboxSoundApplication), dialog->labelSoundApplication, FALSE, FALSE, 0);
    
    dialog->entrySoundApplication = gtk_entry_new();
    gtk_widget_show(dialog->entrySoundApplication);
    gtk_box_pack_start(GTK_BOX(dialog->hboxSoundApplication), dialog->entrySoundApplication, FALSE, FALSE, 0);
    g_warning("Sound application to be displayed: %s\n", soundAppl);
    gtk_entry_set_text(GTK_ENTRY(dialog->entrySoundApplication), (const gchar *)soundAppl);
    /* */
    dialog->closebutton = gtk_button_new_from_stock ("gtk-close");
    gtk_widget_show (dialog->closebutton);
    gtk_dialog_add_action_widget (GTK_DIALOG (dialog->xfcalendar_dialog), dialog->closebutton, GTK_RESPONSE_CLOSE);
    GTK_WIDGET_SET_FLAGS (dialog->closebutton, GTK_CAN_DEFAULT);

    return dialog;
}

static void setup_dialog (Itf * itf)
{
    g_signal_connect (G_OBJECT (itf->xfcalendar_dialog), "response", G_CALLBACK (cb_dialog_response), itf->mcs_plugin);

    g_signal_connect (G_OBJECT (itf->NormalMode_radiobutton), "toggled", G_CALLBACK (cb_mode_changed), itf);
    g_signal_connect (G_OBJECT (itf->show_taskbar_checkbutton), "toggled", G_CALLBACK (cb_taskbar_changed), itf);
    g_signal_connect (G_OBJECT (itf->show_pager_checkbutton), "toggled", G_CALLBACK (cb_pager_changed), itf);
    g_signal_connect (G_OBJECT (itf->show_systray_checkbutton), "toggled", G_CALLBACK (cb_systray_changed), itf);
    g_signal_connect (G_OBJECT (itf->ShowStart_radiobutton), "toggled", G_CALLBACK (cb_start_changed), itf);
    g_signal_connect (G_OBJECT (itf->MiniStart_radiobutton), "toggled", G_CALLBACK (cb_start_changed), itf);
    g_signal_connect (G_OBJECT (itf->entrySoundApplication), "changed", G_CALLBACK (cb_SoundApplication_changed), itf);
    g_signal_connect (G_OBJECT (itf->buttonArchive), "clicked", G_CALLBACK (cb_buttonArchive_clicked), itf);
    g_signal_connect (G_OBJECT (itf->entryArchive), "changed", G_CALLBACK (cb_entryArchive_changed), itf);
    g_signal_connect (G_OBJECT (itf->comboboxArchive), "changed", G_CALLBACK (cb_comboboxArchive_changed), itf);

    xfce_gtk_window_center_on_monitor_with_pointer (GTK_WINDOW (itf->xfcalendar_dialog));
    gtk_widget_show (itf->xfcalendar_dialog);
}

McsPluginInitResult mcs_plugin_init(McsPlugin * mcs_plugin)
{
    /* This is required for UTF-8 at least - Please don't remove it */
    xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    create_channel(mcs_plugin);
    mcs_plugin->plugin_name = g_strdup(PLUGIN_NAME);
    mcs_plugin->caption = g_strdup(_("Xfcalendar"));
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
        archivePath = (gchar *)malloc(255);
        archivePath = NULL;
        if (archivePath = setting->data.v_string) {
        	g_warning("Archive file: %s\n", archivePath);
        }
    }

    setting = mcs_manager_setting_lookup (mcs_plugin->manager, "XFCalendar/Lookback", CHANNEL);
    if (setting)
    {
        switch (setting->data.v_int) {
            case 6: archiveLookback = 6;
                    break;
            case 12: archiveLookback = 12;
                    break;
            default:archiveLookback = 3;
        }      
    }

    setting = mcs_manager_setting_lookup(mcs_plugin->manager, "XFCalendar/SoundApplication", CHANNEL);
    if(setting)
    {
        soundAppl = (gchar*)malloc(255);
        if(!(soundAppl = setting->data.v_string)){
            soundAppl = "play";
        }
    	g_warning("Sound application read from file: %s\n", soundAppl);
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
