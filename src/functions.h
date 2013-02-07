/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2006-2011 Juha Kautto  (juha at xfce.org)
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
#define ORAGE_COLOR_FORMAT "%uR %uG %uB"

#define ORAGE_DIR "orage" G_DIR_SEPARATOR_S
#define ORAGE_PAR_FILE  "oragerc"
#define ORAGE_PAR_DIR_FILE ORAGE_DIR ORAGE_PAR_FILE
#define ORAGE_APP_FILE  "orage.ics"
#define ORAGE_APP_DIR_FILE ORAGE_DIR ORAGE_APP_FILE
#define ORAGE_ARC_FILE  "orage_archive.ics"
#define ORAGE_ARC_DIR_FILE ORAGE_DIR ORAGE_ARC_FILE
#define ORAGE_CATEGORIES_FILE "orage_categories.txt"
#define ORAGE_CATEGORIES_DIR_FILE ORAGE_DIR ORAGE_CATEGORIES_FILE
#define ORAGE_PERSISTENT_ALARMS_FILE "orage_persistent_alarms.txt"
#define ORAGE_PERSISTENT_ALARMS_DIR_FILE ORAGE_DIR ORAGE_PERSISTENT_ALARMS_FILE
#define ORAGE_DEFAULT_ALARM_FILE "orage_default_alarm.txt"
#define ORAGE_DEFAULT_ALARM_DIR_FILE ORAGE_DIR ORAGE_DEFAULT_ALARM_FILE

#define ORAGE_STR_EXISTS(str) ((str != NULL) && (str[0] != 0))

#if !GLIB_CHECK_VERSION(2,14,0)
#define g_timeout_add_seconds(interval,func,data) \
        g_timeout_add((interval)*1000, func, data)
#endif

typedef struct _OrageRc
{
    void *rc;
    gboolean read_only;
    const char *file_name;
    const char *cur_group;
} OrageRc;

void program_log (const char *format, ...);

void orage_message(gint level, const char *format, ...);

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
char *orage_replace_text(char *text, char *old, char *new);
char *orage_process_text_commands(char *text);
GtkWidget *orage_period_hbox_new(gboolean head_space, gboolean tail_space
        , GtkWidget *spin_dd, GtkWidget *dd_label
        , GtkWidget *spin_hh, GtkWidget *hh_label
        , GtkWidget *spin_mm, GtkWidget *mm_label);

struct tm *orage_localtime();
char *orage_localdate_i18();
struct tm orage_i18_time_to_tm_time(const char *i18_time);
struct tm orage_i18_date_to_tm_date(const char *i18_date);
char *orage_i18_time_to_icaltime(const char *i18_time);
char *orage_i18_date_to_icaldate(const char *i18_date);
char *orage_tm_time_to_i18_time(struct tm *tm_date);
char *orage_tm_date_to_i18_date(struct tm *tm_date);
char *orage_tm_time_to_icaltime(struct tm *t);
struct tm orage_icaltime_to_tm_time(const char *i18_date, gboolean real_tm);
char *orage_icaltime_to_i18_time(const char *icaltime);
char *orage_icaltime_to_i18_time_only(const char *icaltime);
struct tm orage_cal_to_tm_time(GtkCalendar *cal, gint hh, gint mm);
char *orage_cal_to_i18_time(GtkCalendar *cal, gint hh, gint mm);
char *orage_cal_to_i18_date(GtkCalendar *cal);
char *orage_cal_to_icaldate(GtkCalendar *cal);
void orage_move_day(struct tm *t, int day);
gint orage_days_between(struct tm *t1, struct tm *t2);

void orage_select_date(GtkCalendar *cal, guint year, guint month, guint day);
void orage_select_today(GtkCalendar *cal);

gchar *orage_data_file_location(char *dir_name);
gchar *orage_config_file_location(char *dir_name);
OrageRc *orage_rc_file_open(char *fpath, gboolean read_only);
void orage_rc_file_close(OrageRc *orc);
gchar **orage_rc_get_groups(OrageRc *orc);
void orage_rc_set_group(OrageRc *orc, char *grp);
void orage_rc_del_group(OrageRc *orc, char *grp);
gchar *orage_rc_get_group(OrageRc *orc);
gchar *orage_rc_get_str(OrageRc *orc, char *key, char *def);
gint   orage_rc_get_int(OrageRc *orc, char *key, gint def);
gboolean orage_rc_get_bool(OrageRc *orc, char *key, gboolean def);
void orage_rc_put_str(OrageRc *orc, char *key, char *val);
void orage_rc_put_int(OrageRc *orc, char *key, gint val);
void orage_rc_put_bool(OrageRc *orc, char *key, gboolean val);
gboolean orage_rc_exists_item(OrageRc *orc, char *key);
void orage_rc_del_item(OrageRc *orc, char *key);

gint orage_info_dialog(GtkWindow *parent
        , char *primary_text, char *secondary_text);
gint orage_warning_dialog(GtkWindow *parent
        , char *primary_text, char *secondary_text
        , char *no_text, char *yes_text);
gint orage_error_dialog(GtkWindow *parent
        , char *primary_text, char *secondary_text);
GtkWidget *orage_create_framebox_with_content(const gchar *title
        , GtkWidget *content);
GtkWidget *orage_create_custom_stock_button(const gchar *stock_id
        , const gchar *label_text);

/* NOTE: the following is in main.c */
void orage_toggle_visible(void);

#endif /* !__ORAGE_FUNCTIONS_H__ */
