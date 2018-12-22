//
// Created by anba8005 on 12/22/18.
//

#ifndef GYVAITV_WEBRTC_NDIWRITER_H
#define GYVAITV_WEBRTC_NDIWRITER_H

#include <string>
#include "api/peerconnectioninterface.h"

#include <Processing.NDI.Lib.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/pixdesc.h>
#include <libavutil/imgutils.h>
}

using namespace std;

class NDIWriter : public rtc::VideoSinkInterface<webrtc::VideoFrame>,
                  public webrtc::AudioTrackSinkInterface {
public:
    NDIWriter(string name, int width, int height);

    ~NDIWriter();

    void setVideoTrack(webrtc::VideoTrackInterface *track);

    void setAudioTrack(webrtc::AudioTrackInterface *track);

    // VideoSinkInterface
    void OnFrame(const webrtc::VideoFrame &frame) override;

    // AudioTrackSinkInterface
    void OnData(const void *audio_data, int bits_per_sample, int sample_rate,
                size_t number_of_channels, size_t number_of_frames) override;

private:
    string _name;
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
    AVPixelFormat _src_pixel_format;
    AVPixelFormat _ndi_pixel_format;

    //
    //
    //
    AVFrame *createVideoFrame(AVPixelFormat pixelFmt, int width, int height, bool alloc);
};

#endif //GYVAITV_WEBRTC_NDIWRITER_H
