UNAME_S := $(shell uname -s 2>/dev/null || echo not)

HEADERS = $(sort $(wildcard src/*.h))
SOURCES = $(sort $(wildcard src/*vido.c))
OBJECTS = $(SOURCES:.c=.o)
TARGET  = vido
DESTDIR =
PREFIX  ?= /usr/local

LDLIBS  += $(shell pkg-config --libs ncurses)

clean:
	$(MAKE) -C src clean
	$(RM) $(TARGET)

install:
	$(MAKE) $(MFLAGS) -C src
	$(CC) $(OBJECTS) $(LDLIBS) -o $(TARGET)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)

uninstall:
	$(RM) $(DESTDIR)$(PREFIX)/bin/$(TARGET)

.PHONY: all clean install src uninstall
