//
// Created by anba8005 on 26/02/2020.
//

#ifndef NDI_WEBRTC_PEER_WORKER_CODECUTILS_H
#define NDI_WEBRTC_PEER_WORKER_CODECUTILS_H

#define OWT_ENABLE_H265

#include <vector>
#include "api/video_codecs/sdp_video_format.h"
#include "api/video/video_codec_type.h"
#include "media/base/h264_profile_level_id.h"

class CodecUtils {
public:

    static webrtc::SdpVideoFormat
    CreateH264Format(webrtc::H264::Profile profile, webrtc::H264::Level level, const std::string &packetization_mode);

    static std::vector<webrtc::SdpVideoFormat> GetAuxH264Codecs();

    static std::vector<webrtc::SdpVideoFormat> GetSupportedH265Codecs();

    static webrtc::VideoCodecType ConvertSdpFormatToCodecType(webrtc::SdpVideoFormat format);
};


#endif //NDI_WEBRTC_PEER_WORKER_CODECUTILS_H
