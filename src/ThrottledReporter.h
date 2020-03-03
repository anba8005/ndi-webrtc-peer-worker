//
// Created by anba8005 on 03/03/2020.
//

#ifndef NDI_WEBRTC_PEER_WORKER_THROTTLEDREPORTER_H
#define NDI_WEBRTC_PEER_WORKER_THROTTLEDREPORTER_H

#include <time.h>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

class ThrottledReporter {
public:
    ThrottledReporter(std::string title, unsigned int timeout = 5);

    virtual ~ThrottledReporter();

    void report();
private:
    std::string title_;
    unsigned int timeout_;

    std::atomic<int> total_reports_;

    std::thread runner_;
    std::atomic<bool> running_;
    std::mutex m_;
    std::condition_variable cv_;

    void run();
};


#endif //NDI_WEBRTC_PEER_WORKER_THROTTLEDREPORTER_H
