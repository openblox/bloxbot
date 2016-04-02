AM_LDFLAGS = -module -shared

lib_LTLIBRARIES = ob.la
ob_la_SOURCES = ob.c
ob_la_LDFLAGS = -module -shared
ob_la_LIBADD = ../../src/bloxbot -lglib-2.0 -ljson-c
ob_la_CPPFLAGS = -I../../src -I/usr/include/glib-2.0 -I/usr/lib64/glib-2.0/include -I/usr/include/json-c
