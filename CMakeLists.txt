cmake_minimum_required(VERSION 3.10.2)
project(ndi-webrtc-peer-worker)

# WEBRTC
IF (WIN32)
	set(WEBRTC_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/vendor/webrtc/webrtc-27678-159c16f-win-x64)
ELSE()
	set(WEBRTC_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/vendor/webrtc/webrtc-27678-159c16f-linux-x64)
ENDIF()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_VERBOSE_MAKEFILE ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

include(CMakeHelpers)
include(CMakeFindExtensions)
include(FindWebRTC)

# FFMPEG
IF (WIN32)
	set(FFMPEG_LIBS ${CMAKE_CURRENT_LIST_DIR}/vendor/ffmpeg/lib/avcodec.lib ${CMAKE_CURRENT_LIST_DIR}/vendor/ffmpeg/lib/avformat.lib
			${CMAKE_CURRENT_LIST_DIR}/vendor/ffmpeg/lib/avutil.lib ${CMAKE_CURRENT_LIST_DIR}/vendor/ffmpeg/lib/swscale.lib
			${CMAKE_CURRENT_LIST_DIR}/vendor/ffmpeg/lib/avdevice.lib)
ELSE()
	set(FFMPEG_LIBS avcodec avformat avutil swscale avdevice)
ENDIF()
set(FFMPEG_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/vendor/ffmpeg/include)

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
	set(SYSTEM_LIBS pthread dl)
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
		src/NDIReader.cpp src/NDIReader.h src/StatsCollectorCallback.cpp src/StatsCollectorCallback.h src/StatsObserver.cpp src/StatsObserver.h)

target_compile_options(ndi-webrtc-peer-worker PUBLIC ${COMPILE_OPTIONS})
target_link_libraries(ndi-webrtc-peer-worker PUBLIC ${WEBRTC_LIBRARIES} ${WEBRTC_DEPENDENCIES} ${FFMPEG_LIBS} ${NDI_LIBS} ${SYSTEM_LIBS})
target_include_directories(ndi-webrtc-peer-worker PUBLIC ${NDI_INCLUDE_DIR}
		${FFMPEG_INCLUDE_DIR} ${WEBRTC_INCLUDE_DIR} ${CMAKE_CURRENT_LIST_DIR}/vendor/abseil-cpp)
