//
// Created by anba8005 on 29/02/2020.
//

#ifndef NDI_WEBRTC_PEER_WORKER_FFMPEGVIDEOENCODER_H
#define NDI_WEBRTC_PEER_WORKER_FFMPEGVIDEOENCODER_H

#include "media/base/codec.h"
#include "modules/video_coding/include/video_codec_interface.h"
#include "rtc_base/system/rtc_export.h"

class RTC_EXPORT FFmpegVideoEncoder : public webrtc::VideoEncoder {
public:
    explicit FFmpegVideoEncoder();

    virtual ~FFmpegVideoEncoder();

    static std::unique_ptr<FFmpegVideoEncoder> Create(cricket::VideoCodec format);

    int InitEncode(const webrtc::VideoCodec *codec_settings, int number_of_cores, size_t max_payload_size) override;

    int Encode(const webrtc::VideoFrame &input_image, const std::vector<webrtc::VideoFrameType> *frame_types) override;

    int RegisterEncodeCompleteCallback(webrtc::EncodedImageCallback *callback) override;

    void SetRates(const RateControlParameters &parameters) override;

    void OnPacketLossRateUpdate(float packet_loss_rate) override;

    void OnRttUpdate(int64_t rtt_ms) override;

    void OnLossNotification(const LossNotification &loss_notification) override;

    EncoderInfo GetEncoderInfo() const override;

    int Release() override;
};


#endif //NDI_WEBRTC_PEER_WORKER_FFMPEGVIDEOENCODER_H
