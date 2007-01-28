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

#include <glib-object.h>

#include <dbus/dbus-glib-lowlevel.h>

#include "orage-dbus-object.h"
#include "orage-dbus-service.h"

/* defined in interface.c */
gboolean orage_import_file(gchar *entry_filename);

struct _OrageDBusClass
{
    GObjectClass parent;
};

struct _OrageDBus
{
    GObject parent;
    DBusGConnection *connection;
};

G_DEFINE_TYPE(OrageDBus, orage_dbus, G_TYPE_OBJECT)

OrageDBus *orage_dbus;

static void orage_dbus_class_init(OrageDBusClass *orage_class)
{
    g_type_init();
    dbus_g_object_type_install_info(G_TYPE_FROM_CLASS(orage_class)
            , &dbus_glib_orage_object_info);
}

static void orage_dbus_init(OrageDBus *orage_dbus)
{
    GError *error = NULL;

  /* try to connect to the session bus */
    orage_dbus->connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
    if (orage_dbus->connection == NULL) {
      /* notify the user that D-BUS service won't be available */
        g_message("Orage: Failed to connect to the D-BUS session bus: %s"
                , error->message);
        g_error_free(error);
    }
    else {
      /* register the /org/xfce/calendar object for orage */
        dbus_g_connection_register_g_object(orage_dbus->connection
              , "/org/xfce/calendar", G_OBJECT(orage_dbus));

      /* request the org.xfce.calendar name for orage */
        dbus_bus_request_name(dbus_g_connection_get_connection(
                  orage_dbus->connection), "org.xfce.calendar"
              , DBUS_NAME_FLAG_REPLACE_EXISTING, NULL);

      /* request the org.xfce.orage name for orage */
        dbus_bus_request_name(dbus_g_connection_get_connection(
                  orage_dbus->connection), "org.xfce.orage"
              , DBUS_NAME_FLAG_REPLACE_EXISTING, NULL);
    }
}

gboolean orage_dbus_service_load_file(DBusGProxy *proxy, const char * IN_str
        , GError **error)
{
    if (orage_import_file((char *)IN_str)) {
        g_message("Orage **: DBUS File added %s", IN_str);
        return(TRUE);
    }
    else {
        g_warning("DBUS File add failed %s", IN_str);
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_INVAL
                , "Invalid ical file \"%s\"", IN_str);

        return(FALSE);
    }
}

void orage_dbus_start()
{
    orage_dbus = g_object_new(ORAGE_DBUS_TYPE, NULL);
}
