#include <soundProviderOgg.h>
#include <iostream>
#include <cmath>

namespace SoundOgg {
	void SoundProviderOgg::checkErr(int *errp) { // Check for errors
		enum STBVorbisError e;
		if (errp == nullptr) { 
			e = static_cast<STBVorbisError>(stb_vorbis_get_error(vorbis_state));
		} else {
			e = static_cast<STBVorbisError>(*errp);
		}
		switch(e) {
			case VORBIS__no_error: break; // No error
			// case : throw OggExceptions::(); break;
			case VORBIS_need_more_data: throw OggExceptions::NeedMoreData(); break;
			case VORBIS_invalid_api_mixing: throw OggExceptions::InvalidApiMixing(); break;
			case VORBIS_outofmem: throw OggExceptions::OutOfMem(); break;
			case VORBIS_feature_not_supported: throw OggExceptions::FeatureNotSupported(); break;
			case VORBIS_too_many_channels: throw OggExceptions::TooManyChannels(); break;
			case VORBIS_file_open_failure: throw OggExceptions::FileOpenFailure(); break;
			case VORBIS_seek_without_length: throw OggExceptions::SeekWithoutLength(); break;
			case VORBIS_unexpected_eof: throw OggExceptions::UnexpectedEOF(); break;
			case VORBIS_seek_invalid: throw OggExceptions::SeekInvalid(); break;
			case VORBIS_invalid_setup: throw OggExceptions::InvalidSetup(); break;
			case VORBIS_invalid_stream: throw OggExceptions::InvalidStream(); break;
			case VORBIS_missing_capture_pattern: throw OggExceptions::MissingCapturePattern(); break;
			case VORBIS_invalid_stream_structure_version: throw OggExceptions::InvalidStreamStructureVersion(); break;
			case VORBIS_continued_packet_flag_invalid: throw OggExceptions::ContinuedPacketFlagInvalid(); break;
			case VORBIS_incorrect_stream_serial_number: throw OggExceptions::IncorrectStreamSerialNumber(); break;
			case VORBIS_invalid_first_page: throw OggExceptions::InvalidFirstPage(); break;
			case VORBIS_bad_packet_type: throw OggExceptions::BadPacketType(); break;
			case VORBIS_cant_find_last_page: throw OggExceptions::CantFindLastPage(); break;
			case VORBIS_seek_failed: throw OggExceptions::SeekFailed(); break;
		};
	};

	void SoundProviderOgg::setup(unsigned int ch_arg) {
		stb_vorbis_info info = stb_vorbis_get_info(vorbis_state);
		assert(ch_arg <= info.channels); // We have requested channel

		ch = ch_arg;
		frequency = info.sample_rate;
	}

	SoundProviderOgg::SoundProviderOgg(const unsigned char *data, int len, unsigned int ch_arg) {
		int err;
		vorbis_state = stb_vorbis_open_memory(data, len, &err, nullptr);
		checkErr(&err);
		//setup(ch_arg);
	}

	#if CONFIG_OGG_SND_STDIO
		SoundProviderOgg::SoundProviderOgg(const char *file, unsigned int ch_arg) {
			int err;
			vorbis_state = stb_vorbis_open_filename(file, &err, nullptr);
			checkErr(&err);
			setup(ch_arg);
		}

		SoundProviderOgg::SoundProviderOgg(FILE *f, unsigned int ch_arg) {
			int err;
			vorbis_state = stb_vorbis_open_file(f, false, &err, nullptr);
			checkErr(&err);
			setup(ch_arg);
		}

		SoundProviderOgg::SoundProviderOgg(FILE *f, unsigned int len, unsigned int ch_arg) {
			int err;
			vorbis_state = stb_vorbis_open_file_section(f, false, &err, nullptr, len);
			checkErr(&err);
			setup(ch_arg);
		}
	#endif

	unsigned long int SoundProviderOgg::getFrequency() {
		return frequency;
	};

	SoundProviderOgg::~SoundProviderOgg() {
		provider_stop();
		stb_vorbis_close(vorbis_state);
	}

	void SoundProviderOgg::task_prestop() {
		//safeState.lock();
	}

	void SoundProviderOgg::task_poststop() {
		//safeState.unlock();
	}

	void SoundProviderOgg::task_code() {
		/*
		//std::cout << "Ogg provider begin" << std::endl;
		//std::unique_lock<std::mutex> lock(safeState);
		std::cout << "Ogg provider begin 2" << std::endl;
		stb_vorbis_seek_start(vorbis_state);
		try {
			checkErr();
		} catch(const OggException& e) {
			std::cout << "Ogg provider failure: " << e.what() << std::endl;
			//lock.unlock(); // In very rare cases removing it will cause blocking
			postControl(FAILURE);
			stopFromTask();
		}
		//lock.unlock();

		while(true) {
			float **buf;
			//lock.lock();
			int samplecnt = stb_vorbis_get_frame_float(vorbis_state, nullptr, &buf);
			//lock.unlock();
			if (samplecnt == 0) {break;};
			for (int i = 0; i < samplecnt; i++) {
				if (seeked) break;
				postSample(std::lround(255.f*buf[ch][i]));
			}
		}
		*/
		vTaskDelay(1000);
	}

	void SoundProviderOgg::seek(unsigned int sample_number) {
		//std::unique_lock<std::mutex> lock(safeState);
		stb_vorbis_seek(vorbis_state, sample_number);
		checkErr();
		seeked = true;
		queueReset();
	};

	void SoundProviderOgg::seek_frame(unsigned int sample_number) {
		//std::unique_lock<std::mutex> lock(safeState);
		stb_vorbis_seek_frame(vorbis_state, sample_number);
		checkErr();
		seeked = true;
		queueReset();
	};

	void SoundProviderOgg::seek_start() {
		//std::unique_lock<std::mutex> lock(safeState);
		stb_vorbis_seek_start(vorbis_state);
		checkErr();
		seeked = true;
		queueReset();
	};

	void SoundProviderOgg::seek_second(float time) {
		seek(static_cast<unsigned int>(static_cast<float>(getFrequency()) * time));
	};

	unsigned int SoundProviderOgg::length_samples() {
		//std::unique_lock<std::mutex> lock(safeState);
		unsigned int ret = stb_vorbis_stream_length_in_samples(vorbis_state);
		checkErr();
		return ret;
	}

	float SoundProviderOgg::length_seconds() {
		//std::unique_lock<std::mutex> lock(safeState);
		float ret = stb_vorbis_stream_length_in_seconds(vorbis_state);
		checkErr();
		return ret;
	}

	unsigned long int SoundProviderOgg::samplesPerSecond() {return getFrequency();};
};
