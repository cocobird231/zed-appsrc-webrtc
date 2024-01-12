cmake_minimum_required(VERSION 3.5)
project(sendrecvzed VERSION 0.1.0)

# Default to C99
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(PkgConfig REQUIRED)
find_package(ZED 3 REQUIRED)
find_package(CUDA ${ZED_CUDA_VERSION} EXACT REQUIRED)
find_package(OpenCV ${VERSION_REQ_OCV} REQUIRED)

include_directories(${ZED_INCLUDE_DIRS})
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${CUDA_INCLUDE_DIRS})

link_directories(${ZED_LIBRARY_DIR})
link_directories(${OpenCV_LIBRARY_DIRS})
link_directories(${CUDA_LIBRARY_DIRS})

add_compile_options(-lpthread)

pkg_check_modules(GST REQUIRED IMPORTED_TARGET gstreamer-1.0 gstreamer-app-1.0 gstreamer-webrtc-1.0 gstreamer-sdp-1.0)
pkg_check_modules(Other REQUIRED IMPORTED_TARGET libsoup-2.4 json-glib-1.0)
#pkg_check_modules(X11 REQUIRED IMPORTED_TARGET x11)
file(GLOB global_src
     "*.h"
     "*.cpp"
     "my_webrtc/*.h"
     "my_webrtc/*.cpp"
     "media/*.h"
     "media/*.cpp"
)

add_executable(${PROJECT_NAME} ${global_src})
target_link_libraries(${PROJECT_NAME} 
        ${ZED_LIBRARIES}
        ${OpenCV_LIBRARIES}
        ${CUDA_CUDA_LIBRARY} ${CUDA_CUDART_LIBRARY} ${CUDA_NPP_LIBRARIES_ZED}
        pthread
        PkgConfig::GST
        PkgConfig::Other
#        PkgConfig::X11
        )
#target_include_directories(${PROJECT_NAME} PUBLIC PkgConfig::GST PkgConfig::Other PkgConfig::X11)
target_include_directories(${PROJECT_NAME} PUBLIC PkgConfig::GST PkgConfig::Other )
