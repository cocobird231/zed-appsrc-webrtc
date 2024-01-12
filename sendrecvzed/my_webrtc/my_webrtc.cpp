#include "my_webrtc.h"
#include "my_datachannel.h"
#include "../signaling.h"
#include "../appdata.h"
#include "negotiate.h"
#include <gst/gst.h>
#include <gst/webrtc/webrtc.h>

/*user_data: MyAppData *app*/
static void
on_negotiation_needed (GstElement * webrtcbin, gpointer user_data)
{
  MyAppData *app = (MyAppData *) user_data;

  //first time or when this side need new offer(ex)
  set_appstate(PEER_CALL_NEGOTIATING);
  g_print("negotiation needed...NEGOTIATING...\n");
  if (app->remote_is_offerer) {
    //the peer(other) need to pased "OFFER_REQUEST" message
    //request the remote side stream
    
    //Signaling_send_OFFER_REQUEST_to_peer();
  } else {
    //local send stream
    GstPromise *promise;
    promise =
        gst_promise_new_with_change_func (on_offer_created, webrtcbin, NULL);
    g_signal_emit_by_name (webrtcbin, "create-offer", NULL, promise);
  }
}

static void
on_ice_candidate (GstElement * webrtc G_GNUC_UNUSED, guint mlineindex,
    gchar * candidate, gpointer user_data G_GNUC_UNUSED)
{
  
}

static void
on_data_channel (GstElement * webrtcbin, GObject * data_channel,
    gpointer user_data)
{
  MyAppData *app = (MyAppData *) user_data;
  connect_data_channel_signals (data_channel);
  /*one datachannel*/
  app->gstdata->receive_channel = data_channel;
}

static void
on_ice_gathering_state_notify (GstElement * webrtcbin, GParamSpec * pspec,
    gpointer user_data)
{
  GstWebRTCICEGatheringState ice_gather_state;
  const gchar *new_state = "unknown";

  g_object_get (webrtcbin, "ice-gathering-state", &ice_gather_state, NULL);
  switch (ice_gather_state) {
    case GST_WEBRTC_ICE_GATHERING_STATE_NEW:
      new_state = "new";
      break;
    case GST_WEBRTC_ICE_GATHERING_STATE_GATHERING:
      new_state = "gathering";
      break;
    case GST_WEBRTC_ICE_GATHERING_STATE_COMPLETE:
      new_state = "complete";
      break;
  }
  g_print ("ICE gathering state changed to %s\n", new_state);
}

static void
on_signaling_state_notify (GstElement * webrtcbin, GParamSpec * pspec,
    gpointer user_data)
{
  GstWebRTCSignalingState signaling_state;
  const gchar *new_state = "unknown";

  g_object_get (webrtcbin, "signaling-state", &signaling_state, NULL);
  switch (signaling_state) {
    case GST_WEBRTC_SIGNALING_STATE_STABLE:
      new_state = "stable";
      break;
    case GST_WEBRTC_SIGNALING_STATE_CLOSED:
      new_state = "closed";
      break;
    case GST_WEBRTC_SIGNALING_STATE_HAVE_LOCAL_OFFER:
      new_state = "have-local-offer";
      break;
    case GST_WEBRTC_SIGNALING_STATE_HAVE_REMOTE_OFFER:
      new_state = "have-remote-offer";
      break;
    case GST_WEBRTC_SIGNALING_STATE_HAVE_LOCAL_PRANSWER:
      new_state = "have-local-pranswer";
      break;
    case GST_WEBRTC_SIGNALING_STATE_HAVE_REMOTE_PRANSWER:
      new_state = "have-remote-pranswer";
      break;
  }
  g_print ("signaling state changed to %s\n", new_state);
}

extern void
my_webrtcbin_init (MyAppData *app_data)
{
  g_print("webrtcbin init\n");
  GstElement * webrtc = app_data->gstdata->webrtc1;
  /* This is the gstwebrtc entry point where we create the offer and so on. It
   * will be called when the pipeline goes to PLAYING. */
  g_signal_connect (webrtc, "on-negotiation-needed",
      G_CALLBACK (on_negotiation_needed), app_data);
  /* We need to transmit this ICE candidate to the browser via the websockets
   * signalling server. Incoming ice candidates from the browser need to be
   * added by us too, see on_server_message() */
  g_signal_connect (webrtc, "on-ice-candidate",
      G_CALLBACK (Signaling_send_ice_candidate_message), NULL);
  
  /*notify*/
  g_signal_connect (webrtc, "notify::ice-gathering-state",
      G_CALLBACK (on_ice_gathering_state_notify), NULL);
  //g_signal_connect (webrtc, "notify::signaling-state",
  //    G_CALLBACK (on_signaling_state_notify), NULL);

    g_signal_connect (webrtc, "on-data-channel", G_CALLBACK (on_data_channel),
      app_data);

    //send_text_to_peer_by_signaling = signaling_send;
}