/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2005-2008 Juha Kautto  (juha at xfce.org)
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
#include "mainbox.h"
#include "tray_icon.h"


void create_wAbout(GtkWidget *widget, gpointer user_data)
{
  CalWin *xfcal = (CalWin *)user_data;
  GtkWidget *dialog;
  GdkPixbuf *orage_logo;
  GtkAboutDialog *about;
  const gchar *authors[] = {"Juha Kautto <juha@xfce.org>", _("Maintainer"), NULL};

  dialog = gtk_about_dialog_new();
  about = (GtkAboutDialog *) dialog;
  gtk_about_dialog_set_program_name(about, "Orage");
  gtk_about_dialog_set_version(about, VERSION);
  gtk_about_dialog_set_copyright(about, "Copyright © 2003-2010 Juha Kautto");
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
/*
void create_wAbout_old(GtkWidget *widget, gpointer user_data)
{
  CalWin *xfcal = (CalWin *)user_data;
  GtkWidget *dialog;
  GdkPixbuf *orage_logo;
  XfceAboutInfo *about;

  about = xfce_about_info_new("Orage", VERSION
          , _("Manage your time with Xfce4")
          , XFCE_COPYRIGHT_TEXT("2003-2008", " Juha Kautto")
          , XFCE_LICENSE_GPL);
  orage_logo = orage_create_icon(FALSE, 48);
  xfce_about_info_set_homepage(about, "http://www.xfce.org");

  xfce_about_info_add_credit(about,
			     "Juha Kautto",
			     "juha@xfce.org",
			     _("Maintainer"));

  xfce_about_info_add_credit(about,
			     "Mickael 'Korbinus' Graf",
			     "korbinus@xfce.org",
			     _("Original creator, retired maintainer"));

  xfce_about_info_add_credit(about,
			     "Benedikt Meurer",
			     "benny@xfce.org",
			     _("Contributor"));

  dialog = xfce_about_dialog_new_with_values(GTK_WINDOW(xfcal->mWindow)
          , about, orage_logo);

  gtk_window_set_default_size(GTK_WINDOW(dialog), 520, 440);

  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
  xfce_about_info_free(about);
  g_object_unref(orage_logo);
}
*/
