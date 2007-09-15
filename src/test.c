#include <gtk/gtk.h>
#include <gst/gst.h>

#include "ige-osx-video-embed.h"

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

static void
add_control_button (GtkWidget   *box, 
                    const gchar *stock,
                    GCallback    callback,
                    gpointer     user_data)
{
        GtkWidget *image;
        GtkWidget *button;
        GdkColor   blue = { 0, 0x5555, 0x9898, 0xd7d7 };

        image = gtk_image_new_from_stock (stock, GTK_ICON_SIZE_BUTTON);
        button = gtk_button_new ();
        gtk_container_add (GTK_CONTAINER (button), image);

        gtk_widget_modify_bg (button, GTK_STATE_NORMAL, &blue);
        gtk_widget_modify_bg (button, GTK_STATE_PRELIGHT, &blue);
        gtk_widget_modify_bg (button, GTK_STATE_ACTIVE, &blue);

        GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);

        gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);

        if (callback) {
                g_signal_connect (button, "clicked", callback, user_data);
        }
}

static void
stop_cb (GtkWidget  *button,
         GstElement *pipeline)
{
        gst_element_set_state (pipeline, GST_STATE_READY);
}

static void
play_cb (GtkWidget  *button,
         GstElement *pipeline)
{
        GstState state;

        gst_element_get_state (pipeline, 
                               &state, NULL,
                               GST_CLOCK_TIME_NONE);

        if (state != GST_STATE_PLAYING) {
                gst_element_set_state (pipeline, GST_STATE_PLAYING);
        } else {
                gst_element_set_state (pipeline, GST_STATE_PAUSED);
        }
}

int
main (int argc, char **argv)
{
        GstElement *pipeline;
        GstElement *playbin;
        GstElement *video_sink;
        GstElement *audio_sink;
        GtkWidget  *window;
        GtkWidget  *main_vbox;
        GtkWidget  *control_hbox;
        GtkWidget  *widget;
        GtkWidget  *area;
        GdkColor    black = { 0, 0, 0, 0 };

        gst_init (&argc, &argv);
        gtk_init (&argc, &argv);

        pipeline = gst_pipeline_new ("pipeline");

        playbin = gst_element_factory_make ("playbin", "playbin");

        video_sink = gst_element_factory_make ("igeosxvideosink", "video_sink");
        audio_sink = gst_element_factory_make ("osxaudiosink", "audio_sink");

        g_object_set (playbin,
                      "video-sink", video_sink,
                      "audio-sink", audio_sink,
                      "uri", "file:///Users/rhult/ali_512_32.mp4",
                      NULL);

        gst_bin_add (GST_BIN (pipeline), playbin);

        /* Setup a test window. */
        window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
        main_vbox = gtk_vbox_new (FALSE, 6);
        gtk_container_add (GTK_CONTAINER (window), main_vbox);
        gtk_container_set_border_width (GTK_CONTAINER (window), 12);
        g_signal_connect (window, 
                          "destroy",
                          G_CALLBACK (gtk_main_quit), NULL);

        gtk_widget_modify_bg (window, GTK_STATE_NORMAL, &black);

        widget = gtk_label_new ("<big><span foreground='white'>"
                                "Testing GTK+ OS X video sink"
                                "</span></big>");
        gtk_label_set_use_markup (GTK_LABEL (widget), TRUE);
        gtk_box_pack_start (GTK_BOX (main_vbox), widget, FALSE, FALSE, 0);

        area = gtk_drawing_area_new ();
        gtk_box_pack_start (GTK_BOX (main_vbox), area, TRUE, TRUE, 0);
        gtk_widget_set_size_request (area, 320, 240);
        gtk_widget_modify_bg (area, GTK_STATE_NORMAL, &black);

        control_hbox = gtk_hbutton_box_new ();
        gtk_button_box_set_layout (GTK_BUTTON_BOX (control_hbox), 
                                   GTK_BUTTONBOX_CENTER);
        gtk_box_pack_start (GTK_BOX (main_vbox), control_hbox, FALSE, FALSE, 0);

        add_control_button (control_hbox, GTK_STOCK_MEDIA_PREVIOUS,
                            NULL, NULL);
        add_control_button (control_hbox, GTK_STOCK_MEDIA_STOP,
                            G_CALLBACK (stop_cb), pipeline);
        add_control_button (control_hbox, GTK_STOCK_MEDIA_PLAY,
                            G_CALLBACK (play_cb), pipeline);
        add_control_button (control_hbox, GTK_STOCK_MEDIA_NEXT,
                            NULL, NULL);

        ige_osx_video_embed_set_widget (IGE_OSX_VIDEO_EMBED (video_sink), area);

        gtk_widget_show_all (window);

        gst_element_set_state (pipeline, GST_STATE_PLAYING);

        gtk_main ();

        return 0;
}

