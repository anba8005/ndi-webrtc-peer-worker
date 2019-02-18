cmake_minimum_required(VERSION 3.10.2)
project(ndi-webrtc-peer-worker)

set(WEBRTC_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/vendor/webrtc/webrtc-24472-4ab51cb-linux-x64)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_VERBOSE_MAKEFILE ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

include(CMakeHelpers)
include(CMakeFindExtensions)
include(FindWebRTC)

set(FFMPEG_LIBS avcodec avformat avutil swscale avdevice)
set(FFMPEG_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/vendor/ffmpeg/include)
set(FFMPEG_LIB_DIR ${CMAKE_CURRENT_LIST_DIR}/vendor/ffmpeg/lib/)

add_executable(
		ndi-webrtc-peer-worker
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
		src/NDIReader.cpp src/NDIReader.h src/StatsCollectorCallback.cpp src/StatsCollectorCallback.h)

target_link_directories(ndi-webrtc-peer-worker PUBLIC ${FFMPEG_LIB_DIR})
target_link_libraries(ndi-webrtc-peer-worker ${WEBRTC_LIBRARIES} ${WEBRTC_DEPENDENCIES} ${FFMPEG_LIBS} ndi pthread dl)
target_include_directories(ndi-webrtc-peer-worker PUBLIC
		${FFMPEG_INCLUDE_DIR} ${WEBRTC_INCLUDE_DIR} ${CMAKE_CURRENT_LIST_DIR}/vendor/abseil-cpp)
