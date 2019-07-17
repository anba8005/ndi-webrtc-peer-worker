//
// Created by anba8005 on 12/22/18.
//

#include "NDIWriter.h"

#include <iostream>
#include <thread>
#include "third_party/libyuv/include/libyuv.h"

//#define WEBRTC_TIME_BASE 90000
//#define WEBRTC_TIME_BASE_Q (AVRational){1, WEBRTC_TIME_BASE}
//
//#define NDI_TIME_BASE 10000000
//#define NDI_TIME_BASE_Q (AVRational){1, NDI_TIME_BASE}

const AVPixelFormat WEBRTC_PIXEL_FORMAT = av_get_pix_fmt("yuv420p");
const AVPixelFormat NDI_PIXEL_FORMAT = av_get_pix_fmt("uyvy422");

NDIWriter::NDIWriter() : _scaling_context(nullptr), _videoTrack(nullptr), _audioTrack(nullptr),
                         _pNDI_send(nullptr), _p_frame_buffer_idx(0), _rotationBuffer(nullptr) {
    _p_frame_buffers[0] = nullptr;
    _p_frame_buffers[1] = nullptr;
}

NDIWriter::~NDIWriter() {
    if (_videoTrack)
        _videoTrack->RemoveSink(this);

    if (_audioTrack)
        _audioTrack->RemoveSink(this);

    NDIlib_send_send_video_async_v2(_pNDI_send, nullptr);
    NDIlib_send_destroy(_pNDI_send);

    if (_p_frame_buffers[0])
        av_frame_free(&_p_frame_buffers[0]);
    if (_p_frame_buffers[1])
        av_frame_free(&_p_frame_buffers[1]);
    if (_scaling_context)
        sws_freeContext(_scaling_context);
    std::cerr << "close ndi" << std::endl;
}

void NDIWriter::open(Configuration config) {

    std::cerr << "open ndi" << std::endl;
    // validate
    if (config.name.empty())
        throw "Cannot run NDI - name is empty";

    // save settings
    _name = config.name;
    _width = config.width;
    _height = config.height;

    // Create an NDI source
    _NDI_send_create_desc.p_ndi_name = _name.c_str();
    _NDI_send_create_desc.clock_video = false;
    _NDI_send_create_desc.clock_audio = false;

    // Create the NDI sender
    _pNDI_send = NDIlib_send_create(&_NDI_send_create_desc);
    if (!_pNDI_send) {
        throw "Error creating NDI sender";
    }

    // create video frame buffers
    _p_frame_buffers[0] = nullptr;
    _p_frame_buffers[1] = nullptr;
    _p_frame_buffer_idx = 0;

    // configure video frame
    _NDI_video_frame.FourCC = NDIlib_FourCC_type_UYVY;
    if (config.frameRate != 0) {
        _NDI_video_frame.frame_rate_N = config.frameRate;
        _NDI_video_frame.frame_rate_D = 1;
    }
}


void NDIWriter::setVideoTrack(webrtc::VideoTrackInterface *track) {
    _videoTrack = track;
    if (track) {
        _videoTrack->AddOrUpdateSink(this, rtc::VideoSinkWants());
        std::cerr << "NDI video track received" << std::endl;
    }
}

void NDIWriter::setAudioTrack(webrtc::AudioTrackInterface *track) {
    _audioTrack = track;
    if (track) {
        _audioTrack->AddSink(this);
        std::cerr << "NDI audio track received" << std::endl;
    }
}

void NDIWriter::OnFrame(const webrtc::VideoFrame &yuvframe) {
//	std::cerr << "On video frame: " << yuvframe.width() << 'x' << yuvframe.height() << '@' << yuvframe.timestamp()
//			  << std::endl;

    // detect rotation
    bool rotatedVertical = yuvframe.rotation() == webrtc::VideoRotation::kVideoRotation_90 ||
                           yuvframe.rotation() == webrtc::VideoRotation::kVideoRotation_270;

    // define dst width/height
    int width =
            _width != 0 && _height != 0 ? (rotatedVertical ? _height : _width) : (rotatedVertical ? yuvframe.height()
                                                                                                  : yuvframe.width());
    int height =
            _width != 0 && _height != 0 ? (rotatedVertical ? _width : _height) : (rotatedVertical ? yuvframe.width()
                                                                                                  : yuvframe.height());

    //
    int srcWidth = rotatedVertical ? yuvframe.height() : yuvframe.width();
    int srcHeight = rotatedVertical ? yuvframe.width() : yuvframe.height();

    //
    auto yuvbuffer = yuvframe.video_frame_buffer()->GetI420();

    // rotate frame if needed
    if (yuvframe.rotation() != webrtc::VideoRotation::kVideoRotation_0) {
        // get rotation mode
        auto rotationMode = (libyuv::RotationMode) yuvframe.rotation();
        // get/create buffers
        if (!_rotationBuffer || _rotationBuffer->width() != width || _rotationBuffer->height() != height) {
            _rotationBuffer = webrtc::I420Buffer::Create(width, height);
        }
        yuvbuffer = _rotationBuffer;
        auto srcBuffer = yuvframe.video_frame_buffer()->GetI420();
        // rotate
        libyuv::I420Rotate(srcBuffer->DataY(), srcBuffer->StrideY(), srcBuffer->DataU(), srcBuffer->StrideU(),
                           srcBuffer->DataV(), srcBuffer->StrideV(), (uint8_t *) yuvbuffer->DataY(),
                           yuvbuffer->StrideY(),
                           (uint8_t *) yuvbuffer->DataU(), yuvbuffer->StrideU(), (uint8_t *) yuvbuffer->DataV(),
                           yuvbuffer->StrideV(), yuvframe.width(), yuvframe.height(), rotationMode);
    } else {
        _rotationBuffer.release();
    }

    // get scaling context
    _scaling_context = sws_getCachedContext(_scaling_context, srcWidth, srcHeight, WEBRTC_PIXEL_FORMAT,
                                            width, height, NDI_PIXEL_FORMAT, SWS_BICUBIC, nullptr, nullptr, nullptr);
    if (!_scaling_context) {
        std::cerr << "Scaling context creation error" << std::endl;
        return;
    }


    // get src data & linesizes
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
    if (!frame || frame->width != width || frame->height != height) {
        // recreate frame
        frame = createVideoFrame(NDI_PIXEL_FORMAT, width, height, true);
        if (frame) {
            av_frame_free(&_p_frame_buffers[_p_frame_buffer_idx]);
            _p_frame_buffers[_p_frame_buffer_idx] = frame;
        } else {
            std::cerr << "Frame creation error" << std::endl;
            return;
        }
    }
    _p_frame_buffer_idx = _p_frame_buffer_idx == 0 ? 1 : 0; // next time next frame

    // scale
    if (sws_scale(_scaling_context, data, linesize, 0, srcHeight, frame->data, frame->linesize) < 0) {
        std::cerr << "Conversion error" << std::endl;
        return;
    }

    // prepare & send ndi frame
    _NDI_video_frame.xres = width;
    _NDI_video_frame.yres = height;
    _NDI_video_frame.p_data = frame->data[0];
    _NDI_video_frame.line_stride_in_bytes = frame->linesize[0];
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
    NDI_audio_frame.no_samples = number_of_frames;
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

NDIWriter::Configuration::Configuration(json payload) {
    this->name = payload.value("name", "");
    this->width = payload.value("width", 0);
    this->height = payload.value("height", 0);
    this->frameRate = payload.value("frameRate", 0);
    this->persistent = payload.value("persistent", true);
}

bool NDIWriter::Configuration::isEnabled() {
    return !this->name.empty();
}