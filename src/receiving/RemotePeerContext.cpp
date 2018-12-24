//
// Created by anba8005 on 12/20/18.
//

#include "RemotePeerContext.h"
#include <iostream>

RemotePeerContext::RemotePeerContext(shared_ptr<Signaling> signaling) : PeerContext(signaling) {


}

RemotePeerContext::~RemotePeerContext() {

}

void RemotePeerContext::start() {
    writer = make_unique<NDIWriter>("TEST",1280,720);
    //
    context = make_shared<PeerFactoryContext>();
    //
//    this->constraints.SetMandatoryReceiveVideo(true);
//    this->constraints.SetMandatoryReceiveAudio(true);
//    this->constraints.SetAllowDtlsSctpDataChannels();
    //
    pc = context->createPeerConnection(this);
}

void RemotePeerContext::end() {

}

void RemotePeerContext::OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
                                   const vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>> &streams) {
    PeerContext::OnAddTrack(receiver, streams);
    //
    auto track = receiver->track();
    if (track->kind() == track->kVideoKind) {
        writer->setVideoTrack(dynamic_cast<webrtc::VideoTrackInterface*>(track.release()));
    } else if (track->kind() == track->kAudioKind) {
        writer->setAudioTrack(dynamic_cast<webrtc::AudioTrackInterface*>(track.release()));
    }
}

void RemotePeerContext::OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) {
    PeerContext::OnRemoveTrack(receiver);
    //
    auto track = receiver->track();
    if (track->kind() == track->kVideoKind) {
        writer->setVideoTrack(nullptr);
    } else if (track->kind() == track->kAudioKind) {
        writer->setAudioTrack(nullptr);
    }

}

//void RemotePeerContext::OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) {
//    cerr << "ON ADD STREAM" << endl;
//    auto videoTracks = stream->GetVideoTracks();
//
//    if (!videoTracks.empty())
//        writer->setVideoTrack(videoTracks[0]);
//
//    auto audioTracks = stream->GetAudioTracks();
//    if (!audioTracks.empty())
//        writer->setAudioTrack(audioTracks[0]);
//}
//
//void RemotePeerContext::OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) {
//    cerr << "ON REMOVE STREAM" << endl;
//}