#include <config.h>
#include <gtk/gtk.h>

#include "ige-osx-video-embed.h"

GType
ige_osx_video_embed_get_type (void)
{
        static GType embed_type = 0;

        if (!embed_type) {
                const GTypeInfo embed_info = {
                        sizeof (IgeOSXVideoEmbedIface),
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
                                                     "IgeOSXVideoEmbed", &embed_info, 0);
                g_type_interface_add_prerequisite (embed_type, G_TYPE_OBJECT);
                /*g_type_interface_add_prerequisite (embed_type, GST_VIDEO_SINK);*/
        }

        return embed_type;
}

void
ige_osx_video_embed_set_widget (IgeOSXVideoEmbed *embed,
                                GtkWidget        *widget)
{
        g_return_if_fail (IGE_IS_OSX_VIDEO_EMBED (embed));
        g_return_if_fail (widget == NULL || GTK_IS_WIDGET (widget));

        if (IGE_OSX_VIDEO_EMBED_GET_IFACE (embed)->set_widget) {
                IGE_OSX_VIDEO_EMBED_GET_IFACE (embed)->set_widget (embed, widget);
        }
}
