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

#ifndef __ORAGE_DBUS_H__
#define __ORAGE_DBUS_H__

void orage_dbus_start(void);
gboolean orage_dbus_import_file(gchar *file_name);
gboolean orage_dbus_export_file(gchar *file_name, gint type, gchar *uids);
gboolean orage_dbus_foreign_add(gchar *file_name, gboolean read_only
        , gchar *name);
gboolean orage_dbus_foreign_remove(gchar *file_name);

#endif /* !__ORAGE_DBUS_H__ */
