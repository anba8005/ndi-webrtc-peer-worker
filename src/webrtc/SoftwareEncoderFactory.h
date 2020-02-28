//
// Created by anba8005 on 26/02/2020.
//

#ifndef NDI_WEBRTC_PEER_WORKER_SOFTWAREENCODERFACTORY_H
#define NDI_WEBRTC_PEER_WORKER_SOFTWAREENCODERFACTORY_H

#include <vector>
#include "api/video_codecs/video_encoder.h"
#include "api/video_codecs/sdp_video_format.h"
#include "api/video_codecs/video_encoder_factory.h"

class SoftwareEncoderFactory : public webrtc::VideoEncoderFactory {
public:
    std::unique_ptr<webrtc::VideoEncoder> CreateVideoEncoder(const webrtc::SdpVideoFormat &format) override;

    std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override;

    webrtc::VideoEncoderFactory::CodecInfo QueryVideoEncoder(const webrtc::SdpVideoFormat &format) const override;

    static std::unique_ptr<webrtc::VideoEncoderFactory> Create();

    explicit SoftwareEncoderFactory();

    virtual ~SoftwareEncoderFactory();
};


#endif //NDI_WEBRTC_PEER_WORKER_SOFTWAREENCODERFACTORY_H
