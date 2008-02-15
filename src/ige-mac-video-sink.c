/* GTK+ Mac video sink
 *
 * Copyright (C) 2007-2008 Pioneer Research Center USA, Inc.
 * Copyright (C) 2007-2008 Imendio AB
 *
 * The code originally builds on osxvideosink by:
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
#include <gtk/gtk.h>
#include <gst/interfaces/xoverlay.h>

/* The quartz specific header is only available in 2.15.x and newer. */
#if GTK_CHECK_VERSION(2,15,0)
#include <gdk/gdkquartz.h>
#else
NSView * gdk_quartz_window_get_nsview (GdkWindow *window);
#endif

#include "ige-mac-video-sink.h"

/* Define to enable beam synced update interval. Not enabled by default
 * since it seems to trigger freezes on some hardware.
 */
/*#define BEAM_SYNC*/

GST_DEBUG_CATEGORY (debug_ige_mac_video_sink);
#define GST_CAT_DEFAULT debug_ige_mac_video_sink

static const GstElementDetails ige_mac_video_sink_details =
GST_ELEMENT_DETAILS ("GTK+ OS X Video sink",
                     "Sink/Video",
                     "GTK+ OS X videosink",
                     "Richard Hult <richard at imendio dot com>");

static GstStaticPadTemplate ige_mac_video_sink_sink_template_factory =
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
        PROP_0,
        PROP_VIDEO_WINDOW,
        PROP_VIDEO_BELOW_WINDOW,
        PROP_HAS_FRAME
};

#define IGE_ALLOC_POOL NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init]
#define IGE_RELEASE_POOL [pool release]

static GstVideoSinkClass *parent_class = NULL;

struct _IgeMacVideoSink {
        GstVideoSink     videosink;

        /* The GdkWindow to draw on. */
        GtkWidget       *widget;
        GdkWindow       *window;
        NSView          *view;

        /* When no window is given us, we create our own toplevel window
         * with a drawing area to get an NSView from.
         */
        GtkWidget       *toplevel;

        NSOpenGLContext *gl_context;
        GLuint           texture;
        char            *texture_buffer;
        gboolean         init_done;
        gboolean         has_frame;

        gboolean         needs_viewport_update;

        gboolean         video_below_window;

        /* Used for synchronizing the toplevel creation in the main
         * thread when the app doesn't provide a widget to draw on.
         */ 
        GCond           *toplevel_cond;
        GMutex          *toplevel_mutex;

        /* Used for synchronizing access to the GL context. */
        GMutex          *gl_mutex;

        NSRect           last_bounds;
};

struct _IgeMacVideoSinkClass {
        GstVideoSinkClass parent_class;
};

/* Naming convention used is *_unlocked needs the GL mutex locked by the
 * caller.
 */

static void     mac_video_sink_setup_context           (IgeMacVideoSink *sink);
static void     mac_video_sink_delete_texture_unlocked (IgeMacVideoSink *sink);
static void     mac_video_sink_teardown_context        (IgeMacVideoSink *sink);
static void     mac_video_sink_setup_viewport_unlocked (IgeMacVideoSink *sink);
static void     mac_video_sink_set_window              (IgeMacVideoSink *sink,
                                                        GdkWindow       *window);
static void     mac_video_sink_size_allocate_cb        (GtkWidget       *widget,
                                                        GtkAllocation   *allocation,
                                                        IgeMacVideoSink *sink);
static gboolean mac_video_sink_create_toplevel_idle_cb (IgeMacVideoSink *sink);

/* Must be called with the context being current. */
static void
mac_video_sink_init_texture_unlocked (IgeMacVideoSink *sink)
{
        gint width;
        gint height;

        mac_video_sink_delete_texture_unlocked (sink);

        width = GST_VIDEO_SINK_WIDTH (sink);
        height = GST_VIDEO_SINK_HEIGHT (sink);

        /* Note: We could make the buffer allocs handle OOM gracefully
         * in case the incoming buffer is really huge.
         */
        if (sink->texture_buffer) {
                sink->texture_buffer = g_realloc (sink->texture_buffer,
                                                  width * height * 3);
        } else {
                sink->texture_buffer = g_malloc0 (width * height * 3);
        }

        glGenTextures (1, &sink->texture);

        glEnable (GL_TEXTURE_RECTANGLE_EXT);
        glEnable (GL_UNPACK_CLIENT_STORAGE_APPLE);

        glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
        glPixelStorei (GL_UNPACK_ROW_LENGTH, width);

        glBindTexture (GL_TEXTURE_RECTANGLE_EXT, sink->texture);

        /* Note: those two optimizations only does something if the
         * texture's width and height is a power of 2:
         */

        /* Use VRAM texturing. */
        glTexParameteri (GL_TEXTURE_RECTANGLE_EXT,
                         GL_TEXTURE_STORAGE_HINT_APPLE,
                         GL_STORAGE_CACHED_APPLE);

        /* Don't copy the texture data, use our buffer. */
        glPixelStorei (GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);

        /* Linear interpolation for the scaling. */
        glTexParameteri (GL_TEXTURE_RECTANGLE_EXT, 
                         GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_RECTANGLE_EXT, 
                         GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexParameteri (GL_TEXTURE_RECTANGLE_EXT,
                         GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri (GL_TEXTURE_RECTANGLE_EXT,
                         GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexImage2D (GL_TEXTURE_RECTANGLE_EXT, 0, GL_RGBA,
                      width, height, 0,
                      GL_YCBCR_422_APPLE, GL_UNSIGNED_SHORT_8_8_APPLE,
                      sink->texture_buffer);
        
        sink->init_done = TRUE;
}

/* Must be called with the context being current. */
static void
mac_video_sink_reload_texture_unlocked (IgeMacVideoSink *sink)
{
        gint width;
        gint height;

        if (!sink->init_done) {
                return;
        }

        width = GST_VIDEO_SINK_WIDTH (sink);
        height = GST_VIDEO_SINK_HEIGHT (sink);

        glBindTexture (GL_TEXTURE_RECTANGLE_EXT, sink->texture);
        glPixelStorei (GL_UNPACK_ROW_LENGTH, width);

        /* glTexSubImage2D is faster than glTexImage2D
         * http://developer.apple.com/samplecode/Sample_Code/Graphics_3D/
         * TextureRange/MainOpenGLView.m.htm
         */
        glTexSubImage2D (GL_TEXTURE_RECTANGLE_EXT, 0, 0, 0,
                         width, height,
                         GL_YCBCR_422_APPLE, GL_UNSIGNED_SHORT_8_8_APPLE,
                         sink->texture_buffer);

        sink->has_frame = TRUE;
}

/* Must be called with the context being current. */
static void
mac_video_sink_draw_unlocked (IgeMacVideoSink *sink)
{
        if (sink->needs_viewport_update) {
                mac_video_sink_setup_viewport_unlocked (sink);
        }

        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (sink->init_done) {
                gint width;
                gint height;

                width = GST_VIDEO_SINK_WIDTH (sink);
                height = GST_VIDEO_SINK_HEIGHT (sink);

                glBindTexture (GL_TEXTURE_RECTANGLE_EXT, sink->texture);

                /* Draw a quad with the video frame as texture. */
                glBegin (GL_QUADS);

                /* Top left */
                glTexCoord2f (0.0, 0.0);
                glVertex2f (-1.0, 1.0);

                /* Bottom left */
                glTexCoord2f (0.0, (float) height);
                glVertex2f (-1.0, -1.0);

                /* Bottom right */
                glTexCoord2f ((float) width, (float) height);
                glVertex2f (1.0, -1.0);

                /* Top right */
                glTexCoord2f ((float) width, 0.0);
                glVertex2f (1.0, 1.0);

                glEnd ();

                [sink->gl_context flushBuffer];
        }
}

/* Touches the GL context. */
static void
mac_video_sink_display_texture (IgeMacVideoSink *sink)
{
        if (!sink->widget || !sink->view) {
                return;
        }

        /* Avoid warnings by not trying to draw on a zero size view. */
        if (sink->widget->allocation.width <= 0 ||
            sink->widget->allocation.height <= 0) {
                return;
        }

        IGE_ALLOC_POOL;

        gdk_threads_enter ();

        if ([sink->view lockFocusIfCanDraw]) {
                g_mutex_lock (sink->gl_mutex);

                [sink->gl_context makeCurrentContext];
                [sink->gl_context setView:sink->view];

                mac_video_sink_reload_texture_unlocked (sink);
                mac_video_sink_draw_unlocked (sink);

                g_mutex_unlock (sink->gl_mutex);

                [sink->view unlockFocus];
        }

        gdk_threads_leave ();

        IGE_RELEASE_POOL;
}

/* Touches the GL context. */
static void
mac_video_sink_setup_context (IgeMacVideoSink *sink)
{
        g_mutex_lock (sink->gl_mutex);

        if (!sink->gl_context) {
                NSOpenGLContext              *context;
                NSOpenGLPixelFormat          *format;
                NSOpenGLPixelFormatAttribute  attribs[] = {
                        NSOpenGLPFAAccelerated,
                        NSOpenGLPFANoRecovery,
                        NSOpenGLPFADoubleBuffer,
                        NSOpenGLPFAColorSize, 24,
                        NSOpenGLPFAAlphaSize, 8,
                        NSOpenGLPFADepthSize, 16,
                        NSOpenGLPFAWindow,
                        0
                };
#ifdef BEAM_SYNC
                const GLint parm = 1;
#endif
                GLint       order;

                IGE_ALLOC_POOL;

                format = [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];

                if (!format) {
                        GST_WARNING ("Cannot create NSOpenGLPixelFormat");
                        IGE_RELEASE_POOL;
                        return;
                }

                /* Note: We could also fall back to default format if
                 * necessary here:
                 */
                /* format =  [[sink->view class] defaultPixelFormat]; */

                context = [[NSOpenGLContext alloc] initWithFormat:format shareContext:nil];
                sink->gl_context = context;

                [context makeCurrentContext];
                [context update];

#ifdef BEAM_SYNC
                /* Use beam-synced updates. */
                [context setValues:&parm forParameter:NSOpenGLCPSwapInterval];
#endif

                /* Disable anything we don't need that might slow
                 * things down.
                 */
                glDisable (GL_ALPHA_TEST);
                glDisable (GL_DEPTH_TEST);
                glDisable (GL_SCISSOR_TEST);
                glDisable (GL_BLEND);
                glDisable (GL_DITHER);
                glDisable (GL_CULL_FACE);
                glDepthMask (GL_FALSE);
                glStencilMask (0);
                glHint (GL_TRANSFORM_HINT_APPLE, GL_FASTEST);

                /* Black background. */
                glClearColor (0.0, 0.0, 0.0, 0.0);

                if (sink->video_below_window) {
                        order = -1;
                } else {
                        order = 1;
                }
                [context setValues:&order forParameter: NSOpenGLCPSurfaceOrder];

                GST_LOG ("Size: %dx%d", 
                         GST_VIDEO_SINK_WIDTH (sink), 
                         GST_VIDEO_SINK_HEIGHT (sink));

                IGE_RELEASE_POOL;
        }

        g_mutex_unlock (sink->gl_mutex);
}

static void
mac_video_sink_delete_texture_unlocked (IgeMacVideoSink *sink)
{
        if (sink->texture) {
                glDeleteTextures (1, &sink->texture);
                sink->texture = 0;
        }

        sink->has_frame = FALSE;
}

/* Touches the GL context. */
static void
mac_video_sink_teardown_context (IgeMacVideoSink *sink)
{
        g_mutex_lock (sink->gl_mutex);

        if (sink->gl_context) {
                IGE_ALLOC_POOL;

                if (sink->view) {
                        if ([sink->gl_context view] == sink->view) {
                                [sink->gl_context clearDrawable];
                        }
                }

                mac_video_sink_delete_texture_unlocked (sink);

                [sink->gl_context release];
                sink->gl_context = nil;

                g_free (sink->texture_buffer);
                sink->texture_buffer = NULL;

                IGE_RELEASE_POOL;
        }

        g_mutex_unlock (sink->gl_mutex);
}

/* Touches the GL context. */
static gboolean
mac_video_sink_set_caps (GstBaseSink *bsink,
                         GstCaps     *caps)
{
        IgeMacVideoSink *sink;
        GstStructure    *structure;
        gboolean         result;
        gint             video_width, video_height;

        sink = IGE_MAC_VIDEO_SINK (bsink);

        GST_DEBUG_OBJECT (sink, "caps: %" GST_PTR_FORMAT, caps);

        structure = gst_caps_get_structure (caps, 0);
        result = gst_structure_get_int (structure, "width", &video_width);
        result &= gst_structure_get_int (structure, "height", &video_height);

        if (!result) {
                return FALSE;
        }

        GST_DEBUG_OBJECT (sink, "Format: %dx%d",
                          video_width, video_height);

        if (!sink->widget) {
                gst_x_overlay_prepare_xwindow_id (GST_X_OVERLAY (sink));
        }

        /* No widget is given to us, create our own toplevel. */
        if (!sink->widget) {
                /* We have to create the window from the main
                 * thread, add an idle callback and wait for
                 * it here to complete.
                 */
                gdk_threads_add_idle (
                        (GSourceFunc) mac_video_sink_create_toplevel_idle_cb, sink);

                g_mutex_lock (sink->toplevel_mutex);
                while (!sink->toplevel) {
                        g_cond_wait (sink->toplevel_cond, sink->toplevel_mutex);
                }
                g_mutex_unlock (sink->toplevel_mutex);

                mac_video_sink_setup_context (sink);

                /* We could do this, but it doesn't make a lot of sense
                 * since we don't have an id.
                 */
                /*gst_x_overlay_got_xwindow_id (GST_X_OVERLAY (sink), 0);*/
        }

        if (GST_VIDEO_SINK_WIDTH (sink) != video_width || 
            GST_VIDEO_SINK_HEIGHT (sink) != video_height) {
                GST_VIDEO_SINK_WIDTH (sink) = video_width;
                GST_VIDEO_SINK_HEIGHT (sink) = video_height;

                g_mutex_lock (sink->gl_mutex);

                [sink->gl_context makeCurrentContext];
                [sink->gl_context update];

                mac_video_sink_init_texture_unlocked (sink);

                g_mutex_unlock (sink->gl_mutex);

                sink->needs_viewport_update = TRUE;
        }

        return TRUE;
}

static void
mac_video_sink_toplevel_destroy_cb (GtkWidget       *widget,
                                    IgeMacVideoSink *sink)
{
        if (sink->toplevel) {
                sink->toplevel = NULL;
                sink->widget = NULL;
                sink->view = NULL;
                sink->init_done = FALSE;

                mac_video_sink_teardown_context (sink);
        }
}

static void
mac_video_sink_widget_setup_nsview (IgeMacVideoSink *sink,
                                    GdkWindow       *window)
{
        sink->window = window;

        /* Get rid of the default background flickering when resizing. */
        gdk_window_set_back_pixmap (window, NULL, FALSE);

        sink->view = gdk_quartz_window_get_nsview (window);

        /* Needed to get the window below option working. The window must be
         * set to non-opaque. It might be possible to only enable this when
         * video-below-window is enabled.
         */
        [[sink->view window] setOpaque:NO];
        [[sink->view window] setAlphaValue:1.0];
}

static void
mac_video_sink_widget_realize_cb (GtkWidget       *widget,
                                  IgeMacVideoSink *sink)
{
        mac_video_sink_widget_setup_nsview (sink, widget->window);
}

static gboolean
mac_video_sink_create_toplevel_idle_cb (IgeMacVideoSink *sink)
{
        GdkColor black = { 0, 0, 0, 0 };

        g_mutex_lock (sink->toplevel_mutex);

        sink->toplevel = gtk_window_new (GTK_WINDOW_TOPLEVEL);
        gtk_window_set_default_size (GTK_WINDOW (sink->toplevel), 320, 240);

        g_signal_connect (sink->toplevel,
                          "destroy",
                          G_CALLBACK (mac_video_sink_toplevel_destroy_cb),
                          sink);

        gtk_widget_modify_bg (sink->toplevel, GTK_STATE_NORMAL, &black);

        sink->widget = gtk_drawing_area_new ();
        gtk_widget_set_size_request (sink->widget, 100, 100);
        gtk_container_add (GTK_CONTAINER (sink->toplevel), sink->widget);

        g_signal_connect (sink->widget,
                          "realize",
                          G_CALLBACK (mac_video_sink_widget_realize_cb),
                          sink);

        gtk_widget_show_all (sink->toplevel);

        g_signal_connect (sink->widget,
                          "size-allocate",
                          G_CALLBACK (mac_video_sink_size_allocate_cb),
                          sink);

        g_cond_signal (sink->toplevel_cond);
        g_mutex_unlock (sink->toplevel_mutex);

        return FALSE;
}

static GstStateChangeReturn
mac_video_sink_change_state (GstElement     *element,
                             GstStateChange  transition)
{
        IgeMacVideoSink *sink;

        sink = IGE_MAC_VIDEO_SINK (element);

        GST_DEBUG_OBJECT (sink, "%s => %s",
                          gst_element_state_get_name (GST_STATE_TRANSITION_CURRENT (transition)),
                          gst_element_state_get_name (GST_STATE_TRANSITION_NEXT (transition)));

        switch (transition) {
        case GST_STATE_CHANGE_NULL_TO_READY:
                /* Resize if we are in control of the window. */
                if (sink->toplevel && sink->widget) {
                        gdk_threads_enter ();
                        gtk_widget_set_size_request (sink->widget,
                                                     GST_VIDEO_SINK_WIDTH (sink),
                                                     GST_VIDEO_SINK_HEIGHT (sink));
                        gdk_threads_leave ();
                }
                break;

        case GST_STATE_CHANGE_READY_TO_PAUSED:
                break;

        case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
                break;

        case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
                break;

        case GST_STATE_CHANGE_PAUSED_TO_READY:
                break;

        case GST_STATE_CHANGE_READY_TO_NULL:
                /* We unset the texture/buffer here, otherwise we show it
                 * when starting next time, causing flickering before
                 * showing the real first frame.
                 */
                g_mutex_lock (sink->gl_mutex);
                mac_video_sink_delete_texture_unlocked (sink);
                g_mutex_unlock (sink->gl_mutex);

                if (sink->toplevel) {
                        gdk_threads_enter ();
                        gtk_widget_destroy (sink->toplevel);

                        sink->toplevel = NULL;
                        sink->widget = NULL;
                        sink->view = NULL;

                        gdk_threads_leave ();

                        mac_video_sink_teardown_context (sink);
                }

                break;
        }

        return (GST_ELEMENT_CLASS (parent_class))->change_state (element, transition);
}

static GstFlowReturn
mac_video_sink_show_frame (GstBaseSink *bsink,
                           GstBuffer   *buf)
{
        IgeMacVideoSink *sink;

        GST_DEBUG ("show_frame");

        sink = IGE_MAC_VIDEO_SINK (bsink);

        if (!sink->init_done) {
                return GST_FLOW_UNEXPECTED;
        }

        if (!sink->texture_buffer) {
                return GST_FLOW_ERROR;
        }

        memcpy (sink->texture_buffer, GST_BUFFER_DATA (buf), GST_BUFFER_SIZE (buf));
        mac_video_sink_display_texture (sink);

        return GST_FLOW_OK;
}

/* Must be called with the context being current. */
static void
mac_video_sink_setup_viewport_unlocked (IgeMacVideoSink *sink)
{
        gint width;
        gint height;

        if (!sink->widget) {
                return;
        }

        gdk_drawable_get_size (sink->window, &width, &height);

        /* Scale the viewport to fill the drawable. */
        glViewport (0, 0, width, height);

        sink->needs_viewport_update = FALSE;
}

/* Touches the GL context. */
static void
mac_video_sink_size_allocate_cb (GtkWidget       *widget,
                                 GtkAllocation   *allocation,
                                 IgeMacVideoSink *sink)
{
        NSRect bounds;

        if (!sink->gl_context) {
                return;
        }

        bounds = [sink->view bounds];

        if (sink->last_bounds.origin.x == bounds.origin.x &&
            sink->last_bounds.origin.y == bounds.origin.y &&
            sink->last_bounds.size.width == bounds.size.width &&
            sink->last_bounds.size.height == bounds.size.height) {
                return;
        }

        g_mutex_lock (sink->gl_mutex);

        [sink->gl_context makeCurrentContext];
        [sink->gl_context update];

        mac_video_sink_setup_viewport_unlocked (sink);

        g_mutex_unlock (sink->gl_mutex);

        /* Redraw the latest frame at the new position and size. */
        if (sink->texture) {
                mac_video_sink_display_texture (sink);
        }

        sink->last_bounds = bounds;
}

static void
mac_video_sink_widget_destroy_cb (GtkWidget       *widget,
                                  IgeMacVideoSink *sink)
{
        sink->widget = NULL;
        sink->view = NULL;
        sink->init_done = FALSE;
        sink->last_bounds.size.width = -1;
        sink->last_bounds.size.height = -1;

        mac_video_sink_teardown_context (sink);
}

static void
mac_video_sink_set_window (IgeMacVideoSink *sink,
                           GdkWindow       *window)
{
        gpointer widgetptr = NULL;

        if (sink->widget) {
                g_signal_handlers_disconnect_by_func (
                        sink->widget,
                        G_CALLBACK (mac_video_sink_size_allocate_cb),
                        sink);
        }

        if (sink->toplevel) {
                g_signal_handlers_disconnect_by_func (
                        sink->widget,
                        G_CALLBACK (mac_video_sink_widget_realize_cb),
                        sink);

                gtk_widget_destroy (sink->toplevel);
                sink->toplevel = NULL;
        }

        sink->window = NULL;
        sink->widget = NULL;
        sink->view = NULL;

        gdk_window_get_user_data (window, &widgetptr);
        if (!GTK_IS_WIDGET (widgetptr)) {
                g_warning ("Passed in window doesn't seems to be a valid "
                           "GdkWindow belonging to a GtkWidget");
                return;
        }
        sink->widget = widgetptr;

        if (GTK_WIDGET_NO_WINDOW (sink->widget)) {
                g_warning ("The GTK+ OS X video sink needs a widget that has its "
                           "own window; trying anyway but there will be problems");
        }

        mac_video_sink_widget_setup_nsview (sink, window);
        mac_video_sink_setup_context (sink);

        sink->needs_viewport_update = TRUE;

        g_signal_connect (sink->widget,
                          "size-allocate",
                          G_CALLBACK (mac_video_sink_size_allocate_cb),
                          sink);

        g_signal_connect (sink->widget,
                          "destroy",
                          G_CALLBACK (mac_video_sink_widget_destroy_cb),
                          sink);
}

static void
mac_video_sink_finalize (GObject *object)
{
        IgeMacVideoSink *sink;

        sink = IGE_MAC_VIDEO_SINK (object);

        mac_video_sink_teardown_context (sink);

        g_mutex_free (sink->toplevel_mutex);
        g_cond_free (sink->toplevel_cond);

        g_mutex_free (sink->gl_mutex);

        if (G_OBJECT_CLASS (parent_class)->finalize) {
                G_OBJECT_CLASS (parent_class)->finalize (object);
        }
}

static void
mac_video_sink_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
        IgeMacVideoSink *sink;
        GdkWindow       *window;
        gboolean         below;
        GLint            order;

        sink = IGE_MAC_VIDEO_SINK (object);

        switch (prop_id) {
        case PROP_VIDEO_WINDOW:
                window = g_value_get_object (value);
                mac_video_sink_set_window (sink, window);
                break;
        case PROP_VIDEO_BELOW_WINDOW:
                below = g_value_get_boolean (value);
                if (below != sink->video_below_window) {
                        sink->video_below_window = below;
                        if (below) {
                                order = -1;
                        } else {
                                order = 1;
                        }

                        g_mutex_lock (sink->gl_mutex);
                        [sink->gl_context setValues:&order forParameter: NSOpenGLCPSurfaceOrder];
                        g_mutex_unlock (sink->gl_mutex);
                }
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
mac_video_sink_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
        IgeMacVideoSink *sink;

        sink = IGE_MAC_VIDEO_SINK (object);

        switch (prop_id) {
        case PROP_VIDEO_WINDOW:
                g_value_set_object (value, sink->window);
                break;
        case PROP_VIDEO_BELOW_WINDOW:
                g_value_set_boolean (value, sink->video_below_window);
                break;
        case PROP_HAS_FRAME:
                g_value_set_boolean (value, sink->has_frame);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
ige_mac_video_sink_init (IgeMacVideoSink *sink)
{
        sink->toplevel_cond = g_cond_new ();
        sink->toplevel_mutex = g_mutex_new ();
        sink->gl_mutex = g_mutex_new ();

        sink->video_below_window = FALSE;

        sink->last_bounds.size.width = -1;
        sink->last_bounds.size.height = -1;
}

static void
ige_mac_video_sink_base_init (gpointer g_class)
{
        GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

        gst_element_class_set_details (element_class, &ige_mac_video_sink_details);

        gst_element_class_add_pad_template (
                element_class,
                gst_static_pad_template_get (&ige_mac_video_sink_sink_template_factory));
}

static void
ige_mac_video_sink_class_init (IgeMacVideoSinkClass * klass)
{
        GObjectClass     *gobject_class;
        GstElementClass  *gstelement_class;
        GstBaseSinkClass *gstbasesink_class;

        gobject_class = (GObjectClass *) klass;
        gstelement_class = (GstElementClass *) klass;
        gstbasesink_class = (GstBaseSinkClass *) klass;

        parent_class = g_type_class_ref (GST_TYPE_VIDEO_SINK);

        gobject_class->finalize = mac_video_sink_finalize;
        gobject_class->set_property = mac_video_sink_set_property;
        gobject_class->get_property = mac_video_sink_get_property;

        gstbasesink_class->set_caps = mac_video_sink_set_caps;
        gstbasesink_class->preroll = mac_video_sink_show_frame;
        gstbasesink_class->render = mac_video_sink_show_frame;
        gstelement_class->change_state = mac_video_sink_change_state;

        g_object_class_install_property (
                gobject_class, PROP_VIDEO_WINDOW,
                g_param_spec_object ("video-window", "Video Window",
                                     "The GdkWindow to draw the video on",
                                     GDK_TYPE_WINDOW,
                                     G_PARAM_READWRITE));

        g_object_class_install_property (
                gobject_class, PROP_VIDEO_BELOW_WINDOW,
                g_param_spec_boolean ("video-below-window", "Video Below window",
                                      "Whether to keep the video display on top of the window or below it", FALSE,
                                      G_PARAM_READWRITE));

        g_object_class_install_property (
                gobject_class, PROP_HAS_FRAME,
                g_param_spec_boolean ("has-frame", "Has Frame",
                                      "Whether there is a frame to show or not", FALSE,
                                      G_PARAM_READABLE));
}

static gboolean
mac_video_sink_interface_supported (GstImplementsInterface *iface, 
                                    GType                   type)
{
        g_assert (type == GST_TYPE_X_OVERLAY);
        return TRUE;
}

static void
mac_video_sink_implements_iface_init (GstImplementsInterfaceClass *klass)
{
        klass->supported = mac_video_sink_interface_supported;
}

static void
mac_video_sink_xoverlay_iface_init (GstXOverlayClass *iface)
{
        /* Those (i.e. the entire interface) are currently not implemented
         * by this sink. But it's easier to pretend to do it than changing
         * apps at this stage.
         *
         * iface->set_xwindow_id = mac_video_sink_set_xwindow_id;
         * iface->expose = mac_video_sink_expose;
         * iface->handle_events = mac_video_sink_set_event_handling;
         */
}

GType
ige_mac_video_sink_get_type (void)
{
        static GType sink_type = 0;

        if (!sink_type) {
                const GTypeInfo sink_info = {
                        sizeof (IgeMacVideoSinkClass),
                        ige_mac_video_sink_base_init,
                        NULL,
                        (GClassInitFunc) ige_mac_video_sink_class_init,
                        NULL,
                        NULL,
                        sizeof (IgeMacVideoSink),
                        0,
                        (GInstanceInitFunc) ige_mac_video_sink_init,
                };
                const GInterfaceInfo iface_info = {
                        (GInterfaceInitFunc) mac_video_sink_implements_iface_init,
                        NULL,
                        NULL,
                };
                const GInterfaceInfo overlay_info = {
                        (GInterfaceInitFunc) mac_video_sink_xoverlay_iface_init,
                        NULL,
                        NULL,
                };

                sink_type = g_type_register_static (GST_TYPE_VIDEO_SINK,
                                                    "IgeMacVideoSink", &sink_info, 0);

                g_type_add_interface_static (sink_type, GST_TYPE_IMPLEMENTS_INTERFACE,
                                             &iface_info);
                g_type_add_interface_static (sink_type, GST_TYPE_X_OVERLAY,
                                             &overlay_info);
        }

        return sink_type;
}

static gboolean
plugin_init (GstPlugin *plugin)
{

  if (!gst_element_register (plugin, "igemacvideosink",
          GST_RANK_PRIMARY, IGE_TYPE_MAC_VIDEO_SINK))
    return FALSE;

  GST_DEBUG_CATEGORY_INIT (debug_ige_mac_video_sink, "igemacvideosink", 0,
      "igemacvideosink element");

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "igemacvideosink",
    "GTK+ Mac native video output plugin",
    plugin_init, VERSION, "LGPL", "GTK+ OS X video sink", "http://developer.imendio.com/")
