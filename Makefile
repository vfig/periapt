# Makefile for lg library

.SUFFIXES:
.SUFFIXES: .o .cpp

srcdir = .

# This Makefile is targetted for GCC in Cygwin/Mingw32 
# Possible command-line for Borland: -P -tWD -w-inl -w-par

SH = sh

CC = g++
AR = ar
# GNU ar updates the symbols automatically.
# Set this if you need to do it yourself
RANLIB = echo
DLLWRAP = dllwrap

DEFINES = -DWINVER=0x0400 -D_WIN32_WINNT=0x0400 -DWIN32_LEAN_AND_MEAN

ARFLAGS = rc
LDFLAGS = -mno-cygwin -mwindows -L.
LIBS = -lstdc++ -luuid
INCLUDEDIRS = -I. -I$(srcdir)
# If you care for this... # -Wno-unused-variable 
# A lot of the callbacks have unused parameters, so I turn that off.
CXXFLAGS =  -W -Wall -Wno-unused-parameter -Wno-pmf-conversions \
			$(INCLUDEDIRS) -mno-cygwin -masm=att \
			-fno-pcc-struct-return -mms-bitfields
LDFLAGS = -mno-cygwin -mwindows -L. -llg
DLLFLAGS = --def script.def --add-underscore --target i386-mingw32

ifdef DEBUG
CXXDEBUG = -g -DDEBUG 
LDDEBUG = -g
else
CXXDEBUG = -O3 -DNDEBUG
LDDEBUG =
endif

LG_LIB = liblg.a
LG_SRCS = lg.cpp scrmsgs.cpp iids.cpp
LG_OBJS = lg.o  scrmsgs.o iids.o

LG_LIBD = liblg-d.a
LG_OBJSD = lg-d.o scrmsgs-d.o iids.o

%.o: %.cpp
	$(CC) $(CXXFLAGS) $(CXXDEBUG) $(DEFINES) -o $@ -c $<

%-d.o: %.cpp
	$(CC) $(CXXFLAGS) $(CXXDEBUG) $(DEFINES) -o $@ -c $<


ALL:	$(LG_LIB) $(LG_LIBD)

clean:
	$(RM) $(LG_OBJS)

stamp:
	$(RM) lg/stamp-*
	$(SH) timestamp.sh lg lg $(LG_SRCS)

$(LG_LIB): $(LG_OBJS)
	$(AR) $(ARFLAGS) $@ $?
	-@ ($(RANLIB) $@ || true) >/dev/null 2>&1

$(LG_LIBD): CXXDEBUG = -g -DDEBUG
$(LG_LIBD): LDDEBUG = -g
$(LG_LIBD): $(LG_OBJSD)
	$(AR) $(ARFLAGS) $@ $?
	-@ ($(RANLIB) $@ || true) >/dev/null 2>&1

dump.o: dump.cpp
	$(CC) $(CXXFLAGS) -g -DDEBUG $(DEFINES) -o $@ -c $<

dump.osm: dump.o $(LG_LIBD)
	$(DLLWRAP) $(DLLFLAGS) -o $@ dump.o $(LDFLAGS) -g $(LIBS)


iids.o: iids.cpp
lg.o: lg.cpp lg/types.h lg/dynarray.h lg/dynarray.hpp
scrmsgs.o: scrmsgs.cpp lg/scrmsgs.h
lg-d.o: lg.cpp lg/types.h lg/dynarray.h lg/dynarray.hpp
scrmsgs-d.o: scrmsgs.cpp lg/scrmsgs.h
