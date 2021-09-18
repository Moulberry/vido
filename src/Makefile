UNAME_S := $(shell uname -s 2>/dev/null || echo not)

HEADERS  = $(wildcard *.h)
SOURCES  = $(wildcard *vido.c)
OBJECTS  = $(SOURCES:.c=.o)
CFLAGS   ?= -O3
CFLAGS   += -Wall

ifeq ($(DEBUG),1)
	CFLAGS := -O0 -Wall -g
endif

ifeq ($(OS),Windows_NT)
	ifeq (,$(findstring CYGWIN,$(UNAME_S)))
		CPPFLAGS += -DWIN32
	else
		CPPFLAGS += -DCYGWIN
	endif
endif

ifeq ($(UNAME_S),Linux)
	LSB_RELEASE := $(shell lsb_release -si 2>/dev/null || echo not)
	ifneq ($(filter $(LSB_RELEASE),Debian Ubuntu LinuxMint CrunchBang),)
		CPPFLAGS += -I/usr/include/ncursesw
	endif
endif

all: $(OBJECTS)

clean:
	$(RM) $(OBJECTS)
