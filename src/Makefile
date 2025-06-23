VERSION := $(shell cat VERSION)
CC = cc
EXE = sgrandr
PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin
DESKTOPDIR = $(PREFIX)/share/applications

BASE_SRC = events.c mainwindow.c sgrandr.c

GTK_CFLAGS = `pkg-config --cflags gtk+-3.0`
GTK_LIBS = `pkg-config --libs gtk+-3.0`

# X11 target
SRC_X11 = $(BASE_SRC) outputmanager-x11.c customresolutions-x11.c
OBJ_X11 = $(SRC_X11:.c=.x11.o)
CFLAGS_X11 = $(GTK_CFLAGS) -DX11 -Dpver=\"$(VERSION)\" -Wall -pedantic
LIBS_X11 = $(GTK_LIBS) -lX11 -lXrandr

# Wayland target
SRC_WAYLAND = $(BASE_SRC) outputmanager-wayland.c
OBJ_WAYLAND = $(SRC_WAYLAND:.c=.wayland.o)
CFLAGS_WAYLAND = $(GTK_CFLAGS) -Dwayland -Dpver=\"$(VERSION)\"
LIBS_WAYLAND = $(GTK_LIBS)

# Nolibs target
SRC_NOLIBS = $(BASE_SRC) outputmanager-nolibs.c customresolutions-nolibs.c
OBJ_NOLIBS = $(SRC_NOLIBS:.c=.nolibs.o)
CFLAGS_NOLIBS = $(GTK_CFLAGS) -Dnolibs -Dpver=\"$(VERSION)\"
LIBS_NOLIBS = $(GTK_LIBS)

# todo: wayland not available yet
all: x11

x11: $(OBJ_X11)
	$(CC) $(CFLAGS_X11) -o $(EXE)-x11 $(OBJ_X11) $(LIBS_X11)

wayland: $(OBJ_WAYLAND)
	$(CC) $(CFLAGS_WAYLAND) -o $(EXE)-wayland $(OBJ_WAYLAND) $(LIBS_WAYLAND)

nolibs: $(OBJ_NOLIBS)
	$(CC) $(CFLAGS_NOLIBS) -o $(EXE)-nolibs $(OBJ_NOLIBS) $(LIBS_NOLIBS)

%.x11.o: %.c
	$(CC) $(CFLAGS_X11) -c -o $@ $<

%.wayland.o: %.c
	$(CC) $(CFLAGS_WAYLAND) -c -o $@ $<

%.nolibs.o: %.c
	$(CC) $(CFLAGS_NOLIBS) -c -o $@ $<

debug-x11: $(SRC_X11)
	$(CC) $(CFLAGS_X11) -g $(SRC_X11) -o debug $(LIBS_X11)

debug-nolibs: $(SRC_NOLIBS)
	$(CC) $(CFLAGS_NOLIBS) -g $(SRC_NOLIBS) -o debug $(LIBS_NOLIBS)


test:
	./$(EXE)-nolibs --verbose

clean:
	rm -f *.o *.x11.o *.wayland.o *.nolibs.o $(EXE)-x11 $(EXE)-wayland $(EXE)-nolibs debug


install:
	install -d $(DESTDIR)$(BINDIR)
	[ -f $(EXE)-x11 ] && install -m 755 $(EXE)-x11 $(DESTDIR)$(BINDIR)/$(EXE)-x11 || true
	[ -f $(EXE)-wayland ] && install -m 755 $(EXE)-wayland $(DESTDIR)$(BINDIR)/$(EXE)-wayland || true
	[ -f $(EXE)-nolibs ] && install -m 755 $(EXE)-nolibs $(DESTDIR)$(BINDIR)/$(EXE)-nolibs || true
	install -d $(DESTDIR)$(DESKTOPDIR)
	install -m 644 $(EXE).desktop $(DESTDIR)$(DESKTOPDIR)/$(EXE).desktop
