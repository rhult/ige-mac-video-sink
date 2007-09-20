/* GTK+ Mac video sink
 *
 * Copyright (C) 2007 ....
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

/**
 * SECTION:video-embed
 * @title: IgeMacVideoEmbed
 * @short_description: integrating a GStreamer sink with a GTK+ widget
 *
 * #IgeMacVideoEmbed is an interface that can be implemented by
 * GStreamer video sinks to display the output on a GTK+ widget.
 * 
 * In particular, the IGEMacVideoSink implements this interface.
 *
 **/

#include <config.h>
#include <gtk/gtk.h>

#include "ige-mac-video-embed.h"

GType
ige_mac_video_embed_get_type (void)
{
        static GType embed_type = 0;

        if (!embed_type) {
                const GTypeInfo embed_info = {
                        sizeof (IgeMacVideoEmbedIface),
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        0,
                        0,
                        NULL,
                };

                embed_type = g_type_register_static (G_TYPE_INTERFACE,
                                                     "IgeMacVideoEmbed", &embed_info, 0);
                g_type_interface_add_prerequisite (embed_type, G_TYPE_OBJECT);

                /* Note: in theory we should require this type. */
                /*g_type_interface_add_prerequisite (embed_type, GST_VIDEO_SINK);*/
        }

        return embed_type;
}

/**
 * ige_mac_video_embed_set_widget:
 * @embed: An #IgeMacVideoEmbed instance
 * @widget: #GtkWidget to use for video output
 * 
 * Sets the widget to use for video output by the video sink. The
 * widget's background will currently be set to black to match the
 * black background color used by the sink.
 *
 * The widget should be considered owned by the sink with regards to
 * drawing to it, so no drawing should be done after setting it as
 * embed widget.
 *
 * The video output will be scaled to fit the widget, while keeping
 * the aspect ratio. The image it centered vertically and
 * horizontally.
 **/
void
ige_mac_video_embed_set_widget (IgeMacVideoEmbed *embed,
                                GtkWidget        *widget)
{
        g_return_if_fail (IGE_IS_MAC_VIDEO_EMBED (embed));
        g_return_if_fail (widget == NULL || GTK_IS_WIDGET (widget));

        if (IGE_MAC_VIDEO_EMBED_GET_IFACE (embed)->set_widget) {
                IGE_MAC_VIDEO_EMBED_GET_IFACE (embed)->set_widget (embed, widget);
        }
}
