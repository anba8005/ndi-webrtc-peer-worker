//
// Created by anba8005 on 12/20/18.
//

#ifndef GYVAITV_WEBRTC_SETSESSIONDESCRIPTIONOBSERVER_H
#define GYVAITV_WEBRTC_SETSESSIONDESCRIPTIONOBSERVER_H

#include <api/jsep.h>
#include <api/rtc_error.h>
#include <string>
#include <memory>

#include "json.hpp"

#include "Signaling.h"

using json = nlohmann::json;
using namespace std;

class SetSessionDescriptionObserver : public webrtc::SetSessionDescriptionObserver {
public:
    static SetSessionDescriptionObserver *Create(shared_ptr<Signaling> signaling, string command, int64_t correlation);

    virtual void OnSuccess() override;

    virtual void OnFailure(webrtc::RTCError error) override;

protected:

    SetSessionDescriptionObserver(shared_ptr<Signaling> signaling, string command, int64_t correlation);

private:

    shared_ptr<Signaling> signaling;
    const string command;
    const int64_t correlation;


};

#endif //GYVAITV_WEBRTC_SETSESSIONDESCRIPTIONOBSERVER_H
