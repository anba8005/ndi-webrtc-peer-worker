//
// Created by anba8005 on 2/6/19.
//

#include "StatsCollectorCallback.h"

#include <iostream>
#include <string>

#include "json.hpp"

#include "PeerContext.h"

using json = nlohmann::json;
using namespace std;

StatsCollectorCallback::StatsCollectorCallback(shared_ptr<Signaling> signaling, int64_t correlation) : signaling(
        signaling), correlation(correlation) {
}

StatsCollectorCallback::~StatsCollectorCallback() {
}

void StatsCollectorCallback::OnStatsDelivered(const rtc::scoped_refptr<const webrtc::RTCStatsReport> &report) {
    nlohmann::json payload = json::parse(report->ToJson());
    signaling->replyWithPayload(COMMAND_GET_STATS, payload, correlation);
}

StatsCollectorCallback* StatsCollectorCallback::Create(shared_ptr<Signaling> signaling, int64_t correlation) {
    return new rtc::RefCountedObject<StatsCollectorCallback>(signaling, correlation);
}

