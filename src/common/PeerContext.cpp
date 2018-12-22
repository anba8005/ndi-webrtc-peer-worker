//
// Created by anba8005 on 12/20/18.
//

#include "PeerContext.h"
#include "SetSessionDescriptionObserver.h"
#include "CreateSessionDescriptionObserver.h"

#include <iostream>

PeerContext::PeerContext(shared_ptr<Signaling> signaling) : signaling(signaling) {

}

PeerContext::~PeerContext() {

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
        pc->SetRemoteDescription(observer, description.release());
    } catch (const runtime_error &error) {
        signaling->replyError(COMMAND_SET_LOCAL_DESCRIPTION, string(error.what()), correlation);
    }
}

void PeerContext::createAnswer(int64_t correlation) {
    webrtc::PeerConnectionInterface::RTCOfferAnswerOptions options;
    options.offer_to_receive_video = webrtc::PeerConnectionInterface::RTCOfferAnswerOptions::kOfferToReceiveMediaTrue;
    options.offer_to_receive_audio = webrtc::PeerConnectionInterface::RTCOfferAnswerOptions::kOfferToReceiveMediaTrue;
    //
    auto observer = CreateSessionDescriptionObserver::Create(signaling, COMMAND_CREATE_ANSWER, correlation);
    pc->CreateAnswer(observer, options);
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
    signaling->state("OnDataChannel", payload);
}

void PeerContext::OnRenegotiationNeeded() {
    cerr << "RENEGOTIATION ----------=====================------------" << endl;
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
    payload["sdp"] = sdp;
    signaling->state("OnIceCandidate", payload);
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
