//
// Created by anba8005 on 12/23/18.
//

#include "VideoDeviceModule.h"
#include <iostream>
#include <rtc_base/refcountedobject.h>

const AVPixelFormat WEBRTC_PIXEL_FORMAT = av_get_pix_fmt("yuv420p");
const AVPixelFormat NDI_PIXEL_FORMAT = av_get_pix_fmt("uyvy422");

VideoDeviceModule::VideoDeviceModule() : AdaptedVideoTrackSource(),
                                         _scaling_context(nullptr) {}

VideoDeviceModule::~VideoDeviceModule() {
    if (_scaling_context) {
        sws_freeContext(_scaling_context);
    }
}

void VideoDeviceModule::feedFrame(const int width, const int height, const uint8_t *data,const int linesize) {
    // get scaling context
    _scaling_context = sws_getCachedContext(_scaling_context, width, height, NDI_PIXEL_FORMAT,
                                            width, height, WEBRTC_PIXEL_FORMAT, SWS_BICUBIC,
                                            nullptr, nullptr, nullptr);
    if (!_scaling_context) {
        std::cerr << "Scaling context creation error" << std::endl;
        return;
    }

    // check/create webrtc buffer
    if (!_webrtc_buffer || _webrtc_buffer->height() != height || _webrtc_buffer->width() != width) {
        _webrtc_buffer = webrtc::I420Buffer::Create(width, height);
    }

    // prepare webrtc_data for scaling
    uint8_t *webrtc_data[8];
    webrtc_data[0] = _webrtc_buffer->MutableDataY();
    webrtc_data[1] = _webrtc_buffer->MutableDataU();
    webrtc_data[2] = _webrtc_buffer->MutableDataV();
    int webrtc_linesize[8];
    webrtc_linesize[0] = _webrtc_buffer->StrideY();
    webrtc_linesize[1] = _webrtc_buffer->StrideU();
    webrtc_linesize[2] = _webrtc_buffer->StrideV();

    //
    // scale
    uint8_t *ndi_data[8];
    int ndi_linesize[8];
    ndi_data[0] = (uint8_t *) data;
    ndi_linesize[0] = linesize;
    if (sws_scale(_scaling_context, ndi_data, ndi_linesize, 0, height, webrtc_data, webrtc_linesize) < 0) {
        std::cerr << "Conversion error" << std::endl;
        return;
    }

    // send frame to webrtc engine
    webrtc::VideoFrame frame(_webrtc_buffer, 0, 0, webrtc::VideoRotation::kVideoRotation_0);
    OnFrame(frame);
}

bool VideoDeviceModule::is_screencast() const {
    return false;
}

absl::optional<bool> VideoDeviceModule::needs_denoising() const {
    return absl::optional<bool>();
}

webrtc::MediaSourceInterface::SourceState VideoDeviceModule::state() const {
    return kLive;
}

bool VideoDeviceModule::remote() const {
    return false;
}

rtc::scoped_refptr<VideoDeviceModule> VideoDeviceModule::Create() {
    return new rtc::RefCountedObject<VideoDeviceModule>();
}


