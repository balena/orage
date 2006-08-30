/* tray_icon.c
 *
 * (C) 2004-2006 Mickaël Graf
 * (C)      2006 Juha Kautto
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

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <time.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfcegui4/netk-trayicon.h>
#include <libxfce4mcs/mcs-client.h>

#include "event-list.h"
#include "appointment.h"
#include "functions.h"
#include "mainbox.h"
#include "xfce_trayicon.h"
#include "about-xfcalendar.h"

#define CHANNEL  "orage"

extern gint icon_size_x, icon_size_y;


void
on_Today_activate                      (GtkMenuItem *menuitem,
                                        gpointer user_data)
{
    struct tm *t;
    CalWin *xfcal = (CalWin *)user_data;
    eventlist_win *el;

    t = orage_localtime();
    xfcalendar_select_date(GTK_CALENDAR(xfcal->mCalendar), t->tm_year+1900
            , t->tm_mon, t->tm_mday);
    el = create_eventlist_win();
    manage_eventlist_win(GTK_CALENDAR(xfcal->mCalendar), el);
}

void 
on_preferences_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  mcs_client_show (GDK_DISPLAY (), DefaultScreen (GDK_DISPLAY ()),
		   CHANNEL);
}

void
on_new_appointment_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    appt_win *app;
    struct tm *t;
    char cur_date[9];

    t = orage_localtime();
    g_snprintf(cur_date, 9, "%04d%02d%02d", t->tm_year+1900
               , t->tm_mon+1, t->tm_mday);
    app = create_appt_win("NEW", cur_date, NULL);  
}

void
on_about_activate(GtkMenuItem *menuitem, gpointer user_data)
{
  create_wAbout((GtkWidget *)menuitem, user_data);
}

void 
toggle_visible_cb ()
{
  xfcalendar_toggle_visible ();
}

GdkPixbuf *create_icon(CalWin *xfcal, gint x, gint y)
{
  GdkPixbuf *pixbuf;
  GdkPixmap *pic1;
  GdkGC *pic1_gc1, *pic1_gc2;
  GdkColormap *pic1_cmap;
  GdkColor color;
  PangoLayout *pl_day, *pl_head, *pl_month;
  gint width = 0, height = 0, depth = 16;
  gint red = 239, green = 235, blue = 230;
  PangoRectangle real_rect, log_rect;
  struct tm *t;
  gchar ts[200], month[50];
  gint x_offset = 0, y_offset = 0, y_used = 0, i, limit;
  gint x_day = 0, y_day = 0;
  gint x_head = 0, y_head = 0, y_used_head = 0;
  gint x_month = 0, y_month = 0, y_used_month = 0;
  gboolean draw_head = FALSE, draw_month = FALSE;
  gboolean draw_dynamic = FALSE, work_in_progress = TRUE;
  gchar *day_sizes[] = {"xx-large", "x-large", "large", "medium"
                      , "small", "x-small", "xx-small", "END"};

  if (x <= 12 || y <= 12) {
      g_message("Orage **: Too small icon size, using static icon\n");
      pixbuf = xfce_themed_icon_load("xfcalendar", 16);
      return(pixbuf);
  }
  if (icon_size_x == 0 || icon_size_y == 0) { /* signal to use static icon */
      g_message("Orage **: Icon size set to zero, using static icon\n");
      pixbuf = xfce_themed_icon_load("xfcalendar", x);
      return(pixbuf);
  }

  t = orage_localtime();
  width = x; 
  height = y;
  depth = gdk_visual_get_best_depth();
  pic1 = gdk_pixmap_new(NULL, width, height, depth);
  pic1_cmap = gdk_screen_get_default_colormap(gdk_screen_get_default());
  /* pic1_cmap = gtk_widget_get_colormap(xfcal->mWindow); */
  pic1_gc1 = gdk_gc_new(pic1);
  pic1_gc2 = gdk_gc_new(pic1);
  color.red = red * (65535/255);
  color.green = green * (65535/255);
  color.blue = blue * (65535/255);
  color.pixel = (gulong)(red*65536 + green*256 + blue);
  gdk_color_alloc(pic1_cmap, &color);
  gdk_gc_set_foreground(pic1_gc1, &color);
  gdk_draw_rectangle(pic1, pic1_gc1, TRUE, 0, 0, width, height);
  gdk_draw_rectangle(pic1, pic1_gc2, FALSE, 0, 0, width-1, height-1);
  gdk_draw_rectangle(pic1, pic1_gc2, FALSE, 0, 0, width-3, height-3);

  /* gdk_draw_line(pic1, pic1_gc1, 10, 20, 30, 38); */

  /* create any valid pango layout to get things started */
  /* this does not quite work, but almost
  pl_day = pango_layout_new(gdk_pango_context_get_for_screen(gdk_screen_get_default()));
  */
  pl_day = gtk_widget_create_pango_layout(xfcal->mWindow, "x");
  pl_head = pango_layout_copy(pl_day);
  pl_month = pango_layout_copy(pl_day);

  /* heading: orage */
  g_snprintf(ts, 199
          , "<span foreground=\"blue\" size=\"x-small\">orage</span>");
  pango_layout_set_markup(pl_head, ts, -1);
  pango_layout_set_alignment(pl_head, PANGO_ALIGN_CENTER);
  pango_layout_get_extents(pl_head, &real_rect, &log_rect);
  x_offset = (width - PANGO_PIXELS(log_rect.width) - 2)/2;
  y_offset = -2;
  /* g_print("orage offset x=%d y=%d height=%d text height=%d real text height=%d\n" , x_offset, y_offset, height, PANGO_PIXELS(log_rect.height), PANGO_PIXELS(real_rect.height)); */
  if (x_offset > 0 && (height-PANGO_PIXELS(log_rect.height)-y_offset) > 0) {
      draw_head = TRUE; /* fits */
      x_head = x_offset;
      y_head = y_offset;
      y_used_head = PANGO_PIXELS(real_rect.height);
  }

  /* month */
  if (strftime(month, 19, "%b", t) == 0) {
      g_warning("create_icon: strftime failed");
      g_sprintf(month, "orage");
  }
  g_snprintf(ts, 199
          , "<span foreground=\"blue\" size=\"x-small\">%s</span>"
          , month);
  pango_layout_set_markup(pl_month, ts, -1);
  pango_layout_set_alignment(pl_month, PANGO_ALIGN_CENTER);
  pango_layout_get_extents(pl_month, &real_rect, &log_rect);
  x_offset = (width - PANGO_PIXELS(real_rect.width) - 2)/2;
  y_offset = (height - PANGO_PIXELS(log_rect.height) - 2);
  if (x_offset > 0 && (height - y_offset - PANGO_PIXELS(log_rect.height))) {
      draw_month = TRUE; /* fits */
      x_month = x_offset;
      y_month = y_offset;
      y_used_month = PANGO_PIXELS(real_rect.height);
  }

  do { /* main loop where we try our best to fit header+day+month into icon */
      y_used = 0;
      if (draw_month || draw_head) {
          limit = 3; /* = medium */
          if (draw_head)
              y_used += y_used_head;
          if (draw_month)
              y_used += y_used_month;
      }
      else
          limit = 10; /* no limit */

  /* day */
      for (i = 0, x_offset = 0, y_offset = 0; 
           (strcmp(day_sizes[i], "END") != 0) 
                && (i <= limit)
                && ((x_offset <= 0) || ((y_offset) <= 0));
           i++) {
      g_snprintf(ts, 199
          , "<span foreground=\"red\" weight=\"bold\" size=\"%s\">%02d</span>"
          , day_sizes[i], t->tm_mday);
      pango_layout_set_markup(pl_day, ts, -1);
      pango_layout_set_alignment(pl_day, PANGO_ALIGN_CENTER);
      pango_layout_get_extents(pl_day, &real_rect, &log_rect);
      x_offset = (width - PANGO_PIXELS(log_rect.width))/2;
      y_offset = (height - y_used - PANGO_PIXELS(log_rect.height))/2;
      } /* for */
      if (x_offset >= 0 && (y_offset) >= 0) { /* it fits */
          draw_dynamic = TRUE;
          work_in_progress = FALSE; /* done! */
          x_day = x_offset;
          y_day = (height - PANGO_PIXELS(log_rect.height) - 2)/2;
          if (!draw_head && draw_month)
              y_day -= y_used_head/2;
          if (draw_head && !draw_month)
              y_day += y_used_head/2;
      }
      else {
          if (draw_head)
              draw_head = FALSE; /* does not fit */
          else if (draw_month)
              draw_month = FALSE; /* does not fit */
          else
              work_in_progress = FALSE; /* done! */
      }
  } while (work_in_progress);

  if (draw_dynamic) {
      if (draw_head)
          gdk_draw_layout(pic1, pic1_gc1, x_head, y_head, pl_head);
      if (draw_month)
          gdk_draw_layout(pic1, pic1_gc1, x_month, y_month, pl_month);
      gdk_draw_layout(pic1, pic1_gc1, x_day, y_day, pl_day);

      pixbuf = gdk_pixbuf_get_from_drawable(NULL, pic1, pic1_cmap
              , 0, 0, 0, 0, width, height);
  }
  else
      pixbuf = xfce_themed_icon_load("xfcalendar", 16);

  if (pixbuf == NULL)
      g_warning("create_icon: load failed\n");

  g_object_unref(pic1_gc1);
  g_object_unref(pic1_gc2);
  g_object_unref(pl_day);
  g_object_unref(pl_head);
  g_object_unref(pl_month);
  g_object_unref(pic1);
  
  return(pixbuf);
}

void destroy_TrayIcon(XfceTrayIcon *trayIcon)
{
    if (trayIcon == NULL) 
        return;
    if (trayIcon->tip_text != NULL)
        g_free(trayIcon->tip_text);
    if (trayIcon->tip_private != NULL)
        g_free(trayIcon->tip_private);
    g_object_unref(G_OBJECT(trayIcon->tooltips));
    g_object_unref(G_OBJECT(trayIcon->image));
    /*
    g_object_unref(GTK_OBJECT(trayIcon));
    */
}

XfceTrayIcon* create_TrayIcon(CalWin *xfcal)
{
  XfceTrayIcon *trayIcon = NULL;
  GtkWidget *menuItem;
  GtkWidget *trayMenu;
  GdkPixbuf *pixbuf;

  /*
   * Create the tray icon popup menu
   */
  trayMenu = gtk_menu_new();
  menuItem = gtk_image_menu_item_new_with_mnemonic(_("Today"));
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuItem)
          , gtk_image_new_from_stock(GTK_STOCK_HOME, GTK_ICON_SIZE_MENU));
  g_signal_connect(menuItem, "activate", G_CALLBACK(on_Today_activate)
          , xfcal);
  gtk_menu_shell_append(GTK_MENU_SHELL(trayMenu), menuItem);
  gtk_widget_show_all(menuItem);
  menuItem = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(trayMenu), menuItem);
  gtk_widget_show(menuItem);

  menuItem = gtk_image_menu_item_new_with_label(_("New appointment"));
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuItem)
          , gtk_image_new_from_stock(GTK_STOCK_NEW, GTK_ICON_SIZE_MENU));
  g_signal_connect(menuItem, "activate"
          , G_CALLBACK(on_new_appointment_activate), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(trayMenu), menuItem);
  gtk_widget_show(menuItem);
  menuItem = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(trayMenu), menuItem);
  gtk_widget_show(menuItem);
  
  menuItem = gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, NULL);
  g_signal_connect(menuItem, "activate", G_CALLBACK(on_preferences_activate)
          , NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(trayMenu), menuItem);
  gtk_widget_show(menuItem);
  menuItem = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(trayMenu), menuItem);
  gtk_widget_show(menuItem);

  menuItem = gtk_image_menu_item_new_with_label(_("About Orage"));
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuItem)
          , gtk_image_new_from_stock(GTK_STOCK_ABOUT, GTK_ICON_SIZE_MENU));
  g_signal_connect(menuItem, "activate", G_CALLBACK(on_about_activate)
          , xfcal);
  gtk_menu_shell_append(GTK_MENU_SHELL(trayMenu), menuItem);
  gtk_widget_show(menuItem);
  menuItem = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(trayMenu), menuItem);
  gtk_widget_show(menuItem);

  menuItem = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
  g_signal_connect(menuItem, "activate", G_CALLBACK(gtk_main_quit), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(trayMenu), menuItem);
  gtk_widget_show(menuItem);

  /*
   * Create the tray icon
   */

  /* pixbuf = xfce_themed_icon_load ("xfcalendar", 16); */
  pixbuf = create_icon(xfcal, icon_size_x, icon_size_y);
  trayIcon = xfce_tray_icon_new_with_menu_from_pixbuf(trayMenu, pixbuf);
  g_object_ref(trayIcon);
  gtk_object_sink(GTK_OBJECT(trayIcon));
  g_object_unref(pixbuf);

  g_signal_connect_swapped(G_OBJECT(trayIcon), "clicked",
			   G_CALLBACK(toggle_visible_cb), xfcal);
  return trayIcon;
}
