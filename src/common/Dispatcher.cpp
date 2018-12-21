//
// Created by anba8005 on 12/20/18.
//

#include "Dispatcher.h"

#include <thread>
#include <iostream>
#include "json.hpp"

using json = nlohmann::json;

Dispatcher::Dispatcher(shared_ptr<Signaling> signaling, shared_ptr<PeerContext> peer) : signaling(signaling),
                                                                                        peer(peer) {

}

void Dispatcher::run() {
    peer->start();
    //
    thread dispatcherThread(&Dispatcher::dispatch, this);
    //
    while (!this->finished.load()) {
        peer->processMessages();
        signaling->commitBuffer();
    }
    //
    dispatcherThread.join();
    //
    peer->end();
}

void Dispatcher::dispatch() {
    while (!this->finished.load()) {
        // get command line
        string commandLine = signaling->receive();
        // decode
        json root;
        try {
            // dispatch
            root = json::parse(commandLine);
            string command = root.value("command", "");
            int64_t correlation = root.value("correlation", 0);
            this->exec(command, correlation, root["payload"]);
        } catch (...) {
            // parse error
            cerr << "JSON parse error" << endl;
            cerr << "---" << endl;
            cerr << commandLine << endl;
            cerr << "---" << endl;
        }
    }
}

void Dispatcher::exec(string command, int64_t correlation, json payload) {
    if (command == COMMAND_ADD_ICE_CANDIDATE) {
        //
        const string sdpMid = payload.value("sdpMid", "");
        const int sdpMLineIndex = payload.value("sdpMLineIndex", 1);
        const string candidate = payload.value("candidate", "");
        peer->addIceCandidate(sdpMid, sdpMLineIndex, candidate, correlation);
        //
    } else if (command == COMMAND_SET_REMOTE_DESCRIPTION) {
        //
        const string sdp = payload.value("sdp", "");
        const string type = payload.value("type", "");
        peer->setRemoteDescription(type, sdp, correlation);
        //
    } else if (command == COMMAND_SET_LOCAL_DESCRIPTION) {
        //
        const string sdp = payload.value("sdp", "");
        const string type = payload.value("type", "");
        peer->setLocalDescription(type, sdp, correlation);
    } else if (command == COMMAND_CREATE_OFFER) {
        //

        //
    } else if (command == COMMAND_CREATE_ANSWER) {
        //
        peer->createAnswer(correlation);
        //
    }
}