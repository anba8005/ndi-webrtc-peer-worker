//
// Created by anba8005 on 12/20/18.
//

#include "Signaling.h"

#include <iostream>


void Signaling::send(string s) {
    unique_lock<std::mutex> lock(mutex);
    buffer.push_back(s);
}

void Signaling::commit() {
    unique_lock<std::mutex> lock(mutex);
    // copy from buffer to commit buffer
    vector<string> current;
    for (string &i : buffer) {
        current.push_back(i);
    }
    this->buffer.clear();
    lock.unlock();
    // check
    if (current.empty())
        return;
    // print commit buffer
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