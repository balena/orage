bin_PROGRAMS = tz_convert

man_MANS =                             		 \
	tz_convert.1

if MAINTAINER_MODE
%.1: %.xml
	xsltproc -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<
endif

tz_convert_SOURCES =						\
	tz_convert.c

tz_convert_CFLAGS =						\
	-DPACKAGE_DATA_DIR=\""$(datadir)"\"				\
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\"

tz_convert_LDFLAGS =                        \
	-lpopt


tz_convert_LDADD =						\
	$(INTLLIBS)

EXTRA_DIST =							\
	$(man_MANS)								\
	tz_convert.xml


# vi:set ts=8 sw=8 noet ai:
