//
// Created by anba8005 on 12/23/18.
//

#include "NDIReader.h"

#include <iostream>

NDIReader::NDIReader() : running(false), _pNDI_recv(nullptr) {}

NDIReader::~NDIReader() {
    if (this->isRunning()) {
        this->stop();
    }
    //
    if (_pNDI_recv) {
        NDIlib_recv_destroy(_pNDI_recv);
    }
}

void NDIReader::open(Configuration config) {
	//
	this->maxWidth = config.maxWidth;
	this->maxHeight = config.maxHeight;

    // Create the descriptor of the object to create
    NDIlib_find_create_t find_create;
    find_create.show_local_sources = true;
    find_create.p_groups = nullptr;
    find_create.p_extra_ips = config.ips.empty() ? nullptr : config.ips.c_str();

    // Create a finder
    NDIlib_find_instance_t pNDI_find = NDIlib_find_create_v2(&find_create);
    if (!pNDI_find)
        throw std::runtime_error("Cannot create NDI finder");


    // Find source
	std::cerr << "Looking for sources ..." << std::endl;
    uint32_t no_sources = 0;
	NDIlib_source_t _ndi_source;
	bool found = false;
	uint32_t cycles = 0;
	while (cycles < 30) { // 6 sec max
		if (NDIlib_find_wait_for_sources(pNDI_find, 200)) {
			// get available sources
			const NDIlib_source_t *p_sources = NDIlib_find_get_current_sources(pNDI_find, &no_sources);
			// find target source
			for (uint32_t i = 0; i < no_sources; i++) {
				if (config.name == std::string(p_sources[i].p_ndi_name)) {
					// found
					_ndi_source = p_sources[i];
					found = true;
					break;
				}
			}
			// proceed if source found
			if (found) {
				break;
			}
		} else {
			// show progress
			if (cycles > 0 && cycles % 5 == 0) {
				std::cerr << "." << std::endl;
			}
		}
		//
		cycles++;
	}

    // check if desired source available
    if (!found) {
        throw std::runtime_error("Source " + config.name + " not found");
    }

    if (!_pNDI_recv) {
        // create NDI receiver descriptor
        NDIlib_recv_create_v3_t recv_create_desc;
        recv_create_desc.source_to_connect_to = _ndi_source;
        recv_create_desc.bandwidth = NDIlib_recv_bandwidth_highest;
        recv_create_desc.color_format = NDIlib_recv_color_format_UYVY_RGBA;
        recv_create_desc.p_ndi_recv_name = "NDIReader";
        recv_create_desc.allow_video_fields = false;

        // create NDI receiver
        _pNDI_recv = NDIlib_recv_create_v3(&recv_create_desc);
        if (!_pNDI_recv)
            throw std::runtime_error("Error creating NDI receiver");
    } else {
        // reconnect receiver
        NDIlib_recv_connect(_pNDI_recv, &_ndi_source);
    }

    // cleanup
    NDIlib_find_destroy(pNDI_find);
}

void NDIReader::start(VideoDeviceModule *vdm, BaseAudioDeviceModule *adm) {
    assert(!running.load());

    //
    this->vdm = vdm;
    this->adm = adm;

    // start worker thread
    running = true;
    runner = std::thread(&NDIReader::run, this);
}

void NDIReader::stop() {
    // stop worker thread
    running = false;
    runner.join();
}

bool NDIReader::isRunning() {
    return running.load();
}

void NDIReader::run() {
    while (running.load()) {
        NDIlib_video_frame_v2_t video_frame;
        NDIlib_audio_frame_v2_t audio_frame;
        NDIlib_metadata_frame_t metadata_frame;
        NDIlib_audio_frame_interleaved_16s_t audio_frame_16bpp_interleaved;

        switch (NDIlib_recv_capture_v2(_pNDI_recv, &video_frame, &audio_frame, &metadata_frame, 1000)) {
            // No data
            case NDIlib_frame_type_none:
                std::cerr << "No data received" << std::endl;
                break;

                // Video data
            case NDIlib_frame_type_video:
                // send
                if (vdm)
                    vdm->feedFrame(video_frame.xres, video_frame.yres, video_frame.p_data,
                                   video_frame.line_stride_in_bytes, video_frame.timestamp, maxWidth, maxHeight);
                // free
                NDIlib_recv_free_video_v2(_pNDI_recv, &video_frame);
                break;

                // Audio data
            case NDIlib_frame_type_audio:
                // Allocate enough space for 16bpp interleaved buffer

                audio_frame_16bpp_interleaved.reference_level = 16;     // We are going to have 16dB of headroom
                audio_frame_16bpp_interleaved.p_data = new int16_t[audio_frame.no_samples * audio_frame.no_channels];
                audio_frame_16bpp_interleaved.sample_rate = audio_frame.sample_rate;
                audio_frame_16bpp_interleaved.no_channels = audio_frame.no_samples;

                // Convert it
                NDIlib_util_audio_to_interleaved_16s_v2(&audio_frame, &audio_frame_16bpp_interleaved);
                // Free the original buffer
                NDIlib_recv_free_audio_v2(_pNDI_recv, &audio_frame);

                // send
                if (adm)
                    adm->feedRecorderData(audio_frame_16bpp_interleaved.p_data,
                                          audio_frame_16bpp_interleaved.no_samples);
                // Free the interleaved audio data
                delete[] audio_frame_16bpp_interleaved.p_data;
                break;

                // Meta data
            case NDIlib_frame_type_metadata:
                NDIlib_recv_free_metadata(_pNDI_recv, &metadata_frame);
                break;

                // There is a status change on the receiver (e.g. new web interface)
            case NDIlib_frame_type_status_change:
                // std::cerr << "Receiver connection status changed." << std::endl;
                break;

                // Everything else
            default:
                break;
        }
    }
}

json NDIReader::findSources() {
	// Create the descriptor of the object to create
	NDIlib_find_create_t find_create;
	find_create.show_local_sources = true;
	find_create.p_groups = nullptr;
	find_create.p_extra_ips = nullptr;

	// Create a finder
	NDIlib_find_instance_t pNDI_find = NDIlib_find_create_v2(&find_create);
	if (!pNDI_find)
		throw std::runtime_error("Cannot create NDI finder");

	// Wait until there is one source
	uint32_t no_sources = 0;
	const NDIlib_source_t *p_sources = nullptr;
	std::cerr << "Looking for sources ..." << std::endl;
	uint32_t cycles = 0;
	while (cycles < 30) { // 6 sec max
		if (NDIlib_find_wait_for_sources(pNDI_find, 200)) {
			p_sources = NDIlib_find_get_current_sources(pNDI_find, &no_sources);
		} else {
			if (no_sources > 0) {
				// break if some sources found and no sources again
				break;
			} else if (cycles > 0 && cycles % 5 == 0) {
				std::cerr << "." << std::endl;
			}
		}
		//
		cycles++;
	}
	std::cerr << "Network sources (" << no_sources << " found)" << std::endl;

	// create result
	json result = json::array();
	for (uint32_t i = 0; i < no_sources; i++) {
		//std::cerr << i + 1 << " " << p_sources[i].p_ndi_name << " " << p_sources[i].p_ip_address << std::endl;
		json source;
		source["name"] = p_sources[i].p_ndi_name;
		source["ip"] = p_sources[i].p_ip_address;
		result.push_back(source);
	}

	// cleanup
	NDIlib_find_destroy(pNDI_find);

	return result;
}

NDIReader::Configuration::Configuration(json payload) {
    this->name = payload.value("name", "");
    if (name.empty())
        throw std::runtime_error("NDI Source name is empty");
    this->ips = payload.value("ips", "");
    this->maxWidth = payload.value("width",0);
    this->maxHeight = payload.value("height",0);


}

