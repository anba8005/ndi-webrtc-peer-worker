//
// Created by anba8005 on 12/15/18.
//

#ifndef GYVAITV_WEBRTC_PEERFACTORYCONTEXT_H
#define GYVAITV_WEBRTC_PEERFACTORYCONTEXT_H

#ifdef WIN32
#define NOMINMAX
#undef min
#undef max
#endif
#define USE_BUILTIN_SW_CODECS

#include "api/peer_connection_interface.h"
#include "p2p/client/basic_port_allocator.h"

#include "BaseAudioDeviceModule.h"
#include "VideoDeviceModule.h"
#include "webrtc/CustomDecoderFactory.h"
#include "webrtc/CustomEncoderFactory.h"

#include "json.hpp"

using json = nlohmann::json;

class PeerFactoryContext {
public:
    PeerFactoryContext();

	virtual ~PeerFactoryContext();

    void setConfiguration(json configuration);

    BaseAudioDeviceModule *getADM();

    VideoDeviceModule *getVDM();

    rtc::scoped_refptr<webrtc::PeerConnectionInterface>
    createPeerConnection(webrtc::PeerConnectionObserver *observer);

    rtc::scoped_refptr<webrtc::AudioTrackInterface>
    createAudioTrack(cricket::AudioOptions options, const char *label = "audio");

    rtc::scoped_refptr<webrtc::VideoTrackInterface>
    createVideoTrack(const char *label = "video");


private:
    webrtc::PeerConnectionInterface::RTCConfiguration config;
    //
    rtc::scoped_refptr<BaseAudioDeviceModule> adm;
    rtc::scoped_refptr<VideoDeviceModule> vdm;
    //
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory;
    //
    std::unique_ptr<rtc::Thread> networkThread;
    std::unique_ptr<rtc::Thread> workerThread;
    //
    std::unique_ptr<rtc::BasicNetworkManager> networkManager;
    std::unique_ptr<rtc::PacketSocketFactory> socketFactory;
    std::unique_ptr<cricket::BasicPortAllocator> portAllocator;
    //
    CustomEncoderFactory* encoderFactory;
    CustomDecoderFactory* decoderFactory;
};


#endif //GYVAITV_WEBRTC_PEERFACTORYCONTEXT_H
