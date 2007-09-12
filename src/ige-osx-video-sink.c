/* GTK+ OS X video sink
 *
 * Copyright ....
 *
 * Builds heavily on osxvideosink with:
 * Copyright (C) 2004-6 Zaheer Abbas Merali <zaheerabbas at merali dot org>
 * Copyright (C) 2007 Pioneers of the Inevitable <songbird@songbirdnest.com>
 * Which also has the comment: "inspiration gained from looking at
 * source of osx video out of xine and vlc and is reflected in the
 * code"
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include <config.h>

#import <Cocoa/Cocoa.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>

#include "ige-osx-video-sink.h"
#include "ige-osx-video-embed.h"

/* NOTE: We are declaring this here for now, because GTK+ doesn't
 * install the header yet (planned but won't do it just yet).
 */
NSView * gdk_quartz_window_get_nsview (GdkWindow *window);

/* Debugging category */
GST_DEBUG_CATEGORY (debug_ige_osx_video_sink);
#define GST_CAT_DEFAULT debug_ige_osx_video_sink

/* ElementFactory information */
static const GstElementDetails ige_osx_video_sink_details =
GST_ELEMENT_DETAILS ("GTK+ OS X Video sink",
                     "Sink/Video",
                     "GTK+ OS X videosink",
                     "Richard Hult <richard at imendio dot com>");

/* Default template - initiated with class struct to allow gst-register to work
   without X running */
static GstStaticPadTemplate ige_osx_video_sink_sink_template_factory =
GST_STATIC_PAD_TEMPLATE ("sink",
                         GST_PAD_SINK,
                         GST_PAD_ALWAYS,
                         GST_STATIC_CAPS ("video/x-raw-yuv, "
                                          "framerate = (fraction) [ 0, MAX ], "
                                          "width = (int) [ 1, MAX ], "
                                          "height = (int) [ 1, MAX ], "
#if G_BYTE_ORDER == G_BIG_ENDIAN
                                          "format = (fourcc) YUY2"
#else
                                          "format = (fourcc) UYVY"
#endif
                                 ));

enum {
        ARG_0,
        ARG_FULLSCREEN
};

#define ALLOC_POOL NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init]
#define RELEASE_POOL [pool release]

static GstVideoSinkClass *parent_class = NULL;

struct _IgeOSXVideoSink {
        GstVideoSink     videosink;

        /* The GtkWidget to draw on. */
        GtkWidget       *widget;

        /* When no window is given us, we create our own toplevel window
         * with a drawing area to get an NSView from.
         */
        GtkWidget       *toplevel;

        int              width;
        int              height;

        NSOpenGLContext *gl_context;
        gulong           texture;
        float            f_x;
        float            f_y;
        int              init_done;
        char            *texture_buffer;
};

struct _IgeOSXVideoSinkClass {
        GstVideoSinkClass parent_class;
};

static void
osx_video_sink_init_texture (IgeOSXVideoSink *sink)
{
        [sink->gl_context makeCurrentContext];

        if (sink->texture) {
                glDeleteTextures (1, &sink->texture);
        }

        /* FIXME: Should make the allocs handle oom gracefully perhaps
         * for big buffers?
         */
        if (sink->texture_buffer) {
                sink->texture_buffer = g_realloc (sink->texture_buffer,
                                                  sink->width * sink->height * 3);
        } else {
                sink->texture_buffer = g_malloc0 (sink->width * sink->height * 3);
        }

        glGenTextures (1, &sink->texture);

        glEnable (GL_TEXTURE_RECTANGLE_EXT);
        glEnable (GL_UNPACK_CLIENT_STORAGE_APPLE);

        glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
        glPixelStorei (GL_UNPACK_ROW_LENGTH, sink->width);

        glBindTexture (GL_TEXTURE_RECTANGLE_EXT, sink->texture);

        /* Use VRAM texturing */
        glTexParameteri (GL_TEXTURE_RECTANGLE_EXT,
                         GL_TEXTURE_STORAGE_HINT_APPLE,
                         GL_STORAGE_CACHED_APPLE);

        /* Don't copy the texture data, use our buffer. */
        glPixelStorei (GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);

        /* Linear interpolation */
        glTexParameteri (GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        /* Copied comment: I have no idea what this exactly does, but
         * it seems to be necessary for scaling.
         */
        glTexParameteri (GL_TEXTURE_RECTANGLE_EXT,
                         GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri (GL_TEXTURE_RECTANGLE_EXT,
                         GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Copied from the original sink:
        //glPixelStorei (GL_UNPACK_ROW_LENGTH, 0); //WHY?

        glTexImage2D (GL_TEXTURE_RECTANGLE_EXT, 0, GL_RGBA,
                      sink->width, sink->height, 0,
                      GL_YCBCR_422_APPLE, GL_UNSIGNED_SHORT_8_8_APPLE,
                      sink->texture_buffer);

        sink->init_done = TRUE;
}

static void
osx_video_sink_reload_texture (IgeOSXVideoSink *sink)
{
        if (!sink->init_done) {
                return;
        }

        GST_LOG ("Reloading Texture");

        [sink->gl_context makeCurrentContext];

        glBindTexture (GL_TEXTURE_RECTANGLE_EXT, sink->texture);
        glPixelStorei (GL_UNPACK_ROW_LENGTH, sink->width);

        /* glTexSubImage2D is faster than glTexImage2D
         * http://developer.apple.com/samplecode/Sample_Code/Graphics_3D/
         * TextureRange/MainOpenGLView.m.htm
         */
        glTexSubImage2D (GL_TEXTURE_RECTANGLE_EXT, 0, 0, 0,
                         sink->width, sink->height,
                         GL_YCBCR_422_APPLE, GL_UNSIGNED_SHORT_8_8_APPLE,
                         sink->texture_buffer); //FIXME
}

static void
osx_video_sink_draw_quad (IgeOSXVideoSink *sink)
{
        //sink->f_x = 1.0;
        //sink->f_y = 1.0;

        glBegin (GL_QUADS);

        /* Top left */
        glTexCoord2f (0.0, 0.0);
        glVertex2f (-sink->f_x, sink->f_y);

        /* Bottom left */
        glTexCoord2f (0.0, (float) sink->height);
        glVertex2f (-sink->f_x, -sink->f_y);

        /* Bottom right */
        glTexCoord2f ((float) sink->width, (float) sink->height);
        glVertex2f (sink->f_x, -sink->f_y);

        /* Top right */
        glTexCoord2f ((float) sink->width, 0.0);
        glVertex2f (sink->f_x, sink->f_y);

        glEnd ();
}

static void
osx_video_sink_draw (IgeOSXVideoSink *sink)
{
        long params[] = { 1 };

        [sink->gl_context makeCurrentContext];

        CGLSetParameter (CGLGetCurrentContext (), kCGLCPSwapInterval, params);

        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (!sink->init_done) {
                [sink->gl_context flushBuffer];
                return;
        }

        glBindTexture (GL_TEXTURE_RECTANGLE_EXT, sink->texture); // FIXME

        osx_video_sink_draw_quad (sink);

        [sink->gl_context flushBuffer];
}

static void
osx_video_sink_display_texture (IgeOSXVideoSink *sink)
{
        NSView *view;

        ALLOC_POOL;

        if (!sink->widget || !sink->widget->window) {
                return;
        }

        /* Avoid warnings by not trying to draw on a zero size view. */
        if (sink->widget->allocation.width <= 0 ||
            sink->widget->allocation.height <= 0) {
                return;
        }

        view = gdk_quartz_window_get_nsview (sink->widget->window);

        if ([view lockFocusIfCanDraw]) {
                [sink->gl_context setView:view];

                osx_video_sink_draw (sink);
                osx_video_sink_reload_texture (sink);

                [view unlockFocus];
        }

        RELEASE_POOL;
}

static NSOpenGLContext *
osx_video_sink_get_context (IgeOSXVideoSink *sink)
{
        if (!sink->gl_context) {
                NSOpenGLContext              *context;
                NSOpenGLPixelFormat          *format;
                NSOpenGLPixelFormatAttribute  attribs[] = {
                        NSOpenGLPFAAccelerated,
                        NSOpenGLPFANoRecovery,
                        NSOpenGLPFADoubleBuffer,
                        NSOpenGLPFAColorSize, 24,
                        NSOpenGLPFAAlphaSize, 8,
                        NSOpenGLPFADepthSize, 24,
                        NSOpenGLPFAWindow,
                        0
                };

                ALLOC_POOL;

                format = [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];

                if (!format) {
                        GST_WARNING ("Cannot create NSOpenGLPixelFormat");
                        RELEASE_POOL;
                        return NULL;
                }

                /* FIXME: Could also fall back to default format? */
                /* format =  [[sink->view class] defaultPixelFormat]; */

                context = [[NSOpenGLContext alloc] initWithFormat:format shareContext:nil];
                sink->gl_context = context;

                [context makeCurrentContext];
                [context update];

                /* Black background */
                glClearColor (0.0, 0.0, 0.0, 0.0);

                sink->texture = 0;
                sink->texture_buffer = NULL;
                //sink->width = frame.size.width;
                //sink->height = frame.size.height;

                GST_LOG ("Width: %d Height: %d", sink->width, sink->height);

                sink->init_done = FALSE;
                osx_video_sink_init_texture (sink);

                RELEASE_POOL;
        }

        return sink->gl_context;
}

#if 0
static void
osx_video_sink_destroy_context (IgeOSXVideoSink *sink)
{
        if (sink->gl_context) {
                if ([sink->gl_context view] == sink->view) {
                        [sink->gl_context clearDrawable];
                }
                [sink->gl_context release];
                sink->gl_context = nil;
        }
}
#endif

static gboolean
osx_video_sink_setcaps (GstBaseSink *bsink,
                        GstCaps     *caps)
{
        IgeOSXVideoSink *sink;
        GstStructure    *structure;
        gboolean         result;
        gint             video_width, video_height;
        const GValue    *framerate;

        sink = IGE_OSX_VIDEO_SINK (bsink);

        GST_DEBUG_OBJECT (sink, "caps: %" GST_PTR_FORMAT, caps);

        structure = gst_caps_get_structure (caps, 0);
        result = gst_structure_get_int (structure, "width", &video_width);
        result &= gst_structure_get_int (structure, "height", &video_height);
        framerate = gst_structure_get_value (structure, "framerate");
        result &= (framerate != NULL);

        if (!result) {
                return FALSE;
        }

        GST_DEBUG_OBJECT (sink, "our format is: %dx%d video",
                          video_width, video_height);

        GST_VIDEO_SINK_WIDTH (sink) = video_width;
        GST_VIDEO_SINK_HEIGHT (sink) = video_height;

        return TRUE;
}

#if 0
/* Sends a message to the bus to let the app provide a widget. If the
 * app doesn't, we create a toplevel window containing a drawing area
 * ourselves (mostly for demos and gst-launch testing).
 */
static void
osx_video_sink_prepare_widget (IgeOSXVideoSink *sink)
{
    GstStructure *s;
    GstMessage   *msg;

    s = gst_structure_new ("prepare-widget",
			   NULL);

    GST_DEBUG_OBJECT (sink, "Sending message 'prepare-widget'");

    msg = gst_message_new_element (GST_OBJECT (sink), s);
    gst_element_post_message (GST_ELEMENT (sink), msg);

    GST_LOG_OBJECT (sink, "'prepare-widget' message sent");
}
#endif

static GstStateChangeReturn
osx_video_sink_change_state (GstElement     *element,
                             GstStateChange  transition)
{
        IgeOSXVideoSink *sink;

        sink = IGE_OSX_VIDEO_SINK (element);

        GST_DEBUG_OBJECT (sink, "%s => %s",
                          gst_element_state_get_name (GST_STATE_TRANSITION_CURRENT (transition)),
                          gst_element_state_get_name (GST_STATE_TRANSITION_NEXT (transition)));

        switch (transition) {
        case GST_STATE_CHANGE_NULL_TO_READY:
                /* No widget is given to us, create our own toplevel. */
                if (!sink->widget) {
                        GtkWidget *toplevel;
                        GtkWidget *area;

                        GST_VIDEO_SINK_WIDTH (sink) = 320;
                        GST_VIDEO_SINK_HEIGHT (sink) = 240;

                        toplevel = gtk_window_new (GTK_WINDOW_TOPLEVEL);

                        area = gtk_drawing_area_new ();
                        gtk_widget_set_size_request (area, 320, 240);

                        gtk_container_add (GTK_CONTAINER (toplevel), area);
                        gtk_widget_show_all (toplevel);

                        sink->toplevel = toplevel;
                        sink->widget = area;

                        osx_video_sink_get_context (sink);
                } else {
                        /* Resize if we are in control of the window. */
                        if (sink->toplevel && sink->widget) {
                                gtk_widget_set_size_request (sink->widget,
                                                             GST_VIDEO_SINK_WIDTH (sink),
                                                             GST_VIDEO_SINK_HEIGHT (sink));
                        }
                }
                break;

        case GST_STATE_CHANGE_READY_TO_PAUSED:
                GST_DEBUG ("ready to paused");
                if (sink->widget)
                        ;//osx_video_sink_clear (sink);

                break;

        case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
                break;

        case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
                break;

        case GST_STATE_CHANGE_PAUSED_TO_READY:
                GST_VIDEO_SINK_WIDTH (sink) = 0;
                GST_VIDEO_SINK_HEIGHT (sink) = 0;
                break;

        case GST_STATE_CHANGE_READY_TO_NULL:
                /* Do we need to do anything here really? */
                if (sink->toplevel) {
                        gtk_widget_destroy (sink->toplevel);
                        sink->toplevel = NULL;
                        sink->widget = NULL;
                }
                break;
        }

        return (GST_ELEMENT_CLASS (parent_class))->change_state (element, transition);
}

static GstFlowReturn
osx_video_sink_show_frame (GstBaseSink *bsink,
                           GstBuffer   *buf)
{
        IgeOSXVideoSink *sink;

        GST_DEBUG ("show_frame");

        sink = IGE_OSX_VIDEO_SINK (bsink);

        memcpy (sink->texture_buffer, GST_BUFFER_DATA (buf), GST_BUFFER_SIZE (buf));

        osx_video_sink_display_texture (sink);

        return GST_FLOW_OK;
}

static void
osx_video_sink_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
        IgeOSXVideoSink *sink;

        sink = IGE_OSX_VIDEO_SINK (object);

        switch (prop_id) {
        case ARG_FULLSCREEN:
                //sink->fullscreen = g_value_get_boolean (value);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
osx_video_sink_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
        IgeOSXVideoSink *sink;

        sink = IGE_OSX_VIDEO_SINK (object);

        switch (prop_id) {
        case ARG_FULLSCREEN:
                //g_value_set_boolean (value, sink->fullscreen);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
ige_osx_video_sink_init (IgeOSXVideoSink *sink)
{
        sink->toplevel = NULL;
        sink->widget = NULL;

        sink->width = 320;
        sink->height = 240;

        sink->f_x = 1.0;
        sink->f_y = 1.0;
}

static void
osx_video_sink_set_viewport (IgeOSXVideoSink *sink,
                             gint             width,
                             gint             height)
{
        gdouble in_f;
        gdouble out_f;

        in_f = (gdouble) GST_VIDEO_SINK_HEIGHT (sink) / (gdouble) GST_VIDEO_SINK_WIDTH (sink);
        out_f = (gdouble) height / (gdouble) width;

        /* Keep aspect ratio. */
        if (in_f < out_f) {
                glViewport (0, 0,
                            width,
                            width * in_f);
        } else {
                glViewport (0, 0,
                            height / in_f,
                            height);
        }

        [sink->gl_context makeCurrentContext];
        [sink->gl_context update];
}

static void
osx_video_sink_size_allocate_cb (GtkWidget       *widget,
                                 GtkAllocation   *allocation,
                                 IgeOSXVideoSink *sink)
{
        osx_video_sink_set_viewport (sink,
                                     allocation->width,
                                     allocation->height);
}

/* FIXME: Add teardown too. */
static void
osx_video_sink_setup_size_handling (IgeOSXVideoSink *sink)
{
        g_signal_connect (sink->widget,
                          "size-allocate",
                          G_CALLBACK (osx_video_sink_size_allocate_cb),
                          sink);
}

static void
osx_video_sink_set_widget (IgeOSXVideoEmbed *embed,
                           GtkWidget        *widget)
{
        IgeOSXVideoSink *sink;

        sink = IGE_OSX_VIDEO_SINK (embed);

        if (sink->toplevel) {
                gtk_widget_destroy (sink->toplevel);
        }

        sink->toplevel = NULL;
        sink->widget = NULL;

        if (widget) {
                sink->widget = widget;
                osx_video_sink_get_context (sink);
                osx_video_sink_setup_size_handling (sink);
        }
}

static void
ige_osx_video_sink_base_init (gpointer g_class)
{
        GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

        gst_element_class_set_details (element_class, &ige_osx_video_sink_details);

        gst_element_class_add_pad_template (
                element_class,
                gst_static_pad_template_get (&ige_osx_video_sink_sink_template_factory));
}

static void
ige_osx_video_sink_class_init (IgeOSXVideoSinkClass * klass)
{
        GObjectClass     *gobject_class;
        GstElementClass  *gstelement_class;
        GstBaseSinkClass *gstbasesink_class;

        gobject_class = (GObjectClass *) klass;
        gstelement_class = (GstElementClass *) klass;
        gstbasesink_class = (GstBaseSinkClass *) klass;

        parent_class = g_type_class_ref (GST_TYPE_VIDEO_SINK);

        gobject_class->set_property = osx_video_sink_set_property;
        gobject_class->get_property = osx_video_sink_get_property;

        gstbasesink_class->set_caps = osx_video_sink_setcaps;
        gstbasesink_class->preroll = osx_video_sink_show_frame;
        gstbasesink_class->render = osx_video_sink_show_frame;
        gstelement_class->change_state = osx_video_sink_change_state;

        /**
         * IgeOSXVideoSink:fullscreen
         *
         * Set to #TRUE to have the video displayed in fullscreen.
         **/
        g_object_class_install_property (
                gobject_class, ARG_FULLSCREEN,
                g_param_spec_boolean ("fullscreen", "fullscreen",
                                      "When enabled, the view is fullscreen", FALSE,
                                      G_PARAM_READWRITE));
}

static void
ige_osx_video_embed_iface_init (IgeOSXVideoEmbedIface *iface)
{
        iface->set_widget = osx_video_sink_set_widget;
}

GType
ige_osx_video_sink_get_type (void)
{
        static GType sink_type = 0;

        if (!sink_type) {
                const GTypeInfo sink_info = {
                        sizeof (IgeOSXVideoSinkClass),
                        ige_osx_video_sink_base_init,
                        NULL,
                        (GClassInitFunc) ige_osx_video_sink_class_init,
                        NULL,
                        NULL,
                        sizeof (IgeOSXVideoSink),
                        0,
                        (GInstanceInitFunc) ige_osx_video_sink_init,
                };

                const GInterfaceInfo embed_info = {
                        (GInterfaceInitFunc) ige_osx_video_embed_iface_init,
                        NULL,
                        NULL,
                };

                sink_type = g_type_register_static (GST_TYPE_VIDEO_SINK,
                                                    "IgeOSXVideoSink", &sink_info, 0);

                g_type_add_interface_static (sink_type, IGE_TYPE_OSX_VIDEO_EMBED,
                                             &embed_info);
        }

        return sink_type;
}

static gboolean
plugin_init (GstPlugin *plugin)
{

  if (!gst_element_register (plugin, "igeosxvideosink",
          GST_RANK_PRIMARY, IGE_TYPE_OSX_VIDEO_SINK))
    return FALSE;

  GST_DEBUG_CATEGORY_INIT (debug_ige_osx_video_sink, "igeosxvideosink", 0,
      "igeosxvideosink element");

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "igeosxvideosink",
    "GTK+ OS X native video output plugin",
    plugin_init, VERSION, "LGPL", "GTK+ OS X video sink", "http://developer.imendio.com")
