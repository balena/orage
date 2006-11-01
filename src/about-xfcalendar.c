/* about-xfcalendar.c
 *
 * (C) 2004-2006 Mickaël Graf
 * (C) 2004-2006 Juha Kautto
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
#  include <config.h>
#endif

#include <libxfcegui4/libxfcegui4.h>

#include "mainbox.h"
#include "tray_icon.h"


void create_wAbout(GtkWidget *widget, gpointer user_data)
{
  CalWin *xfcal = (CalWin *)user_data;
  GtkWidget *dialog;
  GdkPixbuf *xfcalendar_logo;
  XfceAboutInfo *about;

  about = xfce_about_info_new("Orage", VERSION
          , _("Manage your time with Xfce4")
          , XFCE_COPYRIGHT_TEXT("2003-2006", "Mickael 'Korbinus' Graf & Juha Kautto")
          , XFCE_LICENSE_GPL);
  /* xfcalendar_logo = xfce_themed_icon_load("xfcalendar", 48); */
  xfcalendar_logo = create_icon(xfcal, 48, 48);
  xfce_about_info_set_homepage(about, "http://www.xfce.org");

  /* Credits */
  xfce_about_info_add_credit(about,
			     "Mickael 'Korbinus' Graf",
			     "korbinus@xfce.org",
			     _("Core developer"));

  xfce_about_info_add_credit(about,
			     "Juha Kautto",
			     "juha@xfce.org",
			     _("Core developer"));

  xfce_about_info_add_credit(about,
			     "Benedikt Meurer",
			     "benny@xfce.org",
			     _("Contributor"));

  xfce_about_info_add_credit(about,
			     "Edscott Wilson Garcia",
			     "edscott@imp.mx",
			     _("Contributor"));

  dialog = xfce_about_dialog_new_with_values(GTK_WINDOW(xfcal->mWindow)
          , about, xfcalendar_logo);

  gtk_window_set_default_size(GTK_WINDOW(dialog), 520, 440);

  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
  xfce_about_info_free(about);
  g_object_unref(xfcalendar_logo);
}
