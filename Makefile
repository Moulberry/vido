UNAME_S := $(shell uname -s 2>/dev/null || echo not)

SOURCES = $(sort $(wildcard src/*.c))
OBJECTS = $(SOURCES:.c=.o)
TARGET  = vido
DESTDIR =
PREFIX  ?= /usr/local

CURSES  = ncursesw
LDFLAGS ?= -s

ifeq (Windows_NT,$(OS))
	ifeq (,$(findstring CYGWIN,$(UNAME_S)))
		CURSES := pdcurses
	endif
endif

ifeq ($(UNAME_S),Darwin)
	CURSES := ncurses
	LDFLAGS :=
endif

ifeq ($(DEBUG),1)
	LDFLAGS :=
endif

LDLIBS   = -l$(CURSES)

all: $(TARGET)

$(TARGET): src
	$(CC) $(OBJECTS) $(LDLIBS) $(LDFLAGS) -o $(TARGET)

src:
	$(MAKE) $(MFLAGS) -C src

clean:
	$(MAKE) -C src clean
	$(RM) $(TARGET)

install:
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	#install -d $(DESTDIR)$(PREFIX)/share/man/man1
	#install -m 644 mdp.1 $(DESTDIR)$(PREFIX)/share/man/man1/$(TARGET).1

uninstall:
	$(RM) $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	#$(RM) $(DESTDIR)$(PREFIX)/share/man/man1/$(TARGET).1

.PHONY: all clean install src uninstall
