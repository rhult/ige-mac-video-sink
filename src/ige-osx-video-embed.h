#ifndef __IGE_OSX_VIDEO_EMBED_H__
#define __IGE_OSX_VIDEO_EMBED_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define IGE_TYPE_OSX_VIDEO_EMBED     (ige_osx_video_embed_get_type ())
#define IGE_OSX_VIDEO_EMBED(obj)     (G_TYPE_CHECK_INSTANCE_CAST ((obj), IGE_TYPE_OSX_VIDEO_EMBED, IgeOSXVideoEmbed))
#define IGE_IS_OSX_VIDEO_EMBED(obj)  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IGE_TYPE_OSX_VIDEO_EMBED))
#define IGE_OSX_VIDEO_EMBED_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), IGE_TYPE_OSX_VIDEO_EMBED, IgeOSXVideoEmbedIface))

typedef struct _IgeOSXVideoEmbed      IgeOSXVideoEmbed;
typedef struct _IgeOSXVideoEmbedIface IgeOSXVideoEmbedIface;

struct _IgeOSXVideoEmbedIface {
        GTypeInterface g_iface;

        /* Virtual functions */
        void (*set_widget) (IgeOSXVideoEmbed *embed,
                            GtkWidget        *widget);
};

GType ige_osx_video_embed_get_type   (void);
void  ige_osx_video_embed_set_widget (IgeOSXVideoEmbed *embed,
                                      GtkWidget        *widget);

G_END_DECLS

#endif /* __IGE_OSX_VIDEO_EMBED_H__ */