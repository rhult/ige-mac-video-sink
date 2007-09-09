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
 
#ifndef __GTK_OSX_VIDEO_SINK_H__
#define __GTK_OSX_VIDEO_SINK_H__

#include <gst/gst.h>
#include <gst/video/gstvideosink.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTK_TYPE_OSX_VIDEO_SINK            (gtk_osx_video_sink_get_type())
#define GTK_OSX_VIDEO_SINK(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), GTK_TYPE_OSX_VIDEO_SINK, GtkOSXVideoSink))
#define GTK_OSX_VIDEO_SINK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), GTK_TYPE_OSX_VIDEO_SINK, GtkOSXVideoSinkClass))
#define GTK_IS_OSX_VIDEO_SINK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), GTK_TYPE_OSX_VIDEO_SINK))
#define GTK_IS_OSX_VIDEO_SINK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GTK_TYPE_OSX_VIDEO_SINK))

typedef struct _GtkOSXVideoSink      GtkOSXVideoSink;
typedef struct _GtkOSXVideoSinkClass GtkOSXVideoSinkClass;

struct _GtkOSXVideoSink {
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

        NSOpenGLContext *gl_context;
        gulong           texture;
        float            f_x;
        float            f_y;
        int              init_done;
        char            *texture_buffer;
};

struct _GtkOSXVideoSinkClass {
        GstVideoSinkClass parent_class;
};

GType gtk_osx_video_sink_get_type (void);

G_END_DECLS

#endif /* __GTK_OSX_VIDEO_SINK_H__ */
