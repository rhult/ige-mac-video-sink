#include <config.h>

#include "gtk-osx-video-embed.h"

GType
gtk_osx_video_embed_get_type (void)
{
        static GType embed_type = 0;

        if (!embed_type) {
                const GTypeInfo embed_info = {
                        sizeof (GtkOSXVideoEmbedIface),
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
                                                     "GtkOSXVideoEmbed", &embed_info, 0);
                g_type_interface_add_prerequisite (embed_type, G_TYPE_OBJECT);
                //g_type_interface_add_prerequisite (embed_type, GST_VIDEO_SINK);
        }

        return embed_type;
}

void
gtk_osx_video_embed_set_widget (GtkOSXVideoEmbed *embed,
                                GtkWidget        *widget)
{
        GtkOSXVideoEmbedIface *iface;

        g_return_if_fail (embed != NULL);
        g_return_if_fail (GTK_IS_OSX_VIDEO_EMBED (embed));

        iface = GTK_OSX_VIDEO_EMBED_GET_IFACE (embed);

        if (iface->set_widget) {
                iface->set_widget (embed, widget);
        }
}
