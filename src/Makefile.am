bin_PROGRAMS = xfcalendar

xfcalendar_SOURCES =							\
	callbacks.c							\
	callbacks.h							\
	interface.c							\
	interface.h							\
	main.c								\
	support.c							\
	support.h

xfcalendar_CFLAGS =							\
	@LIBXFCEGUI4_CFLAGS@						\
	@DBH_CFLAGS@							\
	-DPACKAGE_DATA_DIR=\""$(datadir)"\"				\
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\"


xfcalendar_LDADD =							\
	@LIBXFCEGUI4_LIBS@						\
	@DBH_LIBS@							\
	$(INTLLIBS)

