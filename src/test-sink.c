/*
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
 *
 */

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gst/gst.h>

#include <gst/interfaces/xoverlay.h>

static gboolean
key_press_event_cb (GtkWidget   *widget, 
                    GdkEventKey *event, 
                    GtkWidget   *window)
{
        static gboolean fullscreen;

        switch (event->keyval) {
        case GDK_Escape:
                gtk_main_quit ();
                break;

        case GDK_f:
                fullscreen = !fullscreen;

                if (fullscreen) {
                        gtk_window_fullscreen (GTK_WINDOW (window));
                } else {
                        gtk_window_unfullscreen (GTK_WINDOW (window));
                }
                break;

        default:
                break;
        }
        
	return FALSE;
}

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
get_overlay_position (GtkWidget *window,
                      GtkWidget *overlay,
                      gint      *x,
                      gint      *y)
{
        gint border_width;
        gint width, height;

        border_width = gtk_container_get_border_width (GTK_CONTAINER (window));

        width = window->allocation.width - 2 * border_width;
        height = window->allocation.height - 2 * border_width;

        gtk_window_get_position (GTK_WINDOW (window), x, y);

        *x += border_width + (width - overlay->allocation.width) / 2;
        *y += border_width + (height - overlay->allocation.height);
}

static void
size_allocate_cb (GtkWidget     *window,
                  GtkAllocation *allocation,
                  GtkWidget     *overlay)
{
        gint x, y;

        get_overlay_position (window, overlay, &x, &y);
        gtk_window_move (GTK_WINDOW (overlay), x, y);
}

static void
setup_controls (GtkWidget *window,
                GtkWidget *control_hbox)
{
        GtkWidget *overlay;
        gint       x, y;

        overlay = gtk_window_new (GTK_WINDOW_TOPLEVEL);
        gtk_window_set_opacity (GTK_WINDOW (overlay), 0.7);
        gtk_window_set_decorated (GTK_WINDOW (overlay), FALSE);

        g_signal_connect (window,
                          "size-allocate",
                          G_CALLBACK (size_allocate_cb),
                          overlay);

        gtk_container_add (GTK_CONTAINER (overlay), control_hbox);

        gtk_window_set_transient_for (GTK_WINDOW (overlay), GTK_WINDOW (window));
        gtk_window_set_keep_above (GTK_WINDOW (overlay), TRUE);

        gtk_widget_show_all (overlay);

        get_overlay_position (window, overlay, &x, &y);
        gtk_window_move (GTK_WINDOW (overlay), x, y);
        gtk_window_set_accept_focus (GTK_WINDOW (overlay), FALSE);
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

static GstBusSyncReply
bus_sync_handler_func (GstBus     *bus,
                       GstMessage *message,
                       GtkWidget  *widget)
{
	if (GST_MESSAGE_TYPE (message) != GST_MESSAGE_ELEMENT) {
		return GST_BUS_PASS;
        }
 
	if (gst_structure_has_name (message->structure, "prepare-xwindow-id")) {
		gst_x_overlay_set_xwindow_id (GST_X_OVERLAY (GST_MESSAGE_SRC (message)),
                                              (glong) widget);
  		gst_message_unref (message);
		return GST_BUS_DROP;
	}
	
	return GST_BUS_PASS;
}

int
main (int argc, char **argv)
{
        GstElement *pipeline;
        GstElement *playbin;
        GstElement *video_sink;
        GtkWidget  *window;
        GtkWidget  *main_vbox;
        GtkWidget  *control_hbox;
        GtkWidget  *widget;
        GtkWidget  *area;
        GdkColor    black = { 0, 0, 0, 0 };
        gchar      *uri;
        GstBus     *bus;

        g_thread_init (NULL);
        gdk_threads_init ();
        gdk_threads_enter ();

        gst_init (&argc, &argv);
        gtk_init (&argc, &argv);

        pipeline = gst_pipeline_new ("pipeline");

        /* Use playbin, with the Mac audio and video sinks. */ 
        playbin = gst_element_factory_make ("playbin", "playbin");
        video_sink = gst_element_factory_make ("igemacvideosink", "video_sink");

        uri = g_strconcat ("file://",
                           g_get_home_dir (),
                           "/ali_512_32.mp4",
                           NULL);

        g_object_set (playbin,
                      "video-sink", video_sink,
                      "uri", uri,
                      NULL);

        g_free (uri);

        gst_bin_add (GST_BIN (pipeline), playbin);

        /* Setup a test window. */
        window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title (GTK_WINDOW (window), "IGE Mac Video");
        gtk_window_set_default_size (GTK_WINDOW (window), 400, 300);

        main_vbox = gtk_vbox_new (FALSE, 6);
        gtk_container_add (GTK_CONTAINER (window), main_vbox);
        gtk_container_set_border_width (GTK_CONTAINER (window), 12);
        g_signal_connect (window, 
                          "destroy",
                          G_CALLBACK (gtk_main_quit), NULL);
        g_signal_connect (window,
                          "key-press-event",
                          G_CALLBACK (key_press_event_cb),
                          window);

        widget = gtk_label_new ("<span foreground='white' size='x-large'>"
                                "Testing IGE Mac Video Sink"
                                "</span>");
        gtk_label_set_use_markup (GTK_LABEL (widget), TRUE);
        gtk_box_pack_start (GTK_BOX (main_vbox), widget, FALSE, FALSE, 0);

        /* Create the widget to display the video on. */
        area = gtk_drawing_area_new ();

        gtk_box_pack_start (GTK_BOX (main_vbox), area, TRUE, TRUE, 0);

        /* Set the smallest acceptable size we want. */
        gtk_widget_set_size_request (area, 320, 240);

        /* Add control buttons for play/pause and stop. */
        control_hbox = gtk_hbutton_box_new ();
        gtk_button_box_set_layout (GTK_BUTTON_BOX (control_hbox), 
                                   GTK_BUTTONBOX_CENTER);

        add_control_button (control_hbox, GTK_STOCK_MEDIA_PREVIOUS,
                            NULL, NULL);
        add_control_button (control_hbox, GTK_STOCK_MEDIA_STOP,
                            G_CALLBACK (stop_cb), pipeline);
        add_control_button (control_hbox, GTK_STOCK_MEDIA_PLAY,
                            G_CALLBACK (play_cb), pipeline);
        add_control_button (control_hbox, GTK_STOCK_MEDIA_NEXT,
                            NULL, NULL);

        /* Setup the drawing area widget as the widget to display the
         * video on.
         */
        //ige_mac_video_embed_set_widget (IGE_MAC_VIDEO_EMBED (video_sink), area);

        /* Set black background to make it look nice. */
        gtk_widget_modify_bg (window, GTK_STATE_NORMAL, &black);
        gtk_widget_modify_bg (area, GTK_STATE_NORMAL, &black);

        gtk_widget_show_all (window);

        setup_controls (window, control_hbox);

	bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
	//gst_bus_add_watch (bus, (GstBusFunc)bus_watch_func, player);
	gst_bus_set_sync_handler (bus, (GstBusSyncHandler) bus_sync_handler_func, area);
	gst_object_unref (bus);


        /*gst_element_set_state (pipeline, GST_STATE_PLAYING);*/

        gtk_main ();

        gdk_threads_leave ();

        return 0;
}

