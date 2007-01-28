bin_PROGRAMS = globaltime

globaltime_SOURCES =						\
	globaltime.c							\
	globaltime.h							\
	gt_parfile.c							\
	gt_prefs.c

globaltime_CFLAGS =						\
	$(LIBXFCEGUI4_CFLAGS)						\
	-DPACKAGE_DATA_DIR=\""$(datadir)"\"				\
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\"

globaltime_LDADD =						\
	$(LIBXFCEGUI4_LIBS)						\
	$(LIBXFCEUTIL_LIBS)						\
	$(INTLLIBS)


# vi:set ts=8 sw=8 noet ai:
