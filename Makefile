############################# configurable section #############################

# configure install path

PREFIX:=/usr/local

# configure debug or release build

DEBUG:=-DDEBUG=1 -O0 -g3 -fno-inline -fstack-protector-all
# for debug build, comment the line below
DEBUG:=-O3

# configure default tools

CC?=cc
# make defines AR, so we have to override to gcc-ar, so -flto works
AR=gcc-ar
STRIP?=strip
RANLIB?=gcc-ranlib

########################## end of configurable section #########################

# shared library version

SOVERM:=0
SOVERF:=0.0.0

# change options based on wild guess of compiler brand/type

ifeq ($(lastword $(subst /, ,$(CC))),tcc)
CCOPT:=-Wall $(DEBUG) -I.
else
ifeq ($(lastword $(subst /, ,$(CC))),clang)
CCOPT:=-Wall $(DEBUG) -I. --std=gnu89
else
CCOPT:=-Wall $(DEBUG) -I. --std=gnu89 -flto
endif
endif

# allow to pass additional compiler flags

CFLAGS:=$(CCOPT) $(CFLAGS)

all: libyascreen.a libyascreen.so yastest yastest.shared

yascreen.o: yascreen.c yascreen.h
	$(CC) $(CFLAGS) -o $@ -c $<

yastest.o: yastest.c yascreen.h
	$(CC) $(CFLAGS) -o $@ -c $<

yastest: yastest.o yascreen.o
	$(CC) $(CFLAGS) -o $@ $^ -lrt
	$(STRIP) $@

yastest.shared: yastest.o libyascreen.so
	$(CC) $(CFLAGS) -o $@ $^ -lrt -L. -lyascreen
	$(STRIP) $@

libyascreen.a: yascreen.o
	$(AR) r $@ $^
	$(RANLIB) $@

libyascreen.so: libyascreen.so.$(SOVERM)
	ln -fs $^ $@

libyascreen.so.$(SOVERM): libyascreen.so.$(SOVERF)
	ln -fs $^ $@

libyascreen.so.$(SOVERF): yascreen.c yascreen.h
	$(CC) $(CFLAGS) -o $@ $< -fPIC -lrt -shared
	$(STRIP) $@

install: all
	cp -a libyascreen.a libyascreen.so libyascreen.so.$(SOVERM) libyascreen.so.$(SOVERF) $(PREFIX)/lib/
	cp -a yascreen.h $(PREFIX)/include/

clean:
	rm -f yastest yastest.shared yastest.o yascreen.o libyascreen.a libyascreen.so libyascreen.so.$(SOVERM) libyascreen.so.$(SOVERF)

rebuild:
	$(MAKE) clean
	$(MAKE) all

.PHONY: install clean rebuild all
