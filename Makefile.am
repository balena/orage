SUBDIRS = 								\
	icons								\
	po								\
	src								\
	plugin

desktopdir = $(datadir)/applications
desktop_in_files = xfcalendar.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = 								\
	intltool-extract.in						\
	intltool-merge.in						\
	intltool-update.in						\
	xfcalendar.glade						\
	xfcalendar.gladep						\
	xfcalendar.spec							\
	$(desktop_DATA)

DISTCLEANFILES =							\
	intltool-extract						\
	intltool-merge							\
	intltool-update							\
	$(desktop_DATA)

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.gz

dist-bz2: dist
	zcat $(PACKAGE)-$(VERSION).tar.gz | \
	 bzip2 --best -c > $(PACKAGE)-$(VERSION).tar.bz2

distcheck-bz2: distcheck
	zcat $(PACKAGE)-$(VERSION).tar.gz | \
	 bzip2 --best -c > $(PACKAGE)-$(VERSION).tar.bz2

distclean-local:
	rm -rf *.cache *~ *.spec


