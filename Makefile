# BUILD SETTINGS ###############################################################

ifneq ($(filter Msys Cygwin, $(shell uname -o)), )
    PLATFORM := WIN32
    TYRIAN_DIR = C:\\TYRIAN
else
    PLATFORM := UNIX
    TYRIAN_DIR = $(gamesdir)/tyrian
endif

################################################################################

# see https://www.gnu.org/prep/standards/html_node/Makefile-Conventions.html

SHELL = /bin/sh

CC ?= gcc
INSTALL ?= install
PKG_CONFIG ?= pkg-config

INSTALL_PROGRAM ?= $(INSTALL)
INSTALL_DATA ?= $(INSTALL) -m 644

prefix ?= /usr/local
exec_prefix ?= $(prefix)

bindir ?= $(exec_prefix)/bin
datarootdir ?= $(prefix)/share
datadir ?= $(datarootdir)
docdir ?= $(datarootdir)/doc/arctyr
mandir ?= $(datarootdir)/man
man6dir ?= $(mandir)/man6
man6ext ?= .6

# see http://www.pathname.com/fhs/pub/fhs-2.3.html

gamesdir ?= $(datadir)/games

###

TARGET := arctyr

SRCS := $(wildcard src/*.c) $(wildcard src/*/*.c) $(wildcard src/*/*/*.c)
OBJS := $(SRCS:src/%.c=obj/%.o)
DEPS := $(SRCS:src/%.c=obj/%.d)

###

GIT_REV_SHORT := $(shell git describe --tags --abbrev=0 2>/dev/null && \
                   touch src/version.h)
GIT_REV_FULL := $(shell (git describe --tags --dirty || git rev-parse --short HEAD) 2>/dev/null)
ifneq ($(GIT_REV_SHORT), )
    EXTRA_CPPFLAGS += -DGIT_REV_SHORT='"$(GIT_REV_SHORT)"'
endif
ifneq ($(GIT_REV_FULL), )
    EXTRA_CPPFLAGS += -DGIT_REV_FULL='"$(GIT_REV_FULL)"'
endif

###

CPPFLAGS := -DNDEBUG
CFLAGS := -pedantic
CFLAGS += -MMD
CFLAGS += -Wall \
          -Wextra \
          -Wno-missing-field-initializers
CFLAGS += -O2
LDFLAGS := 
LDLIBS := 

CFLAGS += -no-pie -fno-pie

SDL_CPPFLAGS := $(shell $(PKG_CONFIG) sdl --cflags)
SDL_LDFLAGS := $(shell $(PKG_CONFIG) sdl --libs-only-L --libs-only-other)
SDL_LDLIBS := $(shell $(PKG_CONFIG) sdl --libs-only-l)

ALL_CPPFLAGS = -DTARGET_$(PLATFORM) \
               -DTYRIAN_DIR='"$(TYRIAN_DIR)"' \
               $(EXTRA_CPPFLAGS) \
               $(SDL_CPPFLAGS) \
               $(CPPFLAGS)
ALL_CFLAGS = -std=iso9899:1999 \
             $(CFLAGS)
ALL_LDFLAGS = $(SDL_LDFLAGS) \
              $(LDFLAGS)
ALL_LDLIBS = -lm \
             $(SDL_LDLIBS) \
             $(LDLIBS)

###

.PHONY : all
all : $(TARGET)

.PHONY : debug
debug : CPPFLAGS += -UNDEBUG
debug : CFLAGS += -Werror
debug : CFLAGS += -O0
debug : CFLAGS += -g3
debug : all

.PHONY : installdirs
installdirs :
	mkdir -p $(DESTDIR)$(bindir)
	mkdir -p $(DESTDIR)$(docdir)
	mkdir -p $(DESTDIR)$(man6dir)

.PHONY : install
install : $(TARGET) installdirs
	$(INSTALL_PROGRAM) $(TARGET) $(DESTDIR)$(bindir)/
	$(INSTALL_DATA) CREDITS NEWS README $(DESTDIR)$(docdir)/
	$(INSTALL_DATA) linux/man/arctyr.6 $(DESTDIR)$(man6dir)/arctyr$(man6ext)

.PHONY : uninstall
uninstall :
	rm -f $(DESTDIR)$(bindir)/$(TARGET)
	rm -f $(DESTDIR)$(docdir)/{CREDITS,NEWS,README}
	rm -f $(DESTDIR)$(man6dir)/arctyr$(man6ext)

.PHONY : clean
clean :
	rm -f $(OBJS)
	rm -f $(DEPS)
	rm -f $(TARGET)

$(TARGET) : $(OBJS)
	@echo "Linking: $@"
	@$(CC) $(ALL_CFLAGS) $(ALL_LDFLAGS) -o $@ $^ $(ALL_LDLIBS)

-include $(DEPS)

obj/%.o : src/%.c
	@echo "Compiling: $<"
	@mkdir -p "$(dir $@)"
	@$(CC) $(ALL_CPPFLAGS) $(ALL_CFLAGS) -c -o $@ $<
