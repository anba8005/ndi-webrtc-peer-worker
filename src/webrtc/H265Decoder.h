//
// Created by anba8005 on 27/02/2020.
//

#ifndef NDI_WEBRTC_PEER_WORKER_H265DECODER_H
#define NDI_WEBRTC_PEER_WORKER_H265DECODER_H

#define OWT_ENABLE_H265

extern "C" {
#include "libavcodec/avcodec.h"
}  // extern "C"

// #include "common_video/h264/h264_bitstream_parser.h"
#include "media/base/codec.h"
#include "modules/video_coding/include/video_codec_interface.h"
#include "common_video/include/i420_buffer_pool.h"
#include "rtc_base/system/rtc_export.h"

struct AVCodecContextDeleter {
    void operator()(AVCodecContext *ptr) const { avcodec_free_context(&ptr); }
};

struct AVFrameDeleter {
    void operator()(AVFrame *ptr) const { av_frame_free(&ptr); }
};

class RTC_EXPORT H265Decoder : public webrtc::VideoDecoder {
public:
    explicit H265Decoder();

    ~H265Decoder() override;

    int32_t InitDecode(const webrtc::VideoCodec *codec_settings, int32_t number_of_cores) override;

    int32_t Release() override;

    int32_t RegisterDecodeCompleteCallback(webrtc::DecodedImageCallback *callback) override;

    // |missing_frames|, |fragmentation| and |render_time_ms| are ignored.
    int32_t
    Decode(const webrtc::EncodedImage &input_image, bool /*missing_frames*/, int64_t render_time_ms = -1) override;

    const char *ImplementationName() const override;

    static bool IsSupported();

    static std::unique_ptr<H265Decoder> Create();

private:
    static int AVGetBuffer2(AVCodecContext *context, AVFrame *av_frame, int flags);

    static void AVFreeBuffer2(void *opaque, uint8_t *data);

    bool IsInitialized() const;

    void ReportInit();

    void ReportError();

    webrtc::I420BufferPool pool_;
    std::unique_ptr<AVCodecContext, AVCodecContextDeleter> av_context_;
    std::unique_ptr<AVFrame, AVFrameDeleter> av_frame_;

    webrtc::DecodedImageCallback *decoded_image_callback_;

    bool has_reported_init_;
    bool has_reported_error_;

};


#endif //NDI_WEBRTC_PEER_WORKER_H265DECODER_H
