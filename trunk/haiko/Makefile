CC = gcc
CFLAGS = -DOSX -DHAVE_COCOA -Wall -O2 -pipe -I/usr/local/png/include

GFXLIBS = -framework OpenGL -framework GLUT -lobjc
PNGLIBS = -L/usr/local/png/lib -lpng

YFLAGS = -d


all: tparse tball haiko

tparse: tparse.c libhaiko.a Makefile
	$(CC) $(CFLAGS) -o tparse tparse.c -ll -L. -lhaiko $(GFXLIBS)

tball: tball.c libhaiko.a Makefile
	$(CC) $(CFLAGS) -o tball tball.c -ll -L. -lhaiko $(GFXLIBS)

haiko: haiko.c libhaiko.a Makefile
	$(CC) $(CFLAGS) -o haiko haiko.c -ll -L. -lhaiko $(GFXLIBS) $(PNGLIBS)

voxel_parse.o: voxel_parse.tab.h voxel_parse.c
	$(CC) $(CFLAGS) -c voxel_parse.c

voxel_parse.tab.h: voxel_parse.c
	mv -f y.tab.h voxel_parse.tab.h 

libhaiko.a: voxel_parse.o voxel.o tokenize.o gltrackball.o trackball.o \
            wrap_gl.o
	rm -f libhaiko.a
	ar -r libhaiko.a voxel_parse.o voxel.o tokenize.o gltrackball.o trackball.o wrap_gl.o
	ranlib libhaiko.a

clean:
	- rm *.a *.o *~ tokenize.c voxel_parse.c voxel_parse.tab.h tparse

voxel.o: voxel.h voxel.c Makefile
tokenize.c: tokenize.l Makefile
tokenize.o: tokenize.c Makefile
voxel_parse.c: voxel_parse.y Makefile
voxel_parse.tab.h: voxel_parse.y Makefile
libhaiko.a: voxel.o Makefile
trackball.o: trackball.h trackball.c Makefile
gltrackball.o: gltrackball.h gltrackball.c Makefile
wrap_gl.o: wrap_gl.c Makefile
