//
// Created by anba8005 on 12/20/18.
//
#ifndef GYVAITV_WEBRTC_PEERCONTEXT_H
#define GYVAITV_WEBRTC_PEERCONTEXT_H

#ifdef WIN32
#define NOMINMAX
#undef min
#undef max
#endif

#include "Signaling.h"

#include <string>
#include <memory>

#include "json.hpp"

#include "api/peer_connection_factory_proxy.h"
#include "api/peer_connection_interface.h"
#include "PeerFactoryContext.h"
#include "NDIWriter.h"
#include "NDIReader.h"
#include "StatsCollectorCallback.h"
#include "StatsObserver.h"


using json = nlohmann::json;

using namespace std;

const string COMMAND_CREATE_PEER = "createPeer";
const string COMMAND_SET_LOCAL_DESCRIPTION = "setLocalDescription";
const string COMMAND_SET_REMOTE_DESCRIPTION = "setRemoteDescription";
const string COMMAND_ADD_ICE_CANDIDATE = "addIceCandidate";
const string COMMAND_CREATE_OFFER = "createOffer";
const string COMMAND_CREATE_ANSWER = "createAnswer";
const string COMMAND_CREATE_DATA_CHANNEL = "createDataChannel";
const string COMMAND_GET_STATS = "getStats";
const string COMMAND_GET_STATS_OLD = "getStatsOld";
const string COMMAND_GET_SENDERS = "getSenders";
const string COMMAND_GET_RECEIVERS = "getReceivers";
const string COMMAND_SEND_DATA_MESSAGE = "sendDataMessage";
const string COMMAND_ADD_TRACK = "addTrack";
const string COMMAND_REMOVE_TRACK = "removeTrack";
const string COMMAND_REPLACE_TRACK = "replaceTrack";
const string COMMAND_FIND_NDI_SOURCES = "findNDISources";

class PeerContext : public webrtc::PeerConnectionObserver, public webrtc::DataChannelObserver {
public:
    PeerContext(shared_ptr<Signaling> signaling);

    virtual ~PeerContext();

    //

    void start();

    void end();

    void createPeer(json configuration, int64_t correlation);

    bool hasPeer();

    void processMessages();

    //

    void setLocalDescription(const string &type, const string &sdp, int64_t correlation);

    void setRemoteDescription(const string &type, const string &sdp, int64_t correlation);

    void addIceCandidate(const string &mid, int mlineindex, const string &sdp, int64_t correlation);

    void createAnswer(json payload, int64_t correlation);

    void createOffer(json payload, int64_t correlation);

    void createDataChannel(const string &name, int64_t correlation);

    void getStats(int64_t correlation);

	void getStatsOld(int64_t correlation);

	void getSenders(int64_t correlation);

	void getReceivers(int64_t correlation);

    void sendDataMessage(const string &data, int64_t correlation);

    void addTrack(json payload, int64_t correlation);

    void removeTrack(string trackId, int64_t correlation);

    void replaceTrack(json payload, int64_t correlation);

    void findNDISources(int64_t correlation);

protected:

    shared_ptr<PeerFactoryContext> context;
    rtc::scoped_refptr<webrtc::PeerConnectionInterface> pc;
    rtc::scoped_refptr<webrtc::DataChannelInterface> dc;

    shared_ptr<Signaling> signaling;

    // inherited from PeerConnectionObserver

    void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) override;

    void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override;

    void OnRenegotiationNeeded() override;

    void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override;

    void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override;

    void OnIceCandidate(const webrtc::IceCandidateInterface *candidate) override;

    void OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
                    const vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>> &streams) override;

    void OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override;

    // inherited from DataChannelObserver

    void OnStateChange() override;

    void OnMessage(const webrtc::DataBuffer &buffer) override;

public:


private:

    unique_ptr<webrtc::SessionDescriptionInterface> createSessionDescription(const string &type_str, const string &sdp);

    unique_ptr<NDIWriter> writer;

    unique_ptr<NDIWriter::Configuration> writerConfig;

	unique_ptr<NDIWriter> preview;

	unique_ptr<NDIWriter::Configuration> previewConfig;

    unique_ptr<NDIReader> reader;

    std::map<std::string,std::string> receivedTracks;
};


#endif //GYVAITV_WEBRTC_PEERCONTEXT_H
