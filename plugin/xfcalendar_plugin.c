/* xfcalendar_plugin.c
 *
 *  Copyright        (c) 2006      Juha Kautto <juha at xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; You may only use version 2 of the License,
 *  you have no option to use any other version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>

#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4mcs/mcs-common.h>
#include <libxfce4mcs/mcs-manager.h>
#include <xfce-mcs-manager/manager-plugin.h>

#define PLUGIN_NAME "orage"


static void run_dialog(McsPlugin *mcs_plugin)
{
    GtkWidget *message;
    GError    *error = NULL;

    if (!g_spawn_command_line_async(BINDIR G_DIR_SEPARATOR_S "orage -p"
            , &error)) {
        message = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_ERROR
                , GTK_BUTTONS_OK, "%s.", _("Failed to launch 'orage -p'"));
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG (message)
                , "%s.", error->message);
        gtk_dialog_run(GTK_DIALOG(message));
        gtk_widget_destroy(message);
        g_error_free(error);
    }
}

McsPluginInitResult mcs_plugin_init(McsPlugin *mcs_plugin)
{
    GtkIconTheme *icon_theme;

    /* This is required for UTF-8 at least - Please don't remove it */
    xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    mcs_plugin->plugin_name = g_strdup(PLUGIN_NAME);
    /* the button label in the xfce-mcs-manager dialog */
    mcs_plugin->caption = g_strdup(_("Orage - Calendar"));
    mcs_plugin->run_dialog = run_dialog;

    /* search the icon */
    /* mcs_plugin->icon = xfce_themed_icon_load("xfcalendar", 48); */
    icon_theme = gtk_icon_theme_get_default();
    mcs_plugin->icon = gtk_icon_theme_load_icon(icon_theme, "xfcalendar"
            , 48, 0, NULL);
    /* if that didn't work, we know where we installed the icon, 
     * so load it directly */
    if (mcs_plugin->icon == NULL)
        mcs_plugin->icon = gdk_pixbuf_new_from_file(DATADIR 
                "/icons/hicolor/48x48/apps/xfcalendar.png", NULL);

    /* attach icon name to the plugin icon (if any) */
    if (mcs_plugin->icon != NULL)
        g_object_set_data_full(G_OBJECT(mcs_plugin->icon)
                , "mcs-plugin-icon-name", g_strdup("xfcalendar"), g_free);


    return(MCS_PLUGIN_INIT_OK);
}
