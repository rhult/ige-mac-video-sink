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

#define IGE_TYPE_OSX_VIDEO_SINK            (ige_osx_video_sink_get_type())
#define IGE_OSX_VIDEO_SINK(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), IGE_TYPE_OSX_VIDEO_SINK, IgeOSXVideoSink))
#define IGE_OSX_VIDEO_SINK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), IGE_TYPE_OSX_VIDEO_SINK, IgeOSXVideoSinkClass))
#define IGE_IS_OSX_VIDEO_SINK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), IGE_TYPE_OSX_VIDEO_SINK))
#define IGE_IS_OSX_VIDEO_SINK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), IGE_TYPE_OSX_VIDEO_SINK))

typedef struct _IgeOSXVideoSink      IgeOSXVideoSink;
typedef struct _IgeOSXVideoSinkClass IgeOSXVideoSinkClass;

GType ige_osx_video_sink_get_type (void);

G_END_DECLS

#endif /* __IGE_OSX_VIDEO_SINK_H__ */
