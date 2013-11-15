CPPFLAGS+= -I@abs_top_srcdir@

if ENABLE_ACI
  ACI_DIR= gfx aci
  ACI_LIB= gfx/libgfx.la aci/libaci.la
else
  ACI_DIR= 
  ACI_LIB= 
endif

if ENABLE_BLINK
  BLINK_DIR= blink
  BLINK_LIB= blink/libblink.la
else
  BLINK_DIR= 
  BLINK_LIB= 
endif

if ENABLE_IBOU
  IBOU_DIR= ibou
  IBOU_LIB= ibou/libibou.la
else
  IBOU_DIR= 
  IBOU_LIB= 
endif

SUBDIRS= util drivers sfl $(ACI_DIR) $(BLINK_DIR) $(IBOU_DIR)

lib_LTLIBRARIES=         libcerebrate.la
libcerebrate_la_SOURCES= 
libcerebrate_la_LDFLAGS= -version-info 0:0:0
libcerebrate_la_LIBADD=  drivers/libdrivers.la \
                         sfl/libsfl.la \
                         util/libutil.la \
                         $(ACI_LIB) \
                         $(BLINK_LIB) \
                         $(IBOU_LIB)

pkgconfigdir=            $(libdir)/pkgconfig
pkgconfig_DATA=          cerebrate.pc

#BUILT_SOURCES= incsym
#SRCDIR= @abs_top_srcdir@
#BUILDDIR= @abs_top_builddir@
#
#if ENABLE_NPM
#incsym:
#	test -d $(BUILDDIR)/include || mkdir -p $(BUILDDIR)/include
#	test -L $(BUILDDIR)/include/sfl || ln -s $(SRCDIR)/sunflower $(BUILDDIR)/include/sfl
#	test -L $(BUILDDIR)/include/npm || ln -s $(SRCDIR)/nepumuk $(BUILDDIR)/include/npm
#	test -L $(BUILDDIR)/include/estar || ln -s $(SRCDIR)/estar/estar $(BUILDDIR)/include/estar
#	test -L $(BUILDDIR)/include/gfx || ln -s $(SRCDIR)/estar/gfx $(BUILDDIR)/include/gfx
#	test -L $(BUILDDIR)/include/pnf || ln -s $(SRCDIR)/estar/pnf $(BUILDDIR)/include/pnf
#	test -L $(BUILDDIR)/include/asl || ln -s $(SRCDIR)/asl-mcontrol $(BUILDDIR)/include/asl
#	test -L $(BUILDDIR)/include/smartsfl || ln -s $(SRCDIR)/smartsfl $(BUILDDIR)/include/smartsfl
#else
#endif


etags:
	chdir $(SRCDIR) && \
	  find $(SUBDIRS) \
             -name '*.cpp' -o -name '*.hpp' \
             -name '*.c' -o -name '*.h' \
             | xargs etags -o $(BUILDDIR)/TAGS -a
