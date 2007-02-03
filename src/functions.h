/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2006-2007 Juha Kautto  (juha at xfce.org)
 * Copyright (c) 2005-2006 Mickael Graf (korbinus at xfce.org)
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

#ifndef __ORAGE_FUNCTIONS_H__
#define __ORAGE_FUNCTIONS_H__

#define ORAGE_STR_EXISTS(str) ((str != NULL) && (str[0] != 0))

GtkWidget *orage_toolbar_append_button(GtkWidget *toolbar
        , const gchar *stock_id, GtkTooltips *tooltips
        , const char *tooltip_text, gint pos);

GtkWidget *orage_toolbar_append_separator(GtkWidget *toolbar, gint pos);

GtkWidget *orage_table_new(guint rows, guint border);

void orage_table_add_row(GtkWidget *table, GtkWidget *label
        , GtkWidget *input, guint row
        , GtkAttachOptions input_x_option, GtkAttachOptions input_y_option);

GtkWidget *orage_menu_new(const gchar *menu_header_title, GtkWidget *menu_bar);

GtkWidget *orage_image_menu_item_new_from_stock(const gchar *stock_id
        , GtkWidget *menu, GtkAccelGroup *ag);

GtkWidget *orage_separator_menu_item_new(GtkWidget *menu);

GtkWidget *orage_menu_item_new_with_mnemonic(const gchar *label
        , GtkWidget *menu);

struct tm *orage_localtime();

void orage_select_date(GtkCalendar *cal
        , guint year, guint month, guint day);

void orage_select_today(GtkCalendar *cal);

/*
struct tm orage_i18_date_to_tm_date(const char *display);

char *orage_tm_date_to_i18_date(struct tm tm_date);
*/
#endif /* !__ORAGE_FUNCTIONS_H__ */
