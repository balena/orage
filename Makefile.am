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
