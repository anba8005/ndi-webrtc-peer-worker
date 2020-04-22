//
// Created by anba8005 on 29/02/2020.
//

#include "FFmpegVideoEncoder.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "third_party/libyuv/include/libyuv/planar_functions.h"

#define MAX_NALUS_PERFRAME 32
#define NAL_SC_LENGTH 4
#define DEFAULT_FPS (AVRational) {25, 1}

FFmpegVideoEncoder::FFmpegVideoEncoder(const cricket::VideoCodec &codec, double frame_rate,
                                       CodecUtils::HardwareType hardware_type) : encoded_image_callback_(
        nullptr),
                                                                                 width_(0),
                                                                                 hardware_type_(hardware_type),
                                                                                 height_(0),
                                                                                 frame_rate_(frame_rate) {
    codec_type_ = findCodecType(codec.name);
    coder_profile_level_ = webrtc::H264::ParseSdpProfileLevelId(codec.params);
}

FFmpegVideoEncoder::~FFmpegVideoEncoder() {
    Release();
}

std::unique_ptr<FFmpegVideoEncoder> FFmpegVideoEncoder::Create(const cricket::VideoCodec &codec, double frame_rate,
                                                               CodecUtils::HardwareType hardware_type) {
    return absl::make_unique<FFmpegVideoEncoder>(codec, frame_rate, hardware_type);
}

int
FFmpegVideoEncoder::InitEncode(const webrtc::VideoCodec *codec_settings, int number_of_cores, size_t max_payload_size) {
    if (codec_settings && codec_settings->codecType != codec_type_) {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;

    }

    if (codec_settings->maxFramerate == 0 || codec_settings->width < 1 || codec_settings->height < 1) {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }

    // Release necessary in case of re-initializing.
    int32_t ret = Release();
    if (ret != WEBRTC_VIDEO_CODEC_OK) {
        return ret;
    }
    RTC_DCHECK(!av_context_);

    // Save dimensions
    width_ = codec_settings->width;
    height_ = codec_settings->height;

    // init HW device
    auto hardware_type_str = CodecUtils::ConvertHardwareTypeToString(hardware_type_);
    AVHWDeviceType type = findHWDeviceType(hardware_type_str.c_str());
    if (type == AV_HWDEVICE_TYPE_NONE || !createHWContext(type)) {
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    // find codec name
    const char *codec_name = findEncoderName(codec_type_, type);
    if (!codec_name) {
        RTC_LOG(LS_ERROR) << "Device/codec type combination is not supported";
        Release();
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    // find codec
    AVCodec *codec = avcodec_find_encoder_by_name(codec_name);
    if (!codec) {
        RTC_LOG(LS_ERROR) << "Could not find " << codec_name << " encoder";
        Release();
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    // Initialize AVCodecContext.
    av_context_.reset(avcodec_alloc_context3(codec));
    av_context_->width = width_;
    av_context_->height = height_;
    av_context_->time_base = (AVRational) {1, 90000};
    av_context_->sample_aspect_ratio = (AVRational) {1, 1};
    av_context_->pix_fmt = AV_PIX_FMT_VAAPI;
    // fill configuration
    AVRational fps = frame_rate_ > 0 ? av_d2q(frame_rate_, 65535) : DEFAULT_FPS;
    av_context_->framerate = fps;
    av_context_->max_b_frames = 0;
    av_context_->gop_size = 100;
    av_context_->bit_rate = codec_settings->maxBitrate * 1000;
    AVDictionary *opts = NULL;
    av_dict_set(&opts, "rc_mode", "CBR", 0);
    //
    int profile = getCodecProfile();
    if (profile != FF_PROFILE_UNKNOWN) {
        av_context_->profile = profile;
    }
    //
    int level = getCodecLevel();
    if (level != -1) {
        av_context_->level = level;
    }

    /* set hw_frames_ctx for encoder's AVCodecContext */
    if (!setHWFrameContext(av_context_.get(), hw_context_.get(), width_, height_) < 0) {
        RTC_LOG(LS_ERROR) << "Failed to set hwframe context";
        Release();
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    // open codec
    int res = avcodec_open2(av_context_.get(), codec, &opts);
    if (res < 0) {
        RTC_LOG(LS_ERROR) << "avcodec_open2 error: " << res;
        Release();
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    // alloc frame
    av_frame_.reset(av_frame_alloc());
    RTC_CHECK(av_frame_.get());
    av_frame_->width = width_;
    av_frame_->height = height_;
    av_frame_->format = AV_PIX_FMT_NV12;
    res = av_frame_get_buffer(av_frame_.get(), 32);
    if (res < 0) {
        RTC_LOG(LS_ERROR) << "av_frame_get_buffer error: " << res;
        Release();
        return WEBRTC_VIDEO_CODEC_ERROR;
    }
    return WEBRTC_VIDEO_CODEC_OK;
}

int
FFmpegVideoEncoder::Encode(const webrtc::VideoFrame &input_image,
                           const std::vector<webrtc::VideoFrameType> *frame_types) {
    if (!encoded_image_callback_) {
        RTC_LOG(LS_WARNING)
        << "InitEncode() has been called, but a callback function "
        << "has not been set with RegisterEncodeCompleteCallback()";
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }

    if (!IsInitialized()) {
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }

    if (input_image.size() >
        static_cast<size_t>(std::numeric_limits<int>::max())) {
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    // convert input image to nv12
    rtc::scoped_refptr<webrtc::I420BufferInterface> buffer(input_image.video_frame_buffer()->ToI420());
    int res = libyuv::I420ToNV12(buffer->DataY(), buffer->StrideY(), buffer->DataU(), buffer->StrideU(),
                                 buffer->DataV(),
                                 buffer->StrideV(), av_frame_->data[0], av_frame_->linesize[0], av_frame_->data[1],
                                 av_frame_->linesize[1], width_, height_);
    if (res != 0) {
        RTC_LOG(LS_ERROR) << "I420ToNV12 error";
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    // get & fill hw frame
    std::unique_ptr<AVFrame, AVFrameDeleter> hw_frame(av_frame_alloc());
    res = av_hwframe_get_buffer(av_context_->hw_frames_ctx, hw_frame.get(), 0);
    if (res < 0) {
        RTC_LOG(LS_ERROR) << "av_hwframe_get_buffer error";
        char buffer[AV_ERROR_MAX_STRING_SIZE];
        RTC_LOG(LS_ERROR) << "Error code: " << av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, res);
        return WEBRTC_VIDEO_CODEC_ERROR;
    }
    res = av_hwframe_transfer_data(hw_frame.get(), av_frame_.get(), 0);
    if (res < 0) {
        RTC_LOG(LS_ERROR) << "av_hwframe_transfer_data error";
        char buffer[AV_ERROR_MAX_STRING_SIZE];
        RTC_LOG(LS_ERROR) << "Error code: " << av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, res);
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    // set ts
    hw_frame->pts = input_image.timestamp();

    // decide if keyframe required
    if (frame_types) {
        for (auto frame_type : *frame_types) {
            if (frame_type == webrtc::VideoFrameType::kVideoFrameKey) {
                hw_frame->pict_type = AV_PICTURE_TYPE_I;
                break;
            }
        }
    }

    // init packet
    AVPacket packet;
    av_init_packet(&packet);
    packet.data = nullptr;
    packet.size = 0;

    // send frame
    res = avcodec_send_frame(av_context_.get(), hw_frame.get());
    if (res < 0) {
        RTC_LOG(LS_ERROR) << "avcodec_send_frame error";
        char buffer[AV_ERROR_MAX_STRING_SIZE];
        RTC_LOG(LS_ERROR) << "Error code: " << av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, res);
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    // encode cycle
    while (1) {
        res = avcodec_receive_packet(av_context_.get(), &packet);
        if (res == AVERROR(EAGAIN)) {
            break;
        } else if (res < 0) {
            char buffer[AV_ERROR_MAX_STRING_SIZE];
            RTC_LOG(LS_ERROR) << "Error code: " << av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, res);
            return WEBRTC_VIDEO_CODEC_ERROR;
        }

        // write packet
        webrtc::EncodedImage encodedFrame(packet.data, packet.size, packet.size);
        encodedFrame._encodedHeight = input_image.height();
        encodedFrame._encodedWidth = input_image.width();
        encodedFrame._completeFrame = true;
        encodedFrame.SetTimestamp(packet.pts);
        encodedFrame._frameType = (packet.flags & AV_PKT_FLAG_KEY) ? webrtc::VideoFrameType::kVideoFrameKey
                                                                   : webrtc::VideoFrameType::kVideoFrameDelta;
        //
        webrtc::CodecSpecificInfo info;
        memset(&info, 0, sizeof(info));
        info.codecType = codec_type_;
        // Generate a header describing a single fragment.
        webrtc::RTPFragmentationHeader header;
        memset(&header, 0, sizeof(header));

        int32_t scPositions[MAX_NALUS_PERFRAME + 1] = {};
        uint8_t scLengths[MAX_NALUS_PERFRAME + 1] = {};
        int32_t scPositionsLength = 0;
        int32_t scPosition = 0;
        while (scPositionsLength < MAX_NALUS_PERFRAME) {
            uint8_t sc_length = 0;
            int32_t naluPosition = NextNaluPosition(
                    packet.data + scPosition, packet.size - scPosition, &sc_length);
            if (naluPosition < 0) {
                break;
            }
            scPosition += naluPosition;
            scLengths[scPositionsLength] = sc_length;
            scPositions[scPositionsLength] = scPosition;
            scPositionsLength++;
            scPosition += sc_length;
        }
        if (scPositionsLength == 0) {
            RTC_LOG(LS_ERROR) << "Start code is not found for codec!";
            av_packet_unref(&packet);
            return WEBRTC_VIDEO_CODEC_ERROR;
        }
        scPositions[scPositionsLength] = packet.size;
        header.VerifyAndAllocateFragmentationHeader(scPositionsLength);
        for (int i = 0; i < scPositionsLength; i++) {
            header.fragmentationOffset[i] = scPositions[i] + scLengths[i];
            header.fragmentationLength[i] =
                    scPositions[i + 1] - header.fragmentationOffset[i];
        }

        // send
        const auto result = encoded_image_callback_->OnEncodedImage(encodedFrame, &info, &header);
        if (result.error != webrtc::EncodedImageCallback::Result::Error::OK) {
            RTC_LOG(LS_ERROR) << "OnEncodedImage error";
            av_packet_unref(&packet);
            return WEBRTC_VIDEO_CODEC_ERROR;
        }

        // clean packet
        av_packet_unref(&packet);
    }

    return WEBRTC_VIDEO_CODEC_OK;
}

int FFmpegVideoEncoder::RegisterEncodeCompleteCallback(webrtc::EncodedImageCallback *callback) {
    encoded_image_callback_ = callback;
    return WEBRTC_VIDEO_CODEC_OK;
}

void FFmpegVideoEncoder::SetRates(const webrtc::VideoEncoder::RateControlParameters &parameters) {
    if (!IsInitialized()) {
        RTC_LOG(LS_WARNING) << "SetRates() while not initialized";
        return;
    }

    if (parameters.framerate_fps < 1.0) {
        RTC_LOG(LS_WARNING) << "Unsupported framerate (must be >= 1.0";
        return;
    }
}

void FFmpegVideoEncoder::OnPacketLossRateUpdate(float packet_loss_rate) {
}

void FFmpegVideoEncoder::OnRttUpdate(int64_t rtt_ms) {
}

void FFmpegVideoEncoder::OnLossNotification(const webrtc::VideoEncoder::LossNotification &loss_notification) {
}

webrtc::VideoEncoder::EncoderInfo FFmpegVideoEncoder::GetEncoderInfo() const {
    EncoderInfo info;
    info.supports_native_handle = false;
    info.is_hardware_accelerated = true;
    info.has_internal_source = false;
    info.implementation_name = "FFmpeg";
    info.has_trusted_rate_controller = true;
    info.scaling_settings = VideoEncoder::ScalingSettings::kOff;
    info.supports_simulcast = false;
    return info;
}

int FFmpegVideoEncoder::Release() {
    av_context_.reset();
    hw_context_.reset();
    av_frame_.reset();
    return WEBRTC_VIDEO_CODEC_OK;
}

bool FFmpegVideoEncoder::IsInitialized() const {
    return av_context_ != nullptr;
}

AVHWDeviceType FFmpegVideoEncoder::findHWDeviceType(const char *name) {
    AVHWDeviceType type = av_hwdevice_find_type_by_name(name);
    if (type == AV_HWDEVICE_TYPE_NONE) {
        RTC_LOG(LS_ERROR) << "Device type " << name << " is not supported";
        RTC_LOG(LS_ERROR) << "Available device types:";
        while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
            RTC_LOG(LS_ERROR) << av_hwdevice_get_type_name(type);
    }
    return type;
}

bool FFmpegVideoEncoder::createHWContext(AVHWDeviceType type) {
    AVBufferRef *context = nullptr;
    int result = av_hwdevice_ctx_create(&context, type, NULL, NULL, 0);
    if (result < 0) {
        RTC_LOG(LS_WARNING) << "Failed to create specified HW device: " << result;
        return false;
    }

    RTC_DCHECK(context);
    hw_context_.reset(context);
    return true;
}

const char *FFmpegVideoEncoder::findEncoderName(webrtc::VideoCodecType codec_type, AVHWDeviceType device_type) {
    if (device_type == AV_HWDEVICE_TYPE_VAAPI) {
        if (codec_type == webrtc::kVideoCodecH264) {
            return "h264_vaapi";
        } else if (codec_type == webrtc::kVideoCodecH265) {
            return "hevc_vaapi";
        } else if (codec_type == webrtc::kVideoCodecVP8) {
            return "vp8_vaapi";
        } else if (codec_type == webrtc::kVideoCodecVP9) {
            return "vp9_vaapi";
        } else {
            return nullptr;
        }
    } else {
        return nullptr;
    }

}

webrtc::VideoCodecType FFmpegVideoEncoder::findCodecType(std::string name) {
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

int FFmpegVideoEncoder::setHWFrameContext(AVCodecContext *ctx, AVBufferRef *hw_device_ctx, int width, int height) {
    AVBufferRef *hw_frames_ref;
    AVHWFramesContext *frames_ctx = NULL;
    int err = 0;

    if (!(hw_frames_ref = av_hwframe_ctx_alloc(hw_device_ctx))) {
        RTC_LOG(LS_ERROR) << "Failed to create VAAPI frame context";
        return -1;
    }
    frames_ctx = (AVHWFramesContext *) (hw_frames_ref->data);
    frames_ctx->format = AV_PIX_FMT_VAAPI;
    frames_ctx->sw_format = AV_PIX_FMT_NV12;
    frames_ctx->width = width;
    frames_ctx->height = height;
    frames_ctx->initial_pool_size = 20;
    if ((err = av_hwframe_ctx_init(hw_frames_ref)) < 0) {
        RTC_LOG(LS_ERROR) << "Failed to initialize VAAPI frame context";
        char buffer[AV_ERROR_MAX_STRING_SIZE];
        RTC_LOG(LS_ERROR) << "Error code: " << av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, err);
        av_buffer_unref(&hw_frames_ref);
        return err;
    }
    ctx->hw_frames_ctx = av_buffer_ref(hw_frames_ref);
    if (!ctx->hw_frames_ctx)
        err = AVERROR(ENOMEM);

    av_buffer_unref(&hw_frames_ref);
    return err;
}

int32_t FFmpegVideoEncoder::NextNaluPosition(uint8_t *buffer, size_t buffer_size, uint8_t *sc_length) {
    if (buffer_size < NAL_SC_LENGTH) {
        return -1;
    }
    *sc_length = 0;
    uint8_t *head = buffer;
    // Set end buffer pointer to 4 bytes before actual buffer end so we can
    // access head[1], head[2] and head[3] in a loop without buffer overrun.
    uint8_t *end = buffer + buffer_size - NAL_SC_LENGTH;

    while (head < end) {
        if (head[0]) {
            head++;
            continue;
        }
        if (head[1]) {  // got 00xx
            head += 2;
            continue;
        }
        if (head[2] > 1) {  // got 0000xx
            head += 3;
            continue;
        }
        if (head[2] != 1 && head[3] != 0x01) {  // got 000000xx
            head++;                               // xx != 1, continue searching.
            continue;
        }

        *sc_length = (head[2] == 1) ? 3 : 4;
        return (int32_t) (head - buffer);
    }
    return -1;
}

int FFmpegVideoEncoder::getCodecProfile() {
    if (codec_type_ == webrtc::kVideoCodecH264) {
        if (coder_profile_level_.has_value()) {
            switch (coder_profile_level_->profile) {
                case webrtc::H264::kProfileHigh:
                case webrtc::H264::kProfileConstrainedHigh:
                    return FF_PROFILE_H264_HIGH;
                case webrtc::H264::kProfileBaseline:
                case webrtc::H264::kProfileConstrainedBaseline:
                    return FF_PROFILE_H264_CONSTRAINED_BASELINE;
                case webrtc::H264::kProfileMain:
                    return FF_PROFILE_H264_MAIN;
                default:
                    return FF_PROFILE_H264_CONSTRAINED_BASELINE;
            }
        } else {
            return FF_PROFILE_H264_CONSTRAINED_BASELINE; // DEFAULT
        }
    } else if (codec_type_ == webrtc::kVideoCodecH265) {
        return FF_PROFILE_HEVC_MAIN;
    } else {
        return FF_PROFILE_UNKNOWN;
    }
}

int FFmpegVideoEncoder::getCodecLevel() {
    if (codec_type_ == webrtc::kVideoCodecH264) {
        if (coder_profile_level_.has_value()) {
            return coder_profile_level_->level;
        } else {
            return 31; // DEFAULT
        }
    } else if (codec_type_ == webrtc::kVideoCodecH265) {
        return 120;
    } else {
        return -1;
    }
}
