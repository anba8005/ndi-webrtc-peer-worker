//
// Created by anba8005 on 12/20/18.
//

#ifndef GYVAITV_WEBRTC_SETSESSIONDESCRIPTIONOBSERVER_H
#define GYVAITV_WEBRTC_SETSESSIONDESCRIPTIONOBSERVER_H

#include <api/jsep.h>
#include <rtc_base/refcountedobject.h>

class SetSessionDescriptionObserver : public webrtc::SetSessionDescriptionObserver {
public:
    static SetSessionDescriptionObserver *Create() {
        return new rtc::RefCountedObject<SetSessionDescriptionObserver>();
    }

    virtual void OnSuccess() override;

    virtual void OnFailure(const std::string &error) override;

protected:
    SetSessionDescriptionObserver() = default;

    ~SetSessionDescriptionObserver() = default;
};

#endif //GYVAITV_WEBRTC_SETSESSIONDESCRIPTIONOBSERVER_H
