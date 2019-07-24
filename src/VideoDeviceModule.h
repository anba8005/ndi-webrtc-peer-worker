//
// Created by anba8005 on 12/23/18.
//

#ifndef GYVAITV_WEBRTC_VIDEODEVICEMODULE_H
#define GYVAITV_WEBRTC_VIDEODEVICEMODULE_H

#ifdef WIN32
#define NOMINMAX
#undef min
#undef max
#endif

#define _WINSOCK2API_
#define _WINSOCKAPI_

#include "media/base/adapted_video_track_source.h"
#include "api/video/i420_buffer.h"

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

class VideoDeviceModule : public rtc::AdaptedVideoTrackSource {
public:
	VideoDeviceModule();

	virtual ~VideoDeviceModule();

	static rtc::scoped_refptr<VideoDeviceModule> Create();

	bool is_screencast() const override;

	absl::optional<bool> needs_denoising() const override;

	SourceState state() const override;

	bool remote() const override;

	void feedFrame(int width, int height, const uint8_t *data, const int linesize, int64_t timestamp,
	               int maxWidth, int maxHeight);

};


#endif //GYVAITV_WEBRTC_VIDEODEVICEMODULE_H
