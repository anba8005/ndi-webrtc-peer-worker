//
// Created by anba8005 on 12/20/18.
//

#include "PeerContext.h"
#include "SetSessionDescriptionObserver.h"
#include "CreateSessionDescriptionObserver.h"
#include "StatsObserver.h"

#include <iostream>

PeerContext::PeerContext(shared_ptr<Signaling> signaling) : signaling(signaling), totalTracks(0) {

}

PeerContext::~PeerContext() {
	// TODO
//  +  1. getstats - kad veiktu "connected"
//  + 2. ondatachannel + proper datachannel observer- kad veiktu "connected"
//  + 3. send receive messages - kad veiktu statistikos siuntims
//   + 5. addtrack removetrack - dynamic reader streams ?
//   + 4. writer refactoring (dynamic dimesnions)
//  + smth with offertoreceivevideo/audio
//  + peerfactorycontext configuration (stun servers)
}

void PeerContext::start() {
	context = make_shared<PeerFactoryContext>();
}

void PeerContext::end() {
	if (pc) {
		if (dc) {
			dc->Close();
			dc->UnregisterObserver();
			dc = nullptr;
		}
		pc->Close();
		pc = nullptr;
	}
}

void PeerContext::createPeer(json configuration, int64_t correlation) {
	if (pc)
		throw std::runtime_error("peer already created");
	//
	context->setConfiguration(configuration);
	pc = context->createPeerConnection(this);
	//
	const json ndi = configuration["ndi"];
	if (!ndi.empty()) {
		writerConfig.reset(new NDIWriter::Configuration(ndi));
	}
	//
	signaling->replyOk(COMMAND_CREATE_PEER, correlation);
}

bool PeerContext::hasPeer() {
	return this->pc.get();
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

void PeerContext::createAnswer(json payload, int64_t correlation) {
	webrtc::PeerConnectionInterface::RTCOfferAnswerOptions options;
	options.offer_to_receive_video = payload.value("offerToReceiveVideo", true);
	options.offer_to_receive_audio = payload.value("offerToReceiveAudio", true);
	options.voice_activity_detection = payload.value("voiceActivityDetection", true);
	//
	auto observer = CreateSessionDescriptionObserver::Create(signaling, COMMAND_CREATE_ANSWER, correlation);
	pc->CreateAnswer(observer, options);
}

void PeerContext::createOffer(json payload, int64_t correlation) {
	webrtc::PeerConnectionInterface::RTCOfferAnswerOptions options;
	options.offer_to_receive_video = payload.value("offerToReceiveVideo", true);
	options.offer_to_receive_audio = payload.value("offerToReceiveAudio", true);
	options.voice_activity_detection = payload.value("voiceActivityDetection", true);
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
	if (dc) {
		signaling->replyError(COMMAND_CREATE_DATA_CHANNEL, "Only one datachannel supported", correlation);
	} else {
		webrtc::DataChannelInit config;
		dc = pc->CreateDataChannel(name, &config);
		dc->RegisterObserver(this);
		signaling->replyOk(COMMAND_CREATE_DATA_CHANNEL, correlation);
	}
}

void PeerContext::sendDataMessage(const string &data, int64_t correlation) {
	if (dc) {
		rtc::CopyOnWriteBuffer buffer(data);
		webrtc::DataBuffer dataBuffer(buffer, false);
		if (!dc->Send(dataBuffer)) {
			switch (dc->state()) {
				case webrtc::DataChannelInterface::kConnecting:
					signaling->replyError(COMMAND_SEND_DATA_MESSAGE, "Send error - Datachannel is connecting",
										  correlation);
					break;
				case webrtc::DataChannelInterface::kOpen:
					signaling->replyError(COMMAND_SEND_DATA_MESSAGE, "Send error", correlation);
					break;
				case webrtc::DataChannelInterface::kClosing:
					signaling->replyError(COMMAND_SEND_DATA_MESSAGE, "Send error - Datachannel is closing",
										  correlation);
					break;
				case webrtc::DataChannelInterface::kClosed:
					signaling->replyError(COMMAND_SEND_DATA_MESSAGE, "Send error - Datachannel is closed", correlation);
					break;
			}
		} else {
			signaling->replyOk(COMMAND_SEND_DATA_MESSAGE, correlation);
		}
	} else {
		signaling->replyError(COMMAND_SEND_DATA_MESSAGE, "Datachannel is closed", correlation);
	}
}

void PeerContext::getStats(int64_t correlation) {
	pc->GetStats(StatsCollectorCallback::Create(signaling, correlation));
}

void PeerContext::getStatsOld(int64_t correlation) {
	pc->GetStats(StatsObserver::Create(signaling, correlation), nullptr,
				 webrtc::PeerConnectionInterface::StatsOutputLevel::kStatsOutputLevelStandard);
}

void PeerContext::getSenders(int64_t correlation) {
	const std::vector<rtc::scoped_refptr<webrtc::RtpSenderInterface>> senders = pc->GetSenders();
	//
	json payload = json::array();
	for (const auto &sender : senders) {
		json s;
		s["track"]["id"] = sender->track()->id();
		s["track"]["kind"] = sender->track()->kind();
		payload.push_back(s);
	}
	//
	signaling->replyWithPayload(COMMAND_GET_SENDERS, payload, correlation);
}

void PeerContext::getReceivers(int64_t correlation) {
	const std::vector<rtc::scoped_refptr<webrtc::RtpReceiverInterface>> receivers = pc->GetReceivers();
	//
	json payload = json::array();
	for (const auto &receiver : receivers) {
		json r;
		r["track"]["id"] = receiver->track()->id();
		r["track"]["kind"] = receiver->track()->kind();
		payload.push_back(r);
	}
	//
	signaling->replyWithPayload(COMMAND_GET_RECEIVERS, payload, correlation);
}

void PeerContext::addTrack(json payload, int64_t correlation) {
	// parse values
	const string id = payload.value("id", "");
	const bool audio = payload.value("audio", false);
	const bool video = payload.value("video", false);
	const json audioOptions = payload["audioOptions"];

	// validate
	if (id.empty()) {
		signaling->replyError(COMMAND_ADD_TRACK, "Stream id is empty", correlation);
		return;
	}
	if (!audio && !video) {
		signaling->replyError(COMMAND_ADD_TRACK, "Both audio & video disabled", correlation);
		return;
	}
	if (audioOptions == nullptr) {
		signaling->replyError(COMMAND_ADD_TRACK, "No audio options", correlation);
		return;
	}

	// create reader
	try {
		reader = make_unique<NDIReader>();
		NDIReader::Configuration configuration(payload);
		reader->open(configuration);
	} catch (const std::exception &e) {
		signaling->replyError(COMMAND_ADD_TRACK, "Failed to create NDI reader: " + string(e.what()), correlation);
		return;
	}

	// create audio options
	cricket::AudioOptions options;
	options.auto_gain_control = audioOptions.value("autoGainControl", false);
	options.noise_suppression = audioOptions.value("noiseSuppression", false);
	options.highpass_filter = audioOptions.value("highPassFilter", false);
	options.echo_cancellation = audioOptions.value("echoCancelation", false);
	options.typing_detection = audioOptions.value("typingDetection", false);
	options.residual_echo_detector = audioOptions.value("residualEchoDetector", false);

	// create content hint (TODO opitions)
	webrtc::VideoTrackInterface::ContentHint hint = webrtc::VideoTrackInterface::ContentHint::kFluid;

	// add audio track
	if (audio) {
		auto audioTrack = context->createAudioTrack(options);
		auto audioResult = pc->AddTrack(audioTrack, {id});
		if (!audioResult.ok()) {
			signaling->replyError(COMMAND_ADD_TRACK,
								  "Failed to add audio track to PeerConnection:" +
								  string(audioResult.error().message()),
								  correlation);
			return;
		}
	}

	// add video track
	if (video) {
		auto videoTrack = context->createVideoTrack();
		videoTrack->set_content_hint(hint);
		auto videoResult = pc->AddTrack(videoTrack, {id});
		if (!videoResult.ok()) {
			signaling->replyError(COMMAND_ADD_TRACK,
								  "Failed to add video track to PeerConnection:" +
								  string(videoResult.error().message()),
								  correlation);
			return;
		}
	}

	// try start reader (addtrack after removetrack case)
	if (pc->ice_gathering_state() == webrtc::PeerConnectionInterface::IceGatheringState::kIceGatheringComplete) {
		reader->start(context->getVDM(), context->getADM());
	}

	// return
	signaling->replyOk(COMMAND_ADD_TRACK, correlation);
}

void PeerContext::removeTrack(string trackId, int64_t correlation) {
	bool found = false;
	for (auto const &sender: pc->GetSenders()) {
		auto ids = sender->stream_ids();
		if (ids.size() > 0 && ids[0] == trackId) {
			found = true;
			auto result = pc->RemoveTrackNew(sender);
			if (!result.ok()) {
				signaling->replyError(COMMAND_REMOVE_TRACK, "Error removing track: " + string(result.message()),
									  correlation);
				return;
			}
		}
	}
	//
	if (found && reader->isRunning()) {
		reader->stop();
		reader.reset();
	}
	//
	signaling->replyOk(COMMAND_REMOVE_TRACK, correlation);
}

void PeerContext::replaceTrack(json payload, int64_t correlation) {
	// parse values
	const bool audio = payload.value("audio", false);
	const bool video = payload.value("video", false);

	// validate
	if (!reader) {
		signaling->replyError(COMMAND_REPLACE_TRACK, "No NDI reader", correlation);
		return;
	}
	if (!audio && !video) {
		signaling->replyError(COMMAND_ADD_TRACK, "Both audio & video disabled", correlation);
		return;
	}

	// reopen reader
	NDIReader::Configuration configuration(payload);
	reader->open(configuration);

	//
	signaling->replyOk(COMMAND_REPLACE_TRACK, correlation);
}

void PeerContext::findNDISources(int64_t correlation) {
	json payload = NDIReader::findSources();
	signaling->replyWithPayload(COMMAND_FIND_NDI_SOURCES,payload,correlation);
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
	if (!dc) {
		dc = data_channel;
		json payload;
		payload["name"] = data_channel->label();
		dc->RegisterObserver(this);
		signaling->state("OnDataChannel", payload);
	}
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
	if (new_state == webrtc::PeerConnectionInterface::IceGatheringState::kIceGatheringComplete && reader) {
		reader->start(context->getVDM(), context->getADM());
	}
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
	if (writerConfig && writerConfig->isEnabled()) {
		//
		if (!writer) {
			writer = make_unique<NDIWriter>();
			writer->open(*(writerConfig.get()));
		}

		//
		if (track->kind() == track->kVideoKind) {
			writer->setVideoTrack(dynamic_cast<webrtc::VideoTrackInterface *>(track.release()));
		} else if (track->kind() == track->kAudioKind) {
			writer->setAudioTrack(dynamic_cast<webrtc::AudioTrackInterface *>(track.release()));
		}
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
	if (writer) {
		if (track->kind() == track->kVideoKind) {
			writer->setVideoTrack(nullptr);
		} else if (track->kind() == track->kAudioKind) {
			writer->setAudioTrack(nullptr);
		}
	}
	//
	totalTracks--;
	//
	if (totalTracks == 0 && writerConfig && !writerConfig->persistent)
		writer.reset();
}

//


void PeerContext::OnStateChange() {
	json payload;
	payload["state"] = dc->state();
	signaling->state("OnDataChannelStateChange", payload);
}

void PeerContext::OnMessage(const webrtc::DataBuffer &buffer) {
	if (buffer.size() > 0) {
		json payload;
		payload["data"] = std::string(buffer.data.data<char>(), buffer.data.size());
		signaling->state("OnDataChannelMessage", payload);
	}
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
