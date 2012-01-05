/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2005-2011 Juha Kautto  (juha at xfce.org)
 * Copyright (c) 2004-2006 Mickael Graf (korbinus at xfce.org)
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
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "orage-i18n.h"
#include "tray_icon.h"
#include "about-xfcalendar.h"


void create_wAbout(GtkWidget *widget, gpointer user_data)
{
  GtkWidget *dialog;
  GdkPixbuf *orage_logo;
  GtkAboutDialog *about;
  const gchar *authors[] = {"Juha Kautto <juha@xfce.org>", _("Maintainer"), NULL};

  dialog = gtk_about_dialog_new();
  about = (GtkAboutDialog *) dialog;
  gtk_about_dialog_set_program_name(about, "Orage");
  gtk_about_dialog_set_version(about, VERSION);
  gtk_about_dialog_set_copyright(about, "Copyright © 2003-2011 Juha Kautto");
  gtk_about_dialog_set_comments(about, _("Manage your time with Orage"));
  /*
  gtk_about_dialog_set_license(about, XFCE_LICENSE_GPL);
  */
  gtk_about_dialog_set_website(about, "http://www.xfce.org");
  gtk_about_dialog_set_authors(about, authors);
  gtk_about_dialog_set_documenters(about, authors);
  orage_logo = orage_create_icon(FALSE, 48);
  gtk_about_dialog_set_logo(about, orage_logo);

  gtk_window_set_default_size(GTK_WINDOW(dialog), 520, 440);

  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
  g_object_unref(orage_logo);
}
