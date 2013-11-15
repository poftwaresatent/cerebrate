CPPFLAGS+= -I@abs_top_srcdir@

noinst_LTLIBRARIES=  libblink.la
libblink_la_SOURCES= Blink.cpp
include_HEADERS=     Blink.hpp

includedir= @includedir@/blink

bin_PROGRAMS=   blinkd
blinkd_SOURCES= blinkd.cpp
blinkd_LDADD=   libblink.la ../drivers/libdrivers.la
