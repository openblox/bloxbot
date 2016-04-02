bin_PROGRAMS = bloxbot
bloxbot_SOURCES = main.c \
	bloxbot.c \
	internal.c \
	plugin.c \
	net_util.c \
	conn_plain.c \
	conn_tls.c
bloxbot_CPPFLAGS = -I/usr/include/glib-2.0 -I/usr/lib64/glib-2.0/include -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -std=gnu11
bloxbot_LDFLAGS = -export-dynamic
bloxbot_LDADD = -lgnutls -ldl -lpthread -lglib-2.0
