# CATCHME

.PHONY: all clean install uninstall

CC = musl-gcc
PROJ_NAME = catchme
PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man
OUT_DIR := out
SRC_DIR := src
OBJ_DIR := obj

# RELEASE or DEBUG
BUILD_MODE ?= RELEASE

# Define compiler flags:
#  -Wall                    turns on most, but not all, compiler warnings
#  -std=c99                 defines C language mode (standard C from 1999 revision)
#  -D_DEFAULT_SOURCE        required for timespec
#  -Werror=pointer-arith    catch unportable code that does direct arithmetic on void pointers
CFLAGS := -Wall -std=c99 -D_DEFAULT_SOURCE -Werror=pointer-arith
# C Pre Processor Flags
CPPFLAGS := -I. -Iinclude -Iinclude/json-c
# -L linker flags
LDFLAGS := -L/usr/lib/musl/lib/ -Llib/ -Wl,--gc-sections
# -l lib flags
LDLIBS   := -static /usr/lib/musl/lib/libc.a -lm -ljson-c

EXE := $(OUT_DIR)/$(PROJ_NAME)
SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

ifeq ($(BUILD_MODE),DEBUG)
	#  -g				include debug information on compilation
	#  additional warnings
	#  -MD				generate dependency files
	CFLAGS += -D_DEBUG -g -Wextra -Wpedantic -Wformat=2 -Wno-unused-parameter -Wshadow -Wwrite-strings -Wstrict-prototypes -Wold-style-definition -Wredundant-decls -Wnested-externs -Wmissing-include-dirs -MD
endif
ifeq ($(BUILD_MODE),RELEASE)
	#  -O3                      defines optimization level
	#  -s                       strip unnecessary symbols from build
	CFLAGS += -s -O2 -fdata-sections -ffunction-sections
endif

all: $(EXE)

$(EXE): $(OBJ) | $(OUT_DIR)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

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
	rm -fv $(EXE) $(OBJ_DIR)/*.o
	rmdir $(OBJ_DIR)

-include $(OBJ:.o=.d)
