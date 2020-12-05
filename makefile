src = $(wildcard *.c)
obj = $(src:.c=.o)

LDFLAGS = -lwayland-client

sway-switcher: xdg-shell.h xdg-shell.c $(obj) 
	$(CC) -std=c17 -o $@ $^ $(LDFLAGS)

xdg-shell.h: /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml
	wayland-scanner client-header $< xdg-shell.h

xdg-shell.c: /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml
	wayland-scanner private-code $< xdg-shell.c

.PHONY: clean
clean:
	rm -f xdg-shell.c xdg-shell.h $(obj) sway-switcher
