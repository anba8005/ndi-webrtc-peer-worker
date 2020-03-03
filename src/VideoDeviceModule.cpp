//
// Created by anba8005 on 12/23/18.
//

#include "VideoDeviceModule.h"
#include <iostream>
#include <rtc_base/ref_counted_object.h>
#include "third_party/libyuv/include/libyuv.h"

VideoDeviceModule::VideoDeviceModule()
        : AdaptedVideoTrackSource(), _updater(nullptr), _skip_frame_reporter("VideoDeviceModule skipped frames: ") {
}

VideoDeviceModule::~VideoDeviceModule() {
}

void
VideoDeviceModule::feedFrame(int width, int height, const uint8_t *data, const int linesize, const int64_t timestamp,
                             int maxWidth, int maxHeight) {
    // adapt frame
    int out_width = 0;
    int out_height = 0;
    int crop_width = 0;
    int crop_height = 0;
    int crop_x = 0;
    int crop_y = 0;
    int target_width = maxWidth != 0 ? maxWidth : width;
    int target_height = maxHeight != 0 ? maxHeight : height;
    int64_t time_us = timestamp / ((int64_t) 10);
    bool result = AdaptFrame(target_width, target_height, time_us, &out_width, &out_height, &crop_width, &crop_height,
                             &crop_x,
                             &crop_y);

    // notify skipped frames
    if (!result) {
        _skip_frame_reporter.report();
        return;
    }

    // convert to yuv420p
    auto yuvBuffer = webrtc::I420Buffer::Create(width, height);
    int res = libyuv::UYVYToI420(data, linesize, yuvBuffer->MutableDataY(), yuvBuffer->StrideY(),
                                 yuvBuffer->MutableDataU(),
                                 yuvBuffer->StrideU(), yuvBuffer->MutableDataV(), yuvBuffer->StrideV(), width, height);
    if (res != 0) {
        return;
    }

    // scale on demand
    if (width != out_width || height != out_height) {
        auto scaleBuffer = webrtc::I420Buffer::Create(out_width, out_height);
        scaleBuffer->ScaleFrom(*yuvBuffer);
        yuvBuffer = scaleBuffer;
    }

    // build frame
    webrtc::VideoFrame::Builder builder;
    OnFrame(builder.set_video_frame_buffer(yuvBuffer).set_timestamp_us(time_us).set_rotation(
            webrtc::VideoRotation::kVideoRotation_0).build());
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

void VideoDeviceModule::updateFrameRate(int num, int den) {
    if (_updater) {
        _updater->updateFrameRate(num, den);
    }
}

void VideoDeviceModule::setFrameRateUpdater(FrameRateUpdater *updater) {
    _updater = updater;
}



