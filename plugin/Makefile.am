xfcalendar_plugindir = @XFCE_MCS_MANAGER_PLUGINSDIR@

xfcalendar_plugin_LTLIBRARIES = xfcalendar_plugin.la

xfcalendar_plugin_la_LDFLAGS =						\
	-avoid-version							\
	-export-dynamic							\
	-module

xfcalendar_plugin_la_SOURCES =						\
	xfcalendar_plugin.c						\
	calendar-icon.h

xfcalendar_plugin_la_CFLAGS = 						\
	@XFCE_MCS_MANAGER_CFLAGS@ 					\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	-DSYSCONFDIR=\"$(sysconfdir)\"					\
	-DDATADIR=\"$(pkgdatadir)\" 

xfcalendar_plugin_la_LIBADD = 						\
	@XFCE_MCS_MANAGER_LIBS@
