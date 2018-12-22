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
    writer.reset(new NDIWriter("TEST",1280,720));
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

void RemotePeerContext::OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) {
    cerr << "ON ADD STREAM" << endl;
    auto videoTracks = stream->GetVideoTracks();
    if (!videoTracks.empty())
        writer->setVideoTrack(videoTracks[0]);

    auto audioTracks = stream->GetAudioTracks();
    if (!audioTracks.empty())
        writer->setAudioTrack(audioTracks[0]);
}

void RemotePeerContext::OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) {
    cerr << "ON REMOVE STREAM" << endl;
}