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

#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"

AWorker::AWorker() {

    networkThread = rtc::Thread::CreateWithSocketServer();
    workerThread = rtc::Thread::Create();
    if (!networkThread->Start() || !workerThread->Start())
        throw std::runtime_error("Failed to start WebRTC threads");

    webrtc::AudioDeviceModule* default_adm = webrtc::AudioDeviceModule::Create(0, webrtc::AudioDeviceModule::kDummyAudio);
    rtc::scoped_refptr<webrtc::AudioEncoderFactory> audio_encoder_factory = webrtc::CreateBuiltinAudioEncoderFactory();
    rtc::scoped_refptr<webrtc::AudioDecoderFactory> audio_decoder_factory = webrtc::CreateBuiltinAudioDecoderFactory();

    factory = webrtc::CreatePeerConnectionFactory(
            networkThread.get(), workerThread.get(), rtc::Thread::Current(),
            default_adm, audio_encoder_factory, audio_decoder_factory,
            nullptr, nullptr);

}

AWorker::~AWorker() {
}

void AWorker::run() {
    if (!this->onStart())
        return;
    //
    while (true) {
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
            if (!this->onCommand(command)) {
                break;
            }
        } else {
            // wait
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
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
    std::unique_lock<std::mutex> lock(print_mutex);
    print_buffer.push_back(s);
}

void AWorker::print_commit() {
    std::unique_lock<std::mutex> lock(print_mutex);
    // copy from buffer to commit buffer
    std::vector<std::string> current;
    for (std::string &i : print_buffer) {
        current.push_back(i);
    }
    print_buffer.clear();
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
