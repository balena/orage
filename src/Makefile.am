bin_PROGRAMS = xfcalendar

xfcalendar_SOURCES =							\
	about-xfcalendar.c						\
	about-xfcalendar.h						\
	appointment.c							\
	appointment.h							\
	event-list.c							\
	event-list.h							\
    functions.c                             \
    functions.h                             \
	ical-code.c							\
	ical-code.h							\
	main.c								\
	mainbox.c                                                       \
	mainbox.h                                                       \
	reminder.c							\
	reminder.h							\
	tray_icon.c                                                     \
	tray_icon.h                                                     \
	xfce_trayicon.c							\
	xfce_trayicon.h

xfcalendar_CFLAGS =							\
	$(LIBXFCEGUI4_CFLAGS)						\
	$(ICAL_CFLAGS)							\
	$(LIBXFCE4MCS_CLIENT_CFLAGS)					\
	-DPACKAGE_DATA_DIR=\""$(datadir)"\"				\
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\"

xfcalendar_LDADD =							\
	$(LIBXFCEGUI4_LIBS)						\
	$(ICAL_LIBS)							\
	$(LIBXFCE4MCS_CLIENT_LIBS)					\
	$(INTLLIBS)

if INCLUDED_LIBICAL
xfcalendar_CFLAGS +=							\
	-I$(top_builddir)/libical/src/libical				\
	-I$(top_builddir)/libical/src/libicalss				\
	-I$(top_srcdir)/libical/src/libical				\
	-I$(top_srcdir)/libical/src/libicalss

xfcalendar_LDADD +=							\
	$(PTHREAD_LIBS)							\
	$(top_builddir)/libical/src/libical/libical.la			\
	$(top_builddir)/libical/src/libicalss/libicalss.la
endif

# vi:set ts=8 sw=8 noet ai:
