# zed-appsrc-webrtc

Web version: https://hackmd.io/@cocobird231/rJ9oJFAu6

*`Updated: 2024/01/12`*

## Description
This repo demonstrates the application of ZED camera streaming via WebRTC without `zed-gstreamer` supported.

## Environment
- OS: Ubuntu 20.04
- Hardware: ZED Box Orin NX, Jetson AGX Orin Developer Kit
- ZED Camera: ZED X series, ZED 2i
- ZED SDK: 4.0

## Prerequisite
1. **CUDA Toolkit**
    If using ZED Box or Jetson AGX Orin, the OS were pre-installed, follow the [official guide](https://docs.nvidia.com/jetson/jetpack/install-jetpack/index.html) to install JetPack.
    ```bash
    sudo apt update
    sudo apt install nvidia-jetpack
    ```
    After JetPack installed, add following lines under `~/.bashrc` for CUDA environment setup.
    ```md
    export CUDA_HOME=/usr/local/cuda
    export LD_LIBRARY_PATH=${CUDA_HOME}/lib64:${LD_LIBRARY_PATH}
    export PATH=${CUDA_HOME}/bin:${PATH}
    ```
    Re-open the terminal and test CUDA setting.
    ```bash
    nvcc -V
    ```
2. **ZED SDK (CUDA required)**
    Download ZED SDK from [here](https://www.stereolabs.com/developers/release) and run installation.
    ```bash
    sudo chmod a+x <zed_sdk_version>.run
    ./<zed_sdk_version>.run
    ```
3. **ZED X Driver (Only ZED X series required)**
    Download ZED X Driver form [here](https://www.stereolabs.com/developers/release#drivers) and run installation.
    ```bash
    sudo dpkg -i <the deb file path>.deb
    ```
    Enable the driver while start-up
    ```bash
    sudo systemctl enable zed_x_daemon
    ```

## Installation
Clone repo and build.
```bash
git clone https://github.com/cocobird231/zed-appsrc-webrtc.git
cd zed-appsrc-webrtc
. install.sh
```

## Usage
In this example, the viewer need to be executed first.

- **Viewer**
    ```bash
    ./sendrecvzed --our-id=0 --remote-offerer
    ```

- **Camera**
    ```bash
    ./sendrecvzed --cam-id=0 --our-id=1 --peer-id=0
    ```
    Use `--help` to get more information.
    
    Camera arguments:
    - `cam-id`
        ZED camera ID.
    - `out-width`
        The width of streaming video.
    - `out-height`
        The height of streaming video.
    - `x264-tune`
        h.264 encode param. See [GstX264EncTune](https://gstreamer.freedesktop.org/documentation/x264/index.html#GstX264EncTune).
    - `x264-qp`
        h.264 encode param. See [quantizer](https://gstreamer.freedesktop.org/documentation/x264/index.html#x264enc:quantizer).
    - `x264-preset`
        h.264 encode param. See [GstX264EncPreset](https://gstreamer.freedesktop.org/documentation/x264/index.html#GstX264EncPreset).

## Reference
[JetPack Install](https://docs.nvidia.com/jetson/jetpack/install-jetpack/index.html)
[ZED SDK](https://www.stereolabs.com/developers/release)
[gstreamer-webrtc](https://github.com/GStreamer/gst-examples)
[x264enc](https://gstreamer.freedesktop.org/documentation/x264/index.html?gi-language=c)