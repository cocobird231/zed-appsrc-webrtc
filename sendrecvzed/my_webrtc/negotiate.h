#pragma once

#include <gst/gst.h>
#include  <gst/sdp/sdp.h>

/* Offer created by our pipeline, to be sent to the peer */
/*userdata : Gstelement *webrtc
*/
extern void
on_offer_created (GstPromise * promise, gpointer user_data);

/* Offer recieved by our pipeline, to create answer and sent to the peer 
*userdata : Gstelement *webrtc
*/
extern void
offer_received (GstSDPMessage *sdp, GstElement *webrtc);

/* answer recieved by our pipeline, to set-remote-description 
*userdata : Gstelement *webrtc
*/
extern void
answer_received (GstSDPMessage *sdp, GstElement *webrtc);

//extern void (*send_text_to_peer_by_signaling)(char *text;);
