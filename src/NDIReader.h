//
// Created by anba8005 on 12/23/18.
//

#ifndef GYVAITV_WEBRTC_NDIREADER_H
#define GYVAITV_WEBRTC_NDIREADER_H

#include <thread>
#include <atomic>
#include <string>

#include <Processing.NDI.Lib.h>
#include "VideoDeviceModule.h"
#include "BaseAudioDeviceModule.h"

class NDIReader {
public:
    NDIReader(std::string name, std::string ips = "");

    void open();

    void start(VideoDeviceModule *vdm, BaseAudioDeviceModule *adm);

    void stop();

    virtual ~NDIReader();

private:
    void run();

    std::thread runner;
    std::atomic<bool> running;

    std::string name;
    std::string ips;
    NDIlib_recv_instance_t _pNDI_recv;

    VideoDeviceModule *vdm;
    BaseAudioDeviceModule *adm;
};


#endif //GYVAITV_WEBRTC_NDIREADER_H
