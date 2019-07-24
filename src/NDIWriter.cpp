//
// Created by anba8005 on 12/22/18.
//

#include "NDIWriter.h"

#include <iostream>
#include <thread>
#include "third_party/libyuv/include/libyuv.h"
#include "rtc_base/memory/aligned_malloc.h"

//#define WEBRTC_TIME_BASE 90000
//#define WEBRTC_TIME_BASE_Q (AVRational){1, WEBRTC_TIME_BASE}
//
//#define NDI_TIME_BASE 10000000
//#define NDI_TIME_BASE_Q (AVRational){1, NDI_TIME_BASE}

NDIWriter::NDIWriter() : _videoTrack(nullptr), _audioTrack(nullptr),                      _pNDI_send(nullptr){
}

NDIWriter::~NDIWriter() {
	if (_videoTrack)
		_videoTrack->RemoveSink(this);

	if (_audioTrack)
		_audioTrack->RemoveSink(this);

	NDIlib_send_send_video_async_v2(_pNDI_send, nullptr);
	NDIlib_send_destroy(_pNDI_send);
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
	_outputMode = config.outputMode;

	// Create an NDI source
	_NDI_send_create_desc.p_ndi_name = _name.c_str();
	_NDI_send_create_desc.clock_video = false;
	_NDI_send_create_desc.clock_audio = false;

	// Create the NDI sender
	_pNDI_send = NDIlib_send_create(&_NDI_send_create_desc);
	if (!_pNDI_send) {
		throw "Error creating NDI sender";
	}

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

	// get dimensions and crop/rotate/pad video
	int srcWidth, srcHeight, width, height = 0;
	rtc::scoped_refptr<webrtc::I420BufferInterface> yuvbuffer;
	switch (_outputMode) {
		case OutputMode::VERTICAL_AS_IS:
			yuvbuffer = processVerticalAsIs(yuvframe, &srcWidth, &srcHeight, &width, &height);
			break;
		case OutputMode::VERTICAL_PAD:
			yuvbuffer = processVerticalPad(yuvframe, &srcWidth, &srcHeight, &width, &height);
			break;
		case OutputMode::SQUARE:
			yuvbuffer = processSquare(yuvframe, &srcWidth, &srcHeight, &width, &height);
			break;
		default:
			return;
	}

	if (yuvbuffer == nullptr) {
		return;
	}

	// apply additional scaling
	if (srcWidth != width || srcHeight != height) {
		auto scaleBuffer = webrtc::I420Buffer::Create(width, height);
		scaleBuffer->ScaleFrom(*yuvbuffer);
		yuvbuffer = scaleBuffer;
	}

	// alloc output frame
	int stride = width * 2;
	void *framePtr = webrtc::AlignedMalloc(stride * height, 64);
	std::shared_ptr<uint8_t> frame((uint8_t*) framePtr, &webrtc::AlignedFree);

	// convert
	int ret = libyuv::I420ToUYVY(yuvbuffer->DataY(), yuvbuffer->StrideY(), yuvbuffer->DataU(), yuvbuffer->StrideU(),
	                   yuvbuffer->DataV(), yuvbuffer->StrideV(), frame.get(), stride, width, height);
	if (ret != 0) {
		std::cerr << "Conversion error" << std::endl;
		return;
	}

	// prepare & send ndi frame
	_NDI_video_frame.xres = width;
	_NDI_video_frame.yres = height;
	_NDI_video_frame.p_data = frame.get();
	_NDI_video_frame.line_stride_in_bytes = stride;
	NDIlib_send_send_video_async_v2(_pNDI_send, &_NDI_video_frame);

	// save current & clean last
	_lastVideoFrame = frame;
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

rtc::scoped_refptr<webrtc::I420BufferInterface>
NDIWriter::processVerticalAsIs(const webrtc::VideoFrame &yuvframe, int *processedWidth, int *processedHeight,
                               int *targetWidth, int *targetHeight) {
	// detect rotation
	bool rotatedVertical = yuvframe.rotation() == webrtc::VideoRotation::kVideoRotation_90 ||
	                       yuvframe.rotation() == webrtc::VideoRotation::kVideoRotation_270;

	// set target (ndi output) dimensions (fixed or variable)
	if (_width != 0 && _height != 0) {
		*targetWidth = _width;
		*targetHeight = _height;
	} else {
		*targetWidth = yuvframe.width();
		*targetHeight = yuvframe.height();
	}

	// update if vertical video
	if (rotatedVertical) {
		std::swap(*targetHeight, *targetWidth);
	}

	// set processed (before swscale) video dimensions
	*processedWidth = rotatedVertical ? yuvframe.height() : yuvframe.width();
	*processedHeight = rotatedVertical ? yuvframe.width() : yuvframe.height();

	// get frame buffer
	auto srcBuffer = yuvframe.video_frame_buffer()->GetI420();

	// rotate frame if needed
	if (yuvframe.rotation() != webrtc::VideoRotation::kVideoRotation_0) {
		return webrtc::I420Buffer::Rotate(*srcBuffer, yuvframe.rotation());
	} else {
		return srcBuffer;
	}
}

rtc::scoped_refptr<webrtc::I420BufferInterface>
NDIWriter::processVerticalPad(const webrtc::VideoFrame &yuvframe, int *processedWidth, int *processedHeight,
                              int *targetWidth, int *targetHeight) {
	// detect rotation
	bool rotatedVertical = yuvframe.rotation() == webrtc::VideoRotation::kVideoRotation_90 ||
	                       yuvframe.rotation() == webrtc::VideoRotation::kVideoRotation_270;

	// set target (ndi output) dimensions (fixed or variable)
	if (_width != 0 && _height != 0) {
		*targetWidth = _width;
		*targetHeight = _height;
	} else {
		*targetWidth = yuvframe.width();
		*targetHeight = yuvframe.height();
	}

	// set processed (before swscale) video dimensions
	*processedWidth = rotatedVertical ? *targetWidth : yuvframe.width();
	*processedHeight = rotatedVertical ? *targetHeight : yuvframe.height();

	// get frame buffer
	auto srcBuffer = yuvframe.video_frame_buffer()->GetI420();

	// rotate frame if needed
	if (yuvframe.rotation() != webrtc::VideoRotation::kVideoRotation_0) {

		// rotate
		auto rotatedBuffer = webrtc::I420Buffer::Rotate(*srcBuffer, yuvframe.rotation());

		// scale + pad
		if (rotatedVertical) {
			// get scale dimensions
			float aspectRatio = (float) yuvframe.width() / (float) yuvframe.height();
			int scaleWidth = std::nearbyint((float) *processedHeight * (1.0f / aspectRatio) * 0.25f) * 4.0f;
			int scaleHeight = *processedHeight;

			// scale
			auto scaleBuffer = webrtc::I420Buffer::Create(scaleWidth, scaleHeight);
			scaleBuffer->ScaleFrom(*rotatedBuffer);

			// get padding dimensions
			int padWidth = (*processedWidth - scaleWidth) / 2;

			// pad
			auto paddingBuffer = webrtc::I420Buffer::Create(*processedWidth, *processedHeight);
			webrtc::I420Buffer::SetBlack(paddingBuffer);
			paddingBuffer->PasteFrom(*scaleBuffer, padWidth, 0);

			return paddingBuffer;
		} else {
			return rotatedBuffer;
		}
	} else {
		return srcBuffer;
	}
}

rtc::scoped_refptr<webrtc::I420BufferInterface>
NDIWriter::processSquare(const webrtc::VideoFrame &yuvframe, int *processedWidth, int *processedHeight,
                         int *targetWidth, int *targetHeight) {

	// set target (ndi output) dimensions (fixed or variable)
	if (_width != 0 && _height != 0) {
		*targetWidth = std::min(_width, _height);
		*targetHeight = *targetWidth;
	} else {
		*targetWidth = std::min(yuvframe.width(), yuvframe.height());
		*targetHeight = *targetWidth;
	}

	// set processed (before swscale) video dimensions
	*processedWidth = std::min(yuvframe.width(), yuvframe.height());
	*processedHeight = *processedWidth;

	// get frame buffer
	auto srcBuffer = yuvframe.video_frame_buffer()->GetI420();

	// crop
	auto croppedBuffer = webrtc::I420Buffer::Create(*processedWidth, *processedHeight);
	croppedBuffer->CropAndScaleFrom(*srcBuffer);

	// rotate frame if needed
	if (yuvframe.rotation() != webrtc::VideoRotation::kVideoRotation_0) {
		return webrtc::I420Buffer::Rotate(*croppedBuffer, yuvframe.rotation());
	} else {
		return croppedBuffer;
	}
}

NDIWriter::Configuration::Configuration(json payload) {
	this->name = payload.value("name", "");
	this->width = payload.value("width", 0);
	this->height = payload.value("height", 0);
	this->frameRate = payload.value("frameRate", 0);
	this->persistent = payload.value("persistent", true);
	//
	std::string om = payload.value("outputMode", "");
	if (om == "vertical") {
		this->outputMode = VERTICAL_AS_IS;
	} else if (om == "square") {
		this->outputMode = SQUARE;
	} else {
		this->outputMode = VERTICAL_PAD;
	}
}

bool NDIWriter::Configuration::isEnabled() {
	return !this->name.empty();
}