CC = gcc
#OPT = -O0 -g
OPT = -O2
CFLAGS = -DLINUX -Wall $(OPT) -pipe

GFXLIBS = -L/usr/X11R6/lib -lglut -lGLU -lGL -lXi -lXmu
PNGLIBS = -lpng

YFLAGS = -d


all: teffect tparse tball haiko

tpath: tpath.c Makefile
	$(CC) $(CFLAGS) -o tpath tpath.c

teffect: teffect.c libhaiko.a Makefile
	$(CC) $(CFLAGS) -o teffect teffect.c -ll -L. -lhaiko $(GFXLIBS)

tparse: tparse.c libhaiko.a Makefile
	$(CC) $(CFLAGS) -o tparse tparse.c -ll -L. -lhaiko $(GFXLIBS)

tball: tball.c libhaiko.a Makefile
	$(CC) $(CFLAGS) -o tball tball.c -ll -L. -lhaiko $(GFXLIBS)

haiko: haiko.c libhaiko.a Makefile
	$(CC) $(CFLAGS) -o haiko haiko.c -ll -L. -lhaiko $(GFXLIBS) $(PNGLIBS)

voxel_tok.c: voxel_tok.l Makefile
	lex -Pvv -t voxel_tok.l > voxel_tok.c

voxel_parse.c: voxel_parse.y Makefile
	yacc -d -p vv voxel_parse.y
	mv -f y.tab.c voxel_parse.c

voxel_parse.o: voxel_parse.tab.h voxel_parse.c Makefile
	$(CC) $(CFLAGS) -c voxel_parse.c

voxel_parse.tab.h: voxel_parse.c Makefile
	mv -f y.tab.h voxel_parse.tab.h 

effect_tok.c: effect_tok.l Makefile
	lex -Pee -t effect_tok.l > effect_tok.c

effect_parse.c: effect_parse.y Makefile
	yacc -d -p ee effect_parse.y
	mv -f y.tab.c effect_parse.c

effect_parse.o: effect_parse.tab.h effect_parse.c Makefile
	$(CC) $(CFLAGS) -c effect_parse.c

effect_parse.tab.h: effect_parse.c Makefile
	mv -f y.tab.h effect_parse.tab.h 

libhaiko.a: voxel.o voxel_parse.o voxel_tok.o \
            effect.o effect_parse.o effect_tok.o \
            gltrackball.o trackball.o wrap_gl.o \
            Makefile
	rm -f libhaiko.a
	ar -r libhaiko.a voxel_parse.o voxel.o voxel_tok.o \
                         effect.o effect_parse.o effect_tok.o \
                         gltrackball.o trackball.o wrap_gl.o
	ranlib libhaiko.a

clean:
	- rm *.a *.o *~
	- rm voxel_tok.c voxel_parse.c voxel_parse.tab.h
	- rm effect_tok.c effect_parse.c effect_parse.tab.h
	- rm teffect tparse tball haiko

voxel.o: voxel.h voxel.c Makefile
voxel_tok.c: voxel_tok.l Makefile
voxel_tok.o: voxel_tok.c Makefile
voxel_parse.c: voxel_parse.y Makefile
voxel_parse.tab.h: voxel_parse.y Makefile
effect.o: effect.h effect.c Makefile
effect_tok.c: effect_tok.l Makefile
effect_tok.o: effect_tok.c Makefile
effect_parse.c: effect.h effect_parse.y Makefile
effect_parse.tab.h: effect.h effect_parse.y Makefile
trackball.o: trackball.h trackball.c Makefile
gltrackball.o: gltrackball.h gltrackball.c Makefile
wrap_gl.o: wrap_gl.c Makefile
