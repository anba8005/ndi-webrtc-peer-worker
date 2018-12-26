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
#include "system_wrappers/include/field_trial.h"
#include "system_wrappers/include/field_trial_default.h"

#include <memory>

PeerFactoryContext::PeerFactoryContext() {
    //
    webrtc::field_trial::InitFieldTrialsFromString("WebRTC-FlexFEC-03-Advertised/WebRTC-FlexFEC-03/");

    // Setup threads
    networkThread = rtc::Thread::CreateWithSocketServer();
    workerThread = rtc::Thread::Create();
    if (!networkThread->Start() || !workerThread->Start())
        throw std::runtime_error("Failed to start WebRTC threads");

    // Create audio device module
    adm = workerThread->Invoke<rtc::scoped_refptr<BaseAudioDeviceModule>>
            (RTC_FROM_HERE,
             []() {
                 return BaseAudioDeviceModule::Create();
             });

    // create video device module
    vdm = workerThread->Invoke<rtc::scoped_refptr<VideoDeviceModule>>
            (RTC_FROM_HERE,
             []() {
                 return VideoDeviceModule::Create();
             });

    // Create the factory
    factory = webrtc::CreatePeerConnectionFactory(
            networkThread.get(), workerThread.get(), rtc::Thread::Current(),
            adm, webrtc::CreateBuiltinAudioEncoderFactory(), webrtc::CreateBuiltinAudioDecoderFactory(),
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
    config.set_cpu_adaptation(true);


    //
    networkManager.reset(new rtc::BasicNetworkManager());
    socketFactory.reset(new rtc::BasicPacketSocketFactory(networkThread.get()));
}

BaseAudioDeviceModule *PeerFactoryContext::getADM() {
    return adm;
}

VideoDeviceModule *PeerFactoryContext::getVDM() {
    return vdm;
}

rtc::scoped_refptr<webrtc::PeerConnectionInterface>
PeerFactoryContext::createPeerConnection(webrtc::PeerConnectionObserver *observer) {
    auto pc = this->factory->CreatePeerConnection(this->config, nullptr, nullptr, observer);
    if (!pc)
        throw std::runtime_error("Failed to create WebRTC peer connection");
    return pc;
}

rtc::scoped_refptr<webrtc::AudioTrackInterface>
PeerFactoryContext::createAudioTrack(cricket::AudioOptions options, const char *label) {
    rtc::scoped_refptr<webrtc::AudioTrackInterface> audioTrack(
            factory->CreateAudioTrack(label, factory->CreateAudioSource(options)));
    return audioTrack;
}

rtc::scoped_refptr<webrtc::VideoTrackInterface>
PeerFactoryContext::createVideoTrack(std::unique_ptr<cricket::VideoCapturer> capturer, const char *label) {
//    std::unique_ptr<cricket::VideoCapturer> capturer;
//    capturer.reset(new VideoCapturer());
//
    rtc::scoped_refptr<webrtc::VideoTrackInterface> videoTrack(
            factory->CreateVideoTrack(label, factory->CreateVideoSource(std::move(capturer))));
    return videoTrack;
}

rtc::scoped_refptr<webrtc::VideoTrackInterface>
PeerFactoryContext::createVideoTrack(const char *label) {
    rtc::scoped_refptr<webrtc::VideoTrackInterface> videoTrack(factory->CreateVideoTrack(label, vdm));
    return videoTrack;
}
