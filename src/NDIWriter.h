//
// Created by anba8005 on 12/22/18.
//

#ifndef GYVAITV_WEBRTC_NDIWRITER_H
#define GYVAITV_WEBRTC_NDIWRITER_H

#ifdef WIN32
#define NOMINMAX
#undef min
#undef max
#endif

#include <string>
#include "api/peer_connection_interface.h"

#include "json.hpp"

#include <Processing.NDI.Lib.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/pixdesc.h>
#include <libavutil/imgutils.h>
}

using json = nlohmann::json;

class NDIWriter : public rtc::VideoSinkInterface<webrtc::VideoFrame>,
                  public webrtc::AudioTrackSinkInterface {
public:
    class Configuration {
    public:
        std::string name;
        int width = 0;
        int height = 0;
        int frameRate = 0;
        bool persistent = true;

        bool isEnabled();

        Configuration(json payload);
    };


    NDIWriter();

    ~NDIWriter();

    void open(Configuration config);

    void setVideoTrack(webrtc::VideoTrackInterface *track);

    void setAudioTrack(webrtc::AudioTrackInterface *track);

    // VideoSinkInterface
    void OnFrame(const webrtc::VideoFrame &frame) override;

    // AudioTrackSinkInterface
    void OnData(const void *audio_data, int bits_per_sample, int sample_rate,
                size_t number_of_channels, size_t number_of_frames) override;

private:
    std::string _name;
    int _width;
    int _height;
    //
    rtc::scoped_refptr<webrtc::VideoTrackInterface> _videoTrack;
    rtc::scoped_refptr<webrtc::AudioTrackInterface> _audioTrack;
    //
    NDIlib_send_create_t _NDI_send_create_desc;
    NDIlib_send_instance_t _pNDI_send;
    //
    NDIlib_video_frame_v2_t _NDI_video_frame;
    AVFrame *_p_frame_buffers[2];
    long long _p_frame_buffer_idx;
    SwsContext *_scaling_context;

    //
    //
    //
    AVFrame *createVideoFrame(AVPixelFormat pixelFmt, int width, int height, bool alloc);
};

#endif //GYVAITV_WEBRTC_NDIWRITER_H
