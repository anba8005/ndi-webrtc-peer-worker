//
// Created by anba8005 on 12/15/18.
//

#ifndef GYVAITV_WEBRTC_PEERFACTORYCONTEXT_H
#define GYVAITV_WEBRTC_PEERFACTORYCONTEXT_H

#define USE_BUILTIN_SW_CODECS

#include "pc/peerconnectionfactory.h"


class PeerFactoryContext {
public:
    PeerFactoryContext();

    webrtc::AudioDeviceModule *getADM();

    rtc::scoped_refptr<webrtc::PeerConnectionInterface>
    createPeerConnection(webrtc::PeerConnectionObserver *observer);

private:
    webrtc::PeerConnectionInterface::RTCConfiguration config;
    rtc::scoped_refptr<webrtc::AudioDeviceModule> adm;
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory;
    //
    std::unique_ptr<rtc::Thread> networkThread;
    std::unique_ptr<rtc::Thread> workerThread;
    //
    std::unique_ptr<rtc::NetworkManager> networkManager;
    std::unique_ptr<rtc::PacketSocketFactory> socketFactory;
    std::unique_ptr<cricket::PortAllocator> portAllocator;
};


#endif //GYVAITV_WEBRTC_PEERFACTORYCONTEXT_H
