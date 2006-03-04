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

    orage mcs plugin   - (c) 2003-2005 Mickael Graf <korbinus at xfce.org>
                       - (c) 2005      Juha Kautto <juha at xfce.org>

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
#define OLDRCDIR "orage"
#define CHANNEL  "orage"
#define RCFILE   "orage.xml"
#define ARCDIR   "xfce4" G_DIR_SEPARATOR_S "orage" G_DIR_SEPARATOR_S
#define ARCFILE  "orage_old.ics" 
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
static gchar *sound_application = NULL;
static gchar *archive_path = NULL;
static gchar *local_timezone = NULL;
static int archive_threshold = 6;

enum {
    LOCATION,
    LOCATION_ENG,
    N_COLUMNS
};

typedef struct _Itf Itf;
struct _Itf
{
    McsPlugin *mcs_plugin;

    GtkWidget *orage_dialog;
    GtkWidget *dialog_header;
    GtkWidget *dialog_vbox1;
    GtkWidget *notebook;
    /* Tabs */
    GtkWidget *display_tab;
    GtkWidget *display_tab_label;
    GtkWidget *display_vbox;
    GtkWidget *archives_tab;
    GtkWidget *archives_tab_label;
    GtkWidget *archive_vbox;
    GtkWidget *sound_tab;
    GtkWidget *sound_tab_label;
    GtkWidget *sound_vbox;
    GtkWidget *timezone_tab;
    GtkWidget *timezone_tab_label;
    GtkWidget *timezone_vbox;
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
    GtkWidget *archive_file_frame;
    GtkWidget *archive_file_table;
    GtkWidget *archive_threshold_label;
    GtkWidget *archive_file_entry;
    GtkWidget *archive_file_open_button;
    GtkWidget *archive_threshold_frame;
    GtkWidget *archive_threshold_table;
    GtkWidget *archive_threshold_combobox;
    /* Choose the sound application for reminders */
    GtkWidget *sound_application_entry;
    GtkWidget *sound_application_frame;
    GtkWidget *sound_application_table;
    GtkWidget *sound_application_open_button;
    /* Choose the timezone for appointments */
    GtkWidget *timezone_entry;
    GtkWidget *timezone_frame;
    GtkWidget *timezone_table;
    GtkWidget *timezone_button;
    /* */
    GtkWidget *close_button;
    GtkWidget *help_button;
    GtkWidget *dialog_action_area1;
};

static void cb_dialog_response(GtkWidget * dialog, gint response_id)
{
    if(response_id == GTK_RESPONSE_HELP)
    {
        xfce_exec("xfhelp4 xfce4-user-guide.html", FALSE, FALSE, NULL);
    }
    else
    {
        is_running = FALSE;
        gtk_widget_destroy(dialog);
    }
}

void post_to_mcs(McsPlugin *mcs_plugin)
{
    mcs_manager_notify(mcs_plugin->manager, CHANNEL);
    write_options(mcs_plugin);
}

static void cb_sound_application_changed(GtkWidget *dialog, gpointer user_data)
{
    Itf *itf = (Itf *) user_data;
    McsPlugin *mcs_plugin = itf->mcs_plugin;
    
    if (sound_application)
        g_free(sound_application);
    sound_application = g_strdup(gtk_entry_get_text(
        GTK_ENTRY(itf->sound_application_entry)));
    mcs_manager_set_string(mcs_plugin->manager, "orage/SoundApplication"
        , CHANNEL, sound_application);
    post_to_mcs(mcs_plugin);
}

void static cb_archive_file_entry_changed(GtkWidget *dialog
    , gpointer user_data)
{
    Itf *itf = (Itf *) user_data;
    McsPlugin *mcs_plugin = itf->mcs_plugin;

    if (archive_path)
        g_free(archive_path);
    archive_path = g_strdup(gtk_entry_get_text(
        GTK_ENTRY(itf->archive_file_entry)));
    mcs_manager_set_string(mcs_plugin->manager, "orage/ArchiveFile"
        , CHANNEL, archive_path);
    post_to_mcs(mcs_plugin);
}

static void cb_mode_changed(GtkWidget *dialog, gpointer user_data)
{
    Itf *itf = (Itf *) user_data;
    McsPlugin *mcs_plugin = itf->mcs_plugin;

    normalmode = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(itf->normal_mode_radiobutton));

    mcs_manager_set_int(mcs_plugin->manager, "orage/NormalMode", CHANNEL, normalmode ? 1 : 0);
    post_to_mcs(mcs_plugin);
}

static void cb_taskbar_changed(GtkWidget *dialog, gpointer user_data)
{
    Itf *itf = (Itf *) user_data;
    McsPlugin *mcs_plugin = itf->mcs_plugin;

    showtaskbar = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(itf->show_taskbar_checkbutton));

    mcs_manager_set_int(mcs_plugin->manager, "orage/TaskBar", CHANNEL, showtaskbar ? 1 : 0);
    post_to_mcs(mcs_plugin);
}

static void cb_pager_changed(GtkWidget *dialog, gpointer user_data)
{
    Itf *itf = (Itf *) user_data;
    McsPlugin *mcs_plugin = itf->mcs_plugin;

    showpager = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(itf->show_pager_checkbutton));

    mcs_manager_set_int(mcs_plugin->manager, "orage/Pager", CHANNEL, showpager ? 1 : 0);
    post_to_mcs(mcs_plugin);
}

static void cb_systray_changed(GtkWidget *dialog, gpointer user_data)
{
    Itf *itf = (Itf *) user_data;
    McsPlugin *mcs_plugin = itf->mcs_plugin;

    showsystray = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(itf->show_systray_checkbutton));

    mcs_manager_set_int(mcs_plugin->manager, "orage/Systray", CHANNEL, showsystray ? 1 : 0);
    post_to_mcs(mcs_plugin);
}

static void cb_start_changed(GtkWidget *dialog, gpointer user_data)
{
    Itf *itf = (Itf *) user_data;
    McsPlugin *mcs_plugin = itf->mcs_plugin;

    showstart = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(itf->visibility_show_radiobutton));
    hidestart = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(itf->visibility_hide_radiobutton));
    ministart = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(itf->visibility_minimized_radiobutton));

    mcs_manager_set_int(mcs_plugin->manager, "orage/ShowStart", CHANNEL, showstart ? 1 : (hidestart ? 0 : 2));
    post_to_mcs(mcs_plugin);
}

static void cb_archive_file_open_button_clicked (GtkButton *button, gpointer user_data)
{
    Itf *itf = (Itf *) user_data;
    GtkWidget *file_chooser;
	XfceFileFilter *filter;
    gchar *rcfile;
    gchar *s; /* to avoid timing problems when updating entry */

    /* Create file chooser */
    file_chooser = xfce_file_chooser_new(_("Select a file..."),
                                        GTK_WINDOW (itf->orage_dialog),
                                        XFCE_FILE_CHOOSER_ACTION_SAVE,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                            NULL);
    /* Add filters */
    filter = xfce_file_filter_new();
	xfce_file_filter_set_name(filter, _("Calendar files"));
	xfce_file_filter_add_pattern(filter, "*.ics");
	xfce_file_chooser_add_filter(XFCE_FILE_CHOOSER(file_chooser), filter);
    filter = xfce_file_filter_new();
	xfce_file_filter_set_name(filter, _("All Files"));
	xfce_file_filter_add_pattern(filter, "*");
	xfce_file_chooser_add_filter(XFCE_FILE_CHOOSER(file_chooser), filter);

    rcfile = xfce_resource_save_location(XFCE_RESOURCE_CONFIG, ARCDIR, TRUE);
    xfce_file_chooser_add_shortcut_folder(XFCE_FILE_CHOOSER(file_chooser)
                , rcfile, NULL);

    /* Set the archive path */
    if (archive_path && strlen(archive_path)) {
        if (! xfce_file_chooser_set_filename(XFCE_FILE_CHOOSER(file_chooser)
            , archive_path))
            xfce_file_chooser_set_current_name(
                XFCE_FILE_CHOOSER(file_chooser), ARCFILE);
    }
    else { 
    /* this should actually never happen since we give default value in
     * creating the channel
     */
        g_warning("orage: archive file missing");
        xfce_file_chooser_set_current_folder(XFCE_FILE_CHOOSER(file_chooser)
            , rcfile);
    }
	if (gtk_dialog_run(GTK_DIALOG(file_chooser)) == GTK_RESPONSE_ACCEPT) {
		archive_path = xfce_file_chooser_get_filename(
            XFCE_FILE_CHOOSER(file_chooser));
        if (archive_path) {
            s = g_strdup(archive_path);
            gtk_entry_set_text(GTK_ENTRY(itf->archive_file_entry)
                , (const gchar*) s);
            g_free(s);
        }
    }
    gtk_widget_destroy(file_chooser);
    g_free(rcfile);
}

static void cb_sound_application_open_button_clicked (GtkButton *button
    , gpointer user_data)
{
    Itf *itf = (Itf *) user_data;
    GtkWidget *file_chooser;
    gchar *s; /* to avoid timing problems when updating entry */

    /* Create file chooser */
    file_chooser = xfce_file_chooser_new(_("Select a file..."),
                                    GTK_WINDOW (itf->orage_dialog),
                                    XFCE_FILE_CHOOSER_ACTION_OPEN,
                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                    GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                        NULL);

    /* Set sound application search path */
    if (sound_application == NULL
    ||  strlen(sound_application) == 0
    ||  sound_application[0] != '/'
    || ! xfce_file_chooser_set_filename(XFCE_FILE_CHOOSER(file_chooser)
        , sound_application))
        xfce_file_chooser_set_current_folder(XFCE_FILE_CHOOSER(file_chooser)
                    , "/usr/bin/");

	if (gtk_dialog_run(GTK_DIALOG(file_chooser)) == GTK_RESPONSE_ACCEPT) {
		sound_application = xfce_file_chooser_get_filename(
            XFCE_FILE_CHOOSER(file_chooser));
        if (sound_application) {
            s = g_strdup(sound_application);
            gtk_entry_set_text(GTK_ENTRY(itf->sound_application_entry)
                , (const gchar*) s);
            g_free(s);
        }
    }
    gtk_widget_destroy(file_chooser);
}

static void cb_timezone_button_clicked (GtkButton *button, gpointer user_data)
{
    Itf *itf = (Itf *) user_data;
    McsPlugin *mcs_plugin = itf->mcs_plugin;

#define MAX_AREA_LENGTH 20
#define MAX_BUFF_LENGTH 80

    GtkTreeStore *store;
    GtkTreeIter iter1, iter2;
    GtkWidget *tree;
    GtkCellRenderer *rend;
    GtkTreeViewColumn *col;
    GtkWidget *window;
    GtkWidget *sw;
    int j, result, latitude, longitude;
    char area_old[MAX_AREA_LENGTH], tz[MAX_BUFF_LENGTH], buf[MAX_BUFF_LENGTH]
        , *loc, *loc_eng, *loc_int;
    GtkTreeSelection *sel;
    GtkTreeModel     *model;
    GtkTreeIter       iter;
    gchar *fpath;
    FILE *fp;
                                                                             
    xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
                                                                             
    fpath = g_strconcat(DATADIR, G_DIR_SEPARATOR_S, "zoneinfo"
                        G_DIR_SEPARATOR_S, "zones.tab", NULL);
    if ((fp = fopen(fpath, "r")) == NULL) {
        g_warning("Orage: Unable to open timezones %s", fpath);
        return;
    }
 
    store = gtk_tree_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);
    strcpy(area_old, "S T a R T");
    while (fgets(buf, MAX_BUFF_LENGTH, fp) != NULL) {
        if (sscanf(buf, "%d %d %s", &latitude, &longitude, tz) != 3) {
            g_warning("Orage: Malformed timezones 1 %s (%s)", fpath, buf);
            return;
        }
        /* first area */
        if (! g_str_has_prefix(tz, area_old)) {
            for (j=0; tz[j] != '/' && j < MAX_AREA_LENGTH; j++) {
                area_old[j] = tz[j];
            }
            if (j >= MAX_AREA_LENGTH) {
                g_warning("Orage: Malformed timezones 2 %s (%s)", fpath, tz);
                return;
            }
            area_old[j] = 0;
                                                                             
            gtk_tree_store_append(store, &iter1, NULL);
            gtk_tree_store_set(store, &iter1
                    , LOCATION, _(area_old)
                    , LOCATION_ENG, area_old
                    , -1);
        }
        /* then city translated and in base form used internally */
        gtk_tree_store_append(store, &iter2, &iter1);
        gtk_tree_store_set(store, &iter2
                , LOCATION, _(tz)
                , LOCATION_ENG, tz
                , -1);
    }
    g_free(fpath);
                                                                             
    /* create view */
    tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    rend = gtk_cell_renderer_text_new();
    col  = gtk_tree_view_column_new_with_attributes(_("Location")
                , rend, "text", LOCATION, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);

    rend = gtk_cell_renderer_text_new();
    col  = gtk_tree_view_column_new_with_attributes(_("Location")
                , rend, "text", LOCATION_ENG, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
    gtk_tree_view_column_set_visible(col, FALSE);
                                                                             
    /* show it */
    window =  gtk_dialog_new_with_buttons(_("Pick local timezone")
            , GTK_WINDOW (itf->orage_dialog)
            , GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT
            , _("UTC"), 1
            , _("floating"), 2
            , GTK_STOCK_OK, GTK_RESPONSE_ACCEPT
            , NULL);
    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(sw), tree);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), sw, TRUE, TRUE, 0);
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 500);
                                                                             
    gtk_widget_show_all(window);
    do {
        result = gtk_dialog_run(GTK_DIALOG(window));
        switch (result) {
            case GTK_RESPONSE_ACCEPT:
                sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
                if (gtk_tree_selection_get_selected(sel, &model, &iter))
                    if (gtk_tree_model_iter_has_child(model, &iter))
                        result = 0;
                    else {
                        gtk_tree_model_get(model, &iter, LOCATION, &loc, -1); 
                        gtk_tree_model_get(model, &iter, LOCATION_ENG, &loc_eng, -1); 
                    }
                else {
                    loc = g_strdup(gtk_button_get_label(GTK_BUTTON(button)));
                    loc_eng = g_object_get_data(G_OBJECT(button), "LOCATION_ENG");
                }
                break;
            case 1:
                loc = g_strdup(_("UTC"));
                loc_eng = g_strdup("UTC");
                break;
            case 2:
                loc = g_strdup(_("floating"));
                loc_eng = g_strdup("floating");
                break;
            default:
                loc = g_strdup(gtk_button_get_label(GTK_BUTTON(button)));
                loc_eng = g_object_get_data(G_OBJECT(button), "LOCATION_ENG");
                break;
        }
    } while (result == 0) ;
    gtk_button_set_label(GTK_BUTTON(button), loc);

    if (loc_int = g_object_get_data(G_OBJECT(button), "LOCATION_ENG"))
                g_free(loc_int);
    loc_int = g_strdup(loc_eng);
    g_object_set_data(G_OBJECT(button), "LOCATION_ENG", loc_int);

    mcs_manager_set_string(mcs_plugin->manager, "orage/Timezone", CHANNEL, loc_eng);
    post_to_mcs(mcs_plugin);

    g_free(loc);
    g_free(loc_eng);
    gtk_widget_destroy(window);
}


static void cb_archive_threshold_combobox_changed (GtkComboBox *cb, gpointer user_data)
{

    Itf *itf = (Itf *) user_data;
    McsPlugin *mcs_plugin = itf->mcs_plugin;
    int index;

    index = gtk_combo_box_get_active(GTK_COMBO_BOX(itf->archive_threshold_combobox));
    switch (index) {
        case 0: archive_threshold = 3;
                break;
        case 1: archive_threshold = 6;
                break;
        case 2: archive_threshold = 12;
                break;
        case 3: archive_threshold = 0; /* never */
                break;
    }

    mcs_manager_set_int(mcs_plugin->manager, "orage/Lookback", CHANNEL, archive_threshold);
    post_to_mcs(mcs_plugin);
}

Itf *create_orage_dialog(McsPlugin * mcs_plugin)
{
    Itf *dialog;

    dialog = g_new(Itf, 1);

    dialog->mcs_plugin = mcs_plugin;
    dialog->mode_radiobutton_group = NULL;
    dialog->visibility_radiobutton_group = NULL;

    dialog->orage_dialog = gtk_dialog_new();
    gtk_window_set_default_size(GTK_WINDOW(dialog->orage_dialog), 300, 350);
    gtk_window_set_title(GTK_WINDOW(dialog->orage_dialog), _("Orage Preferences"));
    gtk_window_set_position(GTK_WINDOW(dialog->orage_dialog), GTK_WIN_POS_CENTER);
    gtk_window_set_modal(GTK_WINDOW(dialog->orage_dialog), FALSE);
    gtk_window_set_resizable(GTK_WINDOW(dialog->orage_dialog), FALSE);
    gtk_window_set_icon(GTK_WINDOW(dialog->orage_dialog), mcs_plugin->icon);

    gtk_dialog_set_has_separator(GTK_DIALOG(dialog->orage_dialog), FALSE);

    dialog->dialog_vbox1 = GTK_DIALOG(dialog->orage_dialog)->vbox;

    dialog->dialog_header = xfce_create_header(mcs_plugin->icon, _("Orage Preferences"));
    gtk_box_pack_start(GTK_BOX(dialog->dialog_vbox1), dialog->dialog_header, FALSE, TRUE, 0);

    dialog->notebook = gtk_notebook_new ();
    gtk_container_add (GTK_CONTAINER (dialog->dialog_vbox1), dialog->notebook);
    gtk_container_set_border_width (GTK_CONTAINER (dialog->notebook), 5);

    /* Here begins display tab */
    dialog->display_tab = xfce_framebox_new (NULL, FALSE);
    dialog->display_tab_label = gtk_label_new (_("Display"));
    gtk_notebook_append_page (GTK_NOTEBOOK (dialog->notebook)
                              , dialog->display_tab
                              , dialog->display_tab_label);

    dialog->display_vbox = gtk_vbox_new (FALSE, 0);
    xfce_framebox_add (XFCE_FRAMEBOX (dialog->display_tab)
                       , dialog->display_vbox);

    /* Display calendar borders or not */
    dialog->mode_frame = xfce_framebox_new (_("Calendar borders"), TRUE);
    gtk_box_pack_start(GTK_BOX(dialog->display_vbox), dialog->mode_frame, TRUE, TRUE, 0);
    dialog->mode_hbox = gtk_hbox_new(TRUE, 0);
    xfce_framebox_add(XFCE_FRAMEBOX(dialog->mode_frame), dialog->mode_hbox);

    dialog->normal_mode_radiobutton = gtk_radio_button_new_with_mnemonic(NULL, _("Displayed"));
    gtk_box_pack_start(GTK_BOX(dialog->mode_hbox), dialog->normal_mode_radiobutton, FALSE, FALSE, 0);
    gtk_radio_button_set_group(GTK_RADIO_BUTTON(dialog->normal_mode_radiobutton), dialog->mode_radiobutton_group);
    dialog->mode_radiobutton_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON (dialog->normal_mode_radiobutton));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->normal_mode_radiobutton), normalmode);

    dialog->compact_mode_radiobutton = gtk_radio_button_new_with_mnemonic(NULL, _("Hidden"));
    gtk_box_pack_start(GTK_BOX(dialog->mode_hbox), dialog->compact_mode_radiobutton, FALSE, FALSE, 0);
    gtk_radio_button_set_group(GTK_RADIO_BUTTON(dialog->compact_mode_radiobutton), dialog->mode_radiobutton_group);
    dialog->mode_radiobutton_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON (dialog->compact_mode_radiobutton));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->compact_mode_radiobutton), !normalmode);
    
    /* Show in... taskbar pager systray */
    dialog->show_frame = xfce_framebox_new(_("Calendar window"), TRUE);
    gtk_box_pack_start(GTK_BOX(dialog->display_vbox), dialog->show_frame, TRUE, TRUE, 5);
     
    dialog->show_vbox = gtk_vbox_new(TRUE, 0);
    xfce_framebox_add(XFCE_FRAMEBOX(dialog->show_frame), dialog->show_vbox);

    dialog->show_taskbar_checkbutton = gtk_check_button_new_with_mnemonic(_("Show in taskbar"));
    gtk_box_pack_start(GTK_BOX(dialog->show_vbox), dialog->show_taskbar_checkbutton, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->show_taskbar_checkbutton), showtaskbar);

    dialog->show_pager_checkbutton = gtk_check_button_new_with_mnemonic(_("Show in pager"));
    gtk_box_pack_start(GTK_BOX(dialog->show_vbox), dialog->show_pager_checkbutton, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (dialog->show_pager_checkbutton), showpager);

    dialog->show_systray_checkbutton = gtk_check_button_new_with_mnemonic(_("Show in systray"));
    gtk_box_pack_start(GTK_BOX(dialog->show_vbox), dialog->show_systray_checkbutton, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->show_systray_checkbutton), showsystray);

    /* */
    dialog->visibility_frame = xfce_framebox_new(_("Calendar start"), TRUE);
    gtk_box_pack_start(GTK_BOX(dialog->display_vbox), dialog->visibility_frame, TRUE, TRUE, 5);
    dialog->visibility_hbox = gtk_hbox_new(TRUE, 0);
    xfce_framebox_add(XFCE_FRAMEBOX(dialog->visibility_frame), dialog->visibility_hbox);

    dialog->visibility_show_radiobutton = gtk_radio_button_new_with_mnemonic(NULL, _("Show"));
    gtk_box_pack_start(GTK_BOX(dialog->visibility_hbox), dialog->visibility_show_radiobutton, FALSE, FALSE, 0);
    gtk_radio_button_set_group(GTK_RADIO_BUTTON(dialog->visibility_show_radiobutton), dialog->visibility_radiobutton_group);
    dialog->visibility_radiobutton_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(dialog->visibility_show_radiobutton));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->visibility_show_radiobutton), showstart);

    dialog->visibility_hide_radiobutton = gtk_radio_button_new_with_mnemonic(NULL, _("Hide"));
    gtk_box_pack_start(GTK_BOX(dialog->visibility_hbox), dialog->visibility_hide_radiobutton, FALSE, FALSE, 0);
    gtk_radio_button_set_group(GTK_RADIO_BUTTON(dialog->visibility_hide_radiobutton), dialog->visibility_radiobutton_group);
    dialog->visibility_radiobutton_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(dialog->visibility_hide_radiobutton));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->visibility_hide_radiobutton), hidestart);

    dialog->visibility_minimized_radiobutton = gtk_radio_button_new_with_mnemonic(NULL, _("Minimized"));
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

    dialog->archive_vbox = gtk_vbox_new (FALSE, 0);
    xfce_framebox_add (XFCE_FRAMEBOX (dialog->archives_tab)
                       , dialog->archive_vbox);
    /* Archive file and periodicity */
    dialog->archive_file_frame = xfce_framebox_new (_("Archive file"), TRUE);
    gtk_box_pack_start (GTK_BOX (dialog->archive_vbox), dialog->archive_file_frame, TRUE, TRUE, 5);

    dialog->archive_file_table = gtk_table_new (1, 2, FALSE);
    gtk_container_set_border_width (GTK_CONTAINER (dialog->archive_file_table), 10);
    gtk_table_set_row_spacings (GTK_TABLE (dialog->archive_file_table), 6);
    gtk_table_set_col_spacings (GTK_TABLE (dialog->archive_file_table), 6);
    xfce_framebox_add (XFCE_FRAMEBOX (dialog->archive_file_frame)
                       , dialog->archive_file_table);

    dialog->archive_file_entry = gtk_entry_new ();
    gtk_table_attach (GTK_TABLE (dialog->archive_file_table), dialog->archive_file_entry, 0, 1, 0, 1,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);
    gtk_entry_set_text(GTK_ENTRY(dialog->archive_file_entry), (const gchar *) archive_path);

    dialog->archive_file_open_button = gtk_button_new_from_stock("gtk-open");
    gtk_table_attach (GTK_TABLE (dialog->archive_file_table), dialog->archive_file_open_button, 1, 2, 0, 1,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);

    dialog->archive_threshold_frame = 
        xfce_framebox_new(_("Archive threshold"), TRUE);
    gtk_box_pack_start(GTK_BOX (dialog->archive_vbox),
        dialog->archive_threshold_frame, TRUE, TRUE, 5);

    dialog->archive_threshold_table = gtk_table_new (1, 1, FALSE);
    gtk_container_set_border_width(
        GTK_CONTAINER(dialog->archive_threshold_table), 10);
    gtk_table_set_row_spacings(GTK_TABLE(dialog->archive_threshold_table), 6);
    gtk_table_set_col_spacings(GTK_TABLE(dialog->archive_threshold_table), 6);
    xfce_framebox_add (XFCE_FRAMEBOX (dialog->archive_threshold_frame)
                       , dialog->archive_threshold_table);

    dialog->archive_threshold_combobox = gtk_combo_box_new_text ();
    gtk_combo_box_append_text(
        GTK_COMBO_BOX(dialog->archive_threshold_combobox), _("3 months"));
    gtk_combo_box_append_text(
        GTK_COMBO_BOX(dialog->archive_threshold_combobox), _("6 months"));
    gtk_combo_box_append_text(
        GTK_COMBO_BOX(dialog->archive_threshold_combobox), _("1 year"));
    gtk_combo_box_append_text(
        GTK_COMBO_BOX(dialog->archive_threshold_combobox), _("never"));
    switch (archive_threshold) {
            case 3: 
                gtk_combo_box_set_active(
                    GTK_COMBO_BOX(dialog->archive_threshold_combobox), 0);
                break;
            case 6: 
                gtk_combo_box_set_active(
                    GTK_COMBO_BOX(dialog->archive_threshold_combobox), 1);
                break;
            case 12:
                gtk_combo_box_set_active(
                    GTK_COMBO_BOX(dialog->archive_threshold_combobox), 2);
                break;
            default:
                gtk_combo_box_set_active(
                    GTK_COMBO_BOX(dialog->archive_threshold_combobox), 3);
    }

    gtk_table_attach (GTK_TABLE (dialog->archive_threshold_table)
        , dialog->archive_threshold_combobox, 0, 1, 0, 1,
                        (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);

    /* Here begins the sound tab */
    dialog->sound_tab = xfce_framebox_new (NULL, FALSE);
    dialog->sound_tab_label = gtk_label_new (_("Sound"));
    gtk_notebook_append_page (GTK_NOTEBOOK (dialog->notebook)
                              , dialog->sound_tab
                              , dialog->sound_tab_label);

    /* Choose a sound application for reminders */
    dialog->sound_vbox = gtk_vbox_new (FALSE, 0);
    xfce_framebox_add (XFCE_FRAMEBOX (dialog->sound_tab)
                       , dialog->sound_vbox);

    dialog->sound_application_frame = xfce_framebox_new(_("Application"), TRUE);
    gtk_box_pack_start(GTK_BOX(dialog->sound_vbox), dialog->sound_application_frame, TRUE, TRUE, 5);

    dialog->sound_application_table = gtk_table_new(1, 2, FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(dialog->sound_application_table), 10);
    gtk_table_set_row_spacings(GTK_TABLE(dialog->sound_application_table), 6);
    gtk_table_set_col_spacings(GTK_TABLE(dialog->sound_application_table), 6);
    xfce_framebox_add(XFCE_FRAMEBOX(dialog->sound_application_frame)
                       , dialog->sound_application_table);

    dialog->sound_application_entry = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(dialog->sound_application_table)
        , dialog->sound_application_entry, 0, 1, 0, 1,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);
    gtk_entry_set_text(GTK_ENTRY(dialog->sound_application_entry), (const gchar *)sound_application);
    dialog->sound_application_open_button = gtk_button_new_from_stock("gtk-open");
    gtk_table_attach (GTK_TABLE (dialog->sound_application_table), dialog->sound_application_open_button, 1, 2, 0, 1,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);

    /* Here begins the timezone tab */
    dialog->timezone_tab = xfce_framebox_new (NULL, FALSE);
    dialog->timezone_tab_label = gtk_label_new (_("Timezone"));
    gtk_notebook_append_page (GTK_NOTEBOOK (dialog->notebook)
                              , dialog->timezone_tab
                              , dialog->timezone_tab_label);

    /* Choose a timezone to be used in appointments */
    dialog->timezone_vbox = gtk_vbox_new (FALSE, 0);
    xfce_framebox_add (XFCE_FRAMEBOX (dialog->timezone_tab)
                       , dialog->timezone_vbox);

    dialog->timezone_frame = xfce_framebox_new (_("Timezone"), TRUE);
    gtk_box_pack_start (GTK_BOX (dialog->timezone_vbox), dialog->timezone_frame, TRUE, TRUE, 5);

    dialog->timezone_table = gtk_table_new (1, 1, FALSE);
    gtk_container_set_border_width (GTK_CONTAINER (dialog->timezone_table), 10);
    gtk_table_set_row_spacings (GTK_TABLE (dialog->timezone_table), 6);
    gtk_table_set_col_spacings (GTK_TABLE (dialog->timezone_table), 6);
    xfce_framebox_add (XFCE_FRAMEBOX (dialog->timezone_frame)
                       , dialog->timezone_table);
    dialog->timezone_button = gtk_button_new();
    if (local_timezone) {
        gtk_button_set_label(GTK_BUTTON(dialog->timezone_button), _(local_timezone));
        g_object_set_data(G_OBJECT(dialog->timezone_button), "LOCATION_ENG", local_timezone);
    }
    else {
        gtk_button_set_label(GTK_BUTTON(dialog->timezone_button), _("floating"));
        g_object_set_data(G_OBJECT(dialog->timezone_button), "LOCATION_ENG", "floating");
    }

    gtk_table_attach (GTK_TABLE (dialog->timezone_table), dialog->timezone_button, 0, 1, 0, 1,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);

    /* the rest */
    dialog->help_button = gtk_button_new_from_stock ("gtk-help");
    gtk_dialog_add_action_widget (GTK_DIALOG (dialog->orage_dialog), dialog->help_button, GTK_RESPONSE_HELP);
    dialog->close_button = gtk_button_new_from_stock ("gtk-close");
    gtk_dialog_add_action_widget (GTK_DIALOG (dialog->orage_dialog), dialog->close_button, GTK_RESPONSE_CLOSE);
    GTK_WIDGET_SET_FLAGS (dialog->close_button, GTK_CAN_DEFAULT);

    return dialog;
}

static void setup_dialog (Itf *dialog)
{
    g_signal_connect (G_OBJECT (dialog->orage_dialog), "response", G_CALLBACK (cb_dialog_response), dialog->mcs_plugin);

    g_signal_connect (G_OBJECT (dialog->normal_mode_radiobutton), "toggled", G_CALLBACK (cb_mode_changed), dialog);
    g_signal_connect (G_OBJECT (dialog->show_taskbar_checkbutton), "toggled", G_CALLBACK (cb_taskbar_changed), dialog);
    g_signal_connect (G_OBJECT (dialog->show_pager_checkbutton), "toggled", G_CALLBACK (cb_pager_changed), dialog);
    g_signal_connect (G_OBJECT (dialog->show_systray_checkbutton), "toggled", G_CALLBACK (cb_systray_changed), dialog);
    g_signal_connect (G_OBJECT (dialog->visibility_show_radiobutton), "toggled", G_CALLBACK (cb_start_changed), dialog);
    g_signal_connect (G_OBJECT (dialog->visibility_minimized_radiobutton), "toggled", G_CALLBACK (cb_start_changed), dialog);
    g_signal_connect (G_OBJECT (dialog->sound_application_entry), "changed", G_CALLBACK (cb_sound_application_changed), dialog);
    g_signal_connect (G_OBJECT (dialog->archive_file_open_button), "clicked", G_CALLBACK (cb_archive_file_open_button_clicked), dialog);
    g_signal_connect (G_OBJECT (dialog->archive_file_entry), "changed", G_CALLBACK (cb_archive_file_entry_changed), dialog);
    g_signal_connect (G_OBJECT (dialog->archive_threshold_combobox), "changed", G_CALLBACK (cb_archive_threshold_combobox_changed), dialog);
    g_signal_connect (G_OBJECT (dialog->sound_application_open_button), "clicked", G_CALLBACK (cb_sound_application_open_button_clicked), dialog);
    g_signal_connect (G_OBJECT (dialog->timezone_button), "clicked", G_CALLBACK (cb_timezone_button_clicked), dialog);

    xfce_gtk_window_center_on_monitor_with_pointer (GTK_WINDOW (dialog->orage_dialog));
    gdk_x11_window_set_user_time(GTK_WIDGET (dialog->orage_dialog)->window, 
            gdk_x11_get_server_time (GTK_WIDGET (dialog->orage_dialog)->window));
    gtk_widget_show_all (dialog->orage_dialog);
}

McsPluginInitResult mcs_plugin_init(McsPlugin * mcs_plugin)
{
    /* This is required for UTF-8 at least - Please don't remove it */
    xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    create_channel(mcs_plugin);
    mcs_plugin->plugin_name = g_strdup(PLUGIN_NAME);
    mcs_plugin->caption = g_strdup(_("Orage"));
    mcs_plugin->run_dialog = run_dialog;
    mcs_plugin->icon = xfce_themed_icon_load("xfcalendar", 48);
    mcs_manager_notify(mcs_plugin->manager, CHANNEL);

    return (MCS_PLUGIN_INIT_OK);
}

static void create_channel(McsPlugin * mcs_plugin)
{
    McsSetting *setting;
    gchar *rcfile;
    
    rcfile = xfce_resource_lookup(XFCE_RESOURCE_CONFIG, 
                                   RCDIR G_DIR_SEPARATOR_S RCFILE);

    if (!rcfile)
        rcfile = xfce_get_userfile(OLDRCDIR, RCFILE, NULL);

    if (g_file_test(rcfile, G_FILE_TEST_EXISTS))
        mcs_manager_add_channel_from_file(mcs_plugin->manager, CHANNEL, rcfile);
    else
        mcs_manager_add_channel(mcs_plugin->manager, CHANNEL);
    
    g_free(rcfile);

    setting = mcs_manager_setting_lookup(mcs_plugin->manager, "orage/NormalMode", CHANNEL);
    if (setting) {
        normalmode = setting->data.v_int ? TRUE: FALSE;
    }
    else {
        normalmode = TRUE;
        mcs_manager_set_int(mcs_plugin->manager, "orage/NormalMode", CHANNEL, normalmode ? 1 : 0);
    }
    
    setting = mcs_manager_setting_lookup(mcs_plugin->manager, "orage/TaskBar", CHANNEL);
    if (setting) {
      showtaskbar = setting->data.v_int ? TRUE: FALSE;
    }
    else {
      showtaskbar = TRUE;
      mcs_manager_set_int(mcs_plugin->manager, "orage/TaskBar", CHANNEL, showtaskbar ? 1 : 0);
    }

    setting = mcs_manager_setting_lookup(mcs_plugin->manager, "orage/Pager", CHANNEL);
    if (setting) {
      showpager = setting->data.v_int ? TRUE: FALSE;
    }
    else {
      showpager = TRUE;
      mcs_manager_set_int(mcs_plugin->manager, "orage/Pager", CHANNEL, showpager ? 1 : 0);
    }

    setting = mcs_manager_setting_lookup(mcs_plugin->manager, "orage/Systray", CHANNEL);
    if (setting) {
      showsystray = setting->data.v_int ? TRUE: FALSE;
    }
    else {
      showsystray = TRUE;
      mcs_manager_set_int(mcs_plugin->manager, "orage/Systray", CHANNEL, showsystray ? 1 : 0);
    }

    setting = mcs_manager_setting_lookup(mcs_plugin->manager, "orage/ShowStart", CHANNEL);
    if (setting) {
        showstart = FALSE;
        hidestart = FALSE;
        ministart = FALSE;
        switch (setting->data.v_int) {
            case 0: hidestart = TRUE; break;
            case 1: showstart = TRUE; break;
            case 2: ministart = TRUE; break;
            default: showstart = TRUE;
        }
    }
    else {
        showstart = TRUE;
        hidestart = FALSE;
        ministart = FALSE;
        mcs_manager_set_int(mcs_plugin->manager, "orage/ShowStart", CHANNEL, 1);
    }

    setting = mcs_manager_setting_lookup (mcs_plugin->manager, "orage/ArchiveFile", CHANNEL);
    if (setting) {
        if (archive_path)
            g_free(archive_path);
        archive_path = g_strdup(setting->data.v_string);
    }
    else {
        if (archive_path)
            g_free(archive_path);
        archive_path = xfce_resource_save_location(XFCE_RESOURCE_CONFIG
            , ARCDIR ARCFILE, TRUE);
        mcs_manager_set_string(mcs_plugin->manager, "orage/ArchiveFile"
            , CHANNEL, archive_path);
    }

    setting = mcs_manager_setting_lookup (mcs_plugin->manager
        , "orage/Lookback", CHANNEL);
    if (setting) {
        archive_threshold = setting->data.v_int;
    }
    else {
        archive_threshold = 0; /* never */
        mcs_manager_set_int(mcs_plugin->manager, "orage/Lookback"
            , CHANNEL, archive_threshold);
    }

    setting = mcs_manager_setting_lookup(mcs_plugin->manager, "orage/SoundApplication", CHANNEL);
    if(setting) {
        if (sound_application)
            g_free(sound_application);
        sound_application = g_strdup(setting->data.v_string);
    }
    else {
        if (sound_application)
            g_free(sound_application);
        sound_application = g_strdup("play");
        mcs_manager_set_string(mcs_plugin->manager, "orage/SoundApplication", CHANNEL, sound_application);
    }

    setting = mcs_manager_setting_lookup(mcs_plugin->manager, "orage/Timezone", CHANNEL);
    if(setting)
    {
        if (local_timezone)
            g_free(local_timezone);
        local_timezone = g_strdup(setting->data.v_string);
    }
    else 
    {
        if (local_timezone)
            g_free(local_timezone);
        local_timezone=g_strdup("floating");
        mcs_manager_set_string(mcs_plugin->manager, "orage/Timezone", CHANNEL, local_timezone);
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
    static Itf *dialog = NULL;

    xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    if (is_running)
    {
        if((dialog) && (dialog->orage_dialog))
        {
            gtk_window_present(GTK_WINDOW(dialog->orage_dialog));
        }
        return;
    }

    is_running = TRUE;
    dialog = create_orage_dialog(mcs_plugin);
    setup_dialog(dialog);
}

/* macro defined in manager-plugin.h */
MCS_PLUGIN_CHECK_INIT
