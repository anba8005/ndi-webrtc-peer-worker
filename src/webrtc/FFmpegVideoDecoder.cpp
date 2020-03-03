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

#include "FFmpegVideoDecoder.h"


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

FFmpegVideoDecoder::FFmpegVideoDecoder(std::string codec_name, CodecUtils::HardwareType hardware_type) : pool_(true),
                                                                                                         decoded_image_callback_(
                                                                                                                 nullptr),
                                                                                                         packet_data_(
                                                                                                                 nullptr),
                                                                                                         packet_data_size_(
                                                                                                                 0),
                                                                                                         hardware_type_(
                                                                                                                 hardware_type),
                                                                                                         hw_pixel_format_(
                                                                                                                 AV_PIX_FMT_NONE) {
    codec_type_ = findCodecType(codec_name);
}

FFmpegVideoDecoder::~FFmpegVideoDecoder() {
    Release();
}

int32_t FFmpegVideoDecoder::InitDecode(const webrtc::VideoCodec *codec_settings, int32_t number_of_cores) {
    if (codec_settings && codec_settings->codecType != codec_type_) {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }

    // Release necessary in case of re-initializing.
    int32_t ret = Release();
    if (ret != WEBRTC_VIDEO_CODEC_OK) {
        return ret;
    }
    RTC_DCHECK(!av_context_);

    // get HW device type
    AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
    if (hardware_type_ != CodecUtils::HW_TYPE_NONE) {
        auto hardware_type_str = CodecUtils::ConvertHardwareTypeToString(hardware_type_);
        type = findHWDeviceType(hardware_type_str.c_str());
    }

    // Initialize AVCodecContext.
    av_context_.reset(avcodec_alloc_context3(nullptr));

    av_context_->codec_type = AVMEDIA_TYPE_VIDEO;
    av_context_->codec_id = findCodecID(codec_type_);
    if (codec_settings) {
        av_context_->coded_width = codec_settings->width;
        av_context_->coded_height = codec_settings->height;
    }
    av_context_->pix_fmt = kPixelFormatDefault;
    av_context_->extradata = nullptr;
    av_context_->extradata_size = 0;
    av_context_->hwaccel_flags = AV_HWACCEL_FLAG_ALLOW_PROFILE_MISMATCH;

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

    // init HW part if available
    if (type != AV_HWDEVICE_TYPE_NONE) {
        // find pixel format
        hw_pixel_format_ = findHWPixelFormat(type, codec);
        //
        av_context_->get_format = getHWFormat;
        av_context_->opaque = this;
        //
        if (!createHWContext(type))
            return WEBRTC_VIDEO_CODEC_ERROR;
    }

    int res = avcodec_open2(av_context_.get(), codec, nullptr);
    if (res < 0) {
        RTC_LOG(LS_ERROR) << "avcodec_open2 error: " << res;
        Release();
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    return WEBRTC_VIDEO_CODEC_OK;
}

int32_t FFmpegVideoDecoder::Release() {
    if (packet_data_) {
        av_freep(&packet_data_);
        packet_data_size_ = 0;
    }
    av_context_.reset();
    hw_context_.reset();
    hw_pixel_format_ = AV_PIX_FMT_NONE;
    return WEBRTC_VIDEO_CODEC_OK;
}


int32_t FFmpegVideoDecoder::RegisterDecodeCompleteCallback(webrtc::DecodedImageCallback *callback) {
    decoded_image_callback_ = callback;
    return WEBRTC_VIDEO_CODEC_OK;
}

int32_t FFmpegVideoDecoder::Decode(const webrtc::EncodedImage &input_image, bool, int64_t render_time_ms) {
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

    // alloc frames
    std::unique_ptr<AVFrame, AVFrameDeleter> av_frame_(av_frame_alloc());
    std::unique_ptr<AVFrame, AVFrameDeleter> sw_frame_;

    result = avcodec_receive_frame(av_context_.get(), av_frame_.get());
    if (result < 0) {
        RTC_LOG(LS_ERROR) << "avcodec_receive_frame error: " << result;
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    // We don't expect reordering. Decoded frame tamestamp should match
    // the input one.
    RTC_DCHECK_EQ(av_frame_->reordered_opaque, frame_timestamp_us);

    AVFrame *frame;
    if (av_frame_->format == hw_pixel_format_) {
        /* retrieve data from GPU to CPU */
        sw_frame_.reset(av_frame_alloc());
        result = av_hwframe_transfer_data(sw_frame_.get(), av_frame_.get(), 0);
        if (result < 0) {
            RTC_LOG(LS_ERROR) << "Error transferring the data to system memory" << result;
            char buffer[AV_ERROR_MAX_STRING_SIZE];
            RTC_LOG(LS_ERROR) << "Error code: " << av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, result);
            return WEBRTC_VIDEO_CODEC_ERROR;
        }
        frame = sw_frame_.get();
    } else {
        frame = av_frame_.get();
    }

    // Create output buffer
    rtc::scoped_refptr<webrtc::I420Buffer> buffer = pool_.CreateBuffer(frame->width, frame->height);
    RTC_DCHECK(buffer.get());

    if (frame->format == AV_PIX_FMT_YUV420P) {
        // Fill output buffer
        RTC_CHECK_EQ(0, libyuv::I420Copy(frame->data[0], frame->linesize[0], frame->data[1],
                                         frame->linesize[1], frame->data[2], frame->linesize[2],
                                         buffer->MutableDataY(),
                                         buffer->StrideY(), buffer->MutableDataU(),
                                         buffer->StrideU(), buffer->MutableDataV(),
                                         buffer->StrideV(), frame->width, frame->height));
    } else if (frame->format == AV_PIX_FMT_NV12) {
        RTC_CHECK_EQ(0, libyuv::NV12ToI420(frame->data[0], frame->linesize[0], frame->data[1], frame->linesize[1],
                                           buffer->MutableDataY(), buffer->StrideY(), buffer->MutableDataU(),
                                           buffer->StrideU(), buffer->MutableDataV(),
                                           buffer->StrideV(), frame->width, frame->height));
    }


    webrtc::VideoFrame decoded_frame = webrtc::VideoFrame::Builder()
            .set_video_frame_buffer(buffer)
            .set_timestamp_rtp(input_image.Timestamp())
            .build();

    // Return decoded frame.
    decoded_image_callback_->Decoded(decoded_frame);

    return WEBRTC_VIDEO_CODEC_OK;
}

const char *FFmpegVideoDecoder::ImplementationName() const {
    return "FFmpeg-Decoder";
}

bool FFmpegVideoDecoder::IsInitialized() const {
    return av_context_ != nullptr;
}

bool FFmpegVideoDecoder::IsSupported() {
    return true;
}

std::unique_ptr<FFmpegVideoDecoder>
FFmpegVideoDecoder::Create(std::string codec_name, CodecUtils::HardwareType hardware_type) {
    return absl::make_unique<FFmpegVideoDecoder>(codec_name, hardware_type);
}

bool FFmpegVideoDecoder::createHWContext(AVHWDeviceType type) {
    AVBufferRef *context = nullptr;
    int result = av_hwdevice_ctx_create(&context, type, NULL, NULL, 0);
    if (result < 0) {
        RTC_LOG(LS_WARNING) << "Failed to create specified HW device: " << result;
        return false;
    }

    RTC_DCHECK(context);
    RTC_DCHECK(av_context_.get());

    av_context_->hw_device_ctx = av_buffer_ref(context);
    hw_context_.reset(context);

    return true;
}

AVHWDeviceType FFmpegVideoDecoder::findHWDeviceType(const char *name) {
    AVHWDeviceType type = av_hwdevice_find_type_by_name(name);
    if (type == AV_HWDEVICE_TYPE_NONE) {
        RTC_LOG(LS_ERROR) << "Device type " << name << " is not supported";
        RTC_LOG(LS_ERROR) << "Available device types:";
        while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
            RTC_LOG(LS_ERROR) << av_hwdevice_get_type_name(type);
    }
    return type;
}

AVPixelFormat FFmpegVideoDecoder::findHWPixelFormat(AVHWDeviceType type, AVCodec *codec) {
    for (int i = 0;; i++) {
        const AVCodecHWConfig *config = avcodec_get_hw_config(codec, i);
        if (!config) {
            RTC_LOG(LS_ERROR) << "Decoder " << codec->name << " does not support device type "
                              << av_hwdevice_get_type_name(type);
            return AV_PIX_FMT_NONE;
        }
        if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX && config->device_type == type) {
            return config->pix_fmt;
        }
    }
}

AVPixelFormat FFmpegVideoDecoder::getHWFormat(AVCodecContext *ctx, const AVPixelFormat *pix_fmts) {
    FFmpegVideoDecoder *decoder = static_cast<FFmpegVideoDecoder *>(ctx->opaque);
    RTC_DCHECK(decoder);

    const enum AVPixelFormat *p;

    for (p = pix_fmts; *p != -1; p++) {
        if (*p == decoder->hw_pixel_format_)
            return *p;
    }

    RTC_LOG(LS_ERROR) << "Failed to get HW surface format";
    return AV_PIX_FMT_NONE;
}

AVCodecID FFmpegVideoDecoder::findCodecID(webrtc::VideoCodecType type) {
    switch (type) {
        case webrtc::kVideoCodecVP8:
            return AV_CODEC_ID_VP8;
        case webrtc::kVideoCodecVP9:
            return AV_CODEC_ID_VP9;
        case webrtc::kVideoCodecH264:
            return AV_CODEC_ID_H264;
        case webrtc::kVideoCodecH265:
            return AV_CODEC_ID_HEVC;
        default:
            return AV_CODEC_ID_NONE;
    }
}

webrtc::VideoCodecType FFmpegVideoDecoder::findCodecType(std::string name) {
    if (name == cricket::kVp8CodecName) {
        return webrtc::kVideoCodecVP8;
    } else if (name == cricket::kVp9CodecName) {
        return webrtc::kVideoCodecVP9;
    } else if (name == cricket::kH264CodecName) {
        return webrtc::kVideoCodecH264;
    } else if (name == cricket::kH265CodecName) {
        return webrtc::kVideoCodecH265;
    } else {
        return webrtc::kVideoCodecGeneric;
    }
}

