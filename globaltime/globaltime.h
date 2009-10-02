typedef struct
{ /* clock name and time font settings */
    GdkColor *clock_fg;
    gboolean clock_fg_modified;
    GdkColor *clock_bg;
    gboolean clock_bg_modified;
    GString  *name_font;
    gboolean name_font_modified;
    GString  *name_underline;
    gboolean name_underline_modified;
    GString  *time_font;
    gboolean time_font_modified;
    GString  *time_underline;
    gboolean time_underline_modified;
} text_attr_struct;

typedef struct
{ /* contains data for one clock */
    GString *tz;
    GString *name;
    gboolean modified;
    GtkWidget *clock_hbox; /* contains clock_separator + clock_ebox */
    GtkWidget *clock_separator;
    GtkWidget *clock_ebox; /* contains vbox so that we can use events */
    GtkWidget *clock_vbox; /* contains name + time */
    GtkWidget *name_label;
    GtkWidget *time_label;
    text_attr_struct clock_attr;
    struct modify *modify_data; /* either null or addr of mod struct */
} clock_struct;

typedef struct
{ /* all clocks */
    GList *clock_list;      /* list of clock_structs */
    gchar time_now[8];      /* 88:88+ null terminated */
    gint previous_secs;
    time_t previous_t;
    gboolean time_adj_act;  /* manual time adjustment active or not */
    gboolean no_update;     /* do not update clocks */
    gint hh_adj;            /* adjustment hours */
    gint mm_adj;            /* adjustment hours */
    GString *local_tz;      /* local timezone. Used to set local_mday */
    gint local_mday;        /* local day of month. Used to set +/- to clock */
    gint x, y;              /* position */
    gint modified;          /* any clock or default being modified */
    gboolean decorations;   /* TRUE=Standard FALSE=None */
    gboolean expand;        /* TRUE=use even sized clocks FALSE=minimum size */
    GtkWidget *window;
    GtkWidget *main_hbox;   /* contains hdr_hbox + clocks_hbox */
    GtkWidget *hdr_hbox;    /* contains hdr_button + hdr_adj */
    GtkWidget *hdr_button;
    GtkWidget *hdr_adj_hh;  /* adjust hours spin */
    GtkWidget *hdr_adj_sep; /* separator : */
    GtkWidget *hdr_adj_mm;  /* adjust minutes spin */
    GtkWidget *clocks_hbox; /* contains clocks = clock_hbox list */
    GtkWidget *hidden;      /* used for interaction with GDK */
    GtkTooltips *tips;
    text_attr_struct clock_default_attr;
} global_times_struct;

/*
+---------------------------------main_hbox-----------------------------------+
|                                                                             |
| +---+ +--------------clocks_hbox------------------------------------------+ |
| |hdr| |                                                                   | |
| |   | | +-----clock_hbox-----------------+                                | |
| |but| | | (clock_separator) (clock_vbox) | ...                            | |
| |ton| | +--------------------------------+                                | |
| |   | |                                                                   | |
| +---+ +-------------------------------------------------------------------+ |
|                                                                             |
+-----------------------------------------------------------------------------+
*/


void init_attr(text_attr_struct *attr);
void write_file(void);
void read_file(void);
void show_clock(clock_struct *clockp, gint *pos);
gboolean default_preferences(GtkWidget *widget);
gboolean clock_parameters(GtkWidget *widget, clock_struct *clockp);

