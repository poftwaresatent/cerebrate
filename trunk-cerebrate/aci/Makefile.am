CPPFLAGS+= -I@abs_top_srcdir@

noinst_LTLIBRARIES= libaci.la

libaci_la_SOURCES=  Behavior.cpp \
                    Cactus.cpp \
                    CircleLSQ.cpp \
                    Effects.cpp \
                    GUIHandler.cpp \
                    Localizer.cpp \
                    MotionManager.cpp \
                    Odometry.cpp \
                    Scanalyzer.cpp \
                    Timeout.cpp \
                    Watchdog.cpp

include_HEADERS=    Behavior.hpp \
                    Cactus.hpp \
                    CircleLSQ.hpp \
                    Effects.hpp \
                    GUIHandler.hpp \
                    Localizer.hpp \
                    MotionManager.hpp \
                    Odometry.hpp \
                    Scanalyzer.hpp \
                    Timeout.hpp \
                    Watchdog.hpp

includedir= @includedir@/aci

LDFLAGS+= @GFXLIBS@

bin_PROGRAMS=      fernandez
fernandez_SOURCES= fernandez.cpp
fernandez_LDADD=   libaci.la \
                   ../gfx/libgfx.la \
                   ../util/libutil.la \
                   ../sfl/libsfl.la \
                   ../drivers/libdrivers.la
