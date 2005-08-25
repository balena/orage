dnl $Id$
dnl
dnl xfcalendar - Calendar application for Xfce
dnl
dnl Copyright (c) 2003-2005
dnl         The Xfce development team. All rights reserved.
dnl
dnl Written for Xfce by Mickael Graf <korbinus@xfce.org>.
dnl

dnl Version information
m4_define([xfcalendar_version_major], [4])
m4_define([xfcalendar_version_minor], [3])
m4_define([xfcalendar_version_micro], [1])
m4_define([xfcalendar_version_nano], [18])  dnl leave this empty to have no nano version
m4_define([xfcalendar_version_tag], [svn])
m4_define([xfcalendar_version], [xfcalendar_version_major().xfcalendar_version_minor().xfcalendar_version_micro()ifelse(xfcalendar_version_nano(), [], [], [.xfcalendar_version_nano()])xfcalendar_version_tag()])


dnl # DO NOT MODIFY ANYTHING BELOW THIS LINE, UNLESS YOU KNOW WHAT
dnl # YOU ARE DOING.


dnl Initialize autoconf
AC_COPYRIGHT([Copyright (c) 2003-2005
        The Xfce development team. All rights reserved.

Written for Xfce by Mickael Graf <korbinus@xfce.org>.])
AC_INIT([xfcalendar], [xfcalendar_version()], [xfce4-dev@xfce.org])
AC_CANONICAL_SYSTEM()
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
AC_PROG_YACC()
AC_PROG_LN_S()
AM_PROG_LEX()

dnl Check for additional programs (libical)
AC_CHECK_PROGS([AR], [ar aal], [ar])
AC_CHECK_PROGS([PERL], [perl5 perl])  
 
dnl Check for standard header files
AC_HEADER_STDC()
AC_CHECK_HEADERS([assert.h errno.h pthread.h stdint.h time.h sys/types.h unistd.h wctype.h])

dnl Checks for typedefs, structures, and compiler characteristics (libical)
AC_C_CONST()
AC_TYPE_SIZE_T()
AC_TYPE_MODE_T()
AC_STRUCT_TM()

dnl Checks for library functions (libical)
AC_CHECK_FUNCS([gmtime_r iswspace strdup snprintf])

dnl Check for i18n support
XDT_I18N([ar az be bg bn_IN ca cs de el en_GB es et eu fa fi fr gu he hi \
          hy it ja ko lt mr ms nl nb_NO pa pl pt_BR pt_PT ro ru sk sv ta \
          tr uk vi zh_CN zh_TW])

dnl Check for required packages
XDT_CHECK_PACKAGE([LIBXFCE4MCS_CLIENT], [libxfce4mcs-client-1.0], [4.2.0])
XDT_CHECK_PACKAGE([LIBXFCEGUI4], [libxfcegui4-1.0], [4.2.0])

dnl configure the mcs plugin
XDT_XFCE_MCS_PLUGIN([XFCE_MCS_MANAGER], [4.2.0])

dnl the ical magic
XDT_CHECK_PACKAGE([ICAL], [libical-0.24], [0.24],
[
  ac_INCLUDED_LIBICAL=no
],
[
  ac_INCLUDED_LIBICAL=yes
 
  AC_MSG_CHECKING(for Berkeley DB4 support)
  AC_ARG_WITH(bdb4, [  --with-bdb4             Add support for Berkeley DB 4.0],
  [case $with_bdb4 in  
      yes)	AC_MSG_RESULT(yes)
      AC_MSG_RESULT(  Adding Berkeley DB support)
      WITH_BDB4="yes"
      ;;
      no)		AC_MSG_RESULT(no)
      ;;
      *)		AC_MSG_RESULT(no)
      AC_MSG_RESULT(Ignoring unknown argument to --with-bdb4: $with_bdb4)
      ;;
  esac],
  AC_MSG_RESULT(no))

  if test x$WITH_BDB4 = xyes; then
    AC_ARG_WITH(bdb4_dir, [  --with-bdb4-dir=PATH    Use PATH as location of BerkeleyDB 4.0],
       BDB_DIR=$withval,
       BDB_DIR=/usr/local/BerkeleyDB.4.0
    )

    AC_MSG_CHECKING(for Berkeley DB include files)
    if test -f $BDB_DIR/include/db.h; then
     BDB_DIR_INCLUDE=$BDB_DIR/include
     AC_MSG_RESULT($BDB_DIR_INCLUDE)
    else
      if test -f $BDB_DIR/include/db4/db.h; then
       BDB_DIR_INCLUDE=$BDB_DIR/include/db4
       AC_MSG_RESULT($BDB_DIR_INCLUDE)
      else
       WITH_BDB4="no"
       AC_MSG_RESULT(not found!)
      fi
    fi

    AC_MSG_CHECKING(for Berkeley DB libraries)
    if test -f $BDB_DIR/lib/libdb-4.0.la; then
     BDB_DIR_LIB=$BDB_DIR/lib
     BDB_LIB=libdb-4.0.la
     AC_MSG_RESULT($BDB_DIR_LIB $BDB_LIB)
    else
      if test -f $BDB_DIR/lib/libdb4.la; then
       BDB_DIR_LIB=$BDB_DIR/lib
       BDB_LIB=libdb4.la
       AC_MSG_RESULT($BDB_DIR_LIB $BDB_LIB)
      else
       WITH_BDB4="no"
       AC_MSG_RESULT(not found!)
      fi
    fi

    AC_SUBST(BDB_DIR)
    AC_SUBST(BDB_DIR_INCLUDE)
    AC_SUBST(BDB_DIR_LIB)
    AC_SUBST(BDB_LIB)
    dnl 4.0 is required...
    BDB_VERSION="4.0"
    AC_SUBST(BDB_VERSION)
  fi

  AC_MSG_CHECKING(whether to define _REENTRANT)
  AC_ARG_ENABLE(reentrant, [  --enable-reentrant      define _REENTRANT to enable reentrant system calls],
  [ case "${enableval}" in
    no)
      AC_MSG_RESULT(no)
      ac_cv_reentrant=no
      ;;
    *) 
      AC_MSG_RESULT(yes)
      AC_DEFINE([ICAL_REENTRANT], ,[Define if we want _REENTRANT]) [CFLAGS="$CFLAGS -D_REENTRANT"]
      ac_cv_reentrant=yes
      ;;
    esac ],
    AC_MSG_RESULT(no)
  )
  case "${host}" in 
          *-*-freebsdelf4* )
      AC_MSG_RESULT(checking for pthread_create in -lpthread... -pthread (FreeBSD 4.x style))
      AC_DEFINE(HAVE_PTHREAD, ,[Define if we have pthread.])
      PTHREAD_LIBS=-pthread;;

    *)
    AC_CHECK_LIB([pthread], [pthread_create], [have_pthread=yes])
    if test "x$have_pthread" = xyes; then
       AC_DEFINE(HAVE_PTHREAD, ,[Define if we have pthread.])
       PTHREAD_LIBS=-lpthread
    fi;;
  esac
  AC_SUBST([PTHREAD_LIBS])
])
AM_CONDITIONAL([INCLUDED_LIBICAL], [test x"$ac_INCLUDED_LIBICAL" = x"yes"])
AM_CONDITIONAL([HAVE_PTHREAD], [test x"$have_pthread" = x"yes"])
AM_CONDITIONAL([WITH_BDB4], [test x"$WITH_BDB4" = x"yes"])

dnl Check for debugging support
BM_DEBUG_SUPPORT()

AC_OUTPUT([ 
Makefile
libical/Makefile
libical/design-data/Makefile
libical/doc/Makefile
libical/scripts/Makefile
libical/src/Makefile
libical/src/libical/Makefile
libical/src/libical/icalversion.h
libical/src/libicalss/Makefile
libical/zoneinfo/Makefile
xfcalendar.spec
icons/Makefile
icons/48x48/Makefile
icons/scalable/Makefile
po/Makefile.in
sounds/Makefile
src/Makefile
plugin/Makefile
])
