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

//#include <unistd.h>
#include "gtk-osx-video-sink.h"

/* NOTE: We are declaring this because GTK+ doesn't install the header
 * for now.
 */
NSView * gdk_quartz_window_get_nsview (GdkWindow *window);

/* Debugging category */
GST_DEBUG_CATEGORY (gst_debug_osx_video_sink);
#define GST_CAT_DEFAULT gst_debug_osx_video_sink

/* ElementFactory information */
static const GstElementDetails gst_osx_video_sink_details =
GST_ELEMENT_DETAILS ("GTK+ OS X Video sink",
    "Sink/Video",
    "GTK+ OS X videosink",
    "Richard Hult <richard at imendio dot com>");

/* Default template - initiated with class struct to allow gst-register to work
   without X running */
static GstStaticPadTemplate gst_osx_video_sink_sink_template_factory =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw-yuv, "
        "framerate = (fraction) [ 0, MAX ], "
        "width = (int) [ 1, MAX ], "
	"height = (int) [ 1, MAX ], "
#if G_BYTE_ORDER == G_BIG_ENDIAN
       "format = (fourcc) YUY2")
#else
        "format = (fourcc) UYVY")
#endif
   );

enum
{
  ARG_0,
  ARG_EMBED,
  ARG_FULLSCREEN
};

#define ALLOC_POOL NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init]
#define RELEASE_POOL [pool release]

static GstVideoSinkClass *parent_class = NULL;

static void
osx_video_sink_init_texture (GstOSXVideoSink *sink)
{
        [sink->gl_context makeCurrentContext];

        if (sink->pi_texture) {
                glDeleteTextures (1, &sink->pi_texture);
        }

        g_print ("creating texture buffer: %d %d\n", sink->width, sink->height);

        if (sink->texture_buffer) {
                sink->texture_buffer = g_realloc (sink->texture_buffer, 
                                                  sink->width * sink->height * sizeof (short)); // short or 3byte?
        } else {
                sink->texture_buffer = g_malloc0 (sink->width * sink->height * sizeof (short));
        }

        glGenTextures (1, &sink->pi_texture);

        glEnable (GL_TEXTURE_RECTANGLE_EXT);
        glEnable (GL_UNPACK_CLIENT_STORAGE_APPLE);

        glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
        glPixelStorei (GL_UNPACK_ROW_LENGTH, sink->width);
  
        glBindTexture (GL_TEXTURE_RECTANGLE_EXT, sink->pi_texture);

        /* Use VRAM texturing */
        glTexParameteri (GL_TEXTURE_RECTANGLE_EXT,
                         GL_TEXTURE_STORAGE_HINT_APPLE, 
                         GL_STORAGE_CACHED_APPLE);

        /* Don't copy the texture data, use our buffer. */
        glPixelStorei (GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);

        /* Linear interpolation */
        glTexParameteri (GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        /* I have no idea what this exactly does, but it seems to be
         * necessary for scaling.
         */
        glTexParameteri (GL_TEXTURE_RECTANGLE_EXT,
                         GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri (GL_TEXTURE_RECTANGLE_EXT,
                         GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // glPixelStorei (GL_UNPACK_ROW_LENGTH, 0); WHY ??

        glTexImage2D (GL_TEXTURE_RECTANGLE_EXT, 0, GL_RGBA,
                      sink->width, sink->height, 0, 
                      GL_YCBCR_422_APPLE, GL_UNSIGNED_SHORT_8_8_APPLE, 
                      sink->texture_buffer);

        sink->init_done = TRUE;
}

static void
osx_video_sink_reload_texture (GstOSXVideoSink *sink)
{
        // needed?
        if (!sink->init_done) {
                return;
        }

        GST_LOG ("Reloading Texture");

        [sink->gl_context makeCurrentContext];

        glBindTexture (GL_TEXTURE_RECTANGLE_EXT, sink->pi_texture);
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
osx_video_sink_draw_quad (GstOSXVideoSink *sink)
{
        sink->f_x = 1.0;
        sink->f_y = 1.0;

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
osx_video_sink_draw (GstOSXVideoSink *sink)
{
        long params[] = { 1 };

        [sink->gl_context makeCurrentContext];

        CGLSetParameter (CGLGetCurrentContext (), kCGLCPSwapInterval, params);

        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (!sink->init_done) {
                [sink->gl_context flushBuffer];
                return;
        }

        glBindTexture (GL_TEXTURE_RECTANGLE_EXT, sink->pi_texture); // FIXME

        osx_video_sink_draw_quad (sink);

        [sink->gl_context flushBuffer];
}

static void
osx_video_sink_display_texture (GstOSXVideoSink *sink)
{
        ALLOC_POOL;

        if ([sink->view lockFocusIfCanDraw]) {
                [sink->gl_context setView:sink->view];
                
                osx_video_sink_draw (sink);
                osx_video_sink_reload_texture (sink);

                [sink->view unlockFocus];
        }

        RELEASE_POOL;
}

static NSOpenGLContext *
osx_video_sink_get_context (GstOSXVideoSink *sink)
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

                // FIXME: Could also fall back to default format?
                // format =  [[sink->view class] defaultPixelFormat];

                context = [[NSOpenGLContext alloc] initWithFormat:format shareContext:nil];
                sink->gl_context = context;

                [context makeCurrentContext];
                [context update];

                /* Black background */
                glClearColor (0.0, 0.0, 0.0, 0.0);

                sink->pi_texture = 0;
                sink->texture_buffer = NULL;
                //sink->width = frame.size.width;
                //sink->height = frame.size.height;

                GST_LOG ("Width: %d Height: %d", sink->width, sink->height);

                osx_video_sink_init_texture (sink);

                RELEASE_POOL;
        }

        return sink->gl_context;
}

#if 0
static void
osx_video_sink_destroy_context (GstOSXVideoSink *sink)
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

/*
 * Element stuff
*/

static gboolean
gst_osx_video_sink_setcaps (GstBaseSink *bsink, 
                            GstCaps     *caps)
{
  GstOSXVideoSink *osxvideosink;
  GstStructure    *structure;
  gboolean         result;
  gint             video_width, video_height;
  const GValue    *framerate;

  osxvideosink = GST_OSX_VIDEO_SINK (bsink);

  GST_DEBUG_OBJECT (osxvideosink, "caps: %" GST_PTR_FORMAT, caps);

  structure = gst_caps_get_structure (caps, 0);
  result = gst_structure_get_int (structure, "width", &video_width);
  result &= gst_structure_get_int (structure, "height", &video_height);
  framerate = gst_structure_get_value (structure, "framerate");
  result &= (framerate != NULL);

  if (!result) {
    goto beach;
  }

  osxvideosink->fps_n = gst_value_get_fraction_numerator (framerate);
  osxvideosink->fps_d = gst_value_get_fraction_denominator (framerate);

  GST_DEBUG_OBJECT (osxvideosink, "our format is: %dx%d video at %d/%d fps",
      video_width, video_height, osxvideosink->fps_n, osxvideosink->fps_d);

  GST_VIDEO_SINK_WIDTH (osxvideosink) = video_width;
  GST_VIDEO_SINK_HEIGHT (osxvideosink) = video_height;

  //gst_osx_video_sink_osxwindow_resize (osxvideosink, osxvideosink->osxwindow,
  //    video_width, video_height);
  result = TRUE;

beach:
  return result;

}

static GstStateChangeReturn
gst_osx_video_sink_change_state (GstElement     *element,
                                 GstStateChange  transition)
{
  GstOSXVideoSink *sink;

  sink = GST_OSX_VIDEO_SINK (element);

  GST_DEBUG_OBJECT (sink, "%s => %s", 
		    gst_element_state_get_name(GST_STATE_TRANSITION_CURRENT (transition)),
		    gst_element_state_get_name(GST_STATE_TRANSITION_NEXT (transition)));

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      /* No window is given to us, create our own toplevel. */
      if (!sink->window) {
        GtkWidget *toplevel;
        GtkWidget *area;

        GST_VIDEO_SINK_WIDTH (sink) = 320;
        GST_VIDEO_SINK_HEIGHT (sink) = 240;

        toplevel = gtk_window_new (GTK_WINDOW_TOPLEVEL);

        area = gtk_drawing_area_new ();
        gtk_widget_set_size_request (area, 320, 240);

        // FIXME: experiment with disabling doublebuffering on the
        // area? unsetting bg etc, things like that.

        gtk_widget_set_double_buffered (area, FALSE);

        gtk_container_add (GTK_CONTAINER (toplevel), area);

        gtk_widget_show_all (toplevel);

        gtk_widget_realize (toplevel); // FIXME
        gtk_widget_realize (area); // FIXME

        sink->toplevel = toplevel;
        sink->area = area;
        sink->window = area->window;
        sink->view = gdk_quartz_window_get_nsview (sink->window);

        osx_video_sink_get_context (sink);
      } else {
        /* Resize if we are in control of the window. */
        if (sink->area) {
          gtk_widget_set_size_request (sink->area,
                                       GST_VIDEO_SINK_WIDTH (sink),
                                       GST_VIDEO_SINK_HEIGHT (sink));
        }
      }
      break;

    case GST_STATE_CHANGE_READY_TO_PAUSED:
      GST_DEBUG ("ready to paused");
      if (sink->window)
        ;//gst_osx_video_sink_osxwindow_clear (osxvideosink,

      sink->time = 0;
      break;

    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      break;

    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      break;

    case GST_STATE_CHANGE_PAUSED_TO_READY:
      sink->fps_n = 0;
      sink->fps_d = 0;
      sink->sw_scaling_failed = FALSE;
      GST_VIDEO_SINK_WIDTH (sink) = 0;
      GST_VIDEO_SINK_HEIGHT (sink) = 0;
      break;

    case GST_STATE_CHANGE_READY_TO_NULL:
      if (sink->toplevel) {
        gtk_widget_destroy (sink->toplevel);
        sink->toplevel = NULL;
        sink->area = NULL;
        sink->window = NULL;
      }

      // FIXME: Should we leave the window if we were given one?

      break;
  }

  return (GST_ELEMENT_CLASS (parent_class))->change_state (element, transition);

}

static GstFlowReturn
gst_osx_video_sink_show_frame (GstBaseSink *bsink, 
                               GstBuffer   *buf)
{
  GstOSXVideoSink *sink;

  GST_DEBUG ("show_frame");

  sink = GST_OSX_VIDEO_SINK (bsink);

  memcpy (sink->texture_buffer, GST_BUFFER_DATA (buf), GST_BUFFER_SIZE (buf));

  osx_video_sink_display_texture (sink);

  return GST_FLOW_OK;
}

static void
gst_osx_video_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstOSXVideoSink *osxvideosink;

  g_return_if_fail (GST_IS_OSX_VIDEO_SINK (object));

  osxvideosink = GST_OSX_VIDEO_SINK (object);

  switch (prop_id) {
    case ARG_EMBED:
            //osxvideosink->embed = g_value_get_boolean (value);
      break;
    case ARG_FULLSCREEN:
            //osxvideosink->fullscreen = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_osx_video_sink_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GstOSXVideoSink *osxvideosink;

  osxvideosink = GST_OSX_VIDEO_SINK (object);

  switch (prop_id) {
    case ARG_EMBED:
            // g_value_set_boolean (value, osxvideosink->embed);
      break;
    case ARG_FULLSCREEN:
            //g_value_set_boolean (value, osxvideosink->fullscreen);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_osx_video_sink_init (GstOSXVideoSink *sink)
{
  sink->window = NULL;
  sink->toplevel = NULL;
  sink->area = NULL;

  sink->fps_n = 0;
  sink->fps_d = 0;

  sink->width = 320;
  sink->height = 240;
}

static void
gst_osx_video_sink_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_set_details (element_class, &gst_osx_video_sink_details);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_osx_video_sink_sink_template_factory));
}

static void
gst_osx_video_sink_class_init (GstOSXVideoSinkClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSinkClass *gstbasesink_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasesink_class = (GstBaseSinkClass *) klass;

  parent_class = g_type_class_ref (GST_TYPE_VIDEO_SINK);

  gobject_class->set_property = gst_osx_video_sink_set_property;
  gobject_class->get_property = gst_osx_video_sink_get_property;

  gstbasesink_class->set_caps = gst_osx_video_sink_setcaps;
  gstbasesink_class->preroll = gst_osx_video_sink_show_frame;
  gstbasesink_class->render = gst_osx_video_sink_show_frame;
  gstelement_class->change_state = gst_osx_video_sink_change_state;

  /**
   * GstOSXVideoSink:embed
   *
   * Set to #TRUE if you are embedding the video window in an application.
   *
   **/

  g_object_class_install_property (gobject_class, ARG_EMBED,
      g_param_spec_boolean ("embed", "embed", "When enabled, it  "
          "can be embedded", FALSE, G_PARAM_READWRITE));

  /**
   * GstOSXVideoSink:fullscreen
   *
   * Set to #TRUE to have the video displayed in fullscreen.
   **/

  g_object_class_install_property (gobject_class, ARG_FULLSCREEN,
      g_param_spec_boolean ("fullscreen", "fullscreen",
          "When enabled, the view  " "is fullscreen", FALSE,
          G_PARAM_READWRITE));
}

GType
gst_osx_video_sink_get_type (void)
{
  static GType osxvideosink_type = 0;

  if (!osxvideosink_type) {
    static const GTypeInfo osxvideosink_info = {
      sizeof (GstOSXVideoSinkClass),
      gst_osx_video_sink_base_init,
      NULL,
      (GClassInitFunc) gst_osx_video_sink_class_init,
      NULL,
      NULL,
      sizeof (GstOSXVideoSink),
      0,
      (GInstanceInitFunc) gst_osx_video_sink_init,
    };

    osxvideosink_type = g_type_register_static (GST_TYPE_VIDEO_SINK,
        "GstOSXVideoSink", &osxvideosink_info, 0);

  }

  return osxvideosink_type;
}

void
gst_osx_video_sink_set_widget (GstOSXVideoSink *sink, 
                               GtkWidget       *widget)
{
        // ...
}


static gboolean
plugin_init (GstPlugin * plugin)
{

  if (!gst_element_register (plugin, "gtkosxvideosink",
          GST_RANK_PRIMARY, GST_TYPE_OSX_VIDEO_SINK))
    return FALSE;

  GST_DEBUG_CATEGORY_INIT (gst_debug_osx_video_sink, "gtkosxvideosink", 0,
      "gtkosxvideosink element");

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "gtkosxvideo",
    "GTK+ OS X native video output plugin",
    plugin_init, VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
