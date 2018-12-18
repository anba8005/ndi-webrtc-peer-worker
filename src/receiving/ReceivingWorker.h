//
// Created by anba8005 on 12/15/18.
//

#ifndef GYVAITV_WEBRTC_RECEIVINGWORKER_H
#define GYVAITV_WEBRTC_RECEIVINGWORKER_H

#include "../common/AWorker.h"

class ReceivingWorker : public AWorker {
public:
    ReceivingWorker();

    virtual ~ReceivingWorker();

protected:
    void onStart() override;

    void onEnd() override;

    void onCommand(std::vector<std::string> command) override;
};


#endif //GYVAITV_WEBRTC_RECEIVINGWORKER_H
