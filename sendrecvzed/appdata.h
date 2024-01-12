#pragma once

#define GST_USE_UNSTABLE_API

#include "appenum.h"
#include <glib.h>
#include <gst/gst.h>
#include <libsoup/soup.h>

extern GOptionEntry entries[];
extern void my_cmdline_check();

// ZED
#include <mutex>
#include <atomic>
#include <thread>

#include <sl/Camera.hpp>
#include <opencv2/opencv.hpp>

using namespace std::chrono_literals;

// Only support LOOP method
enum ZEDAdaptorGrabType { ONCE, LOOP };

class ZEDAdaptor
{
private:
    std::atomic<int> zedID_;
    sl::Camera zed_;
    sl::InitParameters initParams_;
    sl::RuntimeParameters rtParams_;
    std::mutex paramsLock_;

    ZEDAdaptorGrabType gbType_;

    std::thread grabTh_;
    std::atomic<bool> exitF_;

    cv::Mat recvLeftMat_;
    cv::Mat recvDepthMat_;
    std::mutex recvLeftMatLock_;
    std::mutex recvDepthMatLock_;

private:
    void _grabThread();

public:
    ZEDAdaptor(sl::InitParameters initParams, sl::RuntimeParameters rtParams);

    ~ZEDAdaptor();

    bool start(ZEDAdaptorGrabType type = ZEDAdaptorGrabType::LOOP);

    bool grabLeftMat(cv::Mat& out);

    bool grabDepthMat(cv::Mat& out);
};

typedef struct _ZEDSrc
{
    GstElement *appsrc;
    GstCaps *caps;
    guint sourceid;

    ZEDAdaptor *zedAdaptor;
} ZEDSrc;

/*app state*/
extern enum AppState get_appstate();
extern void set_appstate(enum AppState state);

/*MyAppData*/
typedef struct _Custom_GstData
{
    GstElement *pipeline1;
    GstElement  *webrtc1;
    GObject *send_channel, *receive_channel;
} Custom_GstData;

typedef struct _MyAppData
{
    enum AppState *app_state;
    SoupWebsocketConnection *ws_conn; //Non-essential

    /* GLib's Main Loop */
    GMainLoop *main_loop;

    /*gstreamer*/
    Custom_GstData *gstdata;

    /*server & peer*/
    const gchar *our_id;
    const gchar *peer_id;
    const gchar *server_url;
    const gchar *stun_server;
    const gchar *turn_server;
    gboolean remote_is_offerer;
    gboolean enable_turn_server;

    // Streaming resolution
    gint outWidth;
    gint outHeight;

    // H264 encode
    gint x264_tune;
    gint x264_qp;
    gint x264_preset;
    gboolean x264_threads;

    /** ZED Streaming props*/
    int cam_id;
    int cam_fps;
    ZEDSrc *zedsrc;
} MyAppData;


extern MyAppData *MyAppData_new();

static inline const gchar *get_our_id(MyAppData *myappdata)
{
    return myappdata->our_id;
}
static inline const gchar *get_peer_id(MyAppData *myappdata)
{
    return myappdata->peer_id;
}
static inline const gchar *get_server_url(MyAppData *myappdata)
{
    return myappdata->server_url;
}
static inline gboolean get_remote_is_offerer(MyAppData *myappdata)
{
    return myappdata->remote_is_offerer;
}
