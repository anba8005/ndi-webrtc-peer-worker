//
// Created by anba8005 on 26/02/2020.
//

#include "CustomDecoderFactory.h"
#include "FFmpegVideoDecoder.h"
#include "modules/video_coding/codecs/h264/include/h264.h"
#include "modules/video_coding/codecs/vp8/include/vp8.h"
#include "modules/video_coding/codecs/vp9/include/vp9.h"
#include "media/base/media_constants.h"
#include "absl/strings/match.h"

#include <iostream>

CustomDecoderFactory::CustomDecoderFactory() : hardware_type_(CodecUtils::HW_TYPE_NONE) {
}


CustomDecoderFactory::~CustomDecoderFactory() {
}

std::vector<webrtc::SdpVideoFormat> CustomDecoderFactory::GetSupportedFormats() const {
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

std::unique_ptr<webrtc::VideoDecoder> CustomDecoderFactory::CreateVideoDecoder(const webrtc::SdpVideoFormat &format) {
    if (hasSoftwareOverride(format.name) || hardware_type_ == CodecUtils::HW_TYPE_NONE) {
        // use internal decoder (when possible)
        if (absl::EqualsIgnoreCase(format.name, cricket::kVp8CodecName))
            return webrtc::VP8Decoder::Create();
        if (absl::EqualsIgnoreCase(format.name, cricket::kVp9CodecName))
            return webrtc::VP9Decoder::Create();
        if (absl::EqualsIgnoreCase(format.name, cricket::kH264CodecName))
            return webrtc::H264Decoder::Create();
        if (absl::EqualsIgnoreCase(format.name, cricket::kH265CodecName))
            return FFmpegVideoDecoder::Create(cricket::kH265CodecName, CodecUtils::HW_TYPE_NONE);
    } else {
        // use ffmpeg decoder
        if (absl::EqualsIgnoreCase(format.name, cricket::kVp8CodecName))
            return FFmpegVideoDecoder::Create(cricket::kVp8CodecName, hardware_type_);
        if (absl::EqualsIgnoreCase(format.name, cricket::kVp9CodecName))
            return FFmpegVideoDecoder::Create(cricket::kVp9CodecName, hardware_type_);
        if (absl::EqualsIgnoreCase(format.name, cricket::kH264CodecName))
            return FFmpegVideoDecoder::Create(cricket::kH264CodecName, hardware_type_);
        if (absl::EqualsIgnoreCase(format.name, cricket::kH265CodecName))
            return FFmpegVideoDecoder::Create(cricket::kH265CodecName, hardware_type_);
    }
    return nullptr;
}

//
//
//

std::unique_ptr<CustomDecoderFactory> CustomDecoderFactory::Create() {
    return absl::make_unique<CustomDecoderFactory>();
}

void CustomDecoderFactory::setConfiguration(json configuration) {
    std::string type = configuration.value("hardware", "");
    hardware_type_ = CodecUtils::ParseHardwareType(type);
    //
    software_override_.clear();
    json software = configuration["software"];
    if (software != nullptr && software.is_array()) {
        for (auto &codec : software) {
            software_override_.push_back(codec.get<std::string>());
        }
    }
}

bool CustomDecoderFactory::hasSoftwareOverride(std::string codec) {
    for (auto &override : software_override_) {
        if (absl::EqualsIgnoreCase(override, codec)) {
            return true;
        }
    }
    return false;
}

