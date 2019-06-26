//
// Created by anba8005 on 12/20/18.
//

#include "Dispatcher.h"

#include <thread>
#include <iostream>
#include "json.hpp"

using json = nlohmann::json;

Dispatcher::Dispatcher(shared_ptr<Signaling> signaling, shared_ptr<PeerContext> peer) : signaling(signaling),
                                                                                        peer(peer), finished(false) {

}

void Dispatcher::run() {
    peer->start();
    //
    thread runner(&Dispatcher::dispatch, this);
    //
    while (!this->finished.load()) {
        peer->processMessages();
        signaling->commitBuffer();
    }
    //
    runner.join();
    //
    peer->end();
}

void Dispatcher::dispatch() {
    while (!this->finished.load()) {
        // get command line
        string commandLine = signaling->receive();
        //
        if (commandLine == "STOP") {
            finished = true;
            break;
        }
        // decode
        json root;
        try {
            // dispatch
            root = json::parse(commandLine);
            string command = root.value("command", "");
            int64_t correlation = root.value("correlation", 0);
            this->exec(command, correlation, root["payload"]);
        } catch (const std::exception& e) {
            // parse error
            cerr << "JSON parse error" << endl;
            cerr << "---" << endl;
            cerr << commandLine << endl;
            cerr << "---->" << endl;
            cerr << e.what() << endl;
            cerr << "--->" << endl;
        }
    }
}

void Dispatcher::exec(string command, int64_t correlation, json payload) {
    if (!peer->hasPeer()) {
        if (command == COMMAND_CREATE_PEER) {
            //
            peer->createPeer(payload, correlation);
            //
        } else if (command == COMMAND_FIND_NDI_SOURCES) {
			//
			peer->findNDISources(correlation);
			//
		} else {
            //
            throw std::runtime_error(command + " error - peer is not created");
            //
        }
    } else {
        if (command == COMMAND_CREATE_PEER) {
            //
            throw std::runtime_error(command + " error - peer is already created");
            //
        } else if (command == COMMAND_ADD_ICE_CANDIDATE) {
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
            //
        } else if (command == COMMAND_CREATE_OFFER) {
            //
            peer->createOffer(payload, correlation);
            //
        } else if (command == COMMAND_CREATE_ANSWER) {
            //
            peer->createAnswer(payload, correlation);
            //
        } else if (command == COMMAND_CREATE_DATA_CHANNEL) {
            //
            const string name = payload.value("name", "");
            peer->createDataChannel(name, correlation);
            //
        } else if (command == COMMAND_GET_STATS) {
            //
            peer->getStats(correlation);
            //
        } else if (command == COMMAND_GET_STATS_OLD) {
			//
			peer->getStatsOld(correlation);
			//
		} else if (command == COMMAND_GET_SENDERS) {
			//
			peer->getSenders(correlation);
			//
		} else if (command == COMMAND_GET_RECEIVERS) {
			//
			peer->getReceivers(correlation);
			//
		} else if (command == COMMAND_SEND_DATA_MESSAGE) {
            //
            const string data = payload.value("data", "");
            peer->sendDataMessage(data, correlation);
            //
        } else if (command == COMMAND_ADD_TRACK) {
            //
            peer->addTrack(payload, correlation);
            //
        } else if (command == COMMAND_REMOVE_TRACK) {
            //
            const string trackId = payload.value("trackId","");
            peer->removeTrack(trackId, correlation);
            //
        } else if (command == COMMAND_REPLACE_TRACK) {
            //
            peer->replaceTrack(payload, correlation);
            //
        }
    }
}