//
// Created by anba8005 on 12/24/18.
//

#ifndef GYVAITV_WEBRTC_VIDEOCAPTURER_H
#define GYVAITV_WEBRTC_VIDEOCAPTURER_H

#include <media/base/videocapturer.h>


class VideoCapturer : public cricket::VideoCapturer {

public:
    VideoCapturer();

    virtual ~VideoCapturer();

    VideoCapturer(const VideoCapturer&) = delete;
    void operator=(const VideoCapturer&) = delete;

    bool GetPreferredFourccs(std::vector<uint32_t> *fourccs) override;

    cricket::CaptureState Start(const cricket::VideoFormat &capture_format) override;

    void Stop() override;

    bool IsRunning() override;

    bool IsScreencast() const override;

};


#endif //GYVAITV_WEBRTC_VIDEOCAPTURER_H
