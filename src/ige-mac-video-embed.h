/* GTK+ Mac video sink
 *
 * Copyright (C) 2007 Pioneer Research Center USA, Inc.
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

#ifndef __IGE_MAC_VIDEO_EMBED_H__
#define __IGE_MAC_VIDEO_EMBED_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define IGE_TYPE_MAC_VIDEO_EMBED     (ige_mac_video_embed_get_type ())
#define IGE_MAC_VIDEO_EMBED(obj)     (G_TYPE_CHECK_INSTANCE_CAST ((obj), IGE_TYPE_MAC_VIDEO_EMBED, IgeMacVideoEmbed))
#define IGE_IS_MAC_VIDEO_EMBED(obj)  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IGE_TYPE_MAC_VIDEO_EMBED))
#define IGE_MAC_VIDEO_EMBED_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), IGE_TYPE_MAC_VIDEO_EMBED, IgeMacVideoEmbedIface))

/**
 * IgeMacVideoEmbed:
 *
 * Opque struct that represents the embed interface instance
 **/
typedef struct _IgeMacVideoEmbed      IgeMacVideoEmbed;
typedef struct _IgeMacVideoEmbedIface IgeMacVideoEmbedIface;

struct _IgeMacVideoEmbedIface {
        GTypeInterface g_iface;

        /* Virtual functions */
        void (*set_widget) (IgeMacVideoEmbed *embed,
                            GtkWidget        *widget);
};

GType ige_mac_video_embed_get_type   (void);
void  ige_mac_video_embed_set_widget (IgeMacVideoEmbed *embed,
                                      GtkWidget        *widget);

G_END_DECLS

#endif /* __IGE_MAC_VIDEO_EMBED_H__ */
