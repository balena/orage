dnl configure.ac
dnl
dnl xfcalendar - Calendar application for XFce
dnl
dnl 2003 Mickael Graf <korbinus@linux.se>
dnl

AC_INIT([configure])

AM_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE([xfcalendar], [4.2.2])

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
BM_I18N([xfcalendar], [af ar az be bg bn_IN ca cs da de el en_GB eo es
                       et eu fa fi fr gu he hi hu hy it ja ko ku lt mr
                       ms nb_NO nl pa pl pt_BR pt_PT ro ru sk sv ta tr
                       uk vi zh_CN zh_TW])

dnl Check for required packages
BM_DEPEND([LIBXFCE4MCS_CLIENT], [libxfce4mcs-client-1.0], [4.2.0])
BM_DEPEND([LIBXFCEGUI4], [libxfcegui4-1.0], [4.2.0])
BM_DEPEND([DBH], [dbh-1.0], [1.0])

dnl configure the mcs plugin
XFCE_MCS_PLUGIN([XFCE_MCS_MANAGER], [4.2.0])

dnl Check for debugging support
BM_DEBUG_SUPPORT

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
