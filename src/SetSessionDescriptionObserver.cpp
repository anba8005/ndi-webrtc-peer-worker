//
// Created by anba8005 on 12/20/18.
//

#include "SetSessionDescriptionObserver.h"

#define _WINSOCK2API_
#define _WINSOCKAPI_

#include <rtc_base/ref_counted_object.h>

SetSessionDescriptionObserver::SetSessionDescriptionObserver(shared_ptr<Signaling> signaling, string command, int64_t correlation)
        : correlation(correlation), signaling(signaling), command(command) {
}

void SetSessionDescriptionObserver::OnSuccess() {
    signaling->replyOk(command,correlation);
}

void SetSessionDescriptionObserver::OnFailure(webrtc::RTCError error) {
    signaling-> replyError(command,"error",correlation);
}

SetSessionDescriptionObserver *
SetSessionDescriptionObserver::Create(shared_ptr<Signaling> signaling, string command, int64_t correlation) {
    return new rtc::RefCountedObject<SetSessionDescriptionObserver>(signaling, command, correlation);
}
