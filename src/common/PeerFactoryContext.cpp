//
// Created by anba8005 on 12/15/18.
//

#include "PeerFactoryContext.h"

#include "api/peerconnectionfactoryproxy.h"
#include "api/peerconnectionproxy.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "p2p/base/basicpacketsocketfactory.h"
#include "p2p/client/basicportallocator.h"
#include "pc/peerconnection.h"
#include "BaseAudioDeviceModule.h"

PeerFactoryContext::PeerFactoryContext() {

    // Setup threads
    networkThread = rtc::Thread::CreateWithSocketServer();
    workerThread = rtc::Thread::Create();
    if (!networkThread->Start() || !workerThread->Start())
        throw std::runtime_error("Failed to start WebRTC threads");

    // Create audio device module
    this->adm = workerThread->Invoke<rtc::scoped_refptr<webrtc::AudioDeviceModule>>
            (RTC_FROM_HERE,
             []() {
                 return BaseAudioDeviceModule::Create();
             });

    // Create the factory
    factory = webrtc::CreatePeerConnectionFactory(
            networkThread.get(), workerThread.get(), rtc::Thread::Current(),
            this->adm, webrtc::CreateBuiltinAudioEncoderFactory(), webrtc::CreateBuiltinAudioDecoderFactory(),
            webrtc::CreateBuiltinVideoEncoderFactory(), webrtc::CreateBuiltinVideoDecoderFactory(), nullptr, nullptr);

    if (!factory)
        throw std::runtime_error("Failed to create WebRTC factory");

    // Update configuration
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

webrtc::AudioDeviceModule *PeerFactoryContext::getADM() {
    return adm;
}

rtc::scoped_refptr<webrtc::PeerConnectionInterface>
PeerFactoryContext::createPeerConnection(webrtc::PeerConnectionObserver *observer) {
    auto pc = this->factory->CreatePeerConnection(this->config, nullptr, nullptr, observer);
    if (!pc)
        throw std::runtime_error("Failed to create WebRTC peer connection");
    return pc;
}

