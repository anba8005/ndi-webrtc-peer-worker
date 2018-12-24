//
// Created by anba8005 on 12/23/18.
//

#ifndef GYVAITV_WEBRTC_VIDEODEVICEMODULE_H
#define GYVAITV_WEBRTC_VIDEODEVICEMODULE_H

#include "media/base/adaptedvideotracksource.h"
#include "api/video/i420_buffer.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/pixdesc.h>
#include <libavutil/imgutils.h>
}

class VideoDeviceModule : public rtc::AdaptedVideoTrackSource {
public:
    VideoDeviceModule();
    virtual ~VideoDeviceModule();

    static rtc::scoped_refptr<VideoDeviceModule> Create();

    bool is_screencast() const override;
    absl::optional<bool> needs_denoising() const override;
    SourceState state() const override;
    bool remote() const override;

    void feedFrame(const int width, const int height, const uint8_t *data,const int linesize);

private:

    SwsContext* _scaling_context;
    rtc::scoped_refptr<webrtc::I420Buffer> _webrtc_buffer;

};


#endif //GYVAITV_WEBRTC_VIDEODEVICEMODULE_H
