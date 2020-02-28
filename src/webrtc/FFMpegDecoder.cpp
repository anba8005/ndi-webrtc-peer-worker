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

#include "FFMpegDecoder.h"


extern "C" {
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
}

#include "api/video/color_space.h"
#include "api/video/i420_buffer.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "third_party/libyuv/include/libyuv/planar_functions.h"

#include "iostream"

const AVPixelFormat kPixelFormatDefault = AV_PIX_FMT_YUV420P;
const AVPixelFormat kPixelFormatFullRange = AV_PIX_FMT_YUVJ420P;
const size_t kYPlaneIndex = 0;
const size_t kUPlaneIndex = 1;
const size_t kVPlaneIndex = 2;

FFMpegDecoder::FFMpegDecoder() : pool_(true), decoded_image_callback_(nullptr), packet_data_(nullptr),
                                 packet_data_size_(0) {
}

FFMpegDecoder::~FFMpegDecoder() {
    Release();
}

int32_t FFMpegDecoder::InitDecode(const webrtc::VideoCodec *codec_settings, int32_t number_of_cores) {
    if (codec_settings && codec_settings->codecType != webrtc::kVideoCodecH265) {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }

    // Release necessary in case of re-initializing.
    int32_t ret = Release();
    if (ret != WEBRTC_VIDEO_CODEC_OK) {
        return ret;
    }
    RTC_DCHECK(!av_context_);

    // Initialize AVCodecContext.
    av_context_.reset(avcodec_alloc_context3(nullptr));

    av_context_->codec_type = AVMEDIA_TYPE_VIDEO;
    av_context_->codec_id = AV_CODEC_ID_H265;
    if (codec_settings) {
        av_context_->coded_width = codec_settings->width;
        av_context_->coded_height = codec_settings->height;
    }
    av_context_->pix_fmt = kPixelFormatDefault;
    av_context_->extradata = nullptr;
    av_context_->extradata_size = 0;

    // If this is ever increased, look at |av_context_->thread_safe_callbacks| and
    // make it possible to disable the thread checker in the frame buffer pool.
    av_context_->thread_count = 1;
    av_context_->thread_type = FF_THREAD_SLICE;

    AVCodec *codec = avcodec_find_decoder(av_context_->codec_id);
    if (!codec) {
        // This is an indication that FFmpeg has not been initialized or it has not
        // been compiled/initialized with the correct set of codecs.
        RTC_LOG(LS_ERROR) << "FFmpeg H.265 decoder not found.";
        Release();
        return WEBRTC_VIDEO_CODEC_ERROR;
    }
    int res = avcodec_open2(av_context_.get(), codec, nullptr);
    if (res < 0) {
        RTC_LOG(LS_ERROR) << "avcodec_open2 error: " << res;
        Release();
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    av_frame_.reset(av_frame_alloc());
    return WEBRTC_VIDEO_CODEC_OK;
}

int32_t FFMpegDecoder::Release() {
    if (packet_data_) {
        av_freep(&packet_data_);
        packet_data_size_ = 0;
    }
    av_context_.reset();
    av_frame_.reset();
    return WEBRTC_VIDEO_CODEC_OK;
}

int32_t FFMpegDecoder::RegisterDecodeCompleteCallback(webrtc::DecodedImageCallback *callback) {
    decoded_image_callback_ = callback;
    return WEBRTC_VIDEO_CODEC_OK;
}

int32_t FFMpegDecoder::Decode(const webrtc::EncodedImage &input_image, bool, int64_t render_time_ms) {
    if (!IsInitialized()) {
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
    if (!decoded_image_callback_) {
        RTC_LOG(LS_WARNING)
        << "InitDecode() has been called, but a callback function "
           "has not been set with RegisterDecodeCompleteCallback()";
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
    if (!input_image.data() || !input_image.size()) {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }

    if (input_image.size() >
        static_cast<size_t>(std::numeric_limits<int>::max())) {
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    AVPacket packet;
    av_init_packet(&packet);
    packet.size = static_cast<int>(input_image.size());
    av_fast_mallocz(&packet_data_, &packet_data_size_, packet.size + AV_INPUT_BUFFER_PADDING_SIZE);
    if (!packet_data_) {
        RTC_LOG(LS_ERROR) << "av_fast_malloc for packet error";
        return WEBRTC_VIDEO_CODEC_ERROR;
    }
    packet.data = packet_data_;
    memcpy(packet.data, input_image.mutable_data(), input_image.size());

    int64_t frame_timestamp_us = input_image.ntp_time_ms_ * 1000;  // ms -> μs
    av_context_->reordered_opaque = frame_timestamp_us;


    int result = avcodec_send_packet(av_context_.get(), &packet);
    if (result < 0) {
        RTC_LOG(LS_ERROR) << "avcodec_send_packet error: " << result;
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    result = avcodec_receive_frame(av_context_.get(), av_frame_.get());
    if (result < 0) {
        RTC_LOG(LS_ERROR) << "avcodec_receive_frame error: " << result;
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    // We don't expect reordering. Decoded frame tamestamp should match
    // the input one.
    RTC_DCHECK_EQ(av_frame_->reordered_opaque, frame_timestamp_us);


    // Create output buffer
    rtc::scoped_refptr<webrtc::I420Buffer> buffer = pool_.CreateBuffer(av_frame_->width, av_frame_->height);
    RTC_DCHECK(buffer.get());

    // Fill output buffer
    RTC_CHECK_EQ(0, libyuv::I420Copy(av_frame_->data[0], av_frame_->linesize[0], av_frame_->data[1],
                                     av_frame_->linesize[1], av_frame_->data[2], av_frame_->linesize[2],
                                     buffer->MutableDataY(),
                                     buffer->StrideY(), buffer->MutableDataU(),
                                     buffer->StrideU(), buffer->MutableDataV(),
                                     buffer->StrideV(), av_frame_->width, av_frame_->height));


    webrtc::VideoFrame decoded_frame = webrtc::VideoFrame::Builder()
            .set_video_frame_buffer(buffer)
            .set_timestamp_rtp(input_image.Timestamp())
            .build();

    // Return decoded frame.
    decoded_image_callback_->Decoded(decoded_frame);

    return WEBRTC_VIDEO_CODEC_OK;
}

const char *FFMpegDecoder::ImplementationName() const {
    return "FFmpeg-Decoder";
}

bool FFMpegDecoder::IsInitialized() const {
    return av_context_ != nullptr;
}

bool FFMpegDecoder::IsSupported() {
    return true;
}

std::unique_ptr<FFMpegDecoder> FFMpegDecoder::Create() {
    return absl::make_unique<FFMpegDecoder>();
}

