
zoneinfodatadir = $(datadir)/$(PACKAGE)/zoneinfo

zoneinfodata_DATA = zones.tab \
					tz_convert.par

DIRS = \
	.			\
	Africa			\
	America			\
	America/Argentina		\
	America/Indiana			\
	America/Kentucky		\
	America/North_Dakota	\
	Antarctica		\
	Arctic			\
	Asia			\
	Atlantic		\
	Australia		\
	Brazil			\
	Canada			\
	Chile			\
	Etc				\
	Europe			\
	Indian			\
	Mexico			\
	Mideast			\
	Pacific			\
	SystemV			\
	US


install-data-local:
	@$(NORMAL_INSTALL)
	for dir in $(DIRS); do \
	  $(mkinstalldirs) $(DESTDIR)$(datadir)/$(PACKAGE)/zoneinfo/$$dir; \
	  for tzfile in $(srcdir)/$$dir/*.ics; do \
	    $(INSTALL_DATA) $$tzfile $(DESTDIR)$(datadir)/$(PACKAGE)/zoneinfo/$$dir; \
	  done \
	done

uninstall-local:
	@$(NORMAL_UNINSTALL)
	for dir in $(DIRS); do \
	  rm -fr $(DESTDIR)$(datadir)/$(PACKAGE)/zoneinfo/$$dir; \
	done

dist-hook:
	for dir in $(DIRS); do \
	  mkdir $(distdir)/$$dir; \
	  for tzfile in $(srcdir)/$$dir/*.ics; do \
	    cp -p $$tzfile $(distdir)/$$dir; \
	  done \
	done

EXTRA_DIST = $(zoneinfodata_DATA)
