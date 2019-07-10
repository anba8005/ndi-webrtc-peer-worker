//
// Created by anba8005 on 2/6/19.
//

#ifndef GYVAITV_WEBRTC_STATSCOLLECTORCALLBACK_H
#define GYVAITV_WEBRTC_STATSCOLLECTORCALLBACK_H

#include <api/stats/rtc_stats_collector_callback.h>
#include "Signaling.h"


class StatsCollectorCallback : public webrtc::RTCStatsCollectorCallback {
public:
    static StatsCollectorCallback* Create(shared_ptr<Signaling> signaling, int64_t correlation);

    virtual ~StatsCollectorCallback();

    void OnStatsDelivered(const rtc::scoped_refptr<const webrtc::RTCStatsReport> &report) override;

protected:
    StatsCollectorCallback(shared_ptr<Signaling> signaling, int64_t correlation);

private:

    shared_ptr<Signaling> signaling;

    int64_t correlation;
};


#endif //GYVAITV_WEBRTC_STATSCOLLECTORCALLBACK_H
