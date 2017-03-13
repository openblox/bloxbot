AM_LDFLAGS = -module -shared

lib_LTLIBRARIES = obus.la
obus_la_SOURCES = obus.c
obus_la_LDFLAGS = -module -shared
obus_la_LIBADD = ../../src/bloxbot -lzmq -ljson-c
obus_la_CPPFLAGS = -I../../src -std=gnu11 -I/usr/include/json-c
