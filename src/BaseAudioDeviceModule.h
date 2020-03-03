//
// Created by anba8005 on 12/22/18.
//

#ifndef GYVAITV_WEBRTC_BASEAUDIODEVICEMODULE_H
#define GYVAITV_WEBRTC_BASEAUDIODEVICEMODULE_H

#ifdef WIN32
#define NOMINMAX
#undef min
#undef max
#endif

#include "rtc_base/critical_section.h"
#include "rtc_base/message_handler.h"
#include "api/scoped_refptr.h"
#include "rtc_base/thread.h"
#include "common_types.h"
#include "modules/audio_device/include/audio_device.h"
#include "AudioBuffer.h"
#include "ThrottledReporter.h"

/// This class implements an `AudioDeviceModule` that can be used to process
/// arbitrary audio packets.
///
/// Note P postfix of a function indicates that it should only be called by the
/// processing thread.
class BaseAudioDeviceModule : public webrtc::AudioDeviceModule,
                          public rtc::MessageHandler {
public:
    typedef uint16_t Sample;

    /// Creates a `BaseAudioDeviceModule` or returns NULL on failure.
    static rtc::scoped_refptr<BaseAudioDeviceModule> Create();

    /// Handles input packets from the capture for sending.
    void feedRecorderData(const void* data, int numSamples);

    int32_t ActiveAudioLayer(AudioLayer *audio_layer) const override;

    int32_t RegisterAudioCallback(webrtc::AudioTransport *audio_callback) override;

    int32_t Init() override;

    int32_t Terminate() override;

    bool Initialized() const override;

    int16_t PlayoutDevices() override;

    int16_t RecordingDevices() override;

    int32_t PlayoutDeviceName(uint16_t index,
                              char name[webrtc::kAdmMaxDeviceNameSize],
                              char guid[webrtc::kAdmMaxGuidSize]) override;

    int32_t RecordingDeviceName(uint16_t index,
                                char name[webrtc::kAdmMaxDeviceNameSize],
                                char guid[webrtc::kAdmMaxGuidSize]) override;

    int32_t SetPlayoutDevice(uint16_t index) override;

    int32_t SetPlayoutDevice(WindowsDeviceType device) override;

    int32_t SetRecordingDevice(uint16_t index) override;

    int32_t SetRecordingDevice(WindowsDeviceType device) override;

    int32_t PlayoutIsAvailable(bool *available) override;

    int32_t InitPlayout() override;

    bool PlayoutIsInitialized() const override;

    int32_t RecordingIsAvailable(bool *available) override;

    int32_t InitRecording() override;

    bool RecordingIsInitialized() const override;

    int32_t StartPlayout() override;

    int32_t StopPlayout() override;

    bool Playing() const override;

    int32_t StartRecording() override;

    int32_t StopRecording() override;

    bool Recording() const override;

    int32_t InitSpeaker() override;

    bool SpeakerIsInitialized() const override;

    int32_t InitMicrophone() override;

    bool MicrophoneIsInitialized() const override;

    int32_t SpeakerVolumeIsAvailable(bool *available) override;

    int32_t SetSpeakerVolume(uint32_t volume) override;

    int32_t SpeakerVolume(uint32_t *volume) const override;

    int32_t MaxSpeakerVolume(uint32_t *max_volume) const override;

    int32_t MinSpeakerVolume(uint32_t *min_volume) const override;

    int32_t MicrophoneVolumeIsAvailable(bool *available) override;

    int32_t SetMicrophoneVolume(uint32_t volume) override;

    int32_t MicrophoneVolume(uint32_t *volume) const override;

    int32_t MaxMicrophoneVolume(uint32_t *max_volume) const override;

    int32_t MinMicrophoneVolume(uint32_t *min_volume) const override;

    int32_t SpeakerMuteIsAvailable(bool *available) override;

    int32_t SetSpeakerMute(bool enable) override;

    int32_t SpeakerMute(bool *enabled) const override;

    int32_t MicrophoneMuteIsAvailable(bool *available) override;

    int32_t SetMicrophoneMute(bool enable) override;

    int32_t MicrophoneMute(bool *enabled) const override;

    int32_t StereoPlayoutIsAvailable(bool *available) const override;

    int32_t SetStereoPlayout(bool enable) override;

    int32_t StereoPlayout(bool *enabled) const override;

    int32_t StereoRecordingIsAvailable(bool *available) const override;

    int32_t SetStereoRecording(bool enable) override;

    int32_t StereoRecording(bool *enabled) const override;

    int32_t PlayoutDelay(uint16_t *delay_ms) const override;

    bool BuiltInAECIsAvailable() const override { return false; }

    int32_t EnableBuiltInAEC(bool enable) override { return -1; }

    bool BuiltInAGCIsAvailable() const override { return false; }

    int32_t EnableBuiltInAGC(bool enable) override { return -1; }

    bool BuiltInNSIsAvailable() const override { return false; }

    int32_t EnableBuiltInNS(bool enable) override { return -1; }

    //

    void OnMessage(rtc::Message *msg) override;

protected:
    explicit BaseAudioDeviceModule();
    virtual ~BaseAudioDeviceModule();

private:
    /// Initializes the state of the `BaseAudioDeviceModule`. This API is called on
    /// creation by the `Create()` API.
    bool Initialize();

    /// Returns true/false depending on if recording or playback has been
    /// enabled/started.
    bool shouldStartProcessing();

    /// Starts or stops the pushing and pulling of audio frames.
    void updateProcessing(bool start);

    /// Starts the periodic calling of `ProcessFrame()` in a thread safe way.
    void startProcessP();

    /// Periodcally called function that ensures that frames are pulled and
    /// pushed
    /// periodically if enabled/started.
    void processFrameP();

    /// Pulls frames from the registered webrtc::AudioTransport.
    void receiveFrameP();

    /// Pushes frames to the registered webrtc::AudioTransport.
    void sendFrameP();

    /// The time in milliseconds when Process() was last called or 0 if no call
    /// has been made.
    int64_t _lastProcessTimeMS;

    /// Callback for playout and recording.
    webrtc::AudioTransport *_audioCallback;

    bool _recording; ///< True when audio is being pushed from the instance.
    bool _playing;   ///< True when audio is being pulled by the instance.

    bool _playIsInitialized; ///< True when the instance is ready to pull audio.
    bool _recIsInitialized;  ///< True when the instance is ready to push audio.

    /// Input to and output from RecordedDataIsAvailable(..) makes it possible
    /// to modify the current mic level. The implementation does not care about
    /// the mic level so it just feeds back what it receives.
    uint32_t _currentMicLevel;

    /// `_nextFrameTime` is updated in a non-drifting manner to indicate the
    /// next wall clock time the next frame should be generated and received.
    /// `_started` ensures that _nextFrameTime can be initialized properly on first call.
    bool _started;
    int64_t _nextFrameTime;

    std::unique_ptr<rtc::Thread> _processThread;

    /// A FIFO buffer that stores samples from the audio source to be sent.
    AudioBuffer _sendFifo;

    /// A buffer with enough storage for a 10ms of samples to send.
    std::vector<Sample> _sendSamples;
    std::vector<Sample> _recvSamples;

    /// Protects variables that are accessed from `_processThread` and
    /// the main thread.
    rtc::CriticalSection _crit;

    /// Protects |_audioCallback| that is accessed from `_processThread` and
    /// the main thread.
    rtc::CriticalSection _critCallback;

    ThrottledReporter _sample_mismatch_reporter;
};


#endif //GYVAITV_WEBRTC_BASEAUDIODEVICEMODULE_H
