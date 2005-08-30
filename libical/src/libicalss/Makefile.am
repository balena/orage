#======================================================================
#  FILE: Makefile.am
#  CREATOR: eric 
#  
#  $Id$
#
#
# (C) COPYRIGHT 2000, Eric Busboom, http://www.softwarestudio.org
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of either: 
#
#    The LGPL as published by the Free Software Foundation, version
#    2.1, available at: http://www.fsf.org/copyleft/lesser.html
#
#  Or:
#
#    The Mozilla Public License Version 1.0. You may obtain a copy of
#    the License at http://www.mozilla.org/MPL/
#
#  
#
#======================================================================


AM_YFLAGS =-d -v -p ss
AM_LFLAGS = -Pss 
LEX_OUTPUT_ROOT = lex.ss

all: icalss.h

# just to get it built
$(srcdir)/icalgauge.c: icalssyacc.h
$(srcdir)/icalsslexer.c: icalssyacc.h 


noinst_LTLIBRARIES = libicalss.la

INCLUDES =			\
	-I$(top_srcdir)/libical/src	\
	-I$(top_builddir)/libical/src	\
	-I$(top_srcdir)/libical/src/libical	\
	-I$(top_builddir)/libical/src/libical

libicalss_la_LDFLAGS = -no-undefined

libicalss_la_DEVSOURCES = icalsslexer.c icalssyacc.c

libicalss_la_SOURCES =		\
	icalcalendar.c		\
	icalcalendar.h		\
	icalclassify.c		\
	icalclassify.h		\
	icalcluster.c		\
	icalcluster.h		\
	icalclusterimpl.h	\
	icalgauge.c		\
	icalgauge.h		\
	icalgaugeimpl.h		\
	icaldirset.c		\
	icaldirset.h		\
	icaldirsetimpl.h	\
	icalfileset.c		\
	icalfileset.h		\
	icalfilesetimpl.h	\
	icalset.c		\
	icalset.h		\
	icalssyacc.h		\
	icalspanlist.c		\
	icalspanlist.h		\
	icalmessage.c		\
	icalmessage.h		\
	$(libicalss_la_DEVSOURCES)

COMBINEDHEADERS = \
	$(srcdir)/icalgauge.h \
	$(srcdir)/icalset.h \
	$(srcdir)/icalcluster.h \
	$(srcdir)/icalfileset.h \
	$(srcdir)/icaldirset.h \
	$(srcdir)/icalcalendar.h  \
	$(srcdir)/icalclassify.h \
	$(srcdir)/icalspanlist.h	\
	$(srcdir)/icalmessage.h


icalss.h: $(COMBINEDHEADERS)
	echo '#ifdef __cplusplus'         >  icalss.h
	echo 'extern "C" {'               >> icalss.h
	echo '#endif'                     >> icalss.h
	echo '/*'                         >> icalss.h
	echo ' $$''Id''$$'                >> icalss.h
	echo '*/'                         >> icalss.h
	cat $(COMBINEDHEADERS)				\
	| egrep -v "#include.*\"ical"			\
	| egrep -v "#include.*\"pvl\.h\""		\
	| egrep -v '\$$(Id|Locker): .+\$$'>> icalss.h
	echo '#ifdef __cplusplus'         >> icalss.h
	echo '};'                         >> icalss.h
	echo '#endif'                     >> icalss.h


noinst_HEADERS = icalss.h  icalcalendar.h icalclassify.h icalcluster.h icaldirset.h icaldirsetimpl.h icalfileset.h icalfilesetimpl.h icalgauge.h icalgaugeimpl.h icalmessage.h icalset.h icalspanlist.h icalssyacc.h $(libicalssinclude_BDBHEADERS)

CONFIG_CLEAN_FILES = y.output

