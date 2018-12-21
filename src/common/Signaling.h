//
// Created by anba8005 on 12/20/18.
//

#ifndef GYVAITV_WEBRTC_SIGNALING_H
#define GYVAITV_WEBRTC_SIGNALING_H

#include <string>
#include <mutex>
#include <vector>
#include <memory>

#include "json.hpp"

using json = nlohmann::json;

using namespace std;

class Signaling {
public:
    void commitBuffer();

    string receive();

    void replyOk(const string command, int64_t correlation);

    void replyWithPayload(const string command, json payload, int64_t correlation);

    void replyError(const string command, string error, int64_t correlation);

    void state(const string command, json payload);

private:
    std::mutex mutex;
    vector<string> buffer;

    void send(string s);

};


#endif //GYVAITV_WEBRTC_SIGNALING_H
