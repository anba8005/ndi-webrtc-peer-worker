//
// Created by anba8005 on 6/14/19.
//

#include "StatsObserver.h"
#include "PeerContext.h"
#include <iostream>

StatsObserver::StatsObserver(shared_ptr<Signaling> signaling, int64_t correlation) : signaling(
		signaling), correlation(correlation) {
}

StatsObserver::~StatsObserver() {}

void StatsObserver::OnComplete(const webrtc::StatsReports& reports) {
	json payload = json::array();
	for (const auto &report : reports) {
		const auto values = report->values();
		json r;
		for(auto i = values.begin(); i != values.end(); ++i) {
			const auto name = i->second->display_name();
			const auto value = i->second->ToString();
			r[name] = value;
		}
		payload.push_back(r);
	}
	signaling->replyWithPayload(COMMAND_GET_STATS_OLD, payload, correlation);

}

StatsObserver* StatsObserver::Create(shared_ptr<Signaling> signaling, int64_t correlation) {
	return new rtc::RefCountedObject<StatsObserver>(signaling, correlation);
}