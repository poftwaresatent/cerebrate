AC_INIT([haiko], [0.0rc1], [roland DOT philippsen AT gmx DOT net])
AC_CONFIG_AUX_DIR([.])
AC_PREREQ(2.57)
AM_INIT_AUTOMAKE([-Wall -Werror foreign])


dnl **************************************************
dnl *** required programs and basic compilation flags

AC_LANG(C)
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_INSTALL

dnl *** we distribute lex/yacc output so people do not need these
dnl
dnl AC_PROG_LEX
dnl AC_PROG_YACC

HAIKO_CPPFLAGS=""
HAIKO_CFLAGS="-pipe"
HAIKO_LDFLAGS=""
HAIKO_GFXLIBS=""


dnl **************************************************
dnl *** some known OS-dependent things, especially
dnl *** for OpenGL and GLUT

AC_CANONICAL_HOST
case $host_os in
  linux*)
    AC_MSG_NOTICE([detected Linux])
    HAIKO_CPPFLAGS="$HAIKO_CPPFLAGS -DLINUX"
    HAIKO_GFXLIBS="-L/usr/X11R6/lib -lglut -lGLU -lGL -lXi -lXmu"
    AC_CHECK_HEADER([GL/gl.h], [],
      [ AC_MSG_ERROR([Haiko requires OpenGL development packages])] )
    AC_CHECK_HEADER([GL/glut.h], [],
      [ AC_MSG_ERROR([Haiko requires GLUT development packages])] );;
  *openbsd*)
    AC_MSG_NOTICE([detected OpenBSD])
    HAIKO_CPPFLAGS="$HAIKO_CPPFLAGS -DOPENBSD -I/usr/local/include -I/usr/X11R6/include"
    HAIKO_LDFLAGS="$HAIKO_LDFLAGS -L/usr/local/lib -L/usr/X11R6/lib"
    HAIKO_GFXLIBS="-L/usr/X11R6/lib -lX11 -lXi -lXmu -lglut -lGLU -lGL"
    AC_CHECK_HEADER([GL/gl.h], [],
      [ AC_MSG_ERROR([Haiko requires OpenGL development packages])] )
    AC_CHECK_HEADER([GL/glut.h], [],
      [ AC_MSG_ERROR([Haiko requires GLUT development packages])] );;
  darwin*)
    AC_MSG_NOTICE([detected Darwin (OS X)])
    HAIKO_CPPFLAGS="$HAIKO_CPPFLAGS -DOSX -DHAVE_COCOA -Wno-long-double"
    HAIKO_GFXLIBS="-framework OpenGL -framework GLUT -lobjc"
    AC_CHECK_HEADER([OpenGL/gl.h], [],
      [ AC_MSG_ERROR([Haiko requires OpenGL development packages])] )
    AC_CHECK_HEADER([GLUT/glut.h], [],
      [ AC_MSG_ERROR([Haiko requires GLUT development packages])] );;
  *)
    AC_MSG_WARN([Unknown host_os $host_os, build will probably break.])
    AC_MSG_WARN([Please update configure.ac.]);;
esac


dnl **************************************************
dnl *** optional PNG support

AC_ARG_WITH(png,
  AC_HELP_STRING([--with-png=PATH], [specify libpng install dir]),
  [ if test "x$withval" != "x" ; then
      HAIKO_CPPFLAGS="$HAIKO_CPPFLAGS -I${withval}/include -I${withval}"
      HAIKO_LDFLAGS="-L${withval}/lib -L${withval} $HAIKO_LDFLAGS"
    fi ], [])
oldCPPFLAGS="$CPPFLAGS"
if test "$prefix" = "NONE"; then
  CPPFLAGS="$HAIKO_CPPFLAGS"
else
  CPPFLAGS="$HAIKO_CPPFLAGS -I$prefix/include"
fi
AC_CHECK_HEADERS([png.h],
  [ HAIKO_LDFLAGS="-lpng -lz $HAIKO_LDFLAGS" ],
  [ AC_MSG_NOTICE([PNG support disabled (CPPFLAGS=$CPPFLAGS)]) ])
CPPFLAGS="$oldCPPFLAGS"


dnl **************************************************
dnl *** general configuration options

AC_ARG_ENABLE(debug,
  AS_HELP_STRING([--enable-debug], [GCC options -g -O0 (else -O3)]),
  [ HAIKO_CPPFLAGS="$HAIKO_CPPFLAGS -DHAIKO_DEBUG"
    HAIKO_CFLAGS="$HAIKO_CFLAGS -g -O0" ],
  [ HAIKO_CFLAGS="$HAIKO_CFLAGS -O3" ])

AC_ARG_ENABLE(pedantic,
  AS_HELP_STRING([--enable-pedantic], [GCC options -pedantic (else -Wall)]),
  [ HAIKO_CFLAGS="$HAIKO_CFLAGS -pedantic" ],
  [ HAIKO_CFLAGS="$HAIKO_CFLAGS -Wall" ])


dnl **************************************************
dnl *** finalize build setup

if test "x$prefix" != "xNONE"; then
  HAIKO_CPPFLAGS="$HAIKO_CPPFLAGS -I$prefix/include"
fi

AC_SUBST(PACKAGE_VERSION)
AC_SUBST(HAIKO_CPPFLAGS)
AC_SUBST(HAIKO_CFLAGS)
AC_SUBST(HAIKO_LDFLAGS)
AC_SUBST(HAIKO_GFXLIBS)
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile haiko.pc])
AC_OUTPUT
