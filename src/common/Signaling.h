//
// Created by anba8005 on 12/20/18.
//

#ifndef GYVAITV_WEBRTC_SIGNALING_H
#define GYVAITV_WEBRTC_SIGNALING_H

#include <string>
#include <mutex>
#include <vector>
#include <memory>

using namespace std;

class Signaling {
public:
    void send(string s);
    void commit();

    string receive();
private:
    std::mutex mutex;
    vector<string> buffer;

};


#endif //GYVAITV_WEBRTC_SIGNALING_H
