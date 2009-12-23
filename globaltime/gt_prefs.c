/*  
 *  Global Time - Set of clocks showing time in different parts of world.
 *  Copyright 2006 Juha Kautto (kautto.juha@kolumbus.fi)
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
 *  To get a copy of the GNU General Public License write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <glib.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>

#include "globaltime.h"
#include "timezone_selection.h"


extern global_times_struct clocks;
extern char *attr_underline[];

typedef struct modify
{ /* contains data for one clock modification window */
    clock_struct *clock;
    GtkWidget *window;
    GtkWidget *name_entry;
    GtkWidget *button_tz;
    GtkWidget *button_clock_fg;
    GtkWidget *check_button_default_fg;
    GtkWidget *button_clock_bg;
    GtkWidget *check_button_default_bg;
    GtkWidget *button_font_name;
    GtkWidget *check_button_default_font_name;
    GtkWidget *button_font_time;
    GtkWidget *check_button_default_font_time;
    GtkWidget *combo_box_underline_name;
    GtkWidget *check_button_default_underline_name;
    GtkWidget *combo_box_underline_time;
    GtkWidget *check_button_default_underline_time;
} modify_struct;



static void create_parameter_formatting(GtkWidget *vbox
        , modify_struct *modify_clock);


static gboolean decoration_radio_button_pressed(GtkWidget *widget, gchar *label)
{
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
        if (strcmp(label, "Standard") == 0)
            clocks.decorations = TRUE;
        else if (strcmp(label, "None") == 0)
            clocks.decorations = FALSE;
        else 
            g_warning("Unknown selection in decoration radio button\n");
        write_file();
        gtk_window_set_decorated(GTK_WINDOW(clocks.window), clocks.decorations);
    }
    return(FALSE);
}

static gboolean clocksize_radio_button_pressed(GtkWidget *widget, gchar *label)
{
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
        if (strcmp(label, "Equal") == 0)
            clocks.expand = TRUE;
        else if (strcmp(label, "Vary") == 0)
            clocks.expand = FALSE;
        else 
            g_warning("Unknown selection in clock size radio button\n");
        write_file();
    }
    return(FALSE);
}

static gboolean release_preferences_window(GtkWidget *widget
        , modify_struct *modify_default)
{
    g_free(modify_default);
    clocks.modified--;
    return(FALSE);
}
                                                                                
static void close_preferences_window(GtkWidget *widget, GtkWidget *window)
{
    gtk_widget_destroy(window);
}
                                                                                
static gboolean save_preferences(GtkWidget *widget
        , modify_struct *modify_default)
{
    write_file();
    return(FALSE);
}

static void add_separator(GtkWidget *box, gchar type)
{
    GtkWidget *separator;

    if (type == 'H') 
        separator = gtk_hseparator_new();
    else
        separator = gtk_vseparator_new();
    gtk_box_pack_start(GTK_BOX(box), separator, FALSE, FALSE, 1);
    gtk_widget_show(separator);
}

static GtkWidget *add_box(GtkWidget *box, gchar type)
{
    GtkWidget *new_box;

    if (type == 'H') 
        new_box = gtk_hbox_new(FALSE, 0);
    else
        new_box = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), new_box, FALSE, FALSE, 2);
    gtk_widget_show(new_box);
    return(new_box);
}

static void add_header(GtkWidget *box, gchar *text, gboolean bold)
{
    GtkWidget *label;
    gchar tmp[100];

    if (bold) {
        label = gtk_label_new(NULL);
        sprintf(tmp, "<b>%s</b>", text);
        gtk_label_set_markup(GTK_LABEL(label), tmp);
    }
    else
        label = gtk_label_new(text);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 1);
    gtk_widget_show(label);
}

GtkWidget *toolbar_append_button(GtkWidget *toolbar, const gchar *stock_id
        , GtkTooltips *tooltips, const char *tooltip_text)
{
    GtkWidget *button;
            
    button = (GtkWidget *) gtk_tool_button_new_from_stock(stock_id);
    gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(button), tooltips
            , (const gchar *) tooltip_text, NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), -1);
    return button;
}

static void toolbar_append_separator(GtkWidget *toolbar)
{
    GtkWidget *separator;

    separator = (GtkWidget *) gtk_separator_tool_item_new();
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(separator), -1); 
}

static gboolean save_clock(GtkWidget *widget
        , modify_struct *modify_clock)
{
    clock_struct *clockp = modify_clock->clock;

    g_string_assign(clockp->name
            , gtk_entry_get_text(GTK_ENTRY(modify_clock->name_entry)));
    gtk_label_set_text(GTK_LABEL(clockp->name_label), clockp->name->str);
    write_file();
    return(FALSE);
}

static gboolean release_modify_clock_window(GtkWidget *widget
        , modify_struct *modify_clock)
{
    modify_clock->clock->modified = FALSE;
    g_free(modify_clock);
    clocks.modified--;
    return(FALSE);
}

static gboolean set_timezone_from_clock(GtkWidget *widget
        , modify_struct *modify_clock)
{
    g_string_assign(clocks.local_tz
            , gtk_button_get_label((GTK_BUTTON(modify_clock->button_tz))));
    write_file();
    return(FALSE);
}

static void close_modify_clock_window(GtkWidget *widget, GtkWidget *window)
{
    gtk_widget_destroy(window);
}

static void copy_attr(text_attr_struct *dst_attr, text_attr_struct *src_attr)
{
    *dst_attr->clock_fg = *src_attr->clock_fg;
    dst_attr->clock_fg_modified = src_attr->clock_fg_modified;
    *dst_attr->clock_bg = *src_attr->clock_bg;
    dst_attr->clock_bg_modified = src_attr->clock_bg_modified;

    g_string_assign(dst_attr->name_font, src_attr->name_font->str);
    dst_attr->name_font_modified = src_attr->name_font_modified;
    g_string_assign(dst_attr->time_font, src_attr->time_font->str);
    dst_attr->time_font_modified = src_attr->time_font_modified;

    g_string_assign(dst_attr->name_underline, src_attr->name_underline->str);
    dst_attr->name_underline_modified = src_attr->name_underline_modified;
    g_string_assign(dst_attr->time_underline, src_attr->time_underline->str);
    dst_attr->time_underline_modified = src_attr->time_underline_modified;
}

void init_attr(text_attr_struct *attr)
{
    text_attr_struct *def_attrp = &clocks.clock_default_attr;

    attr->clock_fg = g_new0(GdkColor,1);
    *attr->clock_fg = *clocks.clock_default_attr.clock_fg;
    attr->clock_fg_modified = FALSE;

    attr->clock_bg = g_new0(GdkColor,1);
    *attr->clock_bg = *clocks.clock_default_attr.clock_bg;
    attr->clock_bg_modified = FALSE;

    attr->name_font = g_string_new(def_attrp->name_font->str);
    attr->name_font_modified = FALSE;

    attr->name_underline = g_string_new(def_attrp->name_underline->str);
    attr->name_underline_modified = FALSE;

    attr->time_font = g_string_new(def_attrp->time_font->str);
    attr->time_font_modified = FALSE;

    attr->time_underline = g_string_new(def_attrp->time_underline->str);
    attr->time_underline_modified = FALSE;
}

static void add_clock(GtkWidget *widget, modify_struct *modify_clock)
{
    clock_struct *clockp_old = modify_clock->clock;
    clock_struct *clockp_new; 
    gint new_pos;
    gboolean ret;

    new_pos = g_list_index(clocks.clock_list, clockp_old)+1;
    clockp_new = g_new0(clock_struct, 1);
    clockp_new->tz = g_string_new(_("NEW"));
    clockp_new->name = g_string_new(_("NEW"));
    clockp_new->modified = FALSE;
    init_attr(&clockp_new->clock_attr);

    clocks.clock_list = g_list_insert(clocks.clock_list
                    , clockp_new, new_pos);
    show_clock(clockp_new, &new_pos);
    write_file();
    g_signal_emit_by_name(clockp_new->clock_ebox, "button_press_event"
            , clockp_new, &ret);
}

static void copy_clock(GtkWidget *widget, modify_struct *modify_clock)
{
    clock_struct *clockp_old = modify_clock->clock;
    clock_struct *clockp_new; 
    gint new_pos;
    gboolean ret;

    new_pos = g_list_index(clocks.clock_list, clockp_old)+1;
    clockp_new = g_new(clock_struct, 1);
    clockp_new->tz = g_string_new(gtk_button_get_label(
            GTK_BUTTON(modify_clock->button_tz)));
    clockp_new->name = g_string_new(_("NEW COPY"));
    clockp_new->modified = FALSE;
    init_attr(&clockp_new->clock_attr);
    copy_attr(&clockp_new->clock_attr, &clockp_old->clock_attr);

    clocks.clock_list = g_list_insert(clocks.clock_list
            , clockp_new, new_pos);
    show_clock(clockp_new, &new_pos);
    write_file();
    g_signal_emit_by_name(clockp_new->clock_ebox, "button_press_event"
            , clockp_new, &ret);
}

static void delete_clock(GtkWidget *widget, modify_struct *modify_clock)
{
    clock_struct *clockp = modify_clock->clock;
    GtkWidget *dialog;

    if (g_list_length(clocks.clock_list) == 1) {
        dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT
                , GTK_MESSAGE_INFO , GTK_BUTTONS_CLOSE
                , _("Not possible to delete the last clock."));
        g_signal_connect_swapped (dialog, "response",
               G_CALLBACK (gtk_widget_destroy), dialog);
        gtk_widget_show(dialog);
        return;
    }

    clocks.clock_list = g_list_remove(clocks.clock_list, clockp);
    gtk_widget_destroy(clockp->clock_hbox);
    g_free(clockp);
    write_file();
    gtk_widget_destroy(modify_clock->window); /* self destruct */
}

static void move_clock(GtkWidget *widget, modify_struct *modify_clock)
{
    clock_struct *clockp = modify_clock->clock;
    gint pos, new_pos = 0, len;
    const gchar *button_text;

    if ((len = g_list_length(clocks.clock_list)) == 1) 
        return; /* not possible to move 1 element */

    pos = g_list_index(clocks.clock_list, clockp); 
    button_text = gtk_tool_button_get_stock_id(GTK_TOOL_BUTTON(widget));

    if (strcmp(button_text, "gtk-go-back") == 0) 
        new_pos = pos-1; /* gtk: -1 == last */
    else if (strcmp(button_text, "gtk-go-forward") == 0)
        new_pos = (pos+1 == len ? 0 : pos+1);
    else if (strcmp(button_text, "gtk-goto-first") == 0)
        new_pos = 0;
    else if (strcmp(button_text, "gtk-goto-last") == 0)
        new_pos = (pos+1 == len ? pos : len);
    else
        g_warning("unknown button pressed %s", button_text);

    if (pos == new_pos)
        return; /* not moving at all */

    gtk_box_reorder_child(GTK_BOX(clocks.clocks_hbox)
            , clockp->clock_hbox, new_pos);

    clocks.clock_list = g_list_remove(clocks.clock_list, clockp);
    clocks.clock_list = g_list_insert(clocks.clock_list, clockp, new_pos);
    write_file();
}

/* We handle here timezone setting for individual clocks, but also
 * the generic (default) setting. We know we are doing defaults when there is no
 * clock pointer */
static void ask_timezone(GtkButton *button, modify_struct *modify_clock)
{
    gchar *tz_name = NULL;
    gchar *clockname = NULL;
    char env_tz[256];

    /* first stop all clocks and reset time to local timezone because
     * timezone list needs to be shown in local timezone (details show time) */
    clocks.no_update = TRUE; 
    if (clocks.local_tz && clocks.local_tz->str && clocks.local_tz->len) {
        g_snprintf(env_tz, 256, "TZ=%s", clocks.local_tz->str);
        putenv(env_tz);
        tzset();
    }
    if (orage_timezone_button_clicked(button, GTK_WINDOW(modify_clock->window)
                , &tz_name, FALSE, NULL)) {
        if (modify_clock->clock) { /* individual, real clock */
            g_string_assign(modify_clock->clock->tz, tz_name);
            if (strlen(gtk_entry_get_text(GTK_ENTRY(modify_clock->name_entry))) 
                    == 0) {
                if ((clockname = strrchr(tz_name, (int)'/')))
                    gtk_entry_set_text(GTK_ENTRY(modify_clock->name_entry)
                            , clockname+1);
                else
                    gtk_entry_set_text(GTK_ENTRY(modify_clock->name_entry)
                            , tz_name);
            }
        }
        else { /* default timezone in main setup */
            g_string_assign(clocks.local_tz, tz_name);
        }
        g_free(tz_name);
    }
    clocks.no_update = FALSE; 
}

static void set_font(GtkWidget *widget, GString *font)
{
    g_string_assign(font
            , gtk_font_button_get_font_name((GtkFontButton *)widget));
    write_file();
}

static void reset_name_underline(GtkWidget *widget, clock_struct *clockp)
{
    clockp->clock_attr.name_underline_modified = !gtk_toggle_button_get_active(
            (GtkToggleButton *)widget);
    gtk_widget_set_sensitive(clockp->modify_data->combo_box_underline_name
            , clockp->clock_attr.name_underline_modified);
    write_file();
}

static void reset_time_underline(GtkWidget *widget, clock_struct *clockp)
{
    clockp->clock_attr.time_underline_modified = !gtk_toggle_button_get_active(
            (GtkToggleButton *)widget);
    gtk_widget_set_sensitive(clockp->modify_data->combo_box_underline_time
            , clockp->clock_attr.time_underline_modified);
    write_file();
}

static void set_color(GtkWidget *widget, GdkColor *color)
{
    gtk_color_button_get_color((GtkColorButton *)widget, color);
    write_file();
}

static void set_sensitivity(GtkWidget *widget, GtkWidget *button)
{
    gtk_widget_set_sensitive(button
            , !gtk_toggle_button_get_active((GtkToggleButton *)widget));
}

static void set_modified(GtkWidget *widget, gboolean *modified)
{
    *modified = !gtk_toggle_button_get_active((GtkToggleButton *)widget);
    write_file();
}

static void fill_combo_box(GtkComboBox *combo_box, char **source
        , gchar *cur_value)
{
    gint i;

    for (i = 0; strcmp(source[i], "END"); i++) {
        gtk_combo_box_append_text(combo_box, source[i]);
        if ((strcmp(cur_value, "NO VALUE") != 0)
        &&  ((i == 0) || (strcmp(cur_value, source[i]) == 0)))
            gtk_combo_box_set_active(combo_box, i);
    }
}

static gboolean underline_combo_box_changed(GtkWidget *widget, GString *uline)
{ 
    gint i;

    i = gtk_combo_box_get_active((GtkComboBox *)widget);
    g_string_assign(uline, attr_underline[i]);
    write_file();
    return(FALSE);
}

static gboolean underline_default_name_combo_box_changed(GtkWidget *widget
        , text_attr_struct *attr)
{ 
    gint i;

    i = gtk_combo_box_get_active((GtkComboBox *)widget);
    g_string_assign(attr->name_underline, attr_underline[i]);
    /* this is special since default = no underline, it does not come from
     * the system, but is hard coded and not selectable, so there is no
     * real reset handler, so we do it here. It is needed by the generic
     * parameter write code.
     */
    if (strcmp(attr->name_underline->str, attr_underline[0]))
        attr->name_underline_modified = TRUE;
    else
        attr->name_underline_modified = FALSE;

    write_file();
    return(FALSE);
}

static gboolean underline_default_time_combo_box_changed(GtkWidget *widget
        , text_attr_struct *attr)
{ 
    gint i;

    i = gtk_combo_box_get_active((GtkComboBox *)widget);
    g_string_assign(attr->time_underline, attr_underline[i]);
    /* this is special since default = no underline, it does not come from
     * the system, but is hard coded and not selectable, so there is no
     * real reset handler, so we do it here. It is needed by the generic
     * parameter write code.
     */
    if (strcmp(attr->time_underline->str, attr_underline[0]))
        attr->time_underline_modified = TRUE;
    else
        attr->time_underline_modified = FALSE;

    write_file();
    return(FALSE);
}

static void create_parameter_toolbar(GtkWidget *vbox
                , modify_struct *modify_clock)
{
    GtkWidget *toolbar;
    GtkWidget *button;

    toolbar = gtk_toolbar_new();
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

    /* -----------------------UPDATE-------------------------------------- */
    button = toolbar_append_button(toolbar, GTK_STOCK_SAVE, clocks.tips
            , _("update this clock")); 
    g_signal_connect(G_OBJECT(button), "clicked"
            , G_CALLBACK(save_clock), modify_clock);
    /* -----------------------INSERT-------------------------------------- */
    button = toolbar_append_button(toolbar, GTK_STOCK_NEW, clocks.tips
            , _("add new empty clock")); 
    g_signal_connect(G_OBJECT(button), "clicked"
            , G_CALLBACK(add_clock), modify_clock);
    /* -----------------------COPY---------------------------------------- */
    button = toolbar_append_button(toolbar, GTK_STOCK_COPY, clocks.tips
            , _("add new clock using this clock as model")); 
    g_signal_connect(G_OBJECT(button), "clicked"
            , G_CALLBACK(copy_clock), modify_clock);
    /* -----------------------DELETE-------------------------------------- */
    button = toolbar_append_button(toolbar, GTK_STOCK_DELETE, clocks.tips
            , _("delete this clock")); 
    g_signal_connect(G_OBJECT(button), "clicked"
            , G_CALLBACK(delete_clock), modify_clock);

    toolbar_append_separator(toolbar); 
    
    /* -----------------------MOVE FIRST---------------------------------- */
    button = toolbar_append_button(toolbar, GTK_STOCK_GOTO_FIRST, clocks.tips
            , _("move this clock first")); 
    g_signal_connect(G_OBJECT(button), "clicked"
            , G_CALLBACK(move_clock), modify_clock);
    /* -----------------------MOVE BACK----------------------------------- */
    button = toolbar_append_button(toolbar, GTK_STOCK_GO_BACK, clocks.tips
            , _("move this clock left")); 
    g_signal_connect(G_OBJECT(button), "clicked"
            , G_CALLBACK(move_clock), modify_clock);
    /* -----------------------MOVE FORWARD-------------------------------- */
    button = toolbar_append_button(toolbar, GTK_STOCK_GO_FORWARD, clocks.tips
            , _("move this clock right")); 
    g_signal_connect(G_OBJECT(button), "clicked"
            , G_CALLBACK(move_clock), modify_clock);
    /* -----------------------MOVE LAST----------------------------------- */
    button = toolbar_append_button(toolbar, GTK_STOCK_GOTO_LAST, clocks.tips
            , _("move this clock last")); 
    g_signal_connect(G_OBJECT(button), "clicked"
            , G_CALLBACK(move_clock), modify_clock);

    toolbar_append_separator(toolbar); 
    
    /* ---------------------QUIT------------------------------------------ */
    button = toolbar_append_button(toolbar, "gtk-media-record", clocks.tips
            , _("set the timezone of this clock to be local timezone"));
    g_signal_connect(G_OBJECT(button), "clicked"
            , G_CALLBACK(set_timezone_from_clock)
            , modify_clock);

    button = toolbar_append_button(toolbar, GTK_STOCK_QUIT, clocks.tips
            , _("close window and exit")); 
    g_signal_connect(G_OBJECT(button), "clicked"
            , G_CALLBACK(close_modify_clock_window)
            , modify_clock->window);
}

gboolean clock_parameters(GtkWidget *widget, clock_struct *clockp)
{
    modify_struct *modify_clock;
    GtkWidget *vbox, *hbox;
    gchar     *window_name;

    if (clockp->modified)
        return(FALSE); /* only one update per clock; safer ;o) */
    clockp->modified = TRUE;
    clocks.modified++;

/* -----------------------WINDOW + base vbox------------------------------ */
    modify_clock = g_new0(modify_struct, 1);
    modify_clock->clock = clockp;
    clockp->modify_data = modify_clock;
    modify_clock->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    window_name = g_strconcat(_("Globaltime preferences "), clockp->name->str, NULL);
    gtk_window_set_title(GTK_WINDOW(modify_clock->window), window_name);
    g_free(window_name);
    g_signal_connect(G_OBJECT(modify_clock->window) , "destroy"
            , G_CALLBACK(release_modify_clock_window), modify_clock);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(modify_clock->window), vbox);

/* ---------------------TOOLBAR--------------------------------------------- */
    create_parameter_toolbar(vbox, modify_clock);

/* -----------------------HEADING------------------------------------------- */
    add_header(vbox, _("Clock Parameters"), TRUE);

/* ---------------------CLOCK PARAMETERS------------------------------------ */
    hbox = add_box(vbox, 'H');
    add_header(hbox, _("Name of the clock:"), TRUE);
    modify_clock->name_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(modify_clock->name_entry), clockp->name->str);
    gtk_box_pack_start(GTK_BOX(hbox), modify_clock->name_entry
            , FALSE, FALSE, 5);
    gtk_tooltips_set_tip(clocks.tips, modify_clock->name_entry
            , _("enter name of clock"), NULL);

    hbox = add_box(vbox, 'H');
    add_header(hbox, _("Timezone of the clock:"), TRUE);
    modify_clock->button_tz = gtk_button_new_from_stock(GTK_STOCK_OPEN);
    if (clockp->tz->str && clockp->tz->len)
         gtk_button_set_label(GTK_BUTTON(modify_clock->button_tz)
                 , _(clockp->tz->str));
    gtk_box_pack_start(GTK_BOX(hbox), modify_clock->button_tz, FALSE, FALSE, 2);
    g_signal_connect(G_OBJECT(modify_clock->button_tz), "clicked"
            , G_CALLBACK(ask_timezone), modify_clock);

/* ---------------------Text Formatting--------------------------------- */
    create_parameter_formatting(vbox, modify_clock);

    gtk_widget_show_all(modify_clock->window);
    return(FALSE);
}

static void create_parameter_formatting(GtkWidget *vbox
        , modify_struct *modify_clock)
{
    clock_struct *clockp = modify_clock->clock;
    text_attr_struct *attrp = &modify_clock->clock->clock_attr;
    GdkColor *clock_fgp = modify_clock->clock->clock_attr.clock_fg;
    GdkColor *clock_bgp = modify_clock->clock->clock_attr.clock_bg;
    GtkWidget *table;
    GtkWidget *label;

    add_separator(vbox, 'H');
    add_header(vbox, _("Text Formatting"), TRUE);
    table = gtk_table_new(6, 3, FALSE); /* rows, columns, homogenous */
    /* table attach parameters: left, right, top, bottom */
    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 2);

    /*------------------------background-------------------------*/
    label = gtk_label_new(_("Background color:"));
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

    modify_clock->button_clock_bg = 
            gtk_color_button_new_with_color(clock_bgp);
    gtk_tooltips_set_tip(clocks.tips, modify_clock->button_clock_bg
            , _("Click to change background colour for clock"), NULL);
    g_signal_connect(G_OBJECT(modify_clock->button_clock_bg), "color-set"
            , G_CALLBACK(set_color), clock_bgp);
    gtk_widget_set_sensitive(modify_clock->button_clock_bg
            , attrp->clock_bg_modified);
    gtk_table_attach_defaults(GTK_TABLE(table)
             , modify_clock->button_clock_bg, 1, 2, 0, 1);

    modify_clock->check_button_default_bg = 
            gtk_check_button_new_with_label(_("Use default"));
    gtk_toggle_button_set_active(
            (GtkToggleButton *)modify_clock->check_button_default_bg
            , !attrp->clock_bg_modified);
    gtk_tooltips_set_tip(clocks.tips, modify_clock->check_button_default_bg
            , _("Cross this to use default instead of selected value"), NULL);
    g_signal_connect(G_OBJECT(modify_clock->check_button_default_bg)
            , "toggled", G_CALLBACK(set_modified)
            , &clockp->clock_attr.clock_bg_modified);
    g_signal_connect(G_OBJECT(modify_clock->check_button_default_bg)
            , "toggled", G_CALLBACK(set_sensitivity)
            , modify_clock->button_clock_bg);
    gtk_table_attach_defaults(GTK_TABLE(table)
            , modify_clock->check_button_default_bg, 2, 3, 0, 1);

    /*------------------------foreground-------------------------*/
    label = gtk_label_new(_("Foreground (=text) color:"));
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

    modify_clock->button_clock_fg = 
            gtk_color_button_new_with_color(clock_fgp);
    gtk_tooltips_set_tip(clocks.tips, modify_clock->button_clock_fg
            , _("Click to change foreground colour for clock"), NULL);
    g_signal_connect(G_OBJECT(modify_clock->button_clock_fg), "color-set"
            , G_CALLBACK(set_color), clock_fgp);
    gtk_widget_set_sensitive(modify_clock->button_clock_fg
            , attrp->clock_fg_modified);
    gtk_table_attach_defaults(GTK_TABLE(table)
            , modify_clock->button_clock_fg, 1, 2, 1, 2);

    modify_clock->check_button_default_fg = 
            gtk_check_button_new_with_label(_("Use default"));
    gtk_toggle_button_set_active(
            (GtkToggleButton *)modify_clock->check_button_default_fg
            , !attrp->clock_fg_modified);
    gtk_tooltips_set_tip(clocks.tips, modify_clock->check_button_default_fg
            , _("Cross this to use default instead of selected value"), NULL);
    g_signal_connect(G_OBJECT(modify_clock->check_button_default_fg)
            , "toggled", G_CALLBACK(set_modified)
            , &clockp->clock_attr.clock_fg_modified);
    g_signal_connect(G_OBJECT(modify_clock->check_button_default_fg)
            , "toggled", G_CALLBACK(set_sensitivity)
            , modify_clock->button_clock_fg);
    gtk_table_attach_defaults(GTK_TABLE(table)
            , modify_clock->check_button_default_fg, 2, 3, 1, 2);

    /*------------------------name font-------------------------*/
    label = gtk_label_new(_("Font for name of clock:"));
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 2, 3);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

    modify_clock->button_font_name = gtk_font_button_new_with_font(
            attrp->name_font->str);
    gtk_tooltips_set_tip(clocks.tips, modify_clock->button_font_name
            , _("Click to change font for clock name"), NULL);
    g_signal_connect(G_OBJECT(modify_clock->button_font_name), "font-set"
            , G_CALLBACK(set_font), attrp->name_font);
    gtk_widget_set_sensitive(modify_clock->button_font_name
            , attrp->name_font_modified);
    gtk_table_attach_defaults(GTK_TABLE(table)
            , modify_clock->button_font_name, 1, 2, 2, 3);

    modify_clock->check_button_default_font_name = 
            gtk_check_button_new_with_label(_("Use default"));
    gtk_toggle_button_set_active(
            (GtkToggleButton *)modify_clock->check_button_default_font_name
            , !attrp->name_font_modified);
    gtk_tooltips_set_tip(clocks.tips
            , modify_clock->check_button_default_font_name
            , _("Cross this to use default instead of selected value"), NULL);
    g_signal_connect(G_OBJECT(modify_clock->check_button_default_font_name)
            , "toggled", G_CALLBACK(set_modified)
            , &clockp->clock_attr.name_font_modified);
    g_signal_connect(G_OBJECT(modify_clock->check_button_default_font_name)
            , "toggled", G_CALLBACK(set_sensitivity)
            , modify_clock->button_font_name);
    gtk_table_attach_defaults(GTK_TABLE(table)
            , modify_clock->check_button_default_font_name, 2, 3, 2, 3);

    /*------------------------time font-------------------------*/
    label = gtk_label_new(_("Font for time of clock:"));
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 3, 4);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

    modify_clock->button_font_time = gtk_font_button_new_with_font(
            attrp->time_font->str);
    gtk_tooltips_set_tip(clocks.tips, modify_clock->button_font_time
            , _("Click to change font for clock time"), NULL);
    g_signal_connect(G_OBJECT(modify_clock->button_font_time), "font-set"
            , G_CALLBACK(set_font), attrp->time_font);
    gtk_widget_set_sensitive(modify_clock->button_font_time
            , attrp->time_font_modified);
    gtk_table_attach_defaults(GTK_TABLE(table)
            , modify_clock->button_font_time, 1, 2, 3, 4);

    modify_clock->check_button_default_font_time = 
            gtk_check_button_new_with_label(_("Use default"));
    gtk_toggle_button_set_active(
            (GtkToggleButton *)modify_clock->check_button_default_font_time
            , !attrp->time_font_modified);
    gtk_tooltips_set_tip(clocks.tips
            , modify_clock->check_button_default_font_time
            , _("Cross this to use default instead of selected value"), NULL);
    g_signal_connect(G_OBJECT(modify_clock->check_button_default_font_time)
            , "toggled", G_CALLBACK(set_modified)
            , &clockp->clock_attr.time_font_modified);
    g_signal_connect(G_OBJECT(modify_clock->check_button_default_font_time)
            , "toggled", G_CALLBACK(set_sensitivity)
            , modify_clock->button_font_time);
    gtk_table_attach_defaults(GTK_TABLE(table)
            , modify_clock->check_button_default_font_time, 2, 3, 3, 4);


    /*------------------------underline name--------------------*/
    label = gtk_label_new(_("Underline name of clock:"));
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 4, 5);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

    modify_clock->combo_box_underline_name = gtk_combo_box_new_text();
    fill_combo_box((GtkComboBox *)modify_clock->combo_box_underline_name
            , attr_underline, attrp->name_underline->str);
    g_signal_connect(G_OBJECT(modify_clock->combo_box_underline_name), "changed"
            , G_CALLBACK(underline_combo_box_changed)
            , attrp->name_underline);
    gtk_widget_set_sensitive(modify_clock->combo_box_underline_name
            , attrp->name_underline_modified);
    gtk_table_attach_defaults(GTK_TABLE(table)
            , modify_clock->combo_box_underline_name, 1, 2, 4, 5);

    modify_clock->check_button_default_underline_name = 
            gtk_check_button_new_with_label(_("Use default"));
    gtk_toggle_button_set_active(
            (GtkToggleButton *)modify_clock->check_button_default_underline_name
            , !attrp->name_underline_modified);
    gtk_tooltips_set_tip(clocks.tips
            , modify_clock->check_button_default_underline_name
            , _("Cross this to use default instead of selected value"), NULL);
    g_signal_connect(G_OBJECT(modify_clock->check_button_default_underline_name)
            , "toggled", G_CALLBACK(reset_name_underline), clockp);
    gtk_table_attach_defaults(GTK_TABLE(table)
            , modify_clock->check_button_default_underline_name, 2, 3, 4, 5);


    /*------------------------underline time--------------------*/
    label = gtk_label_new(_("Underline time of clock:"));
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 5, 6);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

    modify_clock->combo_box_underline_time = gtk_combo_box_new_text();
    fill_combo_box((GtkComboBox *)modify_clock->combo_box_underline_time
            , attr_underline, attrp->time_underline->str);
    g_signal_connect(G_OBJECT(modify_clock->combo_box_underline_time), "changed"
            , G_CALLBACK(underline_combo_box_changed)
            , attrp->time_underline);
    gtk_widget_set_sensitive(modify_clock->combo_box_underline_time
            , attrp->time_underline_modified);
    gtk_table_attach_defaults(GTK_TABLE(table)
            , modify_clock->combo_box_underline_time, 1, 2, 5, 6);

    modify_clock->check_button_default_underline_time = 
            gtk_check_button_new_with_label(_("Use default"));
    gtk_toggle_button_set_active(
        (GtkToggleButton *)modify_clock->check_button_default_underline_time
            , !attrp->time_underline_modified);
    gtk_tooltips_set_tip(clocks.tips
            , modify_clock->check_button_default_underline_time
            , _("Cross this to use default instead of selected value"), NULL);
    g_signal_connect(G_OBJECT(modify_clock->check_button_default_underline_time)
            , "toggled", G_CALLBACK(reset_time_underline), clockp);
    gtk_table_attach_defaults(GTK_TABLE(table)
            , modify_clock->check_button_default_underline_time, 2, 3, 5, 6);
}

static void preferences_formatting(GtkWidget *vbox
        , modify_struct *modify_default)
{

    text_attr_struct *attrp = &clocks.clock_default_attr;
    GdkColor *clock_fgp = clocks.clock_default_attr.clock_fg;
    GdkColor *clock_bgp = clocks.clock_default_attr.clock_bg;
    GtkWidget *table;
    GtkWidget *label;

    add_separator(vbox, 'H');
    add_header(vbox, _("Text Default Formatting"), TRUE);
    table = gtk_table_new(6, 3, FALSE); /* rows, columns, homogenous */
    /* table attach parameters: left, right, top, bottom */
    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 2);

    /*------------------------background-------------------------*/
    label = gtk_label_new(_("Background color:"));
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

    modify_default->button_clock_bg = 
            gtk_color_button_new_with_color(clock_bgp);
    gtk_tooltips_set_tip(clocks.tips, modify_default->button_clock_bg
            , _("Click to change default background colour for clocks"), NULL);
    g_signal_connect(G_OBJECT(modify_default->button_clock_bg), "color-set"
            , G_CALLBACK(set_color), clock_bgp);
    gtk_widget_set_sensitive(modify_default->button_clock_bg
            , attrp->clock_bg_modified);
    gtk_table_attach_defaults(GTK_TABLE(table)
            , modify_default->button_clock_bg, 1, 2, 0, 1);

    modify_default->check_button_default_bg =
            gtk_check_button_new_with_label(_("Use default"));
    gtk_toggle_button_set_active(
            (GtkToggleButton *)modify_default->check_button_default_bg
            , !attrp->clock_bg_modified);
    gtk_tooltips_set_tip(clocks.tips, modify_default->check_button_default_bg
            , _("Cross this to use system default instead of selected color")
            , NULL);
    g_signal_connect(G_OBJECT(modify_default->check_button_default_bg)
            , "toggled", G_CALLBACK(set_modified)
            , &clocks.clock_default_attr.clock_bg_modified);
    g_signal_connect(G_OBJECT(modify_default->check_button_default_bg)
            , "toggled", G_CALLBACK(set_sensitivity)
            , modify_default->button_clock_bg);
    gtk_table_attach_defaults(GTK_TABLE(table)
            , modify_default->check_button_default_bg, 2, 3, 0, 1);

    /*------------------------foreground-------------------------*/
    label = gtk_label_new(_("Foreground (=text) color:"));
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

    modify_default->button_clock_fg = 
            gtk_color_button_new_with_color(clock_fgp);
    gtk_tooltips_set_tip(clocks.tips, modify_default->button_clock_fg
            , _("Click to change default text colour for clocks"), NULL);
    g_signal_connect(G_OBJECT(modify_default->button_clock_fg), "color-set"
            , G_CALLBACK(set_color), clock_fgp);
    gtk_widget_set_sensitive(modify_default->button_clock_fg
            , attrp->clock_fg_modified);
    gtk_table_attach_defaults(GTK_TABLE(table)
            , modify_default->button_clock_fg, 1, 2, 1, 2);

    modify_default->check_button_default_fg =
            gtk_check_button_new_with_label(_("Use default"));
    gtk_toggle_button_set_active(
            (GtkToggleButton *)modify_default->check_button_default_fg
            , !attrp->clock_fg_modified);
    gtk_tooltips_set_tip(clocks.tips, modify_default->check_button_default_fg
            , _("Cross this to use system default instead of selected color")
            , NULL);
    g_signal_connect(G_OBJECT(modify_default->check_button_default_fg)
            , "toggled", G_CALLBACK(set_modified)
            , &clocks.clock_default_attr.clock_fg_modified);
    g_signal_connect(G_OBJECT(modify_default->check_button_default_fg)
            , "toggled", G_CALLBACK(set_sensitivity)
            , modify_default->button_clock_fg);
    gtk_table_attach_defaults(GTK_TABLE(table)
            , modify_default->check_button_default_fg, 2, 3, 1, 2);

    /*------------------------name font-------------------------*/
    label = gtk_label_new(_("Font for name of clock:"));
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 2, 3);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

    modify_default->button_font_name = 
            gtk_font_button_new_with_font(attrp->name_font->str);
    gtk_tooltips_set_tip(clocks.tips, modify_default->button_font_name
            , _("Click to change default font for clock name"), NULL);
    g_signal_connect(G_OBJECT(modify_default->button_font_name), "font-set"
            , G_CALLBACK(set_font), attrp->name_font);
    gtk_widget_set_sensitive(modify_default->button_font_name
            , attrp->name_font_modified);
    gtk_table_attach_defaults(GTK_TABLE(table)
            , modify_default->button_font_name, 1, 2, 2, 3);

    modify_default->check_button_default_font_name = 
            gtk_check_button_new_with_label(_("Use default"));
    gtk_toggle_button_set_active(
            (GtkToggleButton *)modify_default->check_button_default_font_name
            , !attrp->name_font_modified);
    gtk_tooltips_set_tip(clocks.tips
            , modify_default->check_button_default_font_name
            , _("Cross this to use system default font instead of selected font")
            , NULL);
    g_signal_connect(G_OBJECT(modify_default->check_button_default_font_name)
            , "toggled", G_CALLBACK(set_modified)
            , &clocks.clock_default_attr.name_font_modified);
    g_signal_connect(G_OBJECT(modify_default->check_button_default_font_name)
            , "toggled", G_CALLBACK(set_sensitivity)
            , modify_default->button_font_name);
    gtk_table_attach_defaults(GTK_TABLE(table)
            , modify_default->check_button_default_font_name, 2, 3, 2, 3);

    /*------------------------time font-------------------------*/
    label = gtk_label_new(_("Font for time of clock:"));
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 3, 4);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

    modify_default->button_font_time = 
            gtk_font_button_new_with_font(attrp->time_font->str);
    gtk_tooltips_set_tip(clocks.tips, modify_default->button_font_time
            , _("Click to change default font for clock time"), NULL);
    g_signal_connect(G_OBJECT(modify_default->button_font_time), "font-set"
            , G_CALLBACK(set_font), attrp->time_font);
    gtk_widget_set_sensitive(modify_default->button_font_time
            , attrp->time_font_modified);
    gtk_table_attach_defaults(GTK_TABLE(table)
            , modify_default->button_font_time, 1, 2, 3, 4);

    modify_default->check_button_default_font_time = 
            gtk_check_button_new_with_label(_("Use default"));
    gtk_toggle_button_set_active(
            (GtkToggleButton *)modify_default->check_button_default_font_time
            , !attrp->time_font_modified);
    gtk_tooltips_set_tip(clocks.tips
            , modify_default->check_button_default_font_time
            , _("Cross this to use system default font instead of selected font")
            , NULL);
    g_signal_connect(G_OBJECT(modify_default->check_button_default_font_time)
            , "toggled", G_CALLBACK(set_modified)
            , &clocks.clock_default_attr.time_font_modified);
    g_signal_connect(G_OBJECT(modify_default->check_button_default_font_time)
            , "toggled", G_CALLBACK(set_sensitivity)
            , modify_default->button_font_time);
    gtk_table_attach_defaults(GTK_TABLE(table)
            , modify_default->check_button_default_font_time, 2, 3, 3, 4);

    /*------------------------underline name--------------------*/
    label = gtk_label_new(_("Underline for name of clock:"));
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 4, 5);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

    modify_default->combo_box_underline_name = gtk_combo_box_new_text();
    fill_combo_box((GtkComboBox *)modify_default->combo_box_underline_name
            , attr_underline, attrp->name_underline->str);
    g_signal_connect(G_OBJECT(modify_default->combo_box_underline_name)
            , "changed"
            , G_CALLBACK(underline_default_name_combo_box_changed), attrp);
    gtk_table_attach_defaults(GTK_TABLE(table)
            , modify_default->combo_box_underline_name, 1, 3, 4, 5);

    /*------------------------underline time--------------------*/
    label = gtk_label_new(_("Underline for time of clock:"));
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 5, 6);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

    modify_default->combo_box_underline_time = gtk_combo_box_new_text();
    fill_combo_box((GtkComboBox *)modify_default->combo_box_underline_time
            , attr_underline, attrp->time_underline->str);
    g_signal_connect(G_OBJECT(modify_default->combo_box_underline_time)
            , "changed"
            , G_CALLBACK(underline_default_time_combo_box_changed), attrp);
    gtk_table_attach_defaults(GTK_TABLE(table)
            , modify_default->combo_box_underline_time, 1, 3, 5, 6);
}

static void preferences_toolbar(GtkWidget *vbox, modify_struct *modify_default)
{
    GtkWidget *toolbar;
    GtkWidget *button;

    toolbar = gtk_toolbar_new();
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);
    /* -----------------------UPDATE-------------------------------------- */
    button = toolbar_append_button(toolbar, GTK_STOCK_SAVE, clocks.tips
            , _("update preferences"));
    g_signal_connect(G_OBJECT(button), "clicked"
            , G_CALLBACK(save_preferences), modify_default);

    /* -----------------------INSERT-------------------------------------- */
    toolbar_append_separator(toolbar);
    button = toolbar_append_button(toolbar, GTK_STOCK_NEW, clocks.tips
            , _("add new empty clock")); 
    modify_default->clock=NULL; /* used in add_clock */
    g_signal_connect(G_OBJECT(button), "clicked"
            , G_CALLBACK(add_clock), modify_default);

    /* ---------------------QUIT------------------------------------------ */
    toolbar_append_separator(toolbar);
    button = toolbar_append_button(toolbar, GTK_STOCK_QUIT, clocks.tips
            , _("close window and exit"));
    g_signal_connect(G_OBJECT(button), "clicked"
            , G_CALLBACK(close_preferences_window), modify_default->window);
}

gboolean default_preferences(GtkWidget *widget)
{
    modify_struct *modify_default;
    GtkWidget *vbox, *hbox;
    GtkWidget *button;
    GSList *dec_group;

    if (clocks.modified > 0)
        return(FALSE); /* safer with only one update ;o) */
    clocks.modified++;

/* -----------------------WINDOW + base vbox------------------------------ */
    modify_default = g_new0(modify_struct, 1);
    modify_default->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW (modify_default->window)
            , _("Globaltime Preferences"));
    g_signal_connect(G_OBJECT(modify_default->window) , "destroy"
            , G_CALLBACK(release_preferences_window), modify_default);
                                                                                
    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(modify_default->window), vbox);

/* -----------------------TOOLBAR-------------------------------------- */
    preferences_toolbar(vbox, modify_default);

/* -----------------------HEADING-------------------------------------- */
    add_header(vbox, _("General Preferences"), TRUE);

/* -----------------------Decorations---------------------------------- */
    hbox = add_box(vbox, 'H');
    add_header(hbox, _("Decorations:"), TRUE);
    button = gtk_radio_button_new_with_label(NULL, _("Standard"));
    if (clocks.decorations)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
    gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
    gtk_tooltips_set_tip(clocks.tips, button
            , _("Use normal decorations"), NULL);
    g_signal_connect(G_OBJECT(button), "toggled"
            , G_CALLBACK(decoration_radio_button_pressed), "Standard");

    dec_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON (button));
    button = gtk_radio_button_new_with_label_from_widget(
            GTK_RADIO_BUTTON(button), "None");
    if (!clocks.decorations)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
    gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
    gtk_tooltips_set_tip(clocks.tips, button
            , _("Do not show window decorations (borders)"), NULL);
    g_signal_connect(G_OBJECT(button), "toggled"
            , G_CALLBACK(decoration_radio_button_pressed), "None");

/* -----------------------Clock Size---------------------------------- */
    hbox = add_box(vbox, 'H');
    add_header(hbox, _("Clock size:"), TRUE);
    button = gtk_radio_button_new_with_label(NULL, _("Equal"));
    if (clocks.expand)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
    gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
    gtk_tooltips_set_tip(clocks.tips, button
            , _("All clocks have same size"), NULL);
    g_signal_connect(G_OBJECT(button), "toggled"
            , G_CALLBACK(clocksize_radio_button_pressed), "Equal");

    dec_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON (button));
    button = gtk_radio_button_new_with_label(dec_group, _("Varying"));
    if (!clocks.expand)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
    gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
    gtk_tooltips_set_tip(clocks.tips, button
            , _("Clock sizes vary"), NULL);
    g_signal_connect(G_OBJECT(button), "toggled"
            , G_CALLBACK(clocksize_radio_button_pressed), "Vary");

/* -----------------------Local timezone------------------------------ */
    hbox = add_box(vbox, 'H');
    add_header(hbox, _("Local timezone:"), TRUE);
    modify_default->button_tz = gtk_button_new_from_stock(GTK_STOCK_OPEN);
    if (clocks.local_tz->str)
         gtk_button_set_label(GTK_BUTTON(modify_default->button_tz)
                 , _(clocks.local_tz->str));
    gtk_box_pack_start(GTK_BOX(hbox), modify_default->button_tz, FALSE, FALSE
            , 2);
    g_signal_connect(G_OBJECT(modify_default->button_tz), "clicked"
            , G_CALLBACK(ask_timezone), modify_default);

/* ---------------------Text Formatting--------------------------------- */
    preferences_formatting(vbox, modify_default);
                                                                                
    gtk_widget_show_all(modify_default->window);
    return(FALSE);
}

