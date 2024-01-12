/*
*webrtcbin1 negotiate
*/
#include "../signaling.h"
//#include "appdata.h"
#include  <gst/webrtc/webrtc.h>

/* Answer created by our pipeline, to be sent to the peer 
 * (check appstate) remove app-state in future
 * userdata: Gstelement *webrtc
*/
static void
on_answer_created (GstPromise * promise, gpointer user_data)
{
  GstElement *webrtc = (GstElement *)user_data;

  GstWebRTCSessionDescription *answer = NULL;
  const GstStructure *reply;

//  g_assert_cmphex (get_appstate(), ==, PEER_CALL_NEGOTIATING);

  g_assert_cmphex (gst_promise_wait (promise), ==, GST_PROMISE_RESULT_REPLIED);
  reply = gst_promise_get_reply (promise);
  gst_structure_get (reply, "answer",
      GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &answer, NULL);
  gst_promise_unref (promise);

  promise = gst_promise_new ();
  g_signal_emit_by_name (webrtc, "set-local-description", answer, promise);
  gst_promise_interrupt (promise);
  gst_promise_unref (promise);

  /* Send answer to peer */
  Signaling_send_sdp_to_peer (answer);
  gst_webrtc_session_description_free (answer);
}

/* Offer created by our pipeline, to be sent to the peer 
 *argument user_data:webrtc, remove app-state in future
 *userdata: Gstelement *webrtc
*/
extern void
on_offer_created (GstPromise * promise, gpointer user_data)
{
  GstElement *webrtc = (GstElement *)user_data;

  GstWebRTCSessionDescription *offer = NULL;
  const GstStructure *reply;

//  g_assert_cmphex (get_appstate(), ==, PEER_CALL_NEGOTIATING);

  g_assert_cmphex (gst_promise_wait (promise), ==, GST_PROMISE_RESULT_REPLIED);
  reply = gst_promise_get_reply (promise);
  gst_structure_get (reply, "offer",
      GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &offer, NULL);
  gst_promise_unref (promise);

  promise = gst_promise_new ();
  g_signal_emit_by_name (webrtc, "set-local-description", offer, promise);
  gst_promise_interrupt (promise);
  gst_promise_unref (promise);

  /* Send offer to peer */
  Signaling_send_sdp_to_peer (offer);
  gst_webrtc_session_description_free (offer);
}

static void
on_offer_set (GstPromise * promise, gpointer user_data)
{
  GstElement *webrtc = (GstElement *)user_data;

  gst_promise_unref (promise);
  promise = gst_promise_new_with_change_func (on_answer_created, webrtc, NULL);
  g_signal_emit_by_name (webrtc, "create-answer", NULL, promise);
}

extern void
offer_received (GstSDPMessage *sdp, GstElement *webrtc)
{
  GstWebRTCSessionDescription *offer = NULL;
  GstPromise *promise;

  offer = gst_webrtc_session_description_new (GST_WEBRTC_SDP_TYPE_OFFER, sdp);
  g_assert_nonnull (offer);

  /* Set remote description on our pipeline */
  {
    promise = gst_promise_new_with_change_func (on_offer_set, webrtc, NULL);
    g_signal_emit_by_name (webrtc, "set-remote-description", offer,
        promise);
  }
  gst_webrtc_session_description_free (offer);
}

extern void
answer_received (GstSDPMessage *sdp, GstElement *webrtc)
{
  GstWebRTCSessionDescription *answer = NULL;
  answer = gst_webrtc_session_description_new (GST_WEBRTC_SDP_TYPE_ANSWER,
            sdp);
  g_assert_nonnull (answer);

  /* Set remote description on our pipeline */
  {
    GstPromise *promise = gst_promise_new ();
    g_signal_emit_by_name (webrtc, "set-remote-description", answer,
        promise);
    gst_promise_interrupt (promise);
    gst_promise_unref (promise);
  }
}