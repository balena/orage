SUBDIRS = 								\
	po								\
	src								\
	plugin

distclean-local:
	rm -rf *.cache *~ *.spec

EXTRA_DIST = 								\
	AUTHORS								\
	COPYING								\
	ChangeLog							\
	INSTALL								\
	NEWS								\
	README								\
	xfcalendar.glade						\
	xfcalendar.gladep						\
	xfcalendar.spec

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.gz

ACLOCAL_AMFLAGS = -I m4
