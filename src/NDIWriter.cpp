//
// Created by anba8005 on 12/22/18.
//

#include "NDIWriter.h"

#include <iostream>
#include <thread>

#define WEBRTC_TIME_BASE 90000
#define WEBRTC_TIME_BASE_Q (AVRational){1, WEBRTC_TIME_BASE}

#define NDI_TIME_BASE 10000000
#define NDI_TIME_BASE_Q (AVRational){1, NDI_TIME_BASE}

NDIWriter::NDIWriter(string name, int width, int height) : _scaling_context(nullptr) {
    // Init
    if (!NDIlib_initialize()) {
        throw "Cannot run NDI.";
    }

    // Create an NDI source
    _name = name;
    _NDI_send_create_desc.p_ndi_name = _name.c_str();
    _NDI_send_create_desc.clock_video = true;
    _NDI_send_create_desc.clock_audio = false;

    // Create the NDI sender
    _pNDI_send = NDIlib_send_create(&_NDI_send_create_desc);
    if (!_pNDI_send) {
        throw "Error creating NDI sender";
    }

    // create pixel formats
    _src_pixel_format = av_get_pix_fmt("yuv420p");
    _ndi_pixel_format = av_get_pix_fmt("uyvy422");

    // create video frame buffers
    _p_frame_buffers[0] = createVideoFrame(_ndi_pixel_format, width, height, true);
    _p_frame_buffers[1] = createVideoFrame(_ndi_pixel_format, width, height, true);
    _p_frame_buffer_idx = 0;

    // Create video frame
    _NDI_video_frame.xres = width;
    _NDI_video_frame.yres = height;
    _NDI_video_frame.FourCC = NDIlib_FourCC_type_UYVY;
    _NDI_video_frame.frame_rate_N = 30;
    _NDI_video_frame.frame_rate_D = 1;
}

NDIWriter::~NDIWriter() {
    if (_videoTrack)
        _videoTrack->RemoveSink(this);

    if (_audioTrack)
        _audioTrack->RemoveSink(this);

    NDIlib_send_send_video_async_v2(_pNDI_send, nullptr);
    NDIlib_send_destroy(_pNDI_send);
    NDIlib_destroy();

    if (_p_frame_buffers[0])
        av_free(_p_frame_buffers[0]);
    if (_p_frame_buffers[1])
        av_free(_p_frame_buffers[1]);
    if (_scaling_context)
        sws_freeContext(_scaling_context);
}

void NDIWriter::setVideoTrack(webrtc::VideoTrackInterface *track) {
    _videoTrack = track;
    if (track) {
        _videoTrack->AddOrUpdateSink(this, rtc::VideoSinkWants());
        std::cerr << "Video track changed" << std::endl;
    }
}

void NDIWriter::setAudioTrack(webrtc::AudioTrackInterface *track) {
    _audioTrack = track;
    if (track) {
        _audioTrack->AddSink(this);
        std::cerr << "Audio track changed" << std::endl;
    }
}

void NDIWriter::OnFrame(const webrtc::VideoFrame &yuvframe) {
//	std::cerr << "On video frame: " << yuvframe.width() << 'x' << yuvframe.height() << '@' << yuvframe.timestamp()
//			  << std::endl;

    // get scaling context
    _scaling_context = sws_getCachedContext(_scaling_context, yuvframe.width(), yuvframe.height(), _src_pixel_format,
                                            _NDI_video_frame.xres, _NDI_video_frame.yres, _ndi_pixel_format,
                                            SWS_BICUBIC, nullptr, nullptr, nullptr);

    // get src data & linesizes
    auto yuvbuffer = yuvframe.video_frame_buffer()->GetI420();
    uint8_t *data[8];
    data[0] = (uint8_t *) yuvbuffer->DataY();
    data[1] = (uint8_t *) yuvbuffer->DataU();
    data[2] = (uint8_t *) yuvbuffer->DataV();
    int linesize[8];
    linesize[0] = yuvbuffer->StrideY();
    linesize[1] = yuvbuffer->StrideU();
    linesize[2] = yuvbuffer->StrideV();

    // get ndi dst frame
    AVFrame *frame = _p_frame_buffers[_p_frame_buffer_idx];
    _p_frame_buffer_idx = _p_frame_buffer_idx == 0 ? 1 : 0;

    // scale
    if (sws_scale(_scaling_context, data, linesize, 0, yuvframe.height(),
                  frame->data, frame->linesize) < 0) {
        std::cerr << "Conversion error" << std::endl;
        return;
    }

    // prepare & send ndi frame
    _NDI_video_frame.p_data = frame->data[0];
    _NDI_video_frame.line_stride_in_bytes = frame->linesize[0];
    _NDI_video_frame.timecode = av_rescale_q(yuvframe.timestamp(), WEBRTC_TIME_BASE_Q, NDI_TIME_BASE_Q);
    NDIlib_send_send_video_async_v2(_pNDI_send, &_NDI_video_frame);
}


void NDIWriter::OnData(const void *audio_data, int bits_per_sample,
                       int sample_rate, size_t number_of_channels,
                       size_t number_of_frames) {
//	std::cerr << "On audio frame: "
//			  << "number_of_frames=" << number_of_frames << ", "
//			  << "number_of_channels=" << number_of_channels << ", "
//			  << "sample_rate=" << sample_rate << ", "
//			  << "bits_per_sample=" << bits_per_sample << std::endl;

    // Create an audio frame
    NDIlib_audio_frame_interleaved_16s_t NDI_audio_frame;
    NDI_audio_frame.sample_rate = sample_rate;
    NDI_audio_frame.no_channels = number_of_channels;
    NDI_audio_frame.no_samples = number_of_frames * (number_of_channels << 1); //  from ffmpeg
    NDI_audio_frame.p_data = (int16_t *) audio_data;

    // send
    NDIlib_util_send_send_audio_interleaved_16s(_pNDI_send, &NDI_audio_frame);
}

AVFrame *NDIWriter::createVideoFrame(AVPixelFormat pixelFmt, int width, int height, bool alloc) {
    AVFrame *picture = av_frame_alloc();
    if (!picture)
        return nullptr;

    int size = av_image_get_buffer_size(pixelFmt, width, height, 16);
    uint8_t *buffer = nullptr;
    if (alloc) {
        buffer = reinterpret_cast<uint8_t *>(av_malloc(size));
        if (!buffer) {
            av_free(picture);
            return nullptr;
        }
    }

    av_image_fill_arrays(picture->data, picture->linesize, buffer, pixelFmt, width, height, 1);

    picture->width = width;
    picture->height = height;
    picture->format = pixelFmt;

    return picture;
}