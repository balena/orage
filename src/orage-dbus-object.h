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

#ifndef __ORAGE_DBUS_OBJECT_H__
#define __ORAGE_DBUS_OBJECT_H__

#include <glib-object.h>

G_BEGIN_DECLS;

typedef struct _OrageDBusClass OrageDBusClass;
typedef struct _OrageDBus      OrageDBus;


#define ORAGE_DBUS_TYPE             (orage_dbus_get_type())
#define ORAGE_DBUS(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), ORAGE_TYPE_DBUS, OrageDBus))
#define ORAGE_DBUS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), ORAGE_TYPE_DBUS, OrageDBusClass))
#define ORAGE_IS_DBUS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), ORAGE_TYPE_DBUS))
#define ORAGE_IS_DBUS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass), ORAGE_TYPE_DBUS_BRIGDE))
#define ORAGE_DBUS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ORAGE_TYPE_DBUS, OrageDBustClass))

GType orage_dbus_get_type(void);

gboolean orage_dbus_service_load_file(DBusGProxy *proxy
                , const char *IN_file
                , GError **error);
gboolean orage_dbus_service_export_file(DBusGProxy *proxy
                , const char *IN_file, const gint IN_type, const char *IN_uids
                , GError **error);
gboolean orage_dbus_service_add_foreign(DBusGProxy *proxy
                , const char *IN_file, const gboolean IN_mode
                , GError **error);
gboolean orage_dbus_service_remove_foreign(DBusGProxy *proxy
                , const char *IN_file
                , GError **error);

void orage_dbus_start(void);


G_END_DECLS;

#endif /* !__ORAGE_DBUS_OBJECT_H__ */

