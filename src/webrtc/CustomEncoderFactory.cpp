//
// Created by anba8005 on 26/02/2020.
//

#include "CustomEncoderFactory.h"
#include "CodecUtils.h"
#include "modules/video_coding/codecs/h264/include/h264.h"
#include "modules/video_coding/codecs/vp8/include/vp8.h"
#include "modules/video_coding/codecs/vp9/include/vp9.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "media/base/media_constants.h"
#include "absl/strings/match.h"
#include "FFmpegVideoEncoder.h"

#include <iostream>

CustomEncoderFactory::CustomEncoderFactory() : frame_rate(0) {
}

CustomEncoderFactory::~CustomEncoderFactory() {
}

std::unique_ptr<webrtc::VideoEncoder> CustomEncoderFactory::CreateVideoEncoder(const webrtc::SdpVideoFormat &format) {
    std::cerr << "CREATE ENCODER " << format.name << std::endl;
    if (absl::EqualsIgnoreCase(format.name, cricket::kVp8CodecName))
        return webrtc::VP8Encoder::Create();
    if (absl::EqualsIgnoreCase(format.name, cricket::kVp9CodecName))
        return webrtc::VP9Encoder::Create(cricket::VideoCodec(format));
    if (absl::EqualsIgnoreCase(format.name, cricket::kH264CodecName))
        return FFmpegVideoEncoder::Create(cricket::VideoCodec(format), frame_rate);
//        return webrtc::H264Encoder::Create(cricket::VideoCodec(format));
    if (absl::EqualsIgnoreCase(format.name, cricket::kH265CodecName))
        return FFmpegVideoEncoder::Create(cricket::VideoCodec(format), frame_rate);
    return nullptr;
}

std::vector<webrtc::SdpVideoFormat> CustomEncoderFactory::GetSupportedFormats() const {
    std::vector<webrtc::SdpVideoFormat> supported_codecs;
    supported_codecs.push_back(webrtc::SdpVideoFormat(cricket::kVp8CodecName));
    for (const webrtc::SdpVideoFormat &format : webrtc::SupportedVP9Codecs())
        supported_codecs.push_back(format);
    for (const webrtc::SdpVideoFormat &format : CodecUtils::GetAuxH264Codecs())
        supported_codecs.push_back(format);
    for (const webrtc::SdpVideoFormat &format : webrtc::SupportedH264Codecs())
        supported_codecs.push_back(format);
    for (const webrtc::SdpVideoFormat &format : CodecUtils::GetSupportedH265Codecs())
        supported_codecs.push_back(format);
    return supported_codecs;
}

webrtc::VideoEncoderFactory::CodecInfo
CustomEncoderFactory::QueryVideoEncoder(const webrtc::SdpVideoFormat &format) const {
    webrtc::VideoEncoderFactory::CodecInfo info;
    info.is_hardware_accelerated = false;
    info.has_internal_source = false;
    return info;
}

std::unique_ptr<CustomEncoderFactory> CustomEncoderFactory::Create() {
    return absl::make_unique<CustomEncoderFactory>();
}

void CustomEncoderFactory::updateFrameRate(int num, int den) {
    frame_rate = av_q2d({ num, den});
}

