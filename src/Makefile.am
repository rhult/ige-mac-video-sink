AM_CPPFLAGS = -I$(top_srcdir) $(WARN_CFLAGS)

plugin_LTLIBRARIES = libigemacvideosink.la

# Video sink element
libigemacvideosink_la_SOURCES = 	\
	ige-mac-video-sink.c		\
	ige-mac-video-sink.h

libigemacvideosink_la_CFLAGS = $(SINK_CFLAGS) -xobjective-c
libigemacvideosink_la_LIBADD =  $(SINK_LIBS) -lgstvideo-0.10 libigemacvideoembed.la
libigemacvideosink_la_LDFLAGS = -module -avoid-version -framework Cocoa -framework OpenGL

# Embedding interface for the sink element
interface_includedir = $(includedir)
lib_LTLIBRARIES = libigemacvideoembed.la

libigemacvideoembed_la_SOURCES = 	\
	ige-mac-video-embed.c		\
	ige-mac-video-embed.h

libigemacvideoembed_la_CFLAGS = $(SINK_CFLAGS)
libigemacvideoembed_la_LIBADD =  $(SINK_LIBS)

# Test application
noinst_PROGRAMS = test-sink
test_sink_SOURCES = test-sink.c
test_sink_CFLAGS = $(TEST_CFLAGS)
test_sink_LDADD =  $(TEST_LIBS) libigemacvideoembed.la
