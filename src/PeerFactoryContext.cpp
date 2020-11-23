//
// Created by anba8005 on 12/15/18.
//




#include "PeerFactoryContext.h"

#include "api/create_peerconnection_factory.h"
#include "api/peer_connection_factory_proxy.h"
#include "api/peer_connection_proxy.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "p2p/base/basic_packet_socket_factory.h"
#include "p2p/client/basic_port_allocator.h"
#include "pc/peer_connection.h"
#include "system_wrappers/include/field_trial.h"

#include <memory>
#include <iostream>

// Minimum and maximum port for port range
static const int kMinPort = 44000;
static const int kMaxPort = 45000;

PeerFactoryContext::PeerFactoryContext() {
    //
    webrtc::field_trial::InitFieldTrialsFromString("WebRTC-FlexFEC-03-Advertised/Enabled/WebRTC-FlexFEC-03/Enabled/");

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

    // create encoder factory
    auto encoderFactory = CustomEncoderFactory::Create();
    this->encoderFactory = encoderFactory.get();
    vdm->setFrameRateUpdater(encoderFactory.get());

    // create decoder factory
    auto decoderFactory = CustomDecoderFactory::Create();
    this->decoderFactory = decoderFactory.get();

    // Create the factory
    factory = webrtc::CreatePeerConnectionFactory(
            networkThread.get(), workerThread.get(), rtc::Thread::Current(),
            adm, webrtc::CreateBuiltinAudioEncoderFactory(), webrtc::CreateBuiltinAudioDecoderFactory(),
            std::move(encoderFactory), std::move(decoderFactory), nullptr, nullptr);

    // validate
    if (!factory)
        throw std::runtime_error("Failed to create WebRTC factory");

    //
    networkManager.reset(new rtc::BasicNetworkManager());
    socketFactory.reset(new rtc::BasicPacketSocketFactory(networkThread.get()));
    portAllocator.reset(new cricket::BasicPortAllocator(networkManager.get(), socketFactory.get()));
    portAllocator->SetPortRange(kMinPort, kMaxPort);

}

PeerFactoryContext::~PeerFactoryContext() {
    factory.release();
}

void
PeerFactoryContext::setConfiguration(json configuration) {
    //
    json iceServers = configuration["iceServers"];
    if (iceServers != nullptr && iceServers.is_array()) {
        for (auto &iceServer : iceServers) {
            webrtc::PeerConnectionInterface::IceServer srv;
            //
            json urls = iceServer["urls"];
            if (urls.is_array()) {
                for (auto &url : urls) {
                    srv.urls.push_back(url.get<std::string>());
                }
            } else if (urls.is_string()) {
                srv.urls.push_back(urls.get<std::string>());
            } else {
                std::cerr << "invalid iceServer urls" << urls;
            }
            //
            srv.username = iceServer.value("username", "");
            srv.password = iceServer.value("credential", "");
            //
            config.servers.push_back(srv);
        }
    }
    //
#ifdef WEBRTC_MAC
	config.set_cpu_adaptation(false); // videotoolbox keeps stepping down
#else
    config.set_cpu_adaptation(configuration.value("cpuAdaptation", true));
#endif
    //
    const json encoder = configuration["encoder"];
    if (!encoder.empty()) {
        this->encoderFactory->setConfiguration(encoder);
    }
    //
    const json decoder = configuration["decoder"];
    if (!decoder.empty()) {
        this->decoderFactory->setConfiguration(decoder);
    }
    //
#ifdef WEBRTC_LINUX
	auto use = configuration.value("useNonDefaultRoutes", false);
//	networkManager->set_ignore_non_default_routes(!use);
#endif
}

BaseAudioDeviceModule *PeerFactoryContext::getADM() {
    return adm;
}

VideoDeviceModule *PeerFactoryContext::getVDM() {
    return vdm;
}

rtc::scoped_refptr<webrtc::PeerConnectionInterface>
PeerFactoryContext::createPeerConnection(webrtc::PeerConnectionObserver *observer) {
    auto pc = this->factory->CreatePeerConnection(this->config, std::move(portAllocator), nullptr, observer);
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
PeerFactoryContext::createVideoTrack(const char *label) {
    rtc::scoped_refptr<webrtc::VideoTrackInterface> videoTrack(factory->CreateVideoTrack(label, vdm));
    return videoTrack;
}
