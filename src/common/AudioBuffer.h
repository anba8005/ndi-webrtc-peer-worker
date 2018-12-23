//
// Created by anba8005 on 12/22/18.
//

#ifndef GYVAITV_WEBRTC_AUDIOBUFFER_H
#define GYVAITV_WEBRTC_AUDIOBUFFER_H

#include <string>

extern "C" {
#include <libavutil/audio_fifo.h>
}

class AudioBuffer {
public:
    AudioBuffer();
    ~AudioBuffer();

    void alloc(const std::string& sampleFmt, int channels, int numSamples = 1024);
    void reset();
    void close();

    void write(void** samples, int numSamples);
    bool read(void** samples, int numSamples);

    int available() const;
private:
    AVAudioFifo* fifo;
};


#endif //GYVAITV_WEBRTC_AUDIOBUFFER_H
