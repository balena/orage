bin_PROGRAMS = xfcalendar

xfcalendar_SOURCES =							\
	xfcalendar-icon-inline.h					\
	about-xfcalendar.c						\
	about-xfcalendar.h						\
	callbacks.c							\
	callbacks.h							\
	interface.c							\
	interface.h							\
	main.c								\
	reminder.h							\
	reminder.c							\
	support.c							\
	support.h							\
	xfce_trayicon.c							\
	xfce_trayicon.h

xfcalendar_CFLAGS =							\
	@LIBXFCEGUI4_CFLAGS@						\
	@DBH_CFLAGS@							\
	@LIBXFCE4MCS_CLIENT_CFLAGS@					\
	-DPACKAGE_DATA_DIR=\""$(datadir)"\"				\
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\"


xfcalendar_LDADD =							\
	@LIBXFCEGUI4_LIBS@						\
	@DBH_LIBS@							\
	@LIBXFCE4MCS_CLIENT_LIBS@
	$(INTLLIBS)

xfcalendar-icon-inline.h: Makefile $(srcdir)/xfcalendar.png
	gdk-pixbuf-csource --raw --build-list				\
	xfcalendar_icon $(srcdir)/xfcalendar.png > xfcalendar-icon-inline.h

noinst_DATA =								\
	xfcalendar-icon-inline.h

EXTRA_DIST =								\
	xfcalendar-icon-inline.h					\
	xfcalendar.png
