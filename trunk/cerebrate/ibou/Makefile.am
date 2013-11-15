CPPFLAGS+= -I@abs_top_srcdir@

noinst_LTLIBRARIES= libibou.la
libibou_la_SOURCES= Action.cpp
include_HEADERS=    Action.hpp

includedir= @includedir@/ibou

bin_PROGRAMS=  iboud
iboud_SOURCES= iboud.cpp
iboud_LDADD=   libibou.la ../drivers/libdrivers.la ../util/libutil.la
