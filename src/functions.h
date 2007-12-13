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

#define XFICAL_APPT_TIME_FORMAT "%04d%02d%02dT%02d%02d%02d"
#define XFICAL_APPT_TIME_FORMAT_LEN 16
#define XFICAL_APPT_DATE_FORMAT "%04d%02d%02d"
#define XFICAL_APPT_DATE_FORMAT_LEN 9

#define ORAGE_STR_EXISTS(str) ((str != NULL) && (str[0] != 0))

void orage_message(const char *format, ...);

GtkWidget *orage_create_combo_box_with_content(char *text[], int size);

gboolean orage_date_button_clicked(GtkWidget *button, GtkWidget *win);

gboolean orage_exec(const char *cmd, gboolean *cmd_active, GError **error);

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
struct tm orage_i18_date_to_tm_date(const char *display);
char *orage_tm_time_to_i18_time(struct tm *tm_date);
char *orage_tm_date_to_i18_date(struct tm *tm_date);
struct tm orage_icaltime_to_tm_time(const char *i18_date, gboolean real_tm);
char *orage_tm_time_to_icaltime(struct tm *t);
char *orage_icaltime_to_i18_time(const char *icaltime);
char *orage_i18_date_to_icaltime(const char *i18_date);
char *orage_cal_to_i18_date(GtkCalendar *cal);
char *orage_localdate_i18();
gint orage_days_between(struct tm *t1, struct tm *t2);

void orage_select_date(GtkCalendar *cal, guint year, guint month, guint day);
void orage_select_today(GtkCalendar *cal);

#endif /* !__ORAGE_FUNCTIONS_H__ */
