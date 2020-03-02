//
// Created by anba8005 on 26/02/2020.
//

#ifndef NDI_WEBRTC_PEER_WORKER_CUSTOMDECODERFACTORY_H
#define NDI_WEBRTC_PEER_WORKER_CUSTOMDECODERFACTORY_H

#include "CodecUtils.h"
#include "api/video_codecs/sdp_video_format.h"
#include "api/video_codecs/video_decoder.h"
#include "api/video_codecs/video_decoder_factory.h"
#include "rtc_base/system/rtc_export.h"
#include "media/base/h264_profile_level_id.h"
#include "../json.hpp"

using json = nlohmann::json;

class CustomDecoderFactory : public webrtc::VideoDecoderFactory {
public:
    std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override;

    std::unique_ptr<webrtc::VideoDecoder> CreateVideoDecoder(const webrtc::SdpVideoFormat &format) override;

    void setConfiguration(json configuration);

    static std::unique_ptr<CustomDecoderFactory> Create();

    explicit CustomDecoderFactory();

    virtual ~CustomDecoderFactory() override ;

private:
    CodecUtils::HardwareType hardware_type_;
};


#endif //NDI_WEBRTC_PEER_WORKER_CUSTOMDECODERFACTORY_H
