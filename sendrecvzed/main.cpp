#include <gst/gst.h>
#include <gst/sdp/sdp.h>

#define GST_USE_UNSTABLE_API
#include <gst/webrtc/webrtc.h>
#include <string.h>

/* MY function*/
#include "signaling.h" //signaling client
#include "pipeline.h"
#include "appdata.h"

static gboolean check_plugins(void)
{
    g_print("check plugin...\n");
    int i;
    gboolean ret;
    GstPlugin *plugin;
    GstRegistry *registry;
    const gchar *needed[] = { "nice", "webrtc", "dtls", "srtp", "rtpmanager", NULL };

    registry = gst_registry_get();
    ret = TRUE;
    for (i = 0; i < g_strv_length ((gchar **) needed); i++)
    {
        plugin = gst_registry_find_plugin (registry, needed[i]);
        if (!plugin)
        {
            g_print("Required gstreamer plugin '%s' not found\n", needed[i]);
            ret = FALSE;
            continue;
        }
        gst_object_unref(plugin);
    }
    return ret;
}

int main(int argc, char *argv[])
{
    g_print("stun\n");
    g_print("start program...\n");
    GOptionContext *context;
    GError *error = NULL;

    context = g_option_context_new("ZED GStreamer WebRTC");
    g_option_context_add_main_entries(context, entries, NULL);
    g_option_context_add_group(context, gst_init_get_option_group());
    if (!g_option_context_parse(context, &argc, &argv, &error))
    {
        g_printerr("Error initializing: %s\n", error->message);
        return -1;
    }

    my_cmdline_check();
    if(!check_plugins())
        return -1;

    MyAppData *app_data = MyAppData_new();
    app_data->main_loop = g_main_loop_new(NULL, FALSE);

    if(!pipeline_init(app_data))
        return -1;

    g_print("main function start...\n");
    Siggnaling_connect_to_websocket_server_async(app_data);

    g_print("run loop...\n");
    g_main_loop_run(app_data->main_loop);//block here

    g_main_loop_unref(app_data->main_loop);
    app_data->main_loop = NULL;

    if (app_data->gstdata->pipeline1)
    {
        gst_element_set_state(GST_ELEMENT(app_data->gstdata->pipeline1), GST_STATE_NULL);
        g_print("Pipeline stopped\n");
        gst_object_unref(app_data->gstdata->pipeline1);
    }

    return 0;
}