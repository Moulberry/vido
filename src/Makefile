HEADERS  = $(wildcard *.h)
SOURCES  = $(wildcard *vido.c)
OBJECTS  = $(SOURCES:.c=.o)
CFLAGS   ?= -O3
CFLAGS   += -Wall
CFLAGS   += $(shell pkg-config --cflags ncurses)

all: $(OBJECTS)

clean:
	$(RM) $(OBJECTS)
