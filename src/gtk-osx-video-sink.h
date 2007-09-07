/* GTK+ OS X video sink
 *
 * Copyright ....
 *
 * Builds heavily on osxvideosink with:
 * Copyright (C) 2004-6 Zaheer Abbas Merali <zaheerabbas at merali dot org>
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
 */
 
#ifndef __GST_OSX_VIDEO_SINK_H__
#define __GST_OSX_VIDEO_SINK_H__

#include <gst/gst.h>
#include <gst/video/gstvideosink.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GST_TYPE_OSX_VIDEO_SINK \
  (gst_osx_video_sink_get_type())
#define GST_OSX_VIDEO_SINK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_OSX_VIDEO_SINK, GstOSXVideoSink))
#define GST_OSX_VIDEO_SINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_OSX_VIDEO_SINK, GstOSXVideoSinkClass))
#define GST_IS_OSX_VIDEO_SINK(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_OSX_VIDEO_SINK))
#define GST_IS_OSX_VIDEO_SINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_OSX_VIDEO_SINK))

typedef struct _GstOSXVideoSink      GstOSXVideoSink;
typedef struct _GstOSXVideoSinkClass GstOSXVideoSinkClass;

struct _GstOSXVideoSink {
  GstVideoSink     videosink;

  /* The GdkWindow and NSView to draw on. */
  GdkWindow       *window;
  NSView          *view;

  /* When no window is given us, we create our own toplevel window
   * with a drawing area to get an NSView from.
   */
  GtkWidget       *toplevel;
  GtkWidget       *area;

  int              width;
  int              height;
  guint32          format;

  gint             fps_n;
  gint             fps_d;
  
  GstClockTime     time;
  
  gboolean         sw_scaling_failed;

  // 
  int              i_effect;
  gulong           pi_texture;
  float            f_x;
  float            f_y;
  int              init_done;
  char            *texture_buffer;
  NSOpenGLContext *gl_context;
};

struct _GstOSXVideoSinkClass {
  GstVideoSinkClass parent_class;

  /* signal callbacks */
  void (*view_created) (GstElement* element, gpointer view);
};

GType gst_osx_video_sink_get_type (void);

G_END_DECLS

#endif /* __GST_OSX_VIDEO_SINK_H__ */
