#include <gtk/gtk.h>
#include <gst/gst.h>

#include "gtk-osx-video-embed.h"

static void
bus_callback (GstBus *bus, GstMessage *message, gpointer data)
{
        gchar       *message_str;
        const gchar *message_name;
        GError      *error;
        
        if (GST_MESSAGE_TYPE (message) == GST_MESSAGE_ERROR) {
                gst_message_parse_error (message, &error, &message_str);
                g_print ("GST error: %s\n", message_str);
                g_free (error);
                g_free (message_str);
        }
        
        if (GST_MESSAGE_TYPE (message) == GST_MESSAGE_WARNING) {
                gst_message_parse_warning (message, &error, &message_str);
                g_warning ("GST warning: %s\n", message_str);
                g_free (error);
                g_free (message_str);
        }

        if (GST_MESSAGE_TYPE (message) == GST_MESSAGE_APPLICATION) {
                message_name = gst_structure_get_name (gst_message_get_structure (message));
                g_print ("messag: %s\n", message_name);
        }
}

#if 0
static GstBusSyncReply
prepare_widget (GstBus      *bus, 
                GstMessage  *message, 
                GstPipeline *pipeline)
{
        GstOSXVideoSink *sink;

        if (GST_MESSAGE_TYPE (message) != GST_MESSAGE_ELEMENT) {
                return GST_BUS_PASS;
        }

        if (!gst_structure_has_name (message->structure, "prepare-widget")) {
                return GST_BUS_PASS;
        }

        sink = GST_OSX_VIDEO_SINK (GST_MESSAGE_SRC (message));
        
        gst_osx_video_sink_set_widget (sink, area);

        gst_message_unref (message);

        return GST_BUS_DROP;
}
#endif

int
main (int argc, char **argv)
{
        GstElement *pipeline;
        GstElement *playbin;
        GstElement *video_sink;
        GstElement *audio_sink;
        GstBus     *bus;
        GtkWidget  *area;

        gst_init (&argc, &argv);
        gtk_init (&argc, &argv);

        pipeline = gst_pipeline_new ("pipeline");

        playbin = gst_element_factory_make ("playbin", "playbin");

        video_sink = gst_element_factory_make ("gtkosxvideosink", "video_sink");
        audio_sink = gst_element_factory_make ("osxaudiosink", "audio_sink");

        g_object_set (playbin,
                      "video-sink", video_sink,
                      "audio-sink", audio_sink,
                      "uri", "file:///Users/rhult/ali_512_32.mp4",
                      NULL);

        gst_bin_add (GST_BIN (pipeline), playbin);

        area = gtk_drawing_area_new ();

        // FIXME: does this help?
        gtk_widget_set_double_buffered (area, FALSE);

        // FIXME: set background to NULL

        gtk_osx_video_embed_set_widget ((gpointer)video_sink, area); //GTK_OSX_VIDEO_EMBED (video_sink), area);

        bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
        gst_bus_add_watch (bus, (GstBusFunc) bus_callback, NULL);
        //gst_bus_set_sync_handler (bus, (GstBusSyncHandler) prepare_widget, area);
        gst_object_unref (GST_OBJECT (bus));

        gst_element_set_state (pipeline, GST_STATE_PLAYING);

        gtk_main ();

        return 0;
}

