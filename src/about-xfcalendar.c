/* about-xfcalendar.c
 *
 * (C) 2004 Mickaël Graf
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libxfcegui4/libxfcegui4.h>

#include "mainbox.h"


void create_wAbout(GtkWidget *widget, gpointer user_data){

  GtkWidget *dialog;
  CalWin *xfcal = (CalWin *)user_data;

  GdkPixbuf *xfcalendar_logo = xfce_themed_icon_load ("xfcalendar", 48);

  XfceAboutInfo *about = xfce_about_info_new("Orage",
			      VERSION,
			      _("Manage your time with Xfce4"),
			      XFCE_COPYRIGHT_TEXT("2003-2005", "Mickael 'Korbinus' Graf"),
			      XFCE_LICENSE_GPL);

  xfce_about_info_set_homepage(about, "http://www.xfce.org");

  /* Credits */
  xfce_about_info_add_credit(about,
			     "Mickael 'Korbinus' Graf",
			     "korbinus@xfce.org",
			     _("Core developer"));

  xfce_about_info_add_credit(about,
			     "Benedikt Meurer",
			     "benny@xfce.org",
			     _("Contributor"));

  xfce_about_info_add_credit(about,
			     "Edscott Wilson Garcia",
			     "edscott@imp.mx",
			     _("Contributor"));

  xfce_about_info_add_credit(about,
			     "Juha Kautto",
			     "kautto.juha@kolumbus.fi",
			     _("Contributor"));

  dialog = xfce_about_dialog_new(GTK_WINDOW(xfcal->mWindow), about,
                                 xfcalendar_logo);

  gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 400);
  xfce_about_info_free(about);

  gtk_dialog_run(GTK_DIALOG(dialog));

  gtk_widget_destroy(dialog);

  g_object_unref(xfcalendar_logo);
}
