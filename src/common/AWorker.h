//
// Created by anba8005 on 12/15/18.
//

#ifndef GYVAITV_WEBRTC_AWORKER_H
#define GYVAITV_WEBRTC_AWORKER_H

#include <string>
#include <mutex>
#include <vector>

#include "pc/peerconnectionfactory.h"

class AWorker {
public:
    AWorker();
    virtual ~AWorker();

    void run();

protected:
    virtual bool onStart() = 0;
    virtual void onEnd() = 0;
    virtual bool onCommand(std::vector<std::string> command) = 0;

    void print_commit();
    void print(std::string s);
private:
    bool isCommandAvailable();
    std::mutex print_mutex;
    std::vector<std::string> print_buffer;
    //
    std::unique_ptr<rtc::Thread> networkThread;
    std::unique_ptr<rtc::Thread> workerThread;
    std::unique_ptr<rtc::NetworkManager> networkManager;
    std::unique_ptr<rtc::PacketSocketFactory> socketFactory;
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory;
};


#endif //GYVAITV_WEBRTC_AWORKER_H
