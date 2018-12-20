//
// Created by anba8005 on 12/20/18.
//

#include "RemotePeerContext.h"

#include "../common/FakeAudioDeviceModule.h"

RemotePeerContext::RemotePeerContext(shared_ptr<Signaling> signaling) : PeerContext(signaling) {


}

RemotePeerContext::~RemotePeerContext() {

}

void RemotePeerContext::start() {
    this->context = make_shared<PeerFactoryContext>(FakeAudioDeviceModule::Create());
    //
    this->constraints.SetMandatoryReceiveVideo(true);
    this->constraints.SetMandatoryReceiveAudio(true);
    this->constraints.SetAllowDtlsSctpDataChannels();
    //
    this->peer = context->createPeerConnection(&constraints,this);
}

void RemotePeerContext::end() {

}
