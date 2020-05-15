//
// Created by anba8005 on 26/02/2020.
//

#ifndef NDI_WEBRTC_PEER_WORKER_CUSTOMENCODERFACTORY_H
#define NDI_WEBRTC_PEER_WORKER_CUSTOMENCODERFACTORY_H

#ifdef WIN32
#define NOMINMAX
#undef min
#undef max
#endif

#include "CodecUtils.h"
#include "FrameRateUpdater.h"
#include <vector>
#include <atomic>
#include "api/video_codecs/sdp_video_format.h"
#include "api/video_codecs/video_encoder.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "rtc_base/system/rtc_export.h"
#include "media/base/h264_profile_level_id.h"
#include "../json.hpp"

using json = nlohmann::json;

class CustomEncoderFactory : public webrtc::VideoEncoderFactory, public FrameRateUpdater {
public:
    std::unique_ptr<webrtc::VideoEncoder> CreateVideoEncoder(const webrtc::SdpVideoFormat &format) override;

    std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override;

    webrtc::VideoEncoderFactory::CodecInfo QueryVideoEncoder(const webrtc::SdpVideoFormat &format) const override;

    static std::unique_ptr<CustomEncoderFactory> Create();

    void setConfiguration(json configuration);

    explicit CustomEncoderFactory();

    virtual ~CustomEncoderFactory() override;

    void updateFrameRate(int num, int den) override;
private:
    CodecUtils::HardwareType hardware_type_;
    std::atomic<double> frame_rate;
    std::vector<std::string> software_override_;
    bool disable_h264_high_profile_;

    bool hasSoftwareOverride(std::string codec);
};


#endif //NDI_WEBRTC_PEER_WORKER_CUSTOMENCODERFACTORY_H
