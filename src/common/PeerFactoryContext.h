//
// Created by anba8005 on 12/15/18.
//

#ifndef GYVAITV_WEBRTC_PEERFACTORYCONTEXT_H
#define GYVAITV_WEBRTC_PEERFACTORYCONTEXT_H

#include "pc/peerconnectionfactory.h"


class PeerFactoryContext {
public:
    PeerFactoryContext(
            webrtc::AudioDeviceModule *default_adm = nullptr,
            cricket::WebRtcVideoEncoderFactory *video_encoder_factory = nullptr,
            cricket::WebRtcVideoDecoderFactory *video_decoder_factory = nullptr,
            rtc::scoped_refptr<webrtc::AudioEncoderFactory> audio_encoder_factory = nullptr,
            rtc::scoped_refptr<webrtc::AudioDecoderFactory> audio_decoder_factory = nullptr);

    void initCustomNetworkManager();

    rtc::scoped_refptr<webrtc::PeerConnectionInterface>
    createPeerConnection(webrtc::MediaConstraintsInterface *constraints, webrtc::PeerConnectionObserver *observer);

private:
    webrtc::PeerConnectionInterface::RTCConfiguration config;
    //
    std::unique_ptr<rtc::Thread> networkThread;
    std::unique_ptr<rtc::Thread> workerThread;
    std::unique_ptr<rtc::NetworkManager> networkManager;
    std::unique_ptr<rtc::PacketSocketFactory> socketFactory;
    //
    std::unique_ptr<cricket::PortAllocator> portAllocator;
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory;
};


#endif //GYVAITV_WEBRTC_PEERFACTORYCONTEXT_H
