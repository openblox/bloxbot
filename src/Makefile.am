bin_PROGRAMS = bloxbot
bloxbot_SOURCES = main.c \
	bloxbot.c \
	internal.c \
	plugin.c \
	net_util.c \
	conn_plain.c \
	conn_tls.c
bloxbot_CPPFLAGS = -I/usr/include/glib-2.0 -I/usr/lib64/glib-2.0/include
bloxbot_LDADD = -lgnutls -ldl -lglib-2.0
