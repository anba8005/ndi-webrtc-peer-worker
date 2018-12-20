//
// Created by anba8005 on 12/20/18.
//

#ifndef GYVAITV_WEBRTC_REMOTEPEERCONTEXT_H
#define GYVAITV_WEBRTC_REMOTEPEERCONTEXT_H

#include "../common/PeerContext.h"

class RemotePeerContext : public PeerContext {
public:
    RemotePeerContext(shared_ptr<Signaling> signaling);

    virtual ~RemotePeerContext();

    void start() override;

    void end() override;

};


#endif //GYVAITV_WEBRTC_REMOTEPEERCONTEXT_H
