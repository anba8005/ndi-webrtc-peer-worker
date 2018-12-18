//
// Created by anba8005 on 12/15/18.
//

#include "ReceivingWorker.h"

#include <iostream>

#include "modules/audio_device/include/audio_device.h"
#include "../common/FakeAudioDeviceModule.h"
#include "p2p/client/basicportallocator.h"

ReceivingWorker::ReceivingWorker() {

}

ReceivingWorker::~ReceivingWorker() {

}

void ReceivingWorker::onStart() {
    this->context = std::make_shared<PeerFactoryContext>(FakeAudioDeviceModule::Create());
    //
    this->constraints.SetMandatoryReceiveVideo(true);
    this->constraints.SetMandatoryReceiveAudio(true);
    this->peer = context->createPeerConnection(&constraints,this);
}

void ReceivingWorker::onEnd() {

}

void ReceivingWorker::onCommand(std::vector<std::string> command) {

}
