AM_CPPFLAGS =					\
	-I$(top_srcdir)				\
	$(SINK_CFLAGS) 				\
	$(WARN_CFLAGS)

plugin_LTLIBRARIES = libgtkosxvideosink.la

libgtkosxvideosink_la_SOURCES = gtk-osx-video-sink.c
libgtkosxvideosink_la_CFLAGS = $(SINK_CFLAGS) -xobjective-c
libgtkosxvideosink_la_LIBADD =  $(SINK_LIBS) -lgstvideo-0.10

#libgtkosxvideosink_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS) -framework Cocoa -framework OpenGL

libgstosxvideosink_HEADERS = gst-osx-video-sink.h

# Test application
noinst_PROGRAMS = test
test_SOURCES = test.c
test_CFLAGS = $(TEST_CFLAGS)
test_LDADD =  $(TEST_LIBS)
