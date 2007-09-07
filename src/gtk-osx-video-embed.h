#ifndef __GTK_OSX_VIDEO_EMBED_H__
#define __GTK_OSX_VIDEO_EMBED_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTK_TYPE_OSX_VIDEO_EMBED     (gtk_osx_video_embed_get_type ())
#define GTK_OSX_VIDEO_EMBED(obj)     (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_OSX_VIDEO_EMBED, GtkOSXVideoEmbed))
#define GTK_IS_OSX_VIDEO_EMBED(obj)  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_OSX_VIDEO_EMBED))
#define GTK_OSX_VIDEO_EMBED_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GTK_TYPE_OSX_VIDEO_EMBED, GtkOSXVideoEmbedIface))

typedef struct _GtkOSXVideoEmbed      GtkOSXVideoEmbed;
typedef struct _GtkOSXVideoEmbedIface GtkOSXVideoEmbedIface;

struct _GtkOSXVideoEmbedIface {
        GTypeInterface g_iface;

        /* Virtual functions */
        void (*set_widget) (GtkOSXVideoEmbed *embed,
                            GtkWidget        *widget);
};

GType gtk_osx_video_embed_get_type   (void);
void  gtk_osx_video_embed_set_widget (GtkOSXVideoEmbed *embed,
                                      GtkWidget        *widget);

G_END_DECLS

#endif /* __GTK_OSX_VIDEO_EMBED_H__ */
