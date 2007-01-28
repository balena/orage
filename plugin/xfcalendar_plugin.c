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
    g_spawn_command_line_async("orage -p", NULL);
}

McsPluginInitResult mcs_plugin_init(McsPlugin *mcs_plugin)
{
    /* This is required for UTF-8 at least - Please don't remove it */
    xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    mcs_plugin->plugin_name = g_strdup(PLUGIN_NAME);
    /* the button label in the xfce-mcs-manager dialog */
    mcs_plugin->caption = g_strdup(_("Orage - Calendar"));
    mcs_plugin->run_dialog = run_dialog;
    mcs_plugin->icon = xfce_themed_icon_load("xfcalendar", 48);

    return(MCS_PLUGIN_INIT_OK);
}
