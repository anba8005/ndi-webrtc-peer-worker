//
// Created by anba8005 on 12/20/18.
//

#ifndef GYVAITV_WEBRTC_CREATESESSIONDESCRIPTIONOBSERVER_H
#define GYVAITV_WEBRTC_CREATESESSIONDESCRIPTIONOBSERVER_H

#include <api/jsep.h>
#include <string>
#include <memory>

#include "json.hpp"

#include "Signaling.h"

using json = nlohmann::json;
using namespace std;

class CreateSessionDescriptionObserver : public webrtc::CreateSessionDescriptionObserver {
public:
    static CreateSessionDescriptionObserver *Create(shared_ptr<Signaling> signaling, string command, int64_t correlation);

    void OnSuccess(webrtc::SessionDescriptionInterface *desc) override;

    void OnFailure(const string &error) override;

protected:

    CreateSessionDescriptionObserver(shared_ptr<Signaling> signaling, string command, int64_t correlation);

private:

    shared_ptr<Signaling> signaling;
    string command;
    uint64_t correlation;

};


#endif //GYVAITV_WEBRTC_CREATESESSIONDESCRIPTIONOBSERVER_H
