//
// Created by anba8005 on 12/24/18.
//

#include "VideoCapturer.h"

#include <iostream>

VideoCapturer::VideoCapturer() {
    std::vector<cricket::VideoFormat> formats;
    formats.push_back(cricket::VideoFormat(1920, 1080, cricket::VideoFormat::FpsToInterval(25), cricket::FOURCC_UYVY));
    SetSupportedFormats(formats);
}


VideoCapturer::~VideoCapturer() {
}


cricket::CaptureState VideoCapturer::Start(const cricket::VideoFormat &capture_format) {
    try {
        if (capture_state() == cricket::CS_RUNNING) {
            std::cerr << "Start called when it's already started." << std::endl;
            return capture_state();
        }
        SetCaptureFormat(&capture_format);
        std::cerr << "STARTED CAPTURER init " << capture_format.width << " " << capture_format.width << " " << capture_format.fourcc << std::endl;
        return cricket::CS_RUNNING;
    } catch (...) {}
    return cricket::CS_FAILED;
}

void VideoCapturer::Stop() {
    std::cerr << "CREATED CAPTURER 3" << std::endl;
    try {
        std::cerr << "Stop" << std::endl;
        if (capture_state() == cricket::CS_STOPPED) {
            std::cerr << "Stop called when it's already stopped." << std::endl;
            return;
        }
        SetCaptureFormat(NULL);
        SetCaptureState(cricket::CS_STOPPED);
        return;
    } catch (...) {}
    return;
}

bool VideoCapturer::IsRunning() {
    std::cerr << "CREATED CAPTURER 4" << std::endl;
    return capture_state() == cricket::CS_RUNNING;
}

bool VideoCapturer::IsScreencast() const {
    return false;
}

bool VideoCapturer::GetPreferredFourccs(std::vector<uint32_t> *fourccs) {
    std::cerr << "CREATED CAPTURER 5" << std::endl;
    if (!fourccs)
        return false;
    fourccs->push_back(cricket::FOURCC_UYVY);
    return true;
}



