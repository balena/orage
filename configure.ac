dnl $Id$
dnl
dnl xfcalendar - Calendar application for Xfce
dnl
dnl Copyright (c) 2003-2005
dnl         The Xfce development team. All rights reserved.
dnl
dnl Written for Xfce by Mickael Graf <korbinus@linux.se>.
dnl

dnl Version information
m4_define([xfcalendar_version_major], [4])
m4_define([xfcalendar_version_minor], [3])
m4_define([xfcalendar_version_micro], [0])
m4_define([xfcalendar_version_nano], [1])  dnl leave this empty to have no nano version
m4_define([xfcalendar_version_tag], [cvs])
m4_define([xfcalendar_version], [xfcalendar_version_major().xfcalendar_version_minor().xfcalendar_version_micro()ifelse(xfcalendar_version_nano(), [], [], [.xfcalendar_version_nano()])xfcalendar_version_tag()])


dnl # DO NOT MODIFY ANYTHING BELOW THIS LINE, UNLESS YOU KNOW WHAT
dnl # YOU ARE DOING.


dnl Initialize autoconf
AC_COPYRIGHT([Copyright (c) 2003-2005
        The Xfce development team. All rights reserved.

Written for Xfce by Mickael Graf <korbinus@linux.se>.])
AC_INIT([xfcalendar], [xfcalendar_version()], [xfce4-dev@xfce.org])
AC_PREREQ([2.50])
AC_REVISION([$Id$])

dnl Initialize automake
XFCALENDAR_VERSION=xfcalendar_version()
AM_INIT_AUTOMAKE([xfcalendar], [$XFCALENDAR_VERSION])
AM_CONFIG_HEADER([config.h])
AM_MAINTAINER_MODE()

dnl Check for UNIX variants
AC_AIX()
AC_ISC_POSIX()
AC_MINIX()

dnl Check for basic programs
AC_PROG_CC()
AC_PROG_INSTALL()
AC_PROG_INTLTOOL()
AC_PROG_LIBTOOL()

dnl Check for standard header files
AC_HEADER_STDC
AC_CHECK_HEADERS([errno.h])

dnl Check for i18n support
XDT_I18N([ar az be bg ca cs de en_GB es et eu fa fi fr gu he hi \
          it ja ko lt ms nl nb_NO pa pl pt_BR pt_PT ro ru sk sv ta \
          tr uk vi zh_CN zh_TW])

dnl Check for required packages
XDT_CHECK_PACKAGE([LIBXFCE4MCS_CLIENT], [libxfce4mcs-client-1.0], [4.2.0])
XDT_CHECK_PACKAGE([LIBXFCEGUI4], [libxfcegui4-1.0], [4.2.0])
XDT_CHECK_PACKAGE([DBH], [dbh-1.0], [1.0])
XDT_CHECK_PACKAGE([ICAL], [libical-0.24], [0.24])

dnl configure the mcs plugin
XDT_XFCE_MCS_PLUGIN([XFCE_MCS_MANAGER], [4.2.0])

dnl Check for debugging support
BM_DEBUG_SUPPORT()

AC_CONFIG_SUBDIRS([libical])
AC_OUTPUT([
Makefile
xfcalendar.spec
icons/Makefile
icons/48x48/Makefile
icons/scalable/Makefile
po/Makefile.in
src/Makefile
plugin/Makefile
])
