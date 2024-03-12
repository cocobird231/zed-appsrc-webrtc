#include "appdata.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <glib.h>

/** ZED Streaming*/
static gint cam_id = 0;
static gint cam_fps = 30;
static gint stream_type = 0;
static gint outWidth = 1280;
static gint outHeight = 720;
static gint x264_tune = 0x00000004;// GstX264EncTune: zerolatency
static gint x264_qp = 21;
static gint x264_preset = 4;// GstX264EncPreset: faster
static gboolean x264_threads = TRUE;

int getOCVtype(sl::MAT_TYPE type)
{
    int cv_type = -1;
    switch (type)
    {
        case sl::MAT_TYPE::F32_C1: cv_type = CV_32FC1; break;
        case sl::MAT_TYPE::F32_C2: cv_type = CV_32FC2; break;
        case sl::MAT_TYPE::F32_C3: cv_type = CV_32FC3; break;
        case sl::MAT_TYPE::F32_C4: cv_type = CV_32FC4; break;
        case sl::MAT_TYPE::U8_C1: cv_type = CV_8UC1; break;
        case sl::MAT_TYPE::U8_C2: cv_type = CV_8UC2; break;
        case sl::MAT_TYPE::U8_C3: cv_type = CV_8UC3; break;
        case sl::MAT_TYPE::U8_C4: cv_type = CV_8UC4; break;
        default: break;
    }
    return cv_type;
}

cv::Mat slMat2cvMat(sl::Mat& input)
{
    // Since cv::Mat data requires a uchar* pointer, we get the uchar1 pointer from sl::Mat (getPtr<T>())
    // cv::Mat and sl::Mat will share a single memory structure
    return cv::Mat(input.getHeight(), input.getWidth(), getOCVtype(input.getDataType()), input.getPtr<sl::uchar1>(sl::MEM::CPU), input.getStepBytes(sl::MEM::CPU));
}


/*signaling + webrtc*/
static const gchar *our_id = NULL;
static const gchar *peer_id = NULL;
static const gchar *server_url = "http://61.220.23.239:8443";
static const gchar *stun_server = "stun://61.220.23.239";
static const gchar *turn_server = "turn://61.220.23.239";
static gboolean remote_is_offerer = FALSE;
static gboolean enable_turn_server = FALSE;

GOptionEntry entries[] = { {"cam-id", 0, 0, G_OPTION_ARG_INT, &cam_id, "Camera ID (int from 0 to 255)", "INT"}, 
                            {"stream-type", 0, 0, G_OPTION_ARG_INT, &stream_type, "zedsrc stream type", "INT"}, 

                            {"out-width", 0, 0, G_OPTION_ARG_INT, &outWidth, "Streaming width", "UINT"}, 
                            {"out-height", 0, 0, G_OPTION_ARG_INT, &outHeight, "Streaming height", "UINT"}, 

                            {"x264-tune", 0, 0, G_OPTION_ARG_INT, &x264_tune, "H264 tune", "INT"}, 
                            {"x264-qp", 0, 0, G_OPTION_ARG_INT, &x264_qp, "H264 quantizer", "INT"}, 
                            {"x264-preset", 0, 0, G_OPTION_ARG_INT, &x264_preset, "H264 preset", "INT"}, 

                            {"our-id", 0, 0, G_OPTION_ARG_STRING, &our_id, "Signaling ID of this local peer", "ID"}, 
                            {"peer-id", 0, 0, G_OPTION_ARG_STRING, &peer_id, "Signaling ID of the peer to connect to", "ID"}, 
                            {"server", 0, 0, G_OPTION_ARG_STRING, &server_url, "Signaling server to connect to", "URL"}, 
                            {"stun-server", 0, 0, G_OPTION_ARG_STRING, &stun_server, "STUN server", "URL"}, 
                            {"turn-server", 0, 0, G_OPTION_ARG_STRING, &turn_server, "TURN server", "URL"}, 
                            {"remote-offerer", 0, 0, G_OPTION_ARG_NONE, &remote_is_offerer, "Request that the peer generate the offer and we'll answer", NULL}, 
                            {"turn", 0, 0, G_OPTION_ARG_NONE, &enable_turn_server, "Enable turn server", NULL}, 
                            {NULL} };

extern void my_cmdline_check(void)
{
    g_print("cmdline parsed...\n");
    printf("--remote-offerer :%d\n", remote_is_offerer);
    printf("--server :%s\n", server_url);
    printf("--stun-server :%s\n", stun_server);
    printf("--turn-server :%s\n", turn_server);
    printf("--turn :%d\n", enable_turn_server);

    if (!our_id)
    {
        g_printerr("--our-id is a required argument\n");
        exit(-1);
    }
    printf("--our-id :%s\n", our_id);

    if (!remote_is_offerer)
    {
        if (!peer_id)
        {
            g_printerr("--peer-id is a required argument\n");
            exit(-1);
        }
        printf("--peer-id :%s\n", peer_id);
        printf("--cam-id :%d\n", cam_id);
        printf("--cam-fps :%d\n", cam_fps);
        printf("--stream-type :%d\n", stream_type);
        printf("--out-width :%d\n", outWidth);
        printf("--out-height :%d\n", outHeight);
        printf("--x264-tune :%d\n", x264_tune);
        printf("--x264-qp :%d\n", x264_qp);
        printf("--x264-preset :%d\n", x264_preset);
    }
}

/*app state*/
static enum AppState app_state = APP_STATE_UNKNOWN;
static enum AppState *get_appstate_addr()
{
    return &app_state;
}
extern enum AppState get_appstate()
{
    return app_state;
}
extern void set_appstate(enum AppState state)
{
    app_state = state;
}

/*MyAppData*/
static void MyAppData_init(MyAppData *app_data)
{
    app_data->app_state = &app_state;

    app_data->our_id = our_id;
    app_data->peer_id = peer_id;
    app_data->server_url = server_url;
    app_data->remote_is_offerer = remote_is_offerer;
    app_data->enable_turn_server = enable_turn_server;

    app_data->x264_tune = x264_tune;
    app_data->x264_qp = x264_qp;
    app_data->x264_preset = x264_preset;
    app_data->x264_threads = x264_threads;
    app_data->outWidth = outWidth;
    app_data->outHeight = outHeight;

    app_data->cam_id = cam_id;
    app_data->cam_fps = cam_fps;

    // ZED setting
    sl::InitParameters initParams;
    sl::RuntimeParameters rtParams;

    initParams.camera_resolution = sl::RESOLUTION::HD1080;
    initParams.depth_mode = sl::DEPTH_MODE::PERFORMANCE;
    initParams.coordinate_units = sl::UNIT::MILLIMETER;
    initParams.camera_fps = app_data->cam_fps;
    initParams.input.setFromCameraID(app_data->cam_id);

    rtParams.enable_fill_mode = false;
    ZEDAdaptor *zedAdaptor = new ZEDAdaptor(initParams, rtParams);
    zedAdaptor->start();

    // ZEDSrc init
    app_data->zedsrc = (ZEDSrc *)malloc(sizeof(ZEDSrc));
    app_data->zedsrc->zedAdaptor = zedAdaptor;
    app_data->zedsrc->sourceid = 0;
}

extern MyAppData *MyAppData_new()
{
    g_print("new appdata\n");
    MyAppData *app_data = (MyAppData *)malloc(sizeof(MyAppData));
    memset(app_data, 0, sizeof(MyAppData));
    MyAppData_init(app_data);
    return app_data;
}

extern void MyAppData_free(MyAppData *app_data)
{
    g_print("free appdata\n");
    if (app_data)
    {
        if (app_data->zedsrc)
        {
            if (app_data->zedsrc->zedAdaptor)
                delete app_data->zedsrc->zedAdaptor;
            free(app_data->zedsrc);
        }
        if (app_data->gstdata)
            free(app_data->gstdata);
        free(app_data);
    }
}

// ZED
void ZEDAdaptor::_grabThread()
{
    std::unique_lock<std::mutex> recvLeftMatLocker(this->recvLeftMatLock_, std::defer_lock);
    std::unique_lock<std::mutex> recvDepthMatLocker(this->recvDepthMatLock_, std::defer_lock);

    while (!exitF_)
    {
        if (this->zed_.grab(this->rtParams_) != sl::ERROR_CODE::SUCCESS)
        {
            printf("ZED %d grab error.\n", this->zedID_.load());
            std::this_thread::sleep_for(50ms);
        }
        sl::Mat leftSlMat;// BGRA
        sl::Mat depthSlMat;

        this->zed_.retrieveImage(leftSlMat, sl::VIEW::LEFT, sl::MEM::CPU);
        recvLeftMatLocker.lock();
        this->recvLeftMat_ = slMat2cvMat(leftSlMat).clone();
        recvLeftMatLocker.unlock();

        this->zed_.retrieveMeasure(depthSlMat, sl::MEASURE::DEPTH, sl::MEM::CPU);
        recvDepthMatLocker.lock();
        this->recvDepthMat_ = slMat2cvMat(depthSlMat).clone();
        recvDepthMatLocker.unlock();
    }
}

ZEDAdaptor::ZEDAdaptor(sl::InitParameters initParams, sl::RuntimeParameters rtParams) : 
    zedID_(-1), 
    gbType_(ZEDAdaptorGrabType::LOOP), 
    exitF_(false)
{
    this->initParams_ = initParams;
    this->rtParams_ = rtParams;
}

ZEDAdaptor::~ZEDAdaptor()
{
    this->exitF_ = true;
    if (this->gbType_ == ZEDAdaptorGrabType::LOOP)
        this->grabTh_.join();
    if (this->zed_.isOpened())
        this->zed_.close();
    this->recvLeftMat_.release();
    this->recvDepthMat_.release();
}

bool ZEDAdaptor::start(ZEDAdaptorGrabType type)
{
    std::lock_guard<std::mutex> paramsLocker(this->paramsLock_);
    std::lock_guard<std::mutex> recvLeftMatLocker(this->recvLeftMatLock_);
    std::lock_guard<std::mutex> recvDepthMatLocker(this->recvDepthMatLock_);

    this->gbType_ = type;
    sl::ERROR_CODE ret = this->zed_.open(this->initParams_);
    if (ret != sl::ERROR_CODE::SUCCESS)
    {
        printf("ZED open error.\n");
        return false;
    }
    // Test
    sl::Mat leftSlMat, depthSlMat;
    this->zed_.retrieveImage(leftSlMat, sl::VIEW::LEFT, sl::MEM::CPU);
    this->recvLeftMat_ = slMat2cvMat(leftSlMat);
    this->zed_.retrieveMeasure(depthSlMat, sl::MEASURE::DEPTH, sl::MEM::CPU);
    this->recvDepthMat_ = slMat2cvMat(depthSlMat);
    this->zedID_ = this->zed_.getCameraInformation().serial_number;

    if (type == ZEDAdaptorGrabType::LOOP)
    {
        this->grabTh_ = std::thread(std::bind(&ZEDAdaptor::_grabThread, this));
    }
    return true;
}

bool ZEDAdaptor::grabLeftMat(cv::Mat& out)
{
    if (this->zedID_ < 0) return false;
    std::unique_lock<std::mutex> recvLeftMatLocker(this->recvLeftMatLock_, std::defer_lock);

    if (this->gbType_ == ZEDAdaptorGrabType::ONCE)
    {
        // To be continue
    }
    else if (this->gbType_ == ZEDAdaptorGrabType::LOOP)
    {
        recvLeftMatLocker.lock();
        out = this->recvLeftMat_.clone();
        recvLeftMatLocker.unlock();
    }
    return true;
}

bool ZEDAdaptor::grabDepthMat(cv::Mat& out)
{
    if (this->zedID_ < 0) return false;
    std::unique_lock<std::mutex> recvDepthMatLocker(this->recvDepthMatLock_, std::defer_lock);

    if (this->gbType_ == ZEDAdaptorGrabType::ONCE)
    {
        // To be continue
    }
    else if (this->gbType_ == ZEDAdaptorGrabType::LOOP)
    {
        recvDepthMatLocker.lock();
        out = this->recvDepthMat_.clone();
        recvDepthMatLocker.unlock();
    }
    return true;
}