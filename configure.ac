dnl configure.ac
dnl
dnl xfcalendar - Calendar application for XFce
dnl
dnl 2003 Mickael Graf <korbinus@linux.se>
dnl

AC_INIT([configure])

AM_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE([xfcalendar], [0.1.6])

AM_MAINTAINER_MODE

dnl Check for UNIX variants
AC_AIX
AC_ISC_POSIX
AC_MINIX

dnl Check for basic programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

dnl Check for standard header files
AC_HEADER_STDC
AC_CHECK_HEADERS([errno.h])

dnl Check for i18n support
BM_I18N([xfcalendar], [az ca cs de es fa_IR fr hi ko ms nl no pl pt_BR ru sv
	 ta tr zh_CN])
         
dnl Check for required packages
BM_DEPEND([LIBXFCE4MCS_CLIENT], [libxfce4mcs-client-1.0], [4.0.0])
BM_DEPEND([LIBXFCEGUI4], [libxfcegui4-1.0], [4.0.0])
BM_DEPEND([DBH], [dbh-1.0], [1.0])

dnl configure the mcs plugin
XFCE_MCS_PLUGIN([XFCE_MCS_MANAGER], [4.0.0])

dnl Check for debugging support
BM_DEBUG_SUPPORT

AC_OUTPUT([
Makefile
xfcalendar.spec
po/Makefile.in
src/Makefile
plugin/Makefile
])
