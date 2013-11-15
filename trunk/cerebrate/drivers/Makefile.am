CPPFLAGS+= -I@abs_top_srcdir@

noinst_LTLIBRARIES=    libdrivers.la

libdrivers_la_SOURCES= FModIPDCMOT.cpp \
                       FModTCP.cpp \
                       fmod_ipdcmot.c \
                       fmod_tcp.c \
                       fmod_util.c \
                       sick.c \
                       util.c

include_HEADERS=       FModIPDCMOT.hpp \
                       FModTCP.hpp \
                       fmod_ipdcmot.h \
                       fmod_tcp.h \
                       fmod_util.h \
                       sick.h \
                       util.h

includedir= @includedir@/drivers

bin_PROGRAMS=     ttcp cfmod cipdcmot csick ctcp

ttcp_SOURCES=     ttcp.cpp
ttcp_LDADD=       libdrivers.la

cfmod_SOURCES=    cfmod.cpp
cfmod_LDADD=      libdrivers.la

cipdcmot_SOURCES= cipdcmot.cpp
cipdcmot_LDADD=   libdrivers.la

csick_SOURCES=    csick.cpp
csick_LDADD=      libdrivers.la ../util/libutil.la

ctcp_SOURCES=     ctcp.cpp
ctcp_LDADD=       libdrivers.la
