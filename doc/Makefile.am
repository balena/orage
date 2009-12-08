SUBDIRS = \
	C 

noinst_DATA = \
	xfce-nochunk.xsl

man_MANS = \
	orage.1

if MAINTAINER_MODE
orage.1: orage.xml
	xsltproc -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<
endif

EXTRA_DIST = \
	$(noinst_DATA) \
	$(man_MANS) \
	orage.xml

html: Makefile
	make -C C html
