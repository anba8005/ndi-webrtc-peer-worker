//
// Created by anba8005 on 02/03/2020.
//

#ifndef NDI_WEBRTC_PEER_WORKER_FRAMERATEUPDATER_H
#define NDI_WEBRTC_PEER_WORKER_FRAMERATEUPDATER_H

class FrameRateUpdater {
public:
    virtual void updateFrameRate(int num, int den) = 0;
};

#endif //NDI_WEBRTC_PEER_WORKER_FRAMERATEUPDATER_H
