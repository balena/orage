SUBDIRS = \
	images

noinst_DATA = \
	orage.xml

docdir=$(datadir)/orage/doc/C

doc_DATA= \
	orage.html

EXTRA_DIST = $(noinst_DATA) $(doc_DATA)

html: $(srcdir)/orage.xml
	xsltproc $(top_srcdir)/doc/xfce-nochunk.xsl \
		$(srcdir)/orage.xml
