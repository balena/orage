xfcalendar_plugindir = 							\
	$(libdir)/xfce4/mcs-plugins

xfcalendar_plugin_LTLIBRARIES = xfcalendar_plugin.la

xfcalendar_plugin_la_LDFLAGS =						\
	-avoid-version							\
	-module

xfcalendar_plugin_la_SOURCES =						\
	xfcalendar_plugin.c

xfcalendar_plugin_la_CFLAGS = 						\
	$(XFCE_MCS_MANAGER_CFLAGS) 					\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	-DSYSCONFDIR=\"$(sysconfdir)\"					\
	-DDATADIR=\"$(pkgdatadir)\" 

desktopdir = $(datadir)/applications
desktop_in_files = xfce-xfcalendar-settings.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = 								\
	$(desktop_DATA)

DISTCLEANFILES =							\
	$(desktop_DATA)

