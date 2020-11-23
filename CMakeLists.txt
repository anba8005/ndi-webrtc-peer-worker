cmake_minimum_required(VERSION 3.10.2)
project(ndi-webrtc-peer-worker)
set(EXECUTABLE_OUTPUT_PATH "bin")

# WEBRTC
IF (WIN32)
	set(WEBRTC_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/vendor/webrtc/webrtc-40987-06c35ad-win-x64)
ELSEIF(APPLE)
	set(WEBRTC_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/vendor/webrtc/webrtc-40987-06c35ad-mac-x64)
ELSE()
	set(WEBRTC_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/vendor/webrtc/webrtc-40987-06c35ad-linux-x64)
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
	set(FFMPEG_CONFIG_INCLUDE_DIR ${WEBRTC_INCLUDE_DIR}/third_party/ffmpeg/chromium/config/Chrome/win-msvc/x64)
ELSEIF(APPLE)
	set(FFMPEG_CONFIG_INCLUDE_DIR ${WEBRTC_INCLUDE_DIR}/third_party/ffmpeg/chromium/config/Chrome/mac/x64)
ELSE()
	set(FFMPEG_CONFIG_INCLUDE_DIR ${WEBRTC_INCLUDE_DIR}/third_party/ffmpeg/chromium/config/Chrome/linux/x64)
ENDIF()
set(FFMPEG_INCLUDE_DIR ${WEBRTC_INCLUDE_DIR}/third_party/ffmpeg)

# NDI
IF (WIN32)
	set(NDI_LIBS ${CMAKE_CURRENT_LIST_DIR}/vendor/ndi/win/lib/x64/Processing.NDI.Lib.x64.lib)
ELSEIF(APPLE)
	set(NDI_LIBS ${CMAKE_CURRENT_LIST_DIR}/vendor/ndi/mac/lib/x64/libndi.4.dylib)
ELSE()
	set(NDI_LIBS ndi)
ENDIF()
set(NDI_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/vendor/ndi/linux/include)

# libmfx
IF (WIN32)
	set(MFX_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/vendor/mfx/include)
	set(MFX_LIBS ${CMAKE_CURRENT_LIST_DIR}/vendor/mfx/lib/x64/libmfx.lib dxva2.lib d3d9.lib legacy_stdio_definitions.lib)
ELSE()
	#
ENDIF()


# SYSTEM
IF (WIN32)
	#
ELSEIF(APPLE)
	set(SYSTEM_LIBS stdc++)
ELSE()
	set(SYSTEM_LIBS pthread dl)
ENDIF()

# Compile options
IF (WIN32)
	set(COMPILE_OPTIONS "/MT$<$<CONFIG:Debug>:d>")
	add_definitions(-DNOMINMAX)
ELSEIF(APPLE)
	set(COMPILE_OPTIONS "-fvisibility=hidden" "-fvisibility-inlines-hidden" -O2)
ELSE()
	#
ENDIF()

add_definitions(-DOWT_ENABLE_H265)

set(EXECUTABLE_COMMON src/main.cpp
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
		src/NDIReader.cpp src/NDIReader.h
		src/StatsCollectorCallback.cpp src/StatsCollectorCallback.h
		src/StatsObserver.cpp src/StatsObserver.h
		src/webrtc/CustomDecoderFactory.cpp src/webrtc/CustomDecoderFactory.h
		src/webrtc/CustomEncoderFactory.cpp src/webrtc/CustomEncoderFactory.h
		src/webrtc/CodecUtils.cpp src/webrtc/CodecUtils.h
		src/webrtc/FFmpegVideoDecoder.cpp src/webrtc/FFmpegVideoDecoder.h
		src/webrtc/FrameRateUpdater.h src/ThrottledReporter.cpp src/ThrottledReporter.h)

IF (WIN32)
	add_executable(
			ndi-webrtc-peer-worker
            src/webrtc/win/msdkvideoencoder.cc src/webrtc/win/msdkvideoencoder.h
			src/webrtc/win/base_allocator.cc src/webrtc/win/base_allocator.h
			src/webrtc/win/d3d_allocator.cc src/webrtc/win/d3d_allocator.h
			src/webrtc/win/mediautils.cc src/webrtc/win/mediautils.h
			src/webrtc/win/msdkvideobase.cc src/webrtc/win/msdkvideobase.h
			src/webrtc/win/sysmem_allocator.cc src/webrtc/win/sysmem_allocator.h
			src/webrtc/win/commontypes.h src/webrtc/win/d3dnativeframe.h
			src/webrtc/win/msdkcommon.h
			${EXECUTABLE_COMMON})
ELSE()
	add_executable(
			ndi-webrtc-peer-worker
			${EXECUTABLE_COMMON}
			src/webrtc/FFmpegVideoEncoder.cpp src/webrtc/FFmpegVideoEncoder.h)
ENDIF()

target_compile_options(ndi-webrtc-peer-worker PUBLIC ${COMPILE_OPTIONS})
set_target_properties(ndi-webrtc-peer-worker PROPERTIES LINK_FLAGS_RELEASE -s)
target_link_libraries(ndi-webrtc-peer-worker PUBLIC ${WEBRTC_LIBRARIES} ${WEBRTC_DEPENDENCIES} ${NDI_LIBS}
		${MFX_LIBS} ${SYSTEM_LIBS})
target_include_directories(ndi-webrtc-peer-worker PUBLIC ${NDI_INCLUDE_DIR} ${MFX_INCLUDE_DIR}
		${FFMPEG_INCLUDE_DIR} ${FFMPEG_CONFIG_INCLUDE_DIR} ${WEBRTC_INCLUDE_DIR} ${LIBYUV_INCLUDE_DIR}
		${CMAKE_CURRENT_LIST_DIR}/vendor/abseil-cpp)
