AM_CPPFLAGS = -I$(top_srcdir) $(WARN_CFLAGS)

plugin_LTLIBRARIES = libigeosxvideosink.la

libigeosxvideosink_la_SOURCES = ige-osx-video-sink.c
libigeosxvideosink_la_CFLAGS = $(SINK_CFLAGS) -xobjective-c
libigeosxvideosink_la_LIBADD =  $(SINK_LIBS) -lgstvideo-0.10 libigeosxvideoembed.la
libigeosxvideosink_la_LDFLAGS = -module -avoid-version -framework Cocoa -framework OpenGL

noinst_HEADERS = ige-osx-video-sink.h

interface_includedir = $(includedir)
lib_LTLIBRARIES = libigeosxvideoembed.la

libigeosxvideoembed_la_SOURCES = ige-osx-video-embed.c
libigeosxvideoembed_la_CFLAGS = $(SINK_CFLAGS)
libigeosxvideoembed_la_LIBADD =  $(SINK_LIBS)

# Test application
noinst_PROGRAMS = test
test_SOURCES = test.c
test_CFLAGS = $(TEST_CFLAGS)
test_LDADD =  $(TEST_LIBS) libigeosxvideoembed.la
