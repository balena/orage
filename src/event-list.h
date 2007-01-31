/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2005-2007 Juha Kautto  (juha at xfce.org)
 * Copyright (c) 2004-2005 Mickael Graf (korbinus at xfce.org)
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

#ifndef __EVENT_LIST_H__
#define __EVENT_LIST_H__

typedef enum
{
    EVENT_PAGE = 0
   ,TODO_PAGE
   ,JOURNAL_PAGE
   ,SEARCH_PAGE
} el_page;


typedef struct
{
    GtkAccelGroup *accel_group;
    GtkTooltips   *Tooltips;
    GtkWidget *Window;
    GtkWidget *Vbox;

    GtkWidget *Menubar;
    GtkWidget *File_menu;
    GtkWidget *File_menu_new;
    GtkWidget *File_menu_duplicate;
    GtkWidget *File_menu_delete;
    GtkWidget *File_menu_close;
    GtkWidget *View_menu;
    GtkWidget *View_menu_refresh;
    GtkWidget *View_menu_search;
    GtkWidget *Go_menu;
    GtkWidget *Go_menu_today;
    GtkWidget *Go_menu_prev;
    GtkWidget *Go_menu_next;

    GtkWidget *Toolbar;
    GtkWidget *Create_toolbutton;
    GtkWidget *Copy_toolbutton;
    GtkWidget *Delete_toolbutton;
    GtkWidget *Previous_toolbutton;
    GtkWidget *Today_toolbutton;
    GtkWidget *Next_toolbutton;
    GtkWidget *Refresh_toolbutton;
    GtkWidget *Search_toolbutton;
    GtkWidget *Close_toolbutton;

    GtkWidget *Notebook;
    GtkWidget *event_tab_label;
    GtkWidget *event_notebook_page;
    GtkWidget *event_spin;
    GtkWidget *todo_tab_label;
    GtkWidget *todo_notebook_page;
    GtkWidget *journal_tab_label;
    GtkWidget *journal_notebook_page;
    GtkWidget *journal_start_button;
    GtkWidget *search_tab_label;
    GtkWidget *search_notebook_page;
    GtkWidget *search_entry;

    GtkWidget   *ScrolledWindow;
    GtkWidget   *TreeView;
    GtkTreeSelection *TreeSelection;
    GtkListStore     *ListStore;
    GtkTreeSortable  *TreeSortable;

    /* these are used to build the data into event list */
    gboolean today;       /* flag: today or not */
    int      days;        /* how many extra days to show; usually 0 */
    el_page  page;        /* appointment page to show */
    char     time_now[5]; /* hh:mm */
} el_win; /* event list window */

el_win* create_el_win(void);
void refresh_el_win(el_win *el);

#endif /* !__EVENT_LIST_H__ */
