//
// Created by anba8005 on 03/03/2020.
//

#include "ThrottledReporter.h"
#include <iostream>
#include <chrono>

ThrottledReporter::ThrottledReporter(std::string title, unsigned int timeout) {
    title_ = title;
    timeout_ = timeout;
    total_reports_ = 0;
    //
    running_ = true;
    runner_ = std::thread(&ThrottledReporter::run, this);
}

ThrottledReporter::~ThrottledReporter() {
    if (running_.load()) {
        running_ = false;
        cv_.notify_all();
        runner_.join();
    }

}

void ThrottledReporter::report() {
    total_reports_++;
}

void ThrottledReporter::run() {
    while (running_.load()) {
        if (total_reports_ > 0) {
            std::cerr << title_ << total_reports_ << std::endl;
            total_reports_ = 0;
        }
        std::unique_lock<std::mutex> lk(m_);
        cv_.wait_for(lk, std::chrono::seconds(timeout_));
    }

}


