bin_PROGRAMS = xfcalendar

xfcalendar_SOURCES =							\
	calendar-icon.h							\
	callbacks.c							\
	callbacks.h							\
	interface.c							\
	interface.h							\
	main.c								\
	support.c							\
	support.h							\
	xfce_trayicon.c							\
	xfce_trayicon.h

xfcalendar_CFLAGS =							\
	@LIBXFCEGUI4_CFLAGS@						\
	@DBH_CFLAGS@							\
	-DPACKAGE_DATA_DIR=\""$(datadir)"\"				\
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\"


xfcalendar_LDADD =							\
	@LIBXFCEGUI4_LIBS@						\
	@DBH_LIBS@							\
	$(INTLLIBS)

calendar-icon.h: $(srcdir)/xfcalendar.png
	gdk-pixbuf-csource --raw --build-list				\
	calendar_icon_data $(srcdir)/xfcalendar.png > calendar-icon.h

noinst_DATA =								\
	calendar-icon.h

EXTRA_DIST =								\
	xfcalendar.png
