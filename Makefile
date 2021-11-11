#SAKURA ENGINE

.PHONY: all clean install uninstall

# CC = tcc
PROJ_NAME = smpd
PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man
OUT_DIR := out
SRC_DIR := src
OBJ_DIR := obj
INCLUDE_PATHS = -I. -Iinclude

# RELEASE or DEBUG
BUILD_MODE ?= DEBUG
# PLATFORM_DESKTOP, PLATFORM_WEB
PLATFORM ?= PLATFORM_DESKTOP

# Define compiler flags:
#  -O1                      defines optimization level
#  -g                       include debug information on compilation
#  -s                       strip unnecessary data from build
#  -Wall                    turns on most, but not all, compiler warnings
#  -std=c99                 defines C language mode (standard C from 1999 revision)
#  -Wno-missing-braces      ignore invalid warning (GCC bug 53119)
#  -D_DEFAULT_SOURCE        use with -std=c99 on Linux and PLATFORM_WEB, required for timespec
#  -Werror=pointer-arith    catch unportable code that does direct arithmetic on void pointers
##  -fno-strict-aliasing     jar_xm.h does shady stuff (breaks strict aliasing)
CFLAGS := -Wall -std=c99 -D_DEFAULT_SOURCE -Wno-missing-braces -Werror=pointer-arith -MD
# C Pre Processor Flags
CPPFLAGS := $(INCLUDE_PATHS)
# -L linker flags
# LDFLAGS := -L
# -l lib flags
LDLIBS   := -lm -ljson-c

EXE := $(OUT_DIR)/$(PROJ_NAME)
SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

ifeq ($(BUILD_MODE),DEBUG)
	CFLAGS += -D_DEBUG -g -Wextra -Wpedantic -Wformat=2 -Wno-unused-parameter -Wshadow -Wwrite-strings -Wstrict-prototypes -Wold-style-definition -Wredundant-decls -Wnested-externs -Wmissing-include-dirs
endif
ifeq ($(BUILD_MODE),RELEASE)
	CFLAGS += -s -O1
endif

# No uname.exe on MinGW!, but OS=Windows_NT on Windows!
# ifeq ($(UNAME),Msys) -> Windows
ifeq ($(OS),Windows_NT)
	PLATFORM_OS = WINDOWS
else
	UNAMEOS = $(shell uname)
	ifeq ($(UNAMEOS),Linux)
	PLATFORM_OS = LINUX
endif
ifeq ($(UNAMEOS),FreeBSD)
	PLATFORM_OS = BSD
endif
ifeq ($(UNAMEOS),OpenBSD)
	PLATFORM_OS = BSD
endif
ifeq ($(UNAMEOS),NetBSD)
	PLATFORM_OS = BSD
endif
ifeq ($(UNAMEOS),DragonFly)
	PLATFORM_OS = BSD
endif
ifeq ($(UNAMEOS),Darwin)
	PLATFORM_OS = OSX
endif
endif

all: $(EXE)

$(EXE): $(OBJ) | $(OUT_DIR)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -D$(PLATFORM) -o $@

$(OUT_DIR) $(OBJ_DIR):
	mkdir -p $@

install: all
	@echo "INSTALL bin/$(PROJ_NAME)"
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp out/$(PROJ_NAME) $(DESTDIR)$(PREFIX)/bin/
	chmod 755 $(DESTDIR)$(PREFIX)/bin/$(PROJ_NAME)
	# @echo "INSTALL $(PROJ_NAME).1"
	# mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	# chmod 644 $(DESTDIR)$(MANPREFIX)/man1/$(PROJ_NAME).1

uninstall:
	@echo "REMOVE bin/$(PROJ_NAME)"
	rm -f $(DESTDIR)$(PREFIX)/bin/$(PROJ_NAME)
	# @echo "REMOVE $(PROJ_NAME).1"
	# rm -f $(DESTDIR)$(MANPREFIX)/man1/$(PROJ_NAME).1

clean:
	rm -rv $(OUT_DIR) $(OBJ_DIR)

-include $(OBJ:.o=.d)
