//
// Created by anba8005 on 29/02/2020.
//

#include "FFmpegVideoEncoder.h"

FFmpegVideoEncoder::FFmpegVideoEncoder() {

}

FFmpegVideoEncoder::~FFmpegVideoEncoder() {

}

std::unique_ptr<FFmpegVideoEncoder> FFmpegVideoEncoder::Create(cricket::VideoCodec format) {
    return std::unique_ptr<FFmpegVideoEncoder>();
}

int FFmpegVideoEncoder::InitEncode(const webrtc::VideoCodec *codec_settings, int number_of_cores, size_t max_payload_size) {

}

int
FFmpegVideoEncoder::Encode(const webrtc::VideoFrame &input_image, const std::vector<webrtc::VideoFrameType> *frame_types) {
    return 0;
}

int FFmpegVideoEncoder::RegisterEncodeCompleteCallback(webrtc::EncodedImageCallback *callback) {
    return 0;
}

void FFmpegVideoEncoder::SetRates(const webrtc::VideoEncoder::RateControlParameters &parameters) {

}

void FFmpegVideoEncoder::OnPacketLossRateUpdate(float packet_loss_rate) {
    VideoEncoder::OnPacketLossRateUpdate(packet_loss_rate);
}

void FFmpegVideoEncoder::OnRttUpdate(int64_t rtt_ms) {
    VideoEncoder::OnRttUpdate(rtt_ms);
}

void FFmpegVideoEncoder::OnLossNotification(const webrtc::VideoEncoder::LossNotification &loss_notification) {
    VideoEncoder::OnLossNotification(loss_notification);
}

webrtc::VideoEncoder::EncoderInfo FFmpegVideoEncoder::GetEncoderInfo() const {
    return VideoEncoder::GetEncoderInfo();
}

int FFmpegVideoEncoder::Release() {
    return 0;
}
