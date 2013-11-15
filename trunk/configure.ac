AC_REVISION($Id$)
AC_INIT(cerebrate, 1.1, poftwaresatent AT fsfe DOT org)
AC_PREREQ(2.57)
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_CANONICAL_HOST

AC_LANG(C++)
AC_PROG_CC
AC_PROG_CXX
case $host_os in
  darwin*) AC_DISABLE_SHARED;;
esac
AC_PROG_LIBTOOL
AC_PROG_INSTALL

CFLAGS="$CFLAGS -pipe"

AC_ARG_ENABLE(debug,
  AC_HELP_STRING([--enable-debug], [GCC options -g -O0 (else -O3)]),
  [ CPPFLAGS="$CPPFLAGS -DCRB_DEBUG"
    CFLAGS="$CFLAGS -g -O0" ],
  [ CFLAGS="$CFLAGS -O3" ])

AC_ARG_ENABLE(pedantic,
  AC_HELP_STRING([--enable-pedantic], [GCC option -pedantic (else -Wall)]),
  [ CFLAGS="$CFLAGS -pedantic" ],
  [ CFLAGS="$CFLAGS -Wall" ])

case "$host_os" in
  linux*)  AC_MSG_NOTICE([detected Linux])
           CPPFLAGS="$CPPFLAGS -DLINUX"
           LDFLAGS="-lpthread $LDFLAGS"
           GFXLIBS="-L/usr/X11R6/lib -lglut -lGLU -lGL -lXi -lXmu";;
  darwin*) AC_MSG_NOTICE([detected OSX])
           CPPFLAGS="$CPPFLAGS -DOSX"
           CFLAGS="$CFLAGS -Wno-long-double"
           GFXLIBS="-framework OpenGL -framework GLUT -lobjc";;
  *)       AC_MSG_ERROR([unknown host_os $host_os, update configure.ac]);;
esac
AC_SUBST(GFXLIBS)

AC_ARG_ENABLE(aci,
  AC_HELP_STRING([--disable-aci], [disable Fernandez]),
  [], [enable_aci=yes] )

AS_IF([test x$enable_aci != xno],
  [ AC_MSG_NOTICE([Fernandez enabled])
    AM_CONDITIONAL([ENABLE_ACI], [true]) ],
  [ AC_MSG_NOTICE([Fernandez disabled])
    AM_CONDITIONAL([ENABLE_ACI], [false]) ] )

AC_ARG_ENABLE(blink,
  AC_HELP_STRING([--disable-blink], [disable Blink]),
  [], [enable_blink=yes] )

AS_IF([test x$enable_blink != xno],
  [ AC_MSG_NOTICE([Blink enabled])
    AM_CONDITIONAL([ENABLE_BLINK], [true]) ],
  [ AC_MSG_NOTICE([Blink disabled])
    AM_CONDITIONAL([ENABLE_BLINK], [false]) ] )

AC_ARG_ENABLE(ibou,
  AC_HELP_STRING([--disable-ibou], [disable Ibou]),
  [], [enable_ibou=yes] )

AS_IF([test x$enable_ibou != xno],
  [ AC_MSG_NOTICE([Ibou enabled])
    AM_CONDITIONAL([ENABLE_IBOU], [true]) ],
  [ AC_MSG_NOTICE([Ibou disabled])
    AM_CONDITIONAL([ENABLE_IBOU], [false]) ] )

CXXFLAGS="$CFLAGS"
AC_SUBST(PACKAGE_VERSION)
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES(Makefile \
                cerebrate.pc \
                drivers/Makefile \
                sfl/Makefile \
                util/Makefile \
                gfx/Makefile \
                aci/Makefile \
                blink/Makefile \
                ibou/Makefile)
AC_OUTPUT
