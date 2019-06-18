//
// Created by anba8005 on 12/23/18.
//

#ifndef GYVAITV_WEBRTC_NDIREADER_H
#define GYVAITV_WEBRTC_NDIREADER_H

#include <thread>
#include <atomic>
#include <string>

#include "json.hpp"

#include <Processing.NDI.Lib.h>
#include "VideoDeviceModule.h"
#include "BaseAudioDeviceModule.h"

using json = nlohmann::json;

class NDIReader {
public:
    class Configuration {
    public:
        std::string name;
        std::string ips = "";

        Configuration(json payload);
    };

    NDIReader();

    void open(Configuration config);

    void start(VideoDeviceModule *vdm, BaseAudioDeviceModule *adm);

    void stop();

    bool isRunning();

    virtual ~NDIReader();

    static json findSources();

private:
    void run();

    std::thread runner;
    std::atomic<bool> running;

    NDIlib_recv_instance_t _pNDI_recv;

    VideoDeviceModule *vdm;
    BaseAudioDeviceModule *adm;
};


#endif //GYVAITV_WEBRTC_NDIREADER_H
