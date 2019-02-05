cmake_minimum_required(VERSION 3.10.2)
project(gyvaitv_webrtc)

set(WEBRTC_ROOT_DIR /usr/local/share/webrtc/webrtc-24472-4ab51cb-linux-x64)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_VERBOSE_MAKEFILE ON)

list(APPEND CMAKE_MODULE_PATH /usr/local/share/libsourcey/cmake)

include(CMakeHelpers)
include(CMakeFindExtensions)
include(FindWebRTC)

add_executable(
        gyvaitv_webrtc
        src/main.cpp
        src/PeerFactoryContext.cpp src/PeerFactoryContext.h
        src/SetSessionDescriptionObserver.cpp src/SetSessionDescriptionObserver.h
        src/PeerContext.cpp src/PeerContext.h
        src/Signaling.cpp src/Signaling.h
        src/Dispatcher.cpp src/Dispatcher.h
        src/CreateSessionDescriptionObserver.cpp src/CreateSessionDescriptionObserver.h
        src/NDIWriter.cpp src/NDIWriter.h
        src/BaseAudioDeviceModule.cpp src/BaseAudioDeviceModule.h
        src/AudioBuffer.cpp src/AudioBuffer.h
        src/VideoDeviceModule.cpp src/VideoDeviceModule.h
        src/NDIReader.cpp src/NDIReader.h)

target_link_libraries(gyvaitv_webrtc ${WEBRTC_LIBRARIES} ${WEBRTC_DEPENDENCIES} avcodec avformat avutil swscale avdevice ndi pthread dl)
target_include_directories(gyvaitv_webrtc PUBLIC ${WEBRTC_INCLUDE_DIR} /usr/local/share/webrtc/abseil-cpp/)
