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

        xfccalendar mcs plugin   - (c) 2003 Mickael Graf   

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
#include <libxfce4util/i18n.h>
#include <libxfce4util/util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <xfce-mcs-manager/manager-plugin.h>
#include "calendar-icon.h"

#define BORDER 5

#define RCDIR    "xfcalendar"
#define CHANNEL  "xfcalendar"
#define RCFILE   "xfcalendar.xml"
#define PLUGIN_NAME "xfcalendar"

#define SUNDAY TRUE
#define MONDAY FALSE

#define DEFAULT_ICON_SIZE 48

static void create_channel(McsPlugin * mcs_plugin);
static gboolean write_options(McsPlugin * mcs_plugin);
static void run_dialog(McsPlugin * mcs_plugin);

static gboolean is_running = FALSE;
static gboolean startday = SUNDAY;
static gboolean showtaskbar = TRUE;
static gboolean showpager = TRUE;

typedef struct _Itf Itf;
struct _Itf
{
  McsPlugin *mcs_plugin;

  GSList *startday_radiobutton_group;

  GtkWidget *xfcalendar_dialog;
  GtkWidget *start_monday_radiobutton;
  GtkWidget *start_sunday_radiobutton;
  GtkWidget *show_taskbar_checkbutton;
  GtkWidget *show_pager_checkbutton;
  GtkWidget *vbox1;
  GtkWidget *dialog_header;
  GtkWidget *dialog_vbox1;
  GtkWidget *frame1;
  GtkWidget *frame2;
  GtkWidget *hbox1;
  GtkWidget *hbox2;
  GtkWidget *hbox3;
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

static void cb_startday_changed(GtkWidget * dialog, gpointer user_data)
{
    Itf *itf = (Itf *) user_data;
    McsPlugin *mcs_plugin = itf->mcs_plugin;

    startday = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(itf->start_sunday_radiobutton));

    mcs_manager_set_int(mcs_plugin->manager, "XFCalendar/StartDay", CHANNEL, startday ? 1 : 0);
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

Itf *create_xfcalendar_dialog(McsPlugin * mcs_plugin)
{
    Itf *dialog;
    GdkPixbuf *icon;

    dialog = g_new(Itf, 1);

    dialog->mcs_plugin = mcs_plugin;
    dialog->startday_radiobutton_group = NULL;

    icon = inline_icon_at_size(calendar_icon_data, 32, 32);
    dialog->xfcalendar_dialog = gtk_dialog_new();
    gtk_widget_set_size_request (dialog->xfcalendar_dialog, 300, 200);
    gtk_window_set_title (GTK_WINDOW (dialog->xfcalendar_dialog), _("XFCalendar"));
    gtk_window_set_position (GTK_WINDOW (dialog->xfcalendar_dialog), GTK_WIN_POS_CENTER);
    gtk_window_set_modal (GTK_WINDOW (dialog->xfcalendar_dialog), FALSE);
    gtk_window_set_resizable (GTK_WINDOW (dialog->xfcalendar_dialog), TRUE);
    gtk_window_set_icon(GTK_WINDOW(dialog->xfcalendar_dialog), icon);

    gtk_dialog_set_has_separator (GTK_DIALOG (dialog->xfcalendar_dialog), FALSE);

    dialog->dialog_vbox1 = GTK_DIALOG (dialog->xfcalendar_dialog)->vbox;
    gtk_widget_show (dialog->dialog_vbox1);

    dialog->dialog_header = create_header(icon, _("XFCalendar"));
    gtk_widget_show(dialog->dialog_header);
    g_object_unref(icon);
    gtk_box_pack_start(GTK_BOX(dialog->dialog_vbox1), dialog->dialog_header, FALSE, TRUE, 0);

    dialog->hbox1 = gtk_hbox_new (TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (dialog->hbox1), BORDER + 1);
    gtk_widget_show (dialog->hbox1);
    gtk_box_pack_start (GTK_BOX (dialog->dialog_vbox1), dialog->hbox1, TRUE, TRUE, 0);
    
    dialog->vbox1 = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (dialog->vbox1), BORDER);
    gtk_widget_show (dialog->vbox1);
    gtk_box_pack_start (GTK_BOX (dialog->hbox1), dialog->vbox1, TRUE, TRUE, 0);

    /* */    
    dialog->frame1 = xfce_framebox_new (_("Week starts with..."), TRUE);
    gtk_widget_show (dialog->frame1);
    gtk_box_pack_start (GTK_BOX (dialog->vbox1), dialog->frame1, TRUE, TRUE, 0);
    
    dialog->hbox2 = gtk_hbox_new (TRUE, 0);
    gtk_widget_show (dialog->hbox2);
    xfce_framebox_add (XFCE_FRAMEBOX (dialog->frame1), dialog->hbox2);
    
    dialog->start_sunday_radiobutton = gtk_radio_button_new_with_mnemonic (NULL, _("Sunday"));
    gtk_widget_show (dialog->start_sunday_radiobutton);
    gtk_box_pack_start (GTK_BOX (dialog->hbox2), dialog->start_sunday_radiobutton, FALSE, FALSE, 0);
    gtk_radio_button_set_group (GTK_RADIO_BUTTON (dialog->start_sunday_radiobutton), dialog->startday_radiobutton_group);
    dialog->startday_radiobutton_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (dialog->start_sunday_radiobutton));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->start_sunday_radiobutton), startday);

    dialog->start_monday_radiobutton = gtk_radio_button_new_with_mnemonic (NULL, _("Monday"));
    gtk_widget_show (dialog->start_monday_radiobutton);
    gtk_box_pack_start (GTK_BOX (dialog->hbox2), dialog->start_monday_radiobutton, FALSE, FALSE, 0);
    gtk_radio_button_set_group (GTK_RADIO_BUTTON (dialog->start_monday_radiobutton), dialog->startday_radiobutton_group);
    dialog->startday_radiobutton_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (dialog->start_monday_radiobutton));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->start_monday_radiobutton), !startday);

    /* */    
    dialog->frame2 = xfce_framebox_new (_("Show in..."), TRUE);
    gtk_widget_show (dialog->frame2);
    gtk_box_pack_start (GTK_BOX (dialog->vbox1), dialog->frame2, TRUE, TRUE, 0);
    
    dialog->hbox3 = gtk_hbox_new (TRUE, 0);
    gtk_widget_show (dialog->hbox3);
    xfce_framebox_add (XFCE_FRAMEBOX (dialog->frame2), dialog->hbox3);

    dialog->show_taskbar_checkbutton = gtk_check_button_new_with_mnemonic (_("Taskbar"));
    gtk_widget_show (dialog->show_taskbar_checkbutton);
    gtk_box_pack_start (GTK_BOX (dialog->hbox3), dialog->show_taskbar_checkbutton, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->show_taskbar_checkbutton), showtaskbar);

    dialog->show_pager_checkbutton = gtk_check_button_new_with_mnemonic (_("Pager"));
    gtk_widget_show (dialog->show_pager_checkbutton);
    gtk_box_pack_start (GTK_BOX (dialog->hbox3), dialog->show_pager_checkbutton, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->show_pager_checkbutton), showpager);

    /* */
    dialog->closebutton = gtk_button_new_from_stock ("gtk-close");
    gtk_widget_show (dialog->closebutton);
    gtk_dialog_add_action_widget (GTK_DIALOG (dialog->xfcalendar_dialog), dialog->closebutton, GTK_RESPONSE_CLOSE);
    GTK_WIDGET_SET_FLAGS (dialog->closebutton, GTK_CAN_DEFAULT);

    return dialog;
}

static void setup_dialog(Itf * itf)
{
  g_signal_connect(G_OBJECT(itf->xfcalendar_dialog), "response", G_CALLBACK(cb_dialog_response), itf->mcs_plugin);

  g_signal_connect(G_OBJECT(itf->start_sunday_radiobutton), "toggled", G_CALLBACK(cb_startday_changed), itf);
  g_signal_connect(G_OBJECT(itf->show_taskbar_checkbutton), "toggled", G_CALLBACK(cb_taskbar_changed), itf);
  g_signal_connect(G_OBJECT(itf->show_pager_checkbutton), "toggled", G_CALLBACK(cb_pager_changed), itf);

  gtk_window_set_position (GTK_WINDOW (itf->xfcalendar_dialog), GTK_WIN_POS_CENTER);
  gtk_widget_show(itf->xfcalendar_dialog);
}

McsPluginInitResult mcs_plugin_init(McsPlugin * mcs_plugin)
{
#if 0
#ifdef ENABLE_NLS
    /* This is required for UTF-8 at least - Please don't remove it */
    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif
    textdomain (GETTEXT_PACKAGE);
#endif
#else
    /* This is required for UTF-8 at least - Please don't remove it */
    xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
#endif

    create_channel(mcs_plugin);
    mcs_plugin->plugin_name = g_strdup(PLUGIN_NAME);
    mcs_plugin->caption = g_strdup(_("XFCalendar"));
    mcs_plugin->run_dialog = run_dialog;
    mcs_plugin->icon = inline_icon_at_size(calendar_icon_data, DEFAULT_ICON_SIZE, DEFAULT_ICON_SIZE);
    mcs_manager_notify(mcs_plugin->manager, CHANNEL);

    return (MCS_PLUGIN_INIT_OK);
}

static void create_channel(McsPlugin * mcs_plugin)
{
    McsSetting *setting;

    gchar *rcfile;
    
    rcfile = xfce_get_userfile(RCDIR, RCFILE, NULL);
    mcs_manager_add_channel_from_file(mcs_plugin->manager, CHANNEL, rcfile);
    g_free(rcfile);

    setting = mcs_manager_setting_lookup(mcs_plugin->manager, "XFCalendar/StartDay", CHANNEL);
    if(setting)
    {
        startday = setting->data.v_int ? SUNDAY: MONDAY;
    }
    else
    {
        startday = SUNDAY;
        mcs_manager_set_int(mcs_plugin->manager, "XFCalendar/StartDay", CHANNEL, startday ? 0 : 1);
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
}

static gboolean write_options(McsPlugin * mcs_plugin)
{
    gchar *rcfile;
    gboolean result;

    rcfile = xfce_get_userfile(RCDIR, RCFILE, NULL);
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

    dialog = create_xfcalendar_dialog(mcs_plugin);
    setup_dialog(dialog);
}

/* macro defined in manager-plugin.h */
MCS_PLUGIN_CHECK_INIT
