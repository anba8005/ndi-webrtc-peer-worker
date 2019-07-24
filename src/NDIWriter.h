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
#include "api/video/i420_buffer.h"

#include "json.hpp"

#include <Processing.NDI.Lib.h>

using json = nlohmann::json;

class NDIWriter : public rtc::VideoSinkInterface<webrtc::VideoFrame>,
                  public webrtc::AudioTrackSinkInterface {
public:
    enum OutputMode {
        VERTICAL_PAD,
        VERTICAL_AS_IS,
        SQUARE
    };

    class Configuration {
    public:
        std::string name;
        int width = 0;
        int height = 0;
        int frameRate = 0;
        bool persistent = true;
        NDIWriter::OutputMode outputMode = VERTICAL_PAD;

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
    OutputMode _outputMode;
    //
    rtc::scoped_refptr<webrtc::VideoTrackInterface> _videoTrack;
    rtc::scoped_refptr<webrtc::AudioTrackInterface> _audioTrack;
    //
    NDIlib_send_create_t _NDI_send_create_desc;
    NDIlib_send_instance_t _pNDI_send;
    //
    NDIlib_video_frame_v2_t _NDI_video_frame;
    std::shared_ptr<uint8_t> _lastVideoFrame;

    //
    //
    //

    rtc::scoped_refptr<webrtc::I420BufferInterface> processVerticalAsIs(const webrtc::VideoFrame &yuvframe,
                                                                        int *processedWidth, int *processedHeight,
                                                                        int *targetWidth, int *targetHeight);

    rtc::scoped_refptr<webrtc::I420BufferInterface> processVerticalPad(const webrtc::VideoFrame &yuvframe,
                                                                        int *processedWidth, int *processedHeight,
                                                                        int *targetWidth, int *targetHeight);

	rtc::scoped_refptr<webrtc::I420BufferInterface> processSquare(const webrtc::VideoFrame &yuvframe,
	                                                                  int *processedWidth, int *processedHeight,
	                                                                  int *targetWidth, int *targetHeight);

};

#endif //GYVAITV_WEBRTC_NDIWRITER_H
