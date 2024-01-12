sudo apt update

sudo apt install -y make cmake gcc g++ build-essential
sudo apt install -y gstreamer1.0-tools gstreamer1.0-nice gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-plugins-good libgstreamer1.0-dev git libglib2.0-dev libgstreamer-plugins-bad1.0-dev libsoup2.4-dev libjson-glib-dev
sudo apt install -y libavcodec58 ffmpeg libopencv-dev

mkdir -p sendrecvzed/build
cd sendrecvzed/build
cmake ..
make
