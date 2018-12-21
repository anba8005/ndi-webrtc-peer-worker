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

void PeerContext::setRemoteDescription(const string &type, const string &sdp, int64_t correlation) {
    webrtc::SdpParseError error;
    webrtc::SessionDescriptionInterface *desc(webrtc::CreateSessionDescription(type, sdp, &error));
    if (!desc) {
        signaling->replyError(COMMAND_SET_REMOTE_DESCRIPTION, "Can't parse remote SDP: " + error.description,
                              correlation);
        return;
    }

    auto observer = SetSessionDescriptionObserver::Create(signaling, COMMAND_SET_REMOTE_DESCRIPTION, correlation);
    peer->SetRemoteDescription(observer, desc);
}

void PeerContext::setLocalDescription(const string &type, const string &sdp, int64_t correlation) {
    webrtc::SdpParseError error;
    webrtc::SessionDescriptionInterface *desc(webrtc::CreateSessionDescription(type, sdp, &error));
    if (!desc) {
        signaling->replyError(COMMAND_SET_LOCAL_DESCRIPTION, "Can't parse remote SDP: " + error.description,
                              correlation);
        return;
    }

    auto observer = SetSessionDescriptionObserver::Create(signaling, COMMAND_SET_LOCAL_DESCRIPTION, correlation);
    peer->SetLocalDescription(observer, desc);
}

void PeerContext::createAnswer(int64_t correlation) {
    webrtc::PeerConnectionInterface::RTCOfferAnswerOptions options;
    options.offer_to_receive_video = webrtc::PeerConnectionInterface::RTCOfferAnswerOptions::kOfferToReceiveMediaTrue;
    options.offer_to_receive_audio = webrtc::PeerConnectionInterface::RTCOfferAnswerOptions::kOfferToReceiveMediaTrue;
    //
    auto observer = CreateSessionDescriptionObserver::Create(signaling, COMMAND_CREATE_ANSWER, correlation);
    peer->CreateAnswer(observer, options);
}


void PeerContext::addIceCandidate(const string &mid, int mlineindex, const string &sdp, int64_t correlation) {
    webrtc::SdpParseError error;
    unique_ptr<webrtc::IceCandidateInterface> candidate(webrtc::CreateIceCandidate(mid, mlineindex, sdp, &error));
    if (candidate) {
        peer->AddIceCandidate(candidate.get());
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
    cerr << "DATA CHANNEL ----------=====================------------" << endl;
}

void PeerContext::OnRenegotiationNeeded() {
    cerr << "RENEGOTIATION ----------=====================------------" << endl;
}

void PeerContext::OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) {
//    json payload;
//    signaling->state("OnTrack", payload);
    cerr << "************* TRACK TRACK TRACK----------=====================------------" << endl;
}

void PeerContext::OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) {
    cerr << "************* ADD STREAM ----------=====================------------" << endl;
}

void PeerContext::OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) {
    cerr << "************* REMOVE STREAM ----------=====================------------" << endl;
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
