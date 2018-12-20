//
// Created by anba8005 on 12/20/18.
//

#include "PeerContext.h"
#include "SetSessionDescriptionObserver.h"

#include <iostream>

PeerContext::PeerContext(shared_ptr<Signaling> signaling) : signaling(signaling) {

}

PeerContext::~PeerContext() {

}

void PeerContext::processMessages() {
    auto rtcThread = rtc::Thread::Current();
    rtcThread->ProcessMessages(100);
}

void PeerContext::setRemoteDescription(const string &type, const string &sdp) {
    webrtc::SdpParseError error;
    webrtc::SessionDescriptionInterface *desc(webrtc::CreateSessionDescription(type, sdp, &error));
    if (!desc)
        throw runtime_error("Can't parse remote SDP: " + error.description);

    peer->SetRemoteDescription(SetSessionDescriptionObserver::Create(), desc);
}

void PeerContext::setLocalDescription(const string &type, const string &sdp) {
    webrtc::SdpParseError error;
    webrtc::SessionDescriptionInterface *desc(webrtc::CreateSessionDescription(type, sdp, &error));
    if (!desc)
        throw runtime_error("Can't parse remote SDP: " + error.description);

    peer->SetLocalDescription(SetSessionDescriptionObserver::Create(), desc);
}


void PeerContext::addIceCandidate(const string &mid, int mlineindex, const string &sdp) {
    webrtc::SdpParseError error;
    unique_ptr<webrtc::IceCandidateInterface> candidate(webrtc::CreateIceCandidate(mid, mlineindex, sdp, &error));
    if (!candidate) {
        throw runtime_error("Can't parse remote candidate: " + error.description);
    }
    peer->AddIceCandidate(candidate.get());
}

//
//
//

void PeerContext::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) {
    cerr << "SIGNALING CHANGE " << new_state << endl;

}

void PeerContext::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) {

}

void PeerContext::OnRenegotiationNeeded() {

}

void PeerContext::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) {
    cerr << "ICE CONNECTION CHANGE " << new_state << endl;
}

void PeerContext::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) {
    cerr << "ICE GATHERING CHANGE " << new_state << endl;
}

void PeerContext::OnIceCandidate(const webrtc::IceCandidateInterface *candidate) {
    string sdp;
    if (!candidate->ToString(&sdp)) {
        throw new runtime_error("Failed to serialize local candidate");
    }

}
