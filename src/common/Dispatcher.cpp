//
// Created by anba8005 on 12/20/18.
//

#include "Dispatcher.h"

#include <thread>
#include <iostream>
#include "json.hpp"

using json = nlohmann::json;

Dispatcher::Dispatcher(shared_ptr<Signaling> signaling, shared_ptr<PeerContext> peer) : signaling(signaling), peer(peer) {

}

void Dispatcher::run() {
    peer->start();
    //
    thread dispatcherThread(&Dispatcher::dispatch, this);
    //
    while (!this->finished.load()) {
        peer->processMessages();
        signaling->commit();
    }
    //
    dispatcherThread.join();
    //
    peer->end();
}

void Dispatcher::dispatch() {
    cerr << "dispatcher started" << endl;
    while (!this->finished.load()) {
        // get command line
        string commandLine = signaling->receive();
        // decode
        json root;
        try {
            // dispatch
            root = json::parse(commandLine);
            string command = root.value("command","");
            string correlation = root.value("correlation","");
            this->exec(command,correlation,root["payload"]);
        } catch (...) {
            // parse error
            cerr << "JSON parse error" << endl;
            cerr << "---" << endl;
            cerr << commandLine << endl;
            cerr << "---" << endl;
        }
    }
}

void Dispatcher::exec(string command, string correlation, json payload) {
    cerr << "command -> " << command << endl;
    if (command == "addIceCandidate") {
        //
        const string sdpMid = payload.value("sdpMid","");
        const int sdpMLineIndex = payload.value("sdpMLineIndex",1);
        const string candidate = payload.value("candidate","");
        peer->addIceCandidate(sdpMid,sdpMLineIndex,candidate);
        //
    } else if (command == "setRemoteDescription") {
        //
        const string sdp = payload.value("sdp","");
        const string type = payload.value("type","");
        peer->setRemoteDescription(type, sdp);
        //
    } else if (command == "setLocalDescription") {
        //
        const string sdp = payload.value("sdp","");
        const string type = payload.value("type","");
        peer->setLocalDescription(type, sdp);
    } else if (command == "createOffer") {
        //
        const string sdp = payload.value("sdp","");
        const string type = payload.value("type","");
        //this->setRemoteDescription(type, sdp);
        //
    } else if (command == "createAnswer") {
        //
        const string sdp = payload.value("sdp","");
        const string type = payload.value("type","");
        //this->setRemoteDescription(type, sdp);
        //
    }
}