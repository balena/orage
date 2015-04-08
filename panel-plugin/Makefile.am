if HAVE_LIBXFCE4PANEL
plugin_LTLIBRARIES = orageclock.la
endif

plugindir = $(libdir)/xfce4/panel/plugins

orageclock_la_CFLAGS =					\
	-DPACKAGE_DATA_DIR=\"$(datadir)\"				\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"			\
	@LIBXFCE4PANEL_CFLAGS@

orageclock_la_LDFLAGS =					\
	-avoid-version							\
	-module									\
	-no-undefined 							\
	-export-symbols-regex '^xfce_panel_module_(preinit|init|construct)' \
	$(PLATFORM_LDFLAGS)

orageclock_la_LIBADD =					\
	@LIBXFCE4PANEL_LIBS@                            \
	-lX11

orageclock_la_SOURCES = 				\
	oc_config.c								\
	timezone_selection.c					\
	timezone_selection.h					\
	../src/tz_zoneinfo_read.c				\
	../src/tz_zoneinfo_read.h				\
	xfce4-orageclock-plugin.c				\
	xfce4-orageclock-plugin.h               \
	../src/functions.c						\
	../src/functions.h

if HAVE_CYGWIN
orageclock_la_LDFLAGS +=                 \
	-no-undefined
endif

# .desktop file
#
desktop_in_files = xfce4-orageclock-plugin.desktop.in
	
desktopdir = $(datadir)/xfce4/panel/plugins
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = $(desktop_in_files)

DISTCLEANFILES = $(desktop_DATA)
