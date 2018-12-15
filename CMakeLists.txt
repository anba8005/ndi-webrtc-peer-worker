cmake_minimum_required(VERSION 3.10.2)
project(gyvaitv_webrtc)

set(WEBRTC_ROOT_DIR /usr/local/share/libsourcey/webrtc-22215-ab42706-linux-x64)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_VERBOSE_MAKEFILE ON)

list(APPEND CMAKE_MODULE_PATH /usr/local/share/libsourcey/cmake)


include(CMakeHelpers)
include(CMakeFindExtensions)
include(FindWebRTC)

add_executable(
        gyvaitv_webrtc
        src/main.cpp
        src/common/AWorker.cpp src/common/AWorker.h
        src/receiving/ReceivingWorker.cpp src/receiving/ReceivingWorker.h
        src/sending/SendingWorker.cpp src/sending/SendingWorker.h
)

target_link_libraries(gyvaitv_webrtc ${WEBRTC_LIBRARIES} ${WEBRTC_DEPENDENCIES} avcodec avformat avutil swscale avdevice ndi pthread dl)
target_include_directories(gyvaitv_webrtc PUBLIC ${WEBRTC_INCLUDE_DIR})
