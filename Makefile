ifndef NAVISERVER
    NAVISERVER  = /usr/local/ns
endif

#
# Module name
#
MOD      =  nsgdchart.so

#
# Objects to build.
#
OBJS     = gdchart.o nsgdchart.o

CFLAGS	 = -I/usr/local/include/gd -DHAVE_LIBFREETYPE
MODLIBS	 = -L/usr/local/lib -lgd -lm
# RedHat libraries
#MODLIBS  = -L/usr/local/lib /usr/lib/libgd.a -ljpeg -lfreetype -lpng -lz -lm

include  $(NAVISERVER)/include/Makefile.module
