bin_PROGRAMS = orage

orage_SOURCES =					\
	$(orage_dbus_sources)				\
	about-xfcalendar.c					\
	about-xfcalendar.h					\
	appointment.c						\
	appointment.h						\
	day-view.c							\
	day-view.h							\
	event-list.c						\
	event-list.h						\
	functions.c							\
	functions.h							\
	ical-archive.c						\
	ical-code.c							\
	ical-code.h							\
	ical-internal.h						\
	ical-expimp.c						\
	interface.c							\
	interface.h							\
	main.c								\
	mainbox.c							\
	mainbox.h							\
	orage-i18n.h						\
	parameters.c						\
	parameters.h						\
	parameters_internal.h				\
	reminder.c							\
	reminder.h							\
	timezone_names.c					\
	timezone_selection.c				\
	timezone_selection.h				\
	tray_icon.c							\
	tray_icon.h							\
	tz_zoneinfo_read.c					\
	tz_zoneinfo_read.h

orage_CFLAGS =							\
    $(LIBGTK_CFLAGS)                    \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\"	\
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\"

orage_LDADD =							\
    $(LIBGTK_LIBS)                      \
	-lX11            					\
	-lm              					\
	$(INTLLIBS)

if INCLUDED_LIBICAL
orage_CFLAGS +=							\
	-I$(top_builddir)/libical/src/libical				\
	-I$(top_builddir)/libical/src/libicalss				\
	-I$(top_srcdir)/libical/src/libical					\
	-I$(top_srcdir)/libical/src/libicalss

orage_LDADD +=							\
	$(PTHREAD_LIBS)										\
	$(top_builddir)/libical/src/libical/libical.la		\
	$(top_builddir)/libical/src/libicalss/libicalss.la
else
orage_CFLAGS +=							\
	$(LIBICAL_CFLAGS)

orage_LDADD +=							\
	$(LIBICAL_LIBS)
endif

if HAVE_DBUS
orage_dbus_sources =						\
	orage-dbus-object.c						\
	orage-dbus-object.h						\
	orage-dbus-client.c						\
	orage-dbus-client.h						\
	orage-dbus.h							\
	orage-dbus-service.xml					\
	orage-dbus-service.h

orage_CFLAGS +=							\
	-DDBUS_API_SUBJECT_TO_CHANGE			\
	$(DBUS_CFLAGS)

orage_LDADD +=							\
	$(DBUS_LIBS)

orage-dbus-server.h: $(srcdir)/orage-dbus-service.xml Makefile
	dbus-binding-tool --prefix=orage --mode=glib-server $(srcdir)/orage-dbus-service.xml > orage-dbus-service.h
endif

if HAVE_NOTIFY
orage_LDADD +=							\
	$(NOTIFY_LIBS)
endif

# vi:set ts=8 sw=8 noet ai:
