//
// Created by anba8005 on 12/15/18.
//

#include "PeerFactoryContext.h"

#include "api/peerconnectionfactoryproxy.h"
#include "api/peerconnectionproxy.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "p2p/base/basicpacketsocketfactory.h"
#include "p2p/client/basicportallocator.h"
#include "pc/peerconnection.h"

PeerFactoryContext::PeerFactoryContext(
        webrtc::AudioDeviceModule *default_adm,
        cricket::WebRtcVideoEncoderFactory *video_encoder_factory,
        cricket::WebRtcVideoDecoderFactory *video_decoder_factory,
        rtc::scoped_refptr<webrtc::AudioEncoderFactory> audio_encoder_factory,
        rtc::scoped_refptr<webrtc::AudioDecoderFactory> audio_decoder_factory) {

    // Setup threads
    networkThread = rtc::Thread::CreateWithSocketServer();
    workerThread = rtc::Thread::Create();
    if (!networkThread->Start() || !workerThread->Start())
        throw std::runtime_error("Failed to start WebRTC threads");

    // Init required builtin factories if not provided
    if (!audio_encoder_factory)
        audio_encoder_factory = webrtc::CreateBuiltinAudioEncoderFactory();
    if (!audio_decoder_factory)
        audio_decoder_factory = webrtc::CreateBuiltinAudioDecoderFactory();

    // Create the factory
    factory = webrtc::CreatePeerConnectionFactory(
            networkThread.get(), workerThread.get(), rtc::Thread::Current(),
            default_adm, audio_encoder_factory, audio_decoder_factory,
            video_encoder_factory, video_decoder_factory);

    if (!factory)
        throw std::runtime_error("Failed to create WebRTC factory");

    // Create configuration
    webrtc::PeerConnectionInterface::IceServer stun;
    stun.uri = "stun:stun.l.google.com:19302";
    this->config.servers.push_back(stun);
    webrtc::PeerConnectionInterface::IceServer turn;
    turn.urls.push_back("turn:vpn.b3video.lt:3478?transport=udp");
    turn.username = "turn";
    turn.password = "turn";
    this->config.servers.push_back(turn);

    //
    networkManager.reset(new rtc::BasicNetworkManager());
    socketFactory.reset(new rtc::BasicPacketSocketFactory(networkThread.get()));
}

rtc::scoped_refptr<webrtc::PeerConnectionInterface>
PeerFactoryContext::createPeerConnection(webrtc::MediaConstraintsInterface *constraints,
                                         webrtc::PeerConnectionObserver *observer) {
    return this->factory->CreatePeerConnection(this->config, constraints, std::move(portAllocator),
                                                                   nullptr, observer);
}


void PeerFactoryContext::initCustomNetworkManager() {

}
