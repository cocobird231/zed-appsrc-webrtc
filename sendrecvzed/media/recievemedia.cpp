#include "recievemedia.h"

static void handle_media_stream(GstPad *pad, GstElement *pipe, const char *convert_name, const char *sink_name);
static void on_incoming_decodebin_stream(GstElement *decodebin, GstPad *pad, GstElement *pipe);

//"webtrc.pad ! decodebin "
extern void on_incoming_stream(GstElement *webrtc, GstPad *webrtc_pad, GstElement *pipe)
{
    GstElement *decodebin;
    GstPad *sinkpad;

    if (GST_PAD_DIRECTION(webrtc_pad) != GST_PAD_SRC)
        return;

    decodebin = gst_element_factory_make("decodebin", NULL);
    g_assert_nonnull(decodebin);
    g_signal_connect(decodebin, "pad-added", G_CALLBACK(on_incoming_decodebin_stream), pipe);
    gst_bin_add(GST_BIN(pipe), decodebin);
    gst_element_sync_state_with_parent(decodebin);

    sinkpad = gst_element_get_static_pad(decodebin, "sink");
    gst_pad_link(webrtc_pad, sinkpad);
    gst_object_unref(sinkpad);
}

static void on_incoming_decodebin_stream(GstElement *decodebin, GstPad *pad, GstElement *pipe)
{
    GstCaps *caps;
    const gchar *name;

    if (!gst_pad_has_current_caps(pad))
    {
        g_printerr("Pad '%s' has no caps, can't do anything, ignoring\n", GST_PAD_NAME(pad));
        return;
    }

    caps = gst_pad_get_current_caps(pad);
    name = gst_structure_get_name(gst_caps_get_structure(caps, 0));

    if (g_str_has_prefix(name, "video"))
    {
        // handle_media_stream (pad, pipe, "videoconvert", "ximagesink");
        handle_media_stream(pad, pipe, "autovideoconvert", "autovideosink");
    }
    else
    {
        g_printerr("Unknown pad %s, ignoring", GST_PAD_NAME(pad));
    }
}

//decodebin ! queue ! convert ! sink  (example)
static void handle_media_stream(GstPad *pad, GstElement *pipe, const char *convert_name, const char *sink_name)
{
  GstElement *q, *conv, *sink;
  GstPad *q_pad;
  GstPadLinkReturn ret;

  g_print ("Trying to handle stream with: webrtcbin.pad ! decodebin ! queue ! %s ! %s\n", convert_name, sink_name);

  q = gst_element_factory_make ("queue", NULL);
  g_assert_nonnull (q);

  conv = gst_element_factory_make (convert_name, NULL);
  g_assert_nonnull (conv);
  sink = gst_element_factory_make (sink_name, NULL);
  g_assert_nonnull (sink);
  
  g_object_set(G_OBJECT(sink), "sync", FALSE, NULL);

  gst_bin_add_many (GST_BIN (pipe),q , conv, sink, NULL);
  gst_element_sync_state_with_parent (q);
  gst_element_sync_state_with_parent (conv);
  gst_element_sync_state_with_parent (sink);
  gst_element_link_many (q, conv, sink, NULL);

  q_pad = gst_element_get_static_pad (q, "sink");
  ret = gst_pad_link (pad, q_pad);

  g_assert_cmphex (ret, ==, GST_PAD_LINK_OK);
}
