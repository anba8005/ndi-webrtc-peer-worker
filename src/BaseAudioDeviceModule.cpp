//
// Created by anba8005 on 12/22/18.
//

#include "BaseAudioDeviceModule.h"

#include "rtc_base/ref_counted_object.h"
#include "rtc_base/thread.h"
#include "rtc_base/time_utils.h"

#include <iostream>

using namespace std;


// Constants correspond to 10ms of sterio audio at 44kHz.
static const uint8_t kNumberOfChannels = 2;
static const int kSamplesPerSecond = 48000;
static const size_t kNumberSamples = 480;
static const size_t kBytesPerSample = sizeof(BaseAudioDeviceModule::Sample) * kNumberOfChannels;
static const size_t kBufferBytes = kNumberSamples * kBytesPerSample;

static const int kTimePerFrameMs = 10;
static const int kTotalDelayMs = 0;
static const int kClockDriftMs = 0;
static const uint32_t kMaxVolume = 14392;

enum {
    MSG_START_PROCESS,
    MSG_RUN_PROCESS
};


BaseAudioDeviceModule::BaseAudioDeviceModule()
        : _lastProcessTimeMS(0), _audioCallback(nullptr), _recording(false), _playing(false), _playIsInitialized(false),
          _recIsInitialized(false), _currentMicLevel(kMaxVolume), _started(false), _nextFrameTime(0),
          _sendSamples(kBufferBytes), _recvSamples(kBufferBytes) {
}

BaseAudioDeviceModule::~BaseAudioDeviceModule() {
    if (_processThread) {
        _processThread->Stop();
    }
}

rtc::scoped_refptr<BaseAudioDeviceModule> BaseAudioDeviceModule::Create() {
    rtc::scoped_refptr<BaseAudioDeviceModule> capture_module(
            new rtc::RefCountedObject<BaseAudioDeviceModule>());
    if (!capture_module->Initialize()) {
        return nullptr;
    }
    return capture_module;
}

//
void BaseAudioDeviceModule::feedRecorderData(const void *data, int numSamples) {
    {
        rtc::CritScope cs1(&_crit);
        if (!_recording)
            return;
    }

    rtc::CritScope cs(&_critCallback);

    if (!_audioCallback) {
        return;
    }
    _sendFifo.write((void **) &data, numSamples);
}

void BaseAudioDeviceModule::OnMessage(rtc::Message *msg) {
    switch (msg->message_id) {
        case MSG_START_PROCESS:
            startProcessP();
            break;
        case MSG_RUN_PROCESS:
            processFrameP();
            break;
        default:
            // All existing messages should be caught.
            // Getting here should never happen.
            assert(false);
    }
}

bool BaseAudioDeviceModule::Initialize() {
    // Allocate the send audio FIFO buffer
    _sendFifo.close();
    _sendFifo.alloc("s16", kNumberOfChannels);

    _lastProcessTimeMS = rtc::TimeMillis();
    return true;
}

bool BaseAudioDeviceModule::shouldStartProcessing() {
    return _recording || _playing;
}

void BaseAudioDeviceModule::updateProcessing(bool start) {
    if (start) {
        if (!_processThread) {
            _processThread = rtc::Thread::Create();
            _processThread->Start();
        }
        _processThread->Post(RTC_FROM_HERE, this, MSG_START_PROCESS);
    } else {
        if (_processThread) {
            _processThread->Stop();
            _processThread.reset(nullptr);
        }
        _started = false;
    }
}

void BaseAudioDeviceModule::startProcessP() {
    assert(_processThread->IsCurrent());
    if (_started) {
        // Already started.
        return;
    }
    processFrameP();
}

void BaseAudioDeviceModule::processFrameP() {
    assert(_processThread->IsCurrent());
    if (!_started) {
        _nextFrameTime = rtc::TimeMillis();
        _started = true;
    }

    {
        rtc::CritScope cs(&_crit);
        // Receive and send frames every kTimePerFrameMs.
        if (_playing) {
            receiveFrameP();
        }
        if (_recording) {
            sendFrameP();
        }
    }

    _nextFrameTime += kTimePerFrameMs;
    const int64_t current_time = rtc::TimeMillis();
    const int64_t wait_time = (_nextFrameTime > current_time) ? _nextFrameTime - current_time : 0;
    _processThread->PostDelayed(RTC_FROM_HERE, wait_time, this, MSG_RUN_PROCESS);
}


void BaseAudioDeviceModule::sendFrameP() {
    assert(_processThread->IsCurrent());
    rtc::CritScope cs(&_critCallback);
    if (!_audioCallback) {
        return;
    }

    auto samples = &_sendSamples[0];
    if (!_sendFifo.read((void **) &samples, kNumberSamples)) {
        return;
    }

    bool key_pressed = false;
    uint32_t current_mic_level = 0;
    MicrophoneVolume(&current_mic_level);

    if (_audioCallback->RecordedDataIsAvailable(
            samples, kNumberSamples, kBytesPerSample, kNumberOfChannels,
            kSamplesPerSecond, kTotalDelayMs, kClockDriftMs, current_mic_level,
            key_pressed, current_mic_level) != 0) {
        assert(false);
    }

    SetMicrophoneVolume(current_mic_level);
}

void BaseAudioDeviceModule::receiveFrameP() {
    assert(_processThread->IsCurrent());
    {
        rtc::CritScope cs(&_critCallback);
        if (!_audioCallback) {
            return;
        }
        size_t nSamplesOut = 0;
        int64_t elapsed_time_ms = 0;
        int64_t ntp_time_ms = 0;
        auto samples = &_recvSamples[0];
        if (_audioCallback->NeedMorePlayData(kNumberSamples,
                                             kBytesPerSample,
                                             kNumberOfChannels,
                                             kSamplesPerSecond,
                                             (void *) samples, nSamplesOut,
                                             &elapsed_time_ms, &ntp_time_ms) != 0) {
            assert(false);
        }
        if (nSamplesOut != kNumberSamples * kNumberOfChannels) {
            std::cerr << nSamplesOut << std::endl;
        }
    }
}

int32_t BaseAudioDeviceModule::ActiveAudioLayer(AudioLayer * /*audio_layer*/) const {
    assert(false);
    return 0;
}

int32_t BaseAudioDeviceModule::RegisterAudioCallback(webrtc::AudioTransport *audio_callback) {
    rtc::CritScope cs(&_critCallback);
    _audioCallback = audio_callback;
    return 0;
}

int32_t BaseAudioDeviceModule::Init() {
    // Initialize is called by the factory method.
    // Safe to ignore this Init call.
    return 0;
}

int32_t BaseAudioDeviceModule::Terminate() {
    // Clean up in the destructor. No action here, just success.
    return 0;
}

bool BaseAudioDeviceModule::Initialized() const {
    return 0;
}

int16_t BaseAudioDeviceModule::PlayoutDevices() {
    return 0;
}

int16_t BaseAudioDeviceModule::RecordingDevices() {
    return 0;
}

int32_t BaseAudioDeviceModule::PlayoutDeviceName(uint16_t /*index*/,
                                                 char /*name*/[webrtc::kAdmMaxDeviceNameSize],
                                                 char /*guid*/[webrtc::kAdmMaxGuidSize]) {
    return 0;
}

int32_t BaseAudioDeviceModule::RecordingDeviceName(uint16_t /*index*/,
                                                   char /*name*/[webrtc::kAdmMaxDeviceNameSize],
                                                   char /*guid*/[webrtc::kAdmMaxGuidSize]) {
    return 0;
}

int32_t BaseAudioDeviceModule::SetPlayoutDevice(uint16_t /*index*/) {
    // No playout device, just playing from file. Return success.
    return 0;
}

int32_t BaseAudioDeviceModule::SetPlayoutDevice(WindowsDeviceType /*device*/) {
    if (_playIsInitialized) {
        return -1;
    }
    return 0;
}

int32_t BaseAudioDeviceModule::SetRecordingDevice(uint16_t /*index*/) {
    // No recording device, just dropping audio. Return success.
    return 0;
}

int32_t BaseAudioDeviceModule::SetRecordingDevice(WindowsDeviceType /*device*/) {
    if (_recIsInitialized) {
        return -1;
    }
    return 0;
}

int32_t BaseAudioDeviceModule::PlayoutIsAvailable(bool * /*available*/) {
    return 0;
}

int32_t BaseAudioDeviceModule::InitPlayout() {
    _playIsInitialized = true;
    return 0;
}

bool BaseAudioDeviceModule::PlayoutIsInitialized() const {
    return _playIsInitialized;
}

int32_t BaseAudioDeviceModule::RecordingIsAvailable(bool * /*available*/) {
    return 0;
}

int32_t BaseAudioDeviceModule::InitRecording() {
    _recIsInitialized = true;
    return 0;
}

bool BaseAudioDeviceModule::RecordingIsInitialized() const {
    return _recIsInitialized;
}

int32_t BaseAudioDeviceModule::StartPlayout() {
    if (!_playIsInitialized) {
        return -1;
    }
    {
        rtc::CritScope cs(&_crit);
        _playing = true;
    }
    bool start = true;
    updateProcessing(start);
    return 0;
}

int32_t BaseAudioDeviceModule::StopPlayout() {
    bool start = false;
    {
        rtc::CritScope cs(&_crit);
        _playing = false;
        start = shouldStartProcessing();
    }
    updateProcessing(start);
    return 0;
}

bool BaseAudioDeviceModule::Playing() const {
    rtc::CritScope cs(&_crit);
    return _playing;
}

int32_t BaseAudioDeviceModule::StartRecording() {
    if (!_recIsInitialized) {
        return -1;
    }
    {
        rtc::CritScope cs(&_crit);
        _recording = true;
    }
    bool start = true;
    updateProcessing(start);
    return 0;
}

int32_t BaseAudioDeviceModule::StopRecording() {
    bool start = false;
    {
        rtc::CritScope cs(&_crit);
        _recording = false;
        start = shouldStartProcessing();
    }
    updateProcessing(start);
    return 0;
}

bool BaseAudioDeviceModule::Recording() const {
    rtc::CritScope cs(&_crit);
    return _recording;
}

int32_t BaseAudioDeviceModule::InitSpeaker() {
    // No speaker, just playing from file. Return success.
    return 0;
}

bool BaseAudioDeviceModule::SpeakerIsInitialized() const {
    return 0;
}

int32_t BaseAudioDeviceModule::InitMicrophone() {
    // No microphone, just playing from file. Return success.
    return 0;
}

bool BaseAudioDeviceModule::MicrophoneIsInitialized() const {
    return 0;
}

int32_t BaseAudioDeviceModule::SpeakerVolumeIsAvailable(bool * /*available*/) {
    return 0;
}

int32_t BaseAudioDeviceModule::SetSpeakerVolume(uint32_t /*volume*/) {
    return 0;
}

int32_t BaseAudioDeviceModule::SpeakerVolume(uint32_t * /*volume*/) const {
    return 0;
}

int32_t BaseAudioDeviceModule::MaxSpeakerVolume(uint32_t * /*max_volume*/) const {
    return 0;
}

int32_t BaseAudioDeviceModule::MinSpeakerVolume(uint32_t * /*min_volume*/) const {
    return 0;
}


int32_t BaseAudioDeviceModule::MicrophoneVolumeIsAvailable(bool * /*available*/) {
    return 0;
}

int32_t BaseAudioDeviceModule::SetMicrophoneVolume(uint32_t volume) {
    rtc::CritScope cs(&_crit);
    _currentMicLevel = volume;
    return 0;
}

int32_t BaseAudioDeviceModule::MicrophoneVolume(uint32_t *volume) const {
    rtc::CritScope cs(&_crit);
    *volume = _currentMicLevel;
    return 0;
}

int32_t BaseAudioDeviceModule::MaxMicrophoneVolume(uint32_t *max_volume) const {
    *max_volume = kMaxVolume;
    return 0;
}

int32_t BaseAudioDeviceModule::MinMicrophoneVolume(uint32_t * /*min_volume*/) const {
    assert(false);
    return 0;
}

int32_t BaseAudioDeviceModule::SpeakerMuteIsAvailable(bool * /*available*/) {
    return 0;
}

int32_t BaseAudioDeviceModule::SetSpeakerMute(bool /*enable*/) {
    return 0;
}

int32_t BaseAudioDeviceModule::SpeakerMute(bool * /*enabled*/) const {
    return 0;
}

int32_t BaseAudioDeviceModule::MicrophoneMuteIsAvailable(bool * /*available*/) {
    return 0;
}

int32_t BaseAudioDeviceModule::SetMicrophoneMute(bool /*enable*/) {
    return 0;
}

int32_t BaseAudioDeviceModule::MicrophoneMute(bool * /*enabled*/) const {
    return 0;
}

int32_t BaseAudioDeviceModule::StereoPlayoutIsAvailable(bool *available) const {
    // No recording device, just dropping audio. Stereo can be dropped just
    // as easily as mono.
    *available = true;
    return 0;
}

int32_t BaseAudioDeviceModule::SetStereoPlayout(bool /*enable*/) {
    // No recording device, just dropping audio. Stereo can be dropped just
    // as easily as mono.
    return 0;
}

int32_t BaseAudioDeviceModule::StereoPlayout(bool * /*enabled*/) const {
    return 0;
}

int32_t BaseAudioDeviceModule::StereoRecordingIsAvailable(bool *available) const {
    // Keep thing simple. No stereo recording.
    *available = false;
    return 0;
}

int32_t BaseAudioDeviceModule::SetStereoRecording(bool enable) {
    if (!enable) {
        return 0;
    }
    return -1;
}

int32_t BaseAudioDeviceModule::StereoRecording(bool * /*enabled*/) const {
    return 0;
}

int32_t BaseAudioDeviceModule::PlayoutDelay(uint16_t *delay_ms) const {
    // No delay since audio frames are dropped.
    *delay_ms = 0;
    return 0;
}

