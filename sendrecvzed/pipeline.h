#pragma once

#include <gst/gst.h>
#include "appdata.h"
extern gboolean pipeline_init(MyAppData *app_data);

extern gboolean start_pipeline ( Custom_GstData *gstdata, gboolean remote_is_offerer);
