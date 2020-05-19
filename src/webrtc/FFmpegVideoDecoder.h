/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 *
 */
//
// Created by anba8005 on 28/02/2020.
//

#ifndef NDI_WEBRTC_PEER_WORKER_FFMPEGVIDEODECODER_H
#define NDI_WEBRTC_PEER_WORKER_FFMPEGVIDEODECODER_H

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/hwcontext.h"
}

#include "CodecUtils.h"
#include "media/base/codec.h"
#include "modules/video_coding/include/video_codec_interface.h"
#include "common_video/include/i420_buffer_pool.h"
#include "rtc_base/system/rtc_export.h"

struct AVCodecContextDeleter {
    void operator()(AVCodecContext *ptr) const { avcodec_free_context(&ptr); }
};

struct AVFrameDeleter {
    void operator()(AVFrame *ptr) const { av_frame_free(&ptr); }
};

struct AVBufferDeleter {
    void operator()(AVBufferRef *ptr) const { av_buffer_unref(&ptr); }
};


class RTC_EXPORT FFmpegVideoDecoder : public webrtc::VideoDecoder {
public:
    explicit FFmpegVideoDecoder(std::string codec_name, CodecUtils::HardwareType hardware_type);

    ~FFmpegVideoDecoder() override;

    int32_t InitDecode(const webrtc::VideoCodec *codec_settings, int32_t number_of_cores) override;

    int32_t Release() override;

    int32_t RegisterDecodeCompleteCallback(webrtc::DecodedImageCallback *callback) override;

    // |missing_frames|, |fragmentation| and |render_time_ms| are ignored.
    int32_t
    Decode(const webrtc::EncodedImage &input_image, bool /*missing_frames*/, int64_t render_time_ms = -1) override;

    const char *ImplementationName() const override;

    static bool IsSupported();

    static std::unique_ptr<FFmpegVideoDecoder> Create(std::string codec_name, CodecUtils::HardwareType hardware_type);

private:
    bool IsInitialized() const;

    bool createHWContext(AVHWDeviceType type);
    AVHWDeviceType findHWDeviceType(const char* name);
    AVPixelFormat findHWPixelFormat(AVHWDeviceType type, AVCodec* codec);
    AVCodecID findCodecID(webrtc::VideoCodecType type);
    webrtc::VideoCodecType findCodecType(std::string name);

    webrtc::VideoCodecType codec_type_;
    CodecUtils::HardwareType hardware_type_;
    webrtc::I420BufferPool pool_;
    std::unique_ptr<AVCodecContext, AVCodecContextDeleter> av_context_;
    std::unique_ptr<AVBufferRef, AVBufferDeleter> hw_context_;
    AVPixelFormat hw_pixel_format_;
    uint8_t* packet_data_;
    unsigned int packet_data_size_;
    webrtc::DecodedImageCallback *decoded_image_callback_;

    static AVPixelFormat getHWFormat(AVCodecContext* ctx, const AVPixelFormat* pix_fmts);

};

#endif //NDI_WEBRTC_PEER_WORKER_FFMPEGVIDEODECODER_H
