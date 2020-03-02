cmake_minimum_required(VERSION 3.10.2)
project(ndi-webrtc-peer-worker)
set(EXECUTABLE_OUTPUT_PATH "bin")

# WEBRTC
IF (WIN32)
	set(WEBRTC_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/vendor/webrtc/webrtc-29507-8708428-linux-x64)
ELSE()
	set(WEBRTC_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/vendor/webrtc/webrtc-29507-8708428-linux-x64)
ENDIF()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_VERBOSE_MAKEFILE ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

include(CMakeHelpers)
include(CMakeFindExtensions)
include(FindWebRTC)

# LIBYUV
set(LIBYUV_INCLUDE_DIR ${WEBRTC_INCLUDE_DIR}/third_party/libyuv/include)

# FFMPEG
IF (WIN32)
	#
ELSE()
	set(FFMPEG_CONFIG_INCLUDE_DIR ${WEBRTC_INCLUDE_DIR}/third_party/ffmpeg/chromium/config/Chrome/linux/x64)
ENDIF()
set(FFMPEG_INCLUDE_DIR ${WEBRTC_INCLUDE_DIR}/third_party/ffmpeg)

# NDI
IF (WIN32)
	set(NDI_LIBS ${CMAKE_CURRENT_LIST_DIR}/vendor/ndi/lib/x64/Processing.NDI.Lib.x64.lib)
ELSE()
	set(NDI_LIBS ndi)
ENDIF()
set(NDI_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/vendor/ndi/include)

# SYSTEM
IF (WIN32)
	#set(NDI_LIBS ${CMAKE_CURRENT_LIST_DIR}/vendor/ndi/lib/x64/Processing.NDI.Lib.x64.lib)
ELSE()
	set(SYSTEM_LIBS pthread dl va va-drm)
ENDIF()

# Compile options
IF (WIN32)
	set(COMPILE_OPTIONS "/MT$<$<CONFIG:Debug>:d>")
ELSE()
	#
ENDIF()

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
		src/NDIReader.cpp src/NDIReader.h src/StatsCollectorCallback.cpp src/StatsCollectorCallback.h src/StatsObserver.cpp src/StatsObserver.h src/webrtc/CustomDecoderFactory.cpp src/webrtc/CustomDecoderFactory.h src/webrtc/CustomEncoderFactory.cpp src/webrtc/CustomEncoderFactory.h src/webrtc/CodecUtils.cpp src/webrtc/CodecUtils.h src/webrtc/H265Decoder.cpp src/webrtc/H265Decoder.h src/webrtc/FFmpegVideoDecoder.cpp src/webrtc/FFmpegVideoDecoder.h src/webrtc/FFmpegVideoEncoder.cpp src/webrtc/FFmpegVideoEncoder.h)

target_compile_options(ndi-webrtc-peer-worker PUBLIC ${COMPILE_OPTIONS})
set_target_properties(ndi-webrtc-peer-worker PROPERTIES LINK_FLAGS_RELEASE -s)
target_link_libraries(ndi-webrtc-peer-worker PUBLIC ${WEBRTC_LIBRARIES} ${WEBRTC_DEPENDENCIES} ${NDI_LIBS} ${SYSTEM_LIBS})
target_include_directories(ndi-webrtc-peer-worker PUBLIC ${NDI_INCLUDE_DIR}
		${FFMPEG_INCLUDE_DIR} ${FFMPEG_CONFIG_INCLUDE_DIR} ${WEBRTC_INCLUDE_DIR} ${LIBYUV_INCLUDE_DIR}
		${CMAKE_CURRENT_LIST_DIR}/vendor/abseil-cpp)
