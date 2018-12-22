//
// Created by anba8005 on 12/20/18.
//

#ifndef GYVAITV_WEBRTC_PEERCONTEXT_H
#define GYVAITV_WEBRTC_PEERCONTEXT_H

#include "Signaling.h"

#include <string>
#include <memory>

#include "pc/peerconnectionfactory.h"
#include "api/peerconnectioninterface.h"
#include "PeerFactoryContext.h"
#include "api/test/fakeconstraints.h"

using namespace std;

const string COMMAND_SET_LOCAL_DESCRIPTION = "setLocalDescription";
const string COMMAND_SET_REMOTE_DESCRIPTION = "setRemoteDescription";
const string COMMAND_ADD_ICE_CANDIDATE = "addIceCandidate";
const string COMMAND_CREATE_OFFER = "createOffer";
const string COMMAND_CREATE_ANSWER = "createAnswer";

class PeerContext : public webrtc::PeerConnectionObserver {
public:
    PeerContext(shared_ptr<Signaling> signaling);

    virtual ~PeerContext();

    //

    virtual void start() = 0;

    virtual void end() = 0;

    //

    void processMessages();

    //

    void setLocalDescription(const string &type, const string &sdp, int64_t correlation);

    void setRemoteDescription(const string &type, const string &sdp, int64_t correlation);

    void addIceCandidate(const string &mid, int mlineindex, const string &sdp, int64_t correlation);

    void createAnswer(int64_t correlation);

protected:

    shared_ptr<PeerFactoryContext> context;
    rtc::scoped_refptr<webrtc::PeerConnectionInterface> pc;
    webrtc::FakeConstraints constraints;

    shared_ptr<Signaling> signaling;

    // inherited from PeerConnectionObserver

    void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) override;

    void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override;

    void OnRenegotiationNeeded() override;

    void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override;

    void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override;

    void OnIceCandidate(const webrtc::IceCandidateInterface *candidate) override;

private:

    unique_ptr<webrtc::SessionDescriptionInterface> createSessionDescription(const string &type_str, const string &sdp);
};


#endif //GYVAITV_WEBRTC_PEERCONTEXT_H
