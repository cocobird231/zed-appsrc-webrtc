#pragma once

#include "appdata.h"

#include <gst/gst.h>              //gstreamer-webrtc
#include <gst/webrtc/webrtc.h>    //webrtc
#include <json-glib/json-glib.h>  //json parsed

/*signaling client*/
extern void
Siggnaling_connect_to_websocket_server_async (MyAppData * myapp);

/*signaling transport*/
extern gchar *
get_string_from_json_object (JsonObject * object);

extern void
Signaling_send_register(const gchar *my_id);

extern void
Signaling_send_sdp_to_peer (GstWebRTCSessionDescription * desc);

extern void
Signaling_send_ice_candidate_message (GstElement * webrtc G_GNUC_UNUSED, guint mlineindex,
    gchar * candidate, gpointer user_data G_GNUC_UNUSED);

extern void
Signaling_send_OFFER_REQUEST_to_peer();

extern void
Signaling_send_SESSION(const gchar *peer_id);
