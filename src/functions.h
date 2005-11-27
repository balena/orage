/* functions.h
 *
 * Copyright (C) 2005 Mickaël Graf <korbinus at xfce.org>
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

/**************************************
 *  Functions for drawing interfaces  *
 **************************************/

GtkWidget *xfcalendar_toolbar_append_button (GtkWidget *toolbar, const gchar *stock_id, 
                                             GtkTooltips *tooltips, const char *tooltip_text, 
                                             gint pos);

GtkWidget *xfcalendar_toolbar_append_separator (GtkWidget *toolbar, gint pos);

void xfcalendar_combo_box_append_array (GtkWidget *combo_box, char *text[], int size);

GtkWidget *xfcalendar_datetime_hbox_new (GtkWidget *date_button, GtkWidget *time_comboboxentry, GtkWidget *timezone_combobox);

GtkWidget *xfcalendar_table_new (guint rows, guint columns);

void xfcalendar_table_add_row (GtkWidget *table, GtkWidget *label, GtkWidget *input, guint row,
                               GtkAttachOptions input_x_option, GtkAttachOptions input_y_option);

GtkWidget *xfcalendar_menu_new(const gchar *menu_header_title, GtkWidget *menu_bar);

GtkWidget *xfcalendar_image_menu_item_new_from_stock (const gchar *stock_id, GtkWidget *menu, GtkAccelGroup *ag);

GtkWidget *xfcalendar_separator_menu_item_new (GtkWidget *menu);

GtkWidget *xfcalendar_menu_item_new_with_mnemonic (const gchar *label, GtkWidget *menu);

void xfcalendar_select_date (GtkCalendar *cal, guint year, guint month, guint day);
