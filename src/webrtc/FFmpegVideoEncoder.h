//
// Created by anba8005 on 29/02/2020.
//

#ifndef NDI_WEBRTC_PEER_WORKER_FFMPEGVIDEOENCODER_H
#define NDI_WEBRTC_PEER_WORKER_FFMPEGVIDEOENCODER_H

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/hwcontext.h"
}

#include "CodecUtils.h"
#include "media/base/codec.h"
#include "modules/video_coding/include/video_codec_interface.h"
#include "rtc_base/system/rtc_export.h"
#include "media/base/h264_profile_level_id.h"

struct AVCodecContextDeleter {
    void operator()(AVCodecContext *ptr) const { avcodec_free_context(&ptr); }
};

struct AVFrameDeleter {
    void operator()(AVFrame *ptr) const { av_frame_free(&ptr); }
};

struct AVBufferDeleter {
    void operator()(AVBufferRef *ptr) const { av_buffer_unref(&ptr); }
};

class RTC_EXPORT FFmpegVideoEncoder : public webrtc::VideoEncoder {
public:
    explicit FFmpegVideoEncoder(const cricket::VideoCodec &codec, double frame_rate,
                                CodecUtils::HardwareType hardware_type);

    virtual ~FFmpegVideoEncoder() override;

    static std::unique_ptr<FFmpegVideoEncoder>
    Create(const cricket::VideoCodec &codec, double frame_rate, CodecUtils::HardwareType hardware_type);

    int InitEncode(const webrtc::VideoCodec *codec_settings, int number_of_cores, size_t max_payload_size) override;

    int Encode(const webrtc::VideoFrame &input_image, const std::vector<webrtc::VideoFrameType> *frame_types) override;

    int RegisterEncodeCompleteCallback(webrtc::EncodedImageCallback *callback) override;

    void SetRates(const RateControlParameters &parameters) override;

    void OnPacketLossRateUpdate(float packet_loss_rate) override;

    void OnRttUpdate(int64_t rtt_ms) override;

    void OnLossNotification(const LossNotification &loss_notification) override;

    EncoderInfo GetEncoderInfo() const override;

    int Release() override;


private:
    webrtc::VideoCodecType codec_type_;
    CodecUtils::HardwareType hardware_type_;
    absl::optional<webrtc::H264::ProfileLevelId> coder_profile_level_;
    std::unique_ptr<AVCodecContext, AVCodecContextDeleter> av_context_;
    std::unique_ptr<AVFrame, AVFrameDeleter> av_frame_;
    std::unique_ptr<AVBufferRef, AVBufferDeleter> hw_context_;
    webrtc::EncodedImageCallback *encoded_image_callback_;
    int width_;
    int height_;
    double frame_rate_;

    bool IsInitialized() const;

    AVHWDeviceType findHWDeviceType(const char *name);

    AVPixelFormat getDevicePixelFormat();

	AVPixelFormat getFramePixelFormat();

	bool isDeviceNeeded();

    bool createHWContext(AVHWDeviceType type);

    const char *findEncoderName(webrtc::VideoCodecType codec_type);

    webrtc::VideoCodecType findCodecType(std::string name);

    int setHWFrameContext(AVCodecContext *ctx, AVBufferRef *hw_device_ctx, int width, int height);

    int32_t NextNaluPosition(uint8_t *buffer, size_t buffer_size, uint8_t *sc_length);

    int getCodecProfile();

    int getCodecLevel();

};


#endif //NDI_WEBRTC_PEER_WORKER_FFMPEGVIDEOENCODER_H
