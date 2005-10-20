if INCLUDED_LIBICAL
LIBICAL_SUBDIR=								\
	libical
endif

SUBDIRS = 								\
	$(LIBICAL_SUBDIR)						\
	doc								\
	icons								\
	po								\
	sounds								\
	src								\
	plugin

desktopdir = $(datadir)/applications
desktop_in_files = xfcalendar.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = mkinstalldirs  								\
	intltool-extract.in						\
	intltool-merge.in						\
	intltool-update.in						\
	xfcalendar.spec							\
	$(desktop_in_files)

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

html: Makefile
	make -C doc html

ACLOCAL_AMFLAGS = -I m4
