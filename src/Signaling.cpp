//
// Created by anba8005 on 12/20/18.
//

#include "Signaling.h"

#include <iostream>


void Signaling::send(string s) {
    unique_lock<std::mutex> lock(mutex);
    buffer.push_back(s);
}

void Signaling::commitBuffer() {
    unique_lock<std::mutex> lock(mutex);
    // copy from buffer to commitBuffer buffer
    vector<string> current;
    for (string &i : buffer) {
        current.push_back(i);
    }
    this->buffer.clear();
    lock.unlock();
    // check
    if (current.empty())
        return;
    // print commitBuffer buffer
    for (string &i : current) {
        cout << i << endl;
    }
    // flush
    cout.flush();
}

string Signaling::receive() {
    string commandLine;
    getline(cin, commandLine);
    return commandLine;
}

void Signaling::replyOk(const string command, int64_t correlation) {
    json j;
    j["command"] = command;
    j["correlation"] = correlation;
    j["ok"] = true;
    send(j.dump());
}

void Signaling::replyWithPayload(const string command, json payload, int64_t correlation) {
    json j;
    j["command"] = command;
    j["correlation"] = correlation;
    j["ok"] = true;
    j["payload"] = payload;
    send(j.dump());

}

void Signaling::replyError(const string command, string error, const int64_t correlation) {
    json j;
    j["command"] = command;
    j["correlation"] = correlation;
    j["error"] = error;
    send(j.dump());
}

void Signaling::state(const string command, json payload) {
    json j;
    j["command"] = command;
    if (payload != nullptr) {
        j["payload"] = payload;
    }
    send(j.dump());
}