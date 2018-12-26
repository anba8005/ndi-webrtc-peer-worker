//
// Created by anba8005 on 12/20/18.
//

#include "LocalPeerContext.h"

#include "media/base/adaptedvideotracksource.h"
#include <iostream>

LocalPeerContext::LocalPeerContext(shared_ptr<Signaling> signaling) : PeerContext(signaling) {

}

LocalPeerContext::~LocalPeerContext() {

}

void LocalPeerContext::start() {
    reader = make_unique<NDIReader>("ANBA8005-OLD (OBS2)");
    reader->open();
    //
    context = make_shared<PeerFactoryContext>();
    pc = context->createPeerConnection(this);
    //
    addTracks();
    signaling->state("OnNegotiationNeeded");
    //
    reader->start(context->getVDM(), context->getADM());
}

void LocalPeerContext::end() {
    pc->Close();
}

void LocalPeerContext::addTracks() {
    // create audio options
    cricket::AudioOptions options;
    options.auto_gain_control = false;
    options.noise_suppression = false;
    options.highpass_filter = false;
    options.echo_cancellation = false;
    options.typing_detection = false;
    options.residual_echo_detector = false;

    // add audio track
    auto audioTrack = context->createAudioTrack(options);
    auto videoResult = pc->AddTrack(audioTrack, {"stream"});
    if (!videoResult.ok()) {
        throw std::runtime_error(
                "Failed to add audio track to PeerConnection:" + string(videoResult.error().message()));
    }

    // add video track
    auto videoTrack = context->createVideoTrack();
    videoTrack->set_content_hint(webrtc::VideoTrackInterface::ContentHint::kFluid);
    auto audioResult = pc->AddTrack(videoTrack, {"stream"});
    if (!audioResult.ok()) {
        throw std::runtime_error(
                "Failed to add video track to PeerConnection:" + string(audioResult.error().message()));
    }
}
