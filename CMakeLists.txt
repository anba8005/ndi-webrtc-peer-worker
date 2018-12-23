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
        src/common/PeerFactoryContext.cpp src/common/PeerFactoryContext.h
        src/common/SetSessionDescriptionObserver.cpp src/common/SetSessionDescriptionObserver.h
        src/common/PeerContext.cpp src/common/PeerContext.h
        src/common/Signaling.cpp src/common/Signaling.h
        src/common/Dispatcher.cpp src/common/Dispatcher.h
        src/receiving/RemotePeerContext.cpp src/receiving/RemotePeerContext.h
        src/sending/LocalPeerContext.cpp src/sending/LocalPeerContext.h src/common/CreateSessionDescriptionObserver.cpp src/common/CreateSessionDescriptionObserver.h src/receiving/NDIWriter.cpp src/receiving/NDIWriter.h src/common/BaseAudioDeviceModule.cpp src/common/BaseAudioDeviceModule.h src/common/AudioBuffer.cpp src/common/AudioBuffer.h)

target_link_libraries(gyvaitv_webrtc ${WEBRTC_LIBRARIES} ${WEBRTC_DEPENDENCIES} avcodec avformat avutil swscale avdevice ndi pthread dl)
target_include_directories(gyvaitv_webrtc PUBLIC ${WEBRTC_INCLUDE_DIR} /usr/local/share/webrtc/abseil-cpp/)
