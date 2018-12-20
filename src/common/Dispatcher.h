//
// Created by anba8005 on 12/20/18.
//

#ifndef GYVAITV_WEBRTC_DISPATCHER_H
#define GYVAITV_WEBRTC_DISPATCHER_H

#include "Signaling.h"
#include "PeerContext.h"
#include "json.hpp"

#include <atomic>
#include <memory>

using json = nlohmann::json;
using namespace std;

class Dispatcher {
public:
    Dispatcher(shared_ptr<Signaling> signaling, shared_ptr<PeerContext> peer);

    void run();

private:
    shared_ptr<Signaling> signaling;
    shared_ptr<PeerContext> peer;

    atomic<bool> finished;

    void dispatch();

    void exec(string command, string correlation, json payload);
};


#endif //GYVAITV_WEBRTC_DISPATCHER_H
