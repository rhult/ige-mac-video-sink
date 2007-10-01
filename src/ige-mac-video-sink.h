/* GTK+ Mac video sink
 *
 * Copyright (C) 2007 Pioneer Research Center USA, Inc.
 * Copyright (C) 2007 Imendio AB
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; version 2.1
 * of the License.
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
 
#ifndef __GTK_MAC_VIDEO_SINK_H__
#define __GTK_MAC_VIDEO_SINK_H__

#include <gst/gst.h>
#include <gst/video/gstvideosink.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define IGE_TYPE_MAC_VIDEO_SINK            (ige_mac_video_sink_get_type())
#define IGE_MAC_VIDEO_SINK(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), IGE_TYPE_MAC_VIDEO_SINK, IgeMacVideoSink))
#define IGE_MAC_VIDEO_SINK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), IGE_TYPE_MAC_VIDEO_SINK, IgeMacVideoSinkClass))
#define IGE_IS_MAC_VIDEO_SINK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), IGE_TYPE_MAC_VIDEO_SINK))
#define IGE_IS_MAC_VIDEO_SINK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), IGE_TYPE_MAC_VIDEO_SINK))

typedef struct _IgeMacVideoSink      IgeMacVideoSink;
typedef struct _IgeMacVideoSinkClass IgeMacVideoSinkClass;

GType ige_mac_video_sink_get_type (void);

G_END_DECLS

#endif /* __IGE_MAC_VIDEO_SINK_H__ */
