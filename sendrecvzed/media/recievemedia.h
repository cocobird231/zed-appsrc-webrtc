#pragma once
#include <gst/gst.h>

extern void on_incoming_stream(GstElement *webrtc, GstPad *pad, GstElement *pipe);

