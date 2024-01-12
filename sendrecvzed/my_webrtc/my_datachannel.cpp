#include <gst/gst.h>
#include <gst/webrtc/webrtc.h>
#include "my_datachannel.h"

static void
data_channel_on_error (GObject * dc, gpointer user_data)
{
  g_print("user_data");
  //cleanup_and_quit_loop ("Data channel error", 0);
}

static void
data_channel_on_open (GObject * dc, gpointer user_data)
{
  GBytes *bytes = g_bytes_new ("data", strlen ("data"));
  g_print ("data channel opened\n");
  g_signal_emit_by_name (dc, "send-string", "Hi! from GStreamer");
  g_signal_emit_by_name (dc, "send-data", bytes);
  g_bytes_unref (bytes);
}

static void
data_channel_on_close (GObject * dc, gpointer user_data)
{
  //cleanup_and_quit_loop ("Data channel closed", 0);
  g_print("data channel closed\n");
}

static void
data_channel_on_message_string (GObject * dc, gchar * str, gpointer user_data)
{
  g_print ("Received data channel message: %s\n", str);
  //should I need to free str? i dont know see(GDestroyNotify in webrtcdatachannel.c "_emit_have_string" )
  //g_free(str);
}

static void
data_channel_on_message_data (GObject * dc, GBytes * in_data, gpointer user_data)
{
  g_print ("Received data channel data(byte)\n");
  //g_bytes_get_data (GBytes *bytes, gsize *size);
  //gsize g_bytes_get_size (GBytes *bytes);
  //use cast to change userdata;
}

extern void
connect_data_channel_signals (GObject * data_channel)
{
  g_signal_connect (data_channel, "on-error",
      G_CALLBACK (data_channel_on_error), NULL);
  g_signal_connect (data_channel, "on-open", G_CALLBACK (data_channel_on_open),
      NULL);
  g_signal_connect (data_channel, "on-close",
      G_CALLBACK (data_channel_on_close), NULL);
  g_signal_connect (data_channel, "on-message-string",
      G_CALLBACK (data_channel_on_message_string), NULL);
  g_signal_connect (data_channel, "on-message-data",
      G_CALLBACK (data_channel_on_message_data), NULL);
}