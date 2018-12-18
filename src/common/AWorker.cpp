//
// Created by anba8005 on 12/15/18.
//

#include "AWorker.h"
#include <iostream>
#include <poll.h>
#include <sstream>
#include <thread>
#include <chrono>

#include "api/peerconnectionfactoryproxy.h"
#include "modules/audio_device/include/audio_device.h"

AWorker::AWorker() : finished(false) {

}

AWorker::~AWorker() {
}

void AWorker::run() {
    this->onStart();
    //
    print("state:negotiating");
    //
    auto rtcthread = rtc::Thread::Current();
    //
    while (!this->finished.load()) {
        if (this->isCommandAvailable()) {
            // get command line
            std::string commandLine;
            getline(std::cin, commandLine);
            // to upper case
            for (auto &c : commandLine)
                c = std::toupper(c);
            // split
            std::istringstream ss(commandLine);
            std::vector<std::string> command;
            std::string c;
            while (getline(ss, c, ':')) {
                command.push_back(c);
            }
            // execute
            this->onCommand(command);
        }
        // process webrtc
        rtcthread->ProcessMessages(10);
        // respond
        this->print_commit();
    }
    //
    this->onEnd();
}

bool AWorker::isCommandAvailable() {
    pollfd fdinfo;
    fdinfo.fd = fileno(stdin);
    fdinfo.events = POLLIN;
    return poll(&fdinfo, 1, 1) > 0;
}

void AWorker::print(std::string s) {
    std::unique_lock<std::mutex> lock(this->print_mutex);
    this->print_buffer.push_back(s);
}

void AWorker::error(std::string e) {
    std::unique_lock<std::mutex> lock(this->error_mutex);
    this->error_buffer.push_back(e);
}

void AWorker::print_commit() {
    std::unique_lock<std::mutex> lock(this->print_mutex);
    // copy from buffer to commit buffer
    std::vector<std::string> current;
    for (std::string &i : this->print_buffer) {
        current.push_back(i);
    }
    this->print_buffer.clear();
    lock.unlock();
    // check
    if (current.empty())
        return;
    // print commit buffer
    for (std::string &i : current) {
        std::cout << i << std::endl;
    }
    // flush
    std::cout.flush();
}

void AWorker::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) {
    std::cerr << "SIGNALING CHANGE " << new_state << std::endl;

}

void AWorker::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) {

}

void AWorker::OnRenegotiationNeeded() {

}

void AWorker::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) {
    std::cerr << "ICE CONNECTION CHANGE " << new_state << std::endl;
}

void AWorker::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) {
    std::cerr << "ICE GATHERING CHANGE " << new_state << std::endl;
}

void AWorker::OnIceCandidate(const webrtc::IceCandidateInterface *candidate) {
    std::string sdp;
    if (!candidate->ToString(&sdp)) {
        throw new std::runtime_error("Failed to serialize local candidate");
    }

    print("candidate:" + candidate->sdp_mid()+ ":" + std::to_string(candidate->sdp_mline_index()) + ":" + sdp);
}

void AWorker::OnSuccess(webrtc::SessionDescriptionInterface *desc) { // createOffer or createAnswer callback
    peer->SetLocalDescription(DummySetSessionDescriptionObserver::Create(), desc);

    // Serialize sdp
    std::string sdp;
    if (!desc->ToString(&sdp)) {
        throw new std::runtime_error("Failed to serialize local sdp");
    }

    // Send an SDP offer to the peer
    print("sdp:" + desc->type() + ":" + sdp);
}


void AWorker::OnFailure(const std::string &error) { // createOffer or createAnswer callback
    throw new std::runtime_error("Create session error: " + error);
}


void AWorker::recvSDP(const std::string &type, const std::string &sdp) {
    webrtc::SdpParseError error;
    webrtc::SessionDescriptionInterface *desc(webrtc::CreateSessionDescription(type, sdp, &error));
    if (!desc)
        throw std::runtime_error("Can't parse remote SDP: " + error.description);

    this->peer->SetRemoteDescription(DummySetSessionDescriptionObserver::Create(), desc);
}

void AWorker::recvCandidate(const std::string &mid, int mlineindex, const std::string &sdp) {
    webrtc::SdpParseError error;
    std::unique_ptr<webrtc::IceCandidateInterface> candidate(webrtc::CreateIceCandidate(mid, mlineindex, sdp, &error));
    if (!candidate) {
        throw std::runtime_error("Can't parse remote candidate: " + error.description);
    }
    peer->AddIceCandidate(candidate.get());
}


//
//
//

void DummySetSessionDescriptionObserver::OnSuccess() {
}


void DummySetSessionDescriptionObserver::OnFailure(const std::string &error) {
    throw new std::runtime_error("SDP parse error: " + error);
}