//
// Created by anba8005 on 26/02/2020.
//

#include "CustomEncoderFactory.h"
#include "modules/video_coding/codecs/h264/include/h264.h"
#include "modules/video_coding/codecs/vp8/include/vp8.h"
#include "modules/video_coding/codecs/vp9/include/vp9.h"
#include "media/base/media_constants.h"
#include "absl/strings/match.h"
#ifdef WIN32
#include "win/msdkvideoencoder.h"
#else
#include "FFmpegVideoEncoder.h"
#endif

extern "C" {
#include "libavutil/rational.h"
}

#include <iostream>

CustomEncoderFactory::CustomEncoderFactory() : frame_rate(0), hardware_type_(CodecUtils::HW_TYPE_NONE),
                                               disable_h264_high_profile_(false) {
}

CustomEncoderFactory::~CustomEncoderFactory() {
}

std::unique_ptr<webrtc::VideoEncoder> CustomEncoderFactory::CreateVideoEncoder(const webrtc::SdpVideoFormat &format) {
    if (hasSoftwareOverride(format.name) || hardware_type_ == CodecUtils::HW_TYPE_NONE) {
        if (absl::EqualsIgnoreCase(format.name, cricket::kVp8CodecName))
            return webrtc::VP8Encoder::Create();
        if (absl::EqualsIgnoreCase(format.name, cricket::kVp9CodecName))
            return webrtc::VP9Encoder::Create(cricket::VideoCodec(format));
        if (absl::EqualsIgnoreCase(format.name, cricket::kH264CodecName))
            return webrtc::H264Encoder::Create(cricket::VideoCodec(format));
#ifdef WIN32
    } else if (hardware_type_ == CodecUtils::HW_TYPE_MFX) {
        if (absl::EqualsIgnoreCase(format.name, cricket::kVp8CodecName))
            return webrtc::VP8Encoder::Create();
        if (absl::EqualsIgnoreCase(format.name, cricket::kVp9CodecName))
            return webrtc::VP9Encoder::Create(cricket::VideoCodec(format));
        if (absl::EqualsIgnoreCase(format.name, cricket::kH264CodecName) || absl::EqualsIgnoreCase(format.name, cricket::kH265CodecName))
            return owt::base::MSDKVideoEncoder::Create(cricket::VideoCodec(format), frame_rate);
#else
    } else {
        return FFmpegVideoEncoder::Create(cricket::VideoCodec(format), frame_rate, hardware_type_);
#endif
    }
    return nullptr;
}

std::vector<webrtc::SdpVideoFormat> CustomEncoderFactory::GetSupportedFormats() const {
    std::vector<webrtc::SdpVideoFormat> supported_codecs;
    supported_codecs.push_back(webrtc::SdpVideoFormat(cricket::kVp8CodecName));
    for (const webrtc::SdpVideoFormat &format : webrtc::SupportedVP9Codecs())
        supported_codecs.push_back(format);
    if (!disable_h264_high_profile_) {
        for (const webrtc::SdpVideoFormat &format : CodecUtils::GetAuxH264Codecs())
            supported_codecs.push_back(format);
    }
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
    frame_rate = av_q2d({num, den});
    if (frame_rate > 30.0) {
        frame_rate = av_q2d({num / 2, den});
    }
}

void CustomEncoderFactory::setConfiguration(json configuration) {
    //
    std::string type = configuration.value("hardware", "");
    hardware_type_ = CodecUtils::ParseHardwareType(type);
    //
    disable_h264_high_profile_ = configuration.value("disableH264HighProfile", false);
    //
    software_override_.clear();
    json software = configuration["software"];
    if (software != nullptr && software.is_array()) {
        for (auto &codec : software) {
            software_override_.push_back(codec.get<std::string>());
        }
    }
}

bool CustomEncoderFactory::hasSoftwareOverride(std::string codec) {
    for (auto &override : software_override_) {
        if (absl::EqualsIgnoreCase(override, codec)) {
            return true;
        }
    }
    return false;
}
