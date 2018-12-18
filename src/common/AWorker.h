//
// Created by anba8005 on 12/15/18.
//

#ifndef GYVAITV_WEBRTC_AWORKER_H
#define GYVAITV_WEBRTC_AWORKER_H

#include <string>
#include <mutex>
#include <vector>
#include <memory>

#include "pc/peerconnectionfactory.h"
#include "api/peerconnectioninterface.h"
#include "PeerFactoryContext.h"
#include "api/test/fakeconstraints.h"


class AWorker : public webrtc::PeerConnectionObserver, public webrtc::CreateSessionDescriptionObserver {
public:
    AWorker();

    virtual ~AWorker();

    void run();

protected:
    virtual void onStart() = 0;

    virtual void onEnd() = 0;

    virtual void onCommand(std::vector<std::string> command) = 0;

    void print(std::string s);

    void error(std::string e);

    void recvSDP(const std::string &type, const std::string &sdp);

    void recvCandidate(const std::string &mid, int mlineindex, const std::string &sdp);

    std::shared_ptr<PeerFactoryContext> context;
    rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer;
    webrtc::FakeConstraints constraints;

private:
    bool isCommandAvailable();

    void print_commit();

    std::mutex print_mutex;
    std::mutex error_mutex;
    std::vector<std::string> print_buffer;
    std::vector<std::string> error_buffer;

    std::atomic<bool> finished;

protected:

    // inherited from PeerConnectionObserver

    void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) override;

    void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override;

    void OnRenegotiationNeeded() override;

    void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override;

    void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override;

    void OnIceCandidate(const webrtc::IceCandidateInterface *candidate) override;

    // inherited from CreateSessionDescriptionObserver

    virtual void OnSuccess(webrtc::SessionDescriptionInterface *desc) override;

    virtual void OnFailure(const std::string &error) override;

    virtual void AddRef() const override { return; }

    virtual rtc::RefCountReleaseStatus Release() const override { return rtc::RefCountReleaseStatus::kDroppedLastRef; }


};

//
//
//

class DummySetSessionDescriptionObserver
        : public webrtc::SetSessionDescriptionObserver {
public:
    static DummySetSessionDescriptionObserver *Create() {
        return new rtc::RefCountedObject<DummySetSessionDescriptionObserver>();
    }

    virtual void OnSuccess() override;

    virtual void OnFailure(const std::string &error) override;

protected:
    DummySetSessionDescriptionObserver() = default;

    ~DummySetSessionDescriptionObserver() = default;
};


#endif //GYVAITV_WEBRTC_AWORKER_H
