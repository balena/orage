/* reminder.c
 *
 * (C) 2003-2004 Mickaël Graf
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfcegui4/dialogs.h>

void
on_btOkReminder_clicked(GtkButton *button, gpointer user_data)
{
  GtkWidget *wReminder = (GtkWidget *)wReminder;
  gtk_widget_destroy(wReminder); /* destroy the specific appointment window */
}

GtkWidget*
create_wReminder(char *text)
{
  GtkWidget *wReminder;
  GtkWidget *vbReminder;
  GtkWidget *lbReminder;
  GtkWidget *daaReminder;
  GtkWidget *btOkReminder;
  GtkWidget *swReminder;
  GtkWidget *hdReminder;

  wReminder = gtk_dialog_new ();
  gtk_widget_set_size_request (wReminder, 300, 250);
  gtk_window_set_title (GTK_WINDOW (wReminder), _("Reminder"));
  gtk_window_set_position (GTK_WINDOW (wReminder), GTK_WIN_POS_CENTER);
  gtk_window_set_modal (GTK_WINDOW (wReminder), FALSE);
  gtk_window_set_resizable (GTK_WINDOW (wReminder), TRUE);

  vbReminder = GTK_DIALOG (wReminder)->vbox;
  gtk_widget_show (vbReminder);

  hdReminder = xfce_create_header(NULL, _("Reminder"));
  gtk_widget_show(hdReminder);
  gtk_box_pack_start (GTK_BOX (vbReminder), hdReminder, FALSE, TRUE, 0);

  swReminder = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(swReminder), GTK_SHADOW_NONE);
  gtk_widget_show (swReminder);
  gtk_box_pack_start (GTK_BOX (vbReminder), swReminder, TRUE, TRUE, 5);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(swReminder), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  lbReminder = gtk_label_new (_(text));
  gtk_label_set_line_wrap(GTK_LABEL(lbReminder), TRUE);
  gtk_widget_show (lbReminder);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(swReminder), lbReminder);

  daaReminder = GTK_DIALOG (wReminder)->action_area;
  gtk_dialog_set_has_separator(GTK_DIALOG(wReminder), FALSE);
  gtk_widget_show (daaReminder);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (daaReminder), GTK_BUTTONBOX_END);

  btOkReminder = gtk_button_new_from_stock ("gtk-close");
  gtk_widget_show (btOkReminder);
  gtk_dialog_add_action_widget (GTK_DIALOG (wReminder), btOkReminder, GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS (btOkReminder, GTK_CAN_DEFAULT);

  g_signal_connect ((gpointer) btOkReminder, "clicked",
		    G_CALLBACK (on_btOkReminder_clicked),
		    wReminder);

  return wReminder;
}

