AM_LDFLAGS = -module -shared

lib_LTLIBRARIES = cmd.la
cmd_la_SOURCES = cmd.c
cmd_la_LDFLAGS = -module -shared
cmd_la_LIBADD = ../../src/bloxbot -lglib-2.0
cmd_la_CPPFLAGS = -I../../src -I/usr/include/glib-2.0 -I/usr/lib64/glib-2.0/include -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -std=gnu11
