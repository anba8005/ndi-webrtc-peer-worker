//
// Created by anba8005 on 6/14/19.
//

#ifndef NDI_WEBRTC_PEER_WORKER_STATSOBSERVER_H
#define NDI_WEBRTC_PEER_WORKER_STATSOBSERVER_H


#include <api/peerconnectioninterface.h>
#include "Signaling.h"

class StatsObserver : public webrtc::StatsObserver {
public:
	static StatsObserver *Create(shared_ptr<Signaling> signaling, int64_t correlation);

	virtual ~StatsObserver();

	virtual void OnComplete(const webrtc::StatsReports& reports) override;
protected:
	StatsObserver(shared_ptr<Signaling> signaling, int64_t correlation);

private:

	shared_ptr<Signaling> signaling;

	int64_t correlation;
};


#endif //NDI_WEBRTC_PEER_WORKER_STATSOBSERVER_H
