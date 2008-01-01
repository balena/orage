/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2007-2008 Juha Kautto  (juha at xfce.org)
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


#ifndef __ORAGE_I18N_H__
#define __ORAGE_I18N_H__

#include <glib.h>

#if defined(GETTEXT_PACKAGE)
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif

#if !defined(GETTEXT_PACKAGE)

#ifdef gettext
#undef gettext
#endif
#ifdef dgettext
#undef dgettext
#endif
#ifdef dcgettext
#undef dcgettext
#endif
#ifdef ngettext
#undef ngettext
#endif
#ifdef dngettext
#undef dngettext
#endif
#ifdef dcngettext
#undef dcngettext
#endif

#define gettext(s)                                            (s)
#define dgettext(domain,s)                                    (s)
#define dcgettext(domain,s,type)                              (s)
#define ngettext(msgid, msgid_plural, n)                      (((n) > 0) ? (msgid) : (msgid_plural))
#define dngettext(domainname, msgid, msgid_plural, n)         (((n) > 0) ? (msgid) : (msgid_plural))
#define dcngettext(domainname, msgid, msgid_plural, n, type)  (((n) > 0) ? (msgid) : (msgid_plural))

#endif /* !defined(GETTEXT_PACKAGE) */

#endif  /* !__ORAGE_I18N_H__ */
