bin_PROGRAMS = globaltime

man_MANS =                             		 \
	globaltime.1

if MAINTAINER_MODE
%.1: %.xml
	xsltproc -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<
endif

desktopdir = $(datadir)/applications
desktop_in_files = globaltime.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

globaltime_SOURCES =						\
	globaltime.c							\
	globaltime.h							\
	gt_parfile.c							\
	gt_prefs.c								\
	timezone_selection.c                    \
	timezone_selection.h                    \
	tz_zoneinfo_read.c                      \
	tz_zoneinfo_read.h


globaltime_CFLAGS =							\
	$(LIBXFCEGUI4_CFLAGS)					\
	-DPACKAGE_DATA_DIR=\""$(datadir)"\"		\
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\"

globaltime_LDADD =							\
	$(LIBXFCEGUI4_LIBS)						\
	$(LIBXFCEUTIL_LIBS)						\
	$(INTLLIBS) 

EXTRA_DIST =								\
	$(man_MANS)								\
	$(desktop_in_files)						\
	globaltime.xml


# vi:set ts=8 sw=8 noet ai:
