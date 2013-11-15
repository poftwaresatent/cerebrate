SRCDIR=     @abs_top_srcdir@
BUILDDIR=   @abs_top_builddir@

CPPFLAGS=   @HAIKO_CPPFLAGS@
CFLAGS=     $(CPPFLAGS) @HAIKO_CFLAGS@
LDFLAGS=    @HAIKO_LDFLAGS@

include_HEADERS= effect.h \
                 gltrackball.h \
                 trackball.h \
                 voxel.h \
                 wrap_gl.h \
                 wrap_glut.h

lib_LTLIBRARIES=     libhaiko.la
libhaiko_la_SOURCES= effect.c \
                     gltrackball.c \
                     trackball.c \
                     voxel.c \
                     wrap_gl.c \
                     effect_tok.c \
                     effect_parse.c \
                     voxel_tok.c \
                     voxel_parse.c

# # Some manual lex/yacc rules... probably Automake can do something
# # like this automtically. Probably, we do not even have to provide
# # these manual rules, given that the lex/yacc output is included in
# # 'make dist'.
#
# voxel_tok.c: voxel_tok.l
# 	$(LEX) $(LFLAGS) -Pvv -t voxel_tok.l > voxel_tok.c
#
# voxel_tok.lo voxel_tok.o: voxel_parse.tab.h
#
# voxel_parse.c: voxel_parse.y
# 	$(YACC) $(YFLAGS) -d -p vv voxel_parse.y
#
# voxel_parse.tab.h: voxel_parse.c
# 	mv -f y.tab.h voxel_parse.tab.h 
#
# effect_tok.c: effect_tok.l
# 	$(LEX) $(LFLAGS) -Pee -t effect_tok.l > effect_tok.c
#
# effect_tok.lo effect_tok.o: effect_parse.tab.h
#
# effect_parse.c: effect_parse.y
# 	$(YACC) $(YFLAGS) -d -p ee effect_parse.y
#
# effect_parse.tab.h: effect_parse.c
# 	mv -f y.tab.h effect_parse.tab.h 

bin_PROGRAMS=  haiko
haiko_SOURCES= haiko.c
haiko_LDADD=   libhaiko.la @HAIKO_GFXLIBS@

pkgconfig_DATA= haiko.pc
pkgconfigdir=   $(libdir)/pkgconfig

SHAREFILES= cheshire.eff \
            crosshair.vox \
            haiko.eff \
            haiko.vox \
            headbeat.eff \
            hopp.eff \
            lay.vox \
            lay2.vox \
            line.vox \
            seq.vox \
            seqbeam.eff \
            seqhopp.eff \
            sit.vox \
            sit2.vox \
            sit3.vox

data_DATA= $(SHAREFILES)
datadir=   $(prefix)/share/haiko

EXTRA_DIST= bootstrap-buildsystem.sh \
            build-stage.sh \
            effect_tok.l \
            effect_parse.y \
            effect_parse.tab.h \
            voxel_tok.l \
            voxel_parse.y \
            voxel_parse.tab.h \
            $(SHAREFILES)
