# Makefile for lg library

.PHONY: all clean

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

DEFINES = -DWINVER=0x0500 -D_WIN32_WINNT=0x0500 -DWIN32_LEAN_AND_MEAN

ARFLAGS = rc
LIBS = -lstdc++ -luuid
INCLUDEDIRS = -I. -I$(srcdir)
# If you care for this... # -Wno-unused-variable 
# A lot of the callbacks have unused parameters, so I turn that off.
CXXFLAGS =  -W -Wall -Wno-unused-parameter -Wno-pmf-conversions \
			$(INCLUDEDIRS) -std=c++11 -masm=att \
			-fno-pcc-struct-return -mms-bitfields

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


all: $(LG_LIB) $(LG_LIBD)

clean:
	$(RM) $(LG_OBJS) $(LG_OBJSD) $(LG_LIB) $(LG_LIBD)

$(LG_LIB): $(LG_OBJS)
	$(AR) $(ARFLAGS) $@ $?
	-@ ($(RANLIB) $@ || true) >/dev/null 2>&1

$(LG_LIBD): CXXDEBUG = -g -DDEBUG
$(LG_LIBD): LDDEBUG = -g
$(LG_LIBD): $(LG_OBJSD)
	$(AR) $(ARFLAGS) $@ $?
	-@ ($(RANLIB) $@ || true) >/dev/null 2>&1

iids.o: iids.cpp
lg.o: lg.cpp lg/types.h lg/dynarray.h lg/dynarray.hpp
scrmsgs.o: scrmsgs.cpp lg/scrmsgs.h
lg-d.o: lg.cpp lg/types.h lg/dynarray.h lg/dynarray.hpp
scrmsgs-d.o: scrmsgs.cpp lg/scrmsgs.h
