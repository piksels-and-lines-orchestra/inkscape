wrapper: #inkscape-dbus-wrapper.c inkscape-dbus-wrapper.h
	gcc -fPIC -c inkscape-dbus-wrapper.c $(shell pkg-config --cflags --libs glib-2.0 gobject-2.0 dbus-glib-1 dbus-1)
	ld -shared -soname libinkdbus.so.1 -o libinkdbus.so.1.0 -lc inkscape-dbus-wrapper.o
	ln -sf libinkdbus.so.1.0 libinkdbus.so
	ln -sf libinkdbus.so.1.0 libinkdbus.so.1

test: wrapper tester.c
	gcc -Wall -L. tester.c -linkdbus -o test $(shell pkg-config --cflags --libs glib-2.0 gobject-2.0)

