//
// Created by anba8005 on 26/02/2020.
//

#define OWT_ENABLE_H265

#include "SoftwareDecoderFactory.h"
#include "CodecUtils.h"
#include "FFMpegDecoder.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "modules/video_coding/codecs/h264/include/h264.h"
#include "modules/video_coding/codecs/vp8/include/vp8.h"
#include "modules/video_coding/codecs/vp9/include/vp9.h"
#include "media/base/media_constants.h"
#include "absl/strings/match.h"

#include <iostream>

SoftwareDecoderFactory::SoftwareDecoderFactory() {
}


SoftwareDecoderFactory::~SoftwareDecoderFactory() {
}

std::vector<webrtc::SdpVideoFormat> SoftwareDecoderFactory::GetSupportedFormats() const {
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

std::unique_ptr<webrtc::VideoDecoder> SoftwareDecoderFactory::CreateVideoDecoder(const webrtc::SdpVideoFormat &format) {
    std::cerr << "CREATE DECODER " << format.name << std::endl;
    if (absl::EqualsIgnoreCase(format.name, cricket::kVp8CodecName))
        return webrtc::VP8Decoder::Create();
    if (absl::EqualsIgnoreCase(format.name, cricket::kVp9CodecName))
        return webrtc::VP9Decoder::Create();
    if (absl::EqualsIgnoreCase(format.name, cricket::kH264CodecName))
        return webrtc::H264Decoder::Create();
    if (absl::EqualsIgnoreCase(format.name, cricket::kH265CodecName)) {
        return FFMpegDecoder::Create();
    }
    return nullptr;
}

//
//
//

std::unique_ptr<webrtc::VideoDecoderFactory> SoftwareDecoderFactory::Create() {
    return absl::make_unique<SoftwareDecoderFactory>();
}

