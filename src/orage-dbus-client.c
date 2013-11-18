/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2006-2011 Juha Kautto  (juha at xfce.org)
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

#include <glib-object.h>

#include <dbus/dbus-glib-lowlevel.h>

#include "orage-dbus.h"
/*
#include "orage-dbus-object.h"
#include "orage-dbus-service.h"
*/


/* ********************************
 *      CLIENT side call 
 * ********************************
 * */
gboolean orage_dbus_import_file(gchar *file_name)
{
    DBusGConnection *connection;
    GError *error = NULL;
    DBusGProxy *proxy;

    g_type_init();
    connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
    if (connection == NULL) {
          /* notify the user that D-BUS service won't be available */
        g_warning("Failed to connect to the D-BUS session bus: %s"
                , error->message);
        return(FALSE);
    }

/* Create a proxy object for the "bus driver" (name "org.freedesktop.DBus") */
    proxy = dbus_g_proxy_new_for_name(connection
            , "org.xfce.calendar", "/org/xfce/calendar", "org.xfce.calendar");

    /*
    if (orage_dbus_service_load_file(proxy, file_name,  &error)) {
    */
    if (dbus_g_proxy_call(proxy, "LoadFile", &error
                , G_TYPE_STRING, file_name
                , G_TYPE_INVALID, G_TYPE_INVALID)) {
        return(TRUE);
    }
    else {
        return(FALSE);
    };
}

gboolean orage_dbus_export_file(gchar *file_name, gint type, gchar *uids)
{
    DBusGConnection *connection;
    GError *error = NULL;
    DBusGProxy *proxy;

    g_type_init();
    connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
    if (connection == NULL) {
        g_warning("Failed to connect to the D-BUS session bus: %s"
                , error->message);
        return(FALSE);
    }

    proxy = dbus_g_proxy_new_for_name(connection
            , "org.xfce.calendar", "/org/xfce/calendar", "org.xfce.calendar");
    if (dbus_g_proxy_call(proxy, "ExportFile", &error
                , G_TYPE_STRING, file_name
                , G_TYPE_INT, type
                , G_TYPE_STRING, uids
                , G_TYPE_INVALID, G_TYPE_INVALID)) {
        return(TRUE);
    }
    else {
        return(FALSE);
    };
}

gboolean orage_dbus_foreign_add(gchar *file_name, gboolean read_only
        , gchar *name)
{
    DBusGConnection *connection;
    GError *error = NULL;
    DBusGProxy *proxy;

    g_type_init();
    connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
    if (connection == NULL) {
        g_warning("Failed to connect to the D-BUS session bus: %s"
                , error->message);
        return(FALSE);
    }

    proxy = dbus_g_proxy_new_for_name(connection
            , "org.xfce.calendar", "/org/xfce/calendar", "org.xfce.calendar");
    if (dbus_g_proxy_call(proxy, "AddForeign", &error
                , G_TYPE_STRING, file_name
                , G_TYPE_BOOLEAN, read_only
                , G_TYPE_STRING, name
                , G_TYPE_INVALID, G_TYPE_INVALID)) {
        return(TRUE);
    }
    else {
        return(FALSE);
    };
}

gboolean orage_dbus_foreign_remove(gchar *file_name)
{
    DBusGConnection *connection;
    GError *error = NULL;
    DBusGProxy *proxy;

    g_type_init();
    connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
    if (connection == NULL) {
        g_warning("Failed to connect to the D-BUS session bus: %s"
                , error->message);
        return(FALSE);
    }

    proxy = dbus_g_proxy_new_for_name(connection
            , "org.xfce.calendar", "/org/xfce/calendar", "org.xfce.calendar");
    if (dbus_g_proxy_call(proxy, "RemoveForeign", &error
                , G_TYPE_STRING, file_name
                , G_TYPE_INVALID, G_TYPE_INVALID)) {
        return(TRUE);
    }
    else {
        return(FALSE);
    };
}
