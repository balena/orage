/* functions.c
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

#include <stdio.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <libxfce4util/libxfce4util.h>

/**************************************
 *  Functions for drawing interfaces  *
 **************************************/

GtkWidget *xfcalendar_toolbar_append_button (GtkWidget *toolbar, const gchar *stock_id, 
                                             GtkTooltips *tooltips, const char *tooltip_text, 
                                             gint pos){
    GtkWidget *button;

    button = (GtkWidget *) gtk_tool_button_new_from_stock (stock_id);
    gtk_tool_item_set_tooltip (GTK_TOOL_ITEM (button), tooltips, (const gchar *) tooltip_text, NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), pos);
    return button;
}

GtkWidget *xfcalendar_toolbar_append_separator (GtkWidget *toolbar, gint pos){
    GtkWidget *separator;

    separator = (GtkWidget *)gtk_separator_tool_item_new();
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(separator), pos);

    return separator;
}

void xfcalendar_combo_box_append_array (GtkWidget *combo_box, char *text[], int size){
    register int i;
    for(i = 0; i < size; i++){
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo_box),
                (const gchar *)text[i]);
    }
}

GtkWidget *xfcalendar_datetime_hbox_new (GtkWidget *date_button, GtkWidget *time_comboboxentry){

    GtkWidget *hbox, *space_label, *fixed;
    char *hours[48];
    register int i;

    for(i = 0; i < 48 ; i++){
        hours[i] = (char *)calloc(6, sizeof(gchar));
        sprintf(hours[i], "%02d:%02d", (int)(i/2), (i%2)*30);
    }

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), date_button, TRUE, TRUE, 0);

    space_label = gtk_label_new (_("  "));
    gtk_box_pack_start (GTK_BOX (hbox), space_label, FALSE, FALSE, 0);
    gtk_misc_set_alignment (GTK_MISC (space_label), 0.5, 0.43);

    xfcalendar_combo_box_append_array(time_comboboxentry, hours, 48);
    gtk_box_pack_start (GTK_BOX (hbox), time_comboboxentry, FALSE, TRUE, 0);

    fixed = gtk_fixed_new ();
    gtk_box_pack_start (GTK_BOX (hbox), fixed, TRUE, TRUE, 0);

    for( i = 0; i < 48; i++){
        free(hours[i]);
    }

    return hbox;
}

GtkWidget *xfcalendar_table_new (guint rows, guint columns){
    GtkWidget *table;

    table = gtk_table_new (rows, columns, FALSE);
    gtk_container_set_border_width (GTK_CONTAINER (table), 10);
    gtk_table_set_row_spacings (GTK_TABLE (table), 6);
    gtk_table_set_col_spacings (GTK_TABLE (table), 6);
    return table;
}

void xfcalendar_table_add_row (GtkWidget *table, GtkWidget *label, GtkWidget *input, guint row,
                               GtkAttachOptions input_x_option, GtkAttachOptions input_y_option){
    if (label){
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row+1,
                          (GtkAttachOptions) (GTK_FILL),
                          (GtkAttachOptions) (0), 0, 0);
        gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    }

    if (input){
        gtk_table_attach (GTK_TABLE (table), input, 1, 2, row, row+1,
                          input_x_option, input_y_option, 0, 0);
    }
}

GtkWidget *xfcalendar_menu_new (const gchar *menu_header_title, GtkWidget *menu_bar){
    GtkWidget *menu_header,
            *menu;

    menu_header = gtk_menu_item_new_with_mnemonic (menu_header_title);
    gtk_container_add (GTK_CONTAINER (menu_bar), menu_header);

    menu = gtk_menu_new ();
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_header), menu);

    return menu;
}

GtkWidget *xfcalendar_image_menu_item_new_from_stock (const gchar *stock_id, GtkWidget *menu, GtkAccelGroup *ag){
    GtkWidget *menu_item;

    menu_item = gtk_image_menu_item_new_from_stock(stock_id, ag);
    gtk_container_add (GTK_CONTAINER (menu), menu_item);
    return menu_item;
}

GtkWidget *xfcalendar_separator_menu_item_new (GtkWidget *menu){
    GtkWidget *menu_item;

    menu_item = gtk_separator_menu_item_new();
    gtk_container_add (GTK_CONTAINER (menu), menu_item);
    return menu_item;
}

GtkWidget *xfcalendar_menu_item_new_with_mnemonic (const gchar *label, GtkWidget *menu){
    GtkWidget *menu_item;

    menu_item = gtk_menu_item_new_with_mnemonic (label);
    gtk_container_add (GTK_CONTAINER (menu), menu_item);
    return menu_item;
}
