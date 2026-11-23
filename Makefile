CC ?= cc
PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin
DATADIR = $(PREFIX)/share/windwm
CFLAGS ?= -O2 -pipe
CFLAGS += -std=c11 -Wall -Wextra -Wpedantic $(shell pkg-config --cflags x11 xft lua5.4) -DWIND_DATA_DIR='"$(DATADIR)"'
LDLIBS = $(shell pkg-config --libs x11 xft lua5.4)
ABOUT_CFLAGS = $(CFLAGS)
ABOUT_LDLIBS = $(shell pkg-config --libs x11 xft)

SOURCES := $(wildcard src/*.c)
OBJECTS := $(SOURCES:.c=.o)

all: wind wind-about
wind: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDLIBS)
wind-about: apps/wind-about.c
	$(CC) $(ABOUT_CFLAGS) -o $@ $< $(ABOUT_LDLIBS)
src/%.o: src/%.c src/wind.h
	$(CC) $(CFLAGS) -c -o $@ $<
wallpapers:
	./scripts/fetch-wallpapers.sh all

install: wind wind-about
	install -Dm755 wind $(DESTDIR)$(BINDIR)/wind
	install -Dm755 wind-about $(DESTDIR)$(BINDIR)/wind-about
	install -d $(DESTDIR)$(DATADIR)/themes $(DESTDIR)$(DATADIR)/assets
	install -m644 themes/*.lua $(DESTDIR)$(DATADIR)/themes/
	install -m644 assets/wind-logo.svg $(DESTDIR)$(DATADIR)/assets/
	@if ls assets/*.jpg >/dev/null 2>&1; then \
		install -m644 assets/*.jpg $(DESTDIR)$(DATADIR)/assets/; \
	else \
		echo "note: no downloaded wallpapers installed; run make wallpapers first"; \
	fi
	install -Dm644 data/wind.desktop $(DESTDIR)$(PREFIX)/share/xsessions/wind.desktop
	install -Dm644 data/wind-about.desktop $(DESTDIR)$(PREFIX)/share/applications/wind-about.desktop
uninstall:
	rm -f $(DESTDIR)$(BINDIR)/wind $(DESTDIR)$(BINDIR)/wind-about $(DESTDIR)$(PREFIX)/share/xsessions/wind.desktop $(DESTDIR)$(PREFIX)/share/applications/wind-about.desktop
	rm -rf $(DESTDIR)$(DATADIR)
clean:
	rm -f wind wind-about $(OBJECTS)
.PHONY: all wallpapers install uninstall clean
