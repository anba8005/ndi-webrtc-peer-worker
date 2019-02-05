//
// Created by anba8005 on 12/20/18.
//

#include "PeerContext.h"
#include "SetSessionDescriptionObserver.h"
#include "CreateSessionDescriptionObserver.h"

#include <iostream>

PeerContext::PeerContext(shared_ptr<Signaling> signaling) : signaling(signaling), totalTracks(0) {

}

PeerContext::~PeerContext() {
    // TODO
    1. getstats - kad veiktu "connected"
    2. ondatachannel + proper datachannel observer - kad veiktu "connected"
    3. command line options (reader name,e.t.c)
    4. reader & writer refactoring (dynamic dimesnions)
    5. addtrack removetrack - dynamic reader streams ?
}

void PeerContext::start() {
    //
    reader = make_unique<NDIReader>("ANBA8005-OLD (OBS2)");
    reader->open();
//    //
    context = make_shared<PeerFactoryContext>();
    pc = context->createPeerConnection(this);
    //
    addTracks();
}

void PeerContext::end() {
    pc->Close();
}

void PeerContext::processMessages() {
    auto rtcThread = rtc::Thread::Current();
    rtcThread->ProcessMessages(100);
}

void PeerContext::setRemoteDescription(const string &type_str, const string &sdp, int64_t correlation) {
    try {
        auto description = this->createSessionDescription(type_str, sdp);
        auto observer = SetSessionDescriptionObserver::Create(signaling, COMMAND_SET_REMOTE_DESCRIPTION, correlation);
        pc->SetRemoteDescription(observer, description.release());
    } catch (const runtime_error &error) {
        signaling->replyError(COMMAND_SET_REMOTE_DESCRIPTION, string(error.what()), correlation);
    }
}

void PeerContext::setLocalDescription(const string &type_str, const string &sdp, int64_t correlation) {
    try {
        auto description = this->createSessionDescription(type_str, sdp);
        auto observer = SetSessionDescriptionObserver::Create(signaling, COMMAND_SET_LOCAL_DESCRIPTION, correlation);
        pc->SetLocalDescription(observer, description.release());
    } catch (const runtime_error &error) {
        signaling->replyError(COMMAND_SET_LOCAL_DESCRIPTION, string(error.what()), correlation);
    }
}

void PeerContext::createAnswer(int64_t correlation) {
    webrtc::PeerConnectionInterface::RTCOfferAnswerOptions options;
    options.offer_to_receive_video = webrtc::PeerConnectionInterface::RTCOfferAnswerOptions::kOfferToReceiveMediaTrue;
    options.offer_to_receive_audio = webrtc::PeerConnectionInterface::RTCOfferAnswerOptions::kOfferToReceiveMediaTrue;
    options.voice_activity_detection = false;
    //
    auto observer = CreateSessionDescriptionObserver::Create(signaling, COMMAND_CREATE_ANSWER, correlation);
    pc->CreateAnswer(observer, options);
}

void PeerContext::createOffer(int64_t correlation) {
    webrtc::PeerConnectionInterface::RTCOfferAnswerOptions options;
    options.offer_to_receive_video = webrtc::PeerConnectionInterface::RTCOfferAnswerOptions::kOfferToReceiveMediaTrue;
    options.offer_to_receive_audio = webrtc::PeerConnectionInterface::RTCOfferAnswerOptions::kOfferToReceiveMediaTrue;
    options.voice_activity_detection = false;
    //
    auto observer = CreateSessionDescriptionObserver::Create(signaling, COMMAND_CREATE_OFFER, correlation);
    pc->CreateOffer(observer, options);
}


void PeerContext::addIceCandidate(const string &mid, int mlineindex, const string &sdp, int64_t correlation) {
    webrtc::SdpParseError error;
    unique_ptr<webrtc::IceCandidateInterface> candidate(webrtc::CreateIceCandidate(mid, mlineindex, sdp, &error));
    if (candidate) {
        pc->AddIceCandidate(candidate.get());
        signaling->replyOk(COMMAND_ADD_ICE_CANDIDATE, correlation);
    } else {
        signaling->replyError(COMMAND_ADD_ICE_CANDIDATE, "Can't parse remote candidate: " + error.description,
                              correlation);
    }
}

void PeerContext::createDataChannel(const string &name, int64_t correlation) {
    webrtc::DataChannelInit config;
    dc = pc->CreateDataChannel(name,&config);
    signaling->replyOk(COMMAND_ADD_ICE_CANDIDATE, correlation);
}

//
//
//

void PeerContext::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) {
    json payload;
    payload["state"] = new_state;
    signaling->state("OnSignalingChange", payload);
}

void PeerContext::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) {
    json payload;
    std::cerr << "ON DATA CHANNEL" << std::endl;
    signaling->state("OnDataChannel", payload);
}

void PeerContext::OnRenegotiationNeeded() {
}

void PeerContext::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) {
    json payload;
    payload["state"] = new_state;
    signaling->state("OnIceConnectionChange", payload);
}

void PeerContext::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) {
    json payload;
    payload["state"] = new_state;
    signaling->state("OnIceGatheringChange", payload);
    // start reader on connection
    if (new_state == webrtc::PeerConnectionInterface::IceGatheringState::kIceGatheringComplete && reader)
        reader->start(context->getVDM(), context->getADM());
}

void PeerContext::OnIceCandidate(const webrtc::IceCandidateInterface *candidate) {
    string sdp;
    if (!candidate->ToString(&sdp)) {
        cerr << "Failed to serialize local candidate" << endl;
        return;
    }

    json payload;
    payload["sdpMid"] = candidate->sdp_mid();
    payload["sdpMLineIndex"] = candidate->sdp_mline_index();
    payload["candidate"] = sdp;
    signaling->state("OnIceCandidate", payload);
}

void PeerContext::OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
                             const vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>> &streams) {
    std::cerr << "on add track -----------------------" << std::endl;
    auto track = receiver->track();
    json payload;
    payload["kind"] = track->kind();
    payload["id"] = track->id();
    //
    json streamsJson = json::array();
    for (auto stream : streams) {
        json j;
        j["id"] = stream->id();
        streamsJson.push_back(j);
    }
    payload["streams"] = streamsJson;
    //
    signaling->state("OnAddTrack", payload);
    //
    if (!writer)
        writer = make_unique<NDIWriter>("TEST",1280,720);
    //
    if (track->kind() == track->kVideoKind) {
        writer->setVideoTrack(dynamic_cast<webrtc::VideoTrackInterface*>(track.release()));
    } else if (track->kind() == track->kAudioKind) {
        writer->setAudioTrack(dynamic_cast<webrtc::AudioTrackInterface*>(track.release()));
    }
    //
    totalTracks++;
}

void PeerContext::OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) {
    std::cerr << "on remove track -----------------------" << std::endl;
    auto track = receiver->track();
    json payload;
    payload["kind"] = track->kind();
    payload["id"] = track->id();
    //
    signaling->state("OnRemoveTrack", payload);
    //
    if (track->kind() == track->kVideoKind) {
        writer->setVideoTrack(nullptr);
    } else if (track->kind() == track->kAudioKind) {
        writer->setAudioTrack(nullptr);
    }
    //
    totalTracks--;
    //
    if (totalTracks == 0)
        writer.reset();
}

//
//
//

unique_ptr<webrtc::SessionDescriptionInterface>
PeerContext::createSessionDescription(const string &type_str, const string &sdp) {
    absl::optional<webrtc::SdpType> type_maybe = webrtc::SdpTypeFromString(type_str);
    if (!type_maybe) {
        throw runtime_error("Unknown SDP type: " + type_str);
    }
    //
    webrtc::SdpType type = *type_maybe;
    webrtc::SdpParseError error;
    std::unique_ptr<webrtc::SessionDescriptionInterface> session_description =
            webrtc::CreateSessionDescription(type, sdp, &error);
    if (!session_description) {
        throw runtime_error("Can't parse SDP: " + error.description);
    }
    return session_description;
}

void PeerContext::addTracks() {
    // create audio options
    cricket::AudioOptions options;
    options.auto_gain_control = false;
    options.noise_suppression = false;
    options.highpass_filter = false;
    options.echo_cancellation = false;
    options.typing_detection = false;
    options.residual_echo_detector = false;

    // add audio track
    auto audioTrack = context->createAudioTrack(options);
    auto videoResult = pc->AddTrack(audioTrack, {"stream"});
    if (!videoResult.ok()) {
        throw std::runtime_error(
                "Failed to add audio track to PeerConnection:" + string(videoResult.error().message()));
    }

    // add video track
    auto videoTrack = context->createVideoTrack();
    videoTrack->set_content_hint(webrtc::VideoTrackInterface::ContentHint::kFluid);
    auto audioResult = pc->AddTrack(videoTrack, {"stream"});
    if (!audioResult.ok()) {
        throw std::runtime_error(
                "Failed to add video track to PeerConnection:" + string(audioResult.error().message()));
    }
}
