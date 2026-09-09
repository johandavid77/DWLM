.POSIX:
.SUFFIXES:

include config.mk

# flags for compiling
DWLCPPFLAGS = -I$(SRC) -DWLR_USE_UNSTABLE -D_POSIX_C_SOURCE=200809L \
	-DVERSION=\"$(VERSION)\" $(XWAYLAND)
DWLDEVCFLAGS = -g -pedantic -Wall -Wextra -Wdeclaration-after-statement \
	-Wno-unused-parameter -Wshadow -Wunused-macros -Werror=strict-prototypes \
	-Werror=implicit -Werror=return-type -Werror=incompatible-pointer-types \
	-Wfloat-conversion

# CFLAGS / LDFLAGS
PKGS      = $(WLRROOTS) wayland-server xkbcommon libinput $(XLIBS)
DWLCFLAGS = `$(PKG_CONFIG) --cflags $(PKGS)` $(DWLCPPFLAGS) $(DWLDEVCFLAGS) $(CFLAGS)
LDLIBS    = `$(PKG_CONFIG) --libs $(PKGS)` -lm $(LIBS)

# source layout (dwlm keeps sources in src/, packaging in packaging/)
SRC = src
OBJ = $(SRC)/dwlm.o $(SRC)/util.o

all: dwlm
dwlm: $(OBJ)
	$(CC) $(OBJ) $(DWLCFLAGS) $(LDFLAGS) $(LDLIBS) -o $@
$(SRC)/dwlm.o: $(SRC)/dwlm.c $(SRC)/scroll.c $(SRC)/scroll.h $(SRC)/client.h \
	$(SRC)/config.h config.mk $(SRC)/cursor-shape-v1-protocol.h \
	$(SRC)/pointer-constraints-unstable-v1-protocol.h \
	$(SRC)/wlr-layer-shell-unstable-v1-protocol.h \
	$(SRC)/wlr-output-power-management-unstable-v1-protocol.h \
	$(SRC)/xdg-shell-protocol.h
$(SRC)/util.o: $(SRC)/util.c $(SRC)/util.h

# wayland-scanner is a tool which generates C headers and rigging for Wayland
# protocols, which are specified in XML. wlroots requires you to rig these up
# to your build system yourself and provide them in the include path.
WAYLAND_SCANNER   = `$(PKG_CONFIG) --variable=wayland_scanner wayland-scanner`
WAYLAND_PROTOCOLS = `$(PKG_CONFIG) --variable=pkgdatadir wayland-protocols`

$(SRC)/cursor-shape-v1-protocol.h:
	$(WAYLAND_SCANNER) enum-header \
		$(WAYLAND_PROTOCOLS)/staging/cursor-shape/cursor-shape-v1.xml $@
$(SRC)/pointer-constraints-unstable-v1-protocol.h:
	$(WAYLAND_SCANNER) enum-header \
		$(WAYLAND_PROTOCOLS)/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml $@
$(SRC)/wlr-layer-shell-unstable-v1-protocol.h:
	$(WAYLAND_SCANNER) enum-header \
		$(SRC)/protocols/wlr-layer-shell-unstable-v1.xml $@
$(SRC)/wlr-output-power-management-unstable-v1-protocol.h:
	$(WAYLAND_SCANNER) server-header \
		$(SRC)/protocols/wlr-output-power-management-unstable-v1.xml $@
$(SRC)/xdg-shell-protocol.h:
	$(WAYLAND_SCANNER) server-header \
		$(WAYLAND_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml $@

$(SRC)/config.h: $(SRC)/config.def.h
	cp $(SRC)/config.def.h $@
clean:
	rm -f dwlm $(SRC)/*.o $(SRC)/*protocol.h $(SRC)/config.h

dist: clean
	mkdir -p dwlm-$(VERSION)
	cp -R LICENSE* Makefile README.md CHANGELOG.md config.mk $(SRC) \
		dwlm-$(VERSION)
	tar -caf dwlm-$(VERSION).tar.gz dwlm-$(VERSION)
	rm -rf dwlm-$(VERSION)

install: dwlm
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f dwlm $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/dwlm
	mkdir -p $(DESTDIR)$(MANDIR)/man1
	cp -f $(SRC)/dwlm.1 $(DESTDIR)$(MANDIR)/man1
	chmod 644 $(DESTDIR)$(MANDIR)/man1/dwlm.1
	mkdir -p $(DESTDIR)$(DATADIR)/wayland-sessions
	cp -f $(SRC)/dwlm.desktop $(DESTDIR)$(DATADIR)/wayland-sessions/dwlm.desktop
	chmod 644 $(DESTDIR)$(DATADIR)/wayland-sessions/dwlm.desktop
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/dwlm $(DESTDIR)$(MANDIR)/man1/dwlm.1 \
		$(DESTDIR)$(DATADIR)/wayland-sessions/dwlm.desktop

.SUFFIXES: .c .o
$(SRC)/%.o: $(SRC)/%.c
	$(CC) $(CPPFLAGS) $(DWLCFLAGS) -o $@ -c $<