orage_plugindir = 							\
	$(libdir)/xfce4/mcs-plugins

orage_plugin_LTLIBRARIES = orage_plugin.la

orage_plugin_la_LDFLAGS =					\
	-avoid-version							\
	-module

orage_plugin_la_SOURCES =					\
	xfcalendar_plugin.c

orage_plugin_la_CFLAGS = 					\
	$(XFCE_MCS_MANAGER_CFLAGS) 				\
	-DBINDIR=\"$(bindir)\"	\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"	\
	-DSYSCONFDIR=\"$(sysconfdir)\"			\
	-DDATADIR=\"$(pkgdatadir)\" 

desktopdir = $(datadir)/applications
desktop_in_files = xfce-xfcalendar-settings.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = 								\
	$(desktop_in_files)

DISTCLEANFILES =							\
	$(desktop_DATA)

