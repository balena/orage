SUBDIRS = 								\
	icons								\
	po								\
	src								\
	plugin

distclean-local:
	rm -rf *.cache *~ *.spec

desktopdir = $(datadir)/applications
desktop_DATA = xfcalendar.desktop

EXTRA_DIST = 								\
	AUTHORS								\
	COPYING								\
	ChangeLog							\
	INSTALL								\
	NEWS								\
	README								\
	xfcalendar.glade						\
	xfcalendar.gladep						\
	xfcalendar.spec							\
	$(desktop_DATA)

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.gz

ACLOCAL_AMFLAGS = -I m4

dist-bz2: dist
	zcat $(PACKAGE)-$(VERSION).tar.gz | bzip2 --best -c > $(PACKAGE)-$(VERSION).tar.bz2

distcheck-bz2: distcheck
	zcat $(PACKAGE)-$(VERSION).tar.gz | bzip2 --best -c > $(PACKAGE)-$(VERSION)$.tar.bz2
