#include <soundProviderOgg.h>
#include <iostream>
#include <cmath>
#include <cstring>
#include <memory>
#include <limits.h>
extern "C" {
#include <stdio.h>
}

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

namespace SoundOgg {
	void SoundProviderOgg::checkErr(int e) { // TODO: Check for errors
		switch(e) {
			case 0: break; // No error
			default: throw OggExceptions::Unknown(); break;
		};
	};

	void SoundProviderOgg::setup(unsigned int ch_arg) {
		vorbis_info* info = ov_info(&vorbis_file, -1);
		assert(ch_arg > 0);
		assert(ch_arg <= info->channels); // We have requested channel (count from one)

		ch = ch_arg;
		chTotal = info->channels;
		frequency = info->rate;

		stackSize = CONFIG_OGG_STACK_SIZE;
	}

	void SoundProviderOgg::open_file(FILE *f, unsigned int ch_arg, char *initial, long ibytes) {
		checkErr(ov_open(f, &vorbis_file, initial, ibytes));
		setup(ch_arg);
	}

	void SoundProviderOgg::open_callbacks(void *datasource, const ov_callbacks& callbacks, unsigned int ch_arg, char *initial, long ibytes) {
		checkErr(ov_open_callbacks(datasource, &vorbis_file, initial, ibytes, callbacks));
		setup(ch_arg);
	}

	SoundProviderOgg::SoundProviderOgg(FILE *f, unsigned int ch_arg, char *initial, long ibytes) {
		open_file(f, ch_arg, initial, ibytes);
	}

	SoundProviderOgg::SoundProviderOgg(void *datasource, const ov_callbacks& callbacks, unsigned int ch_arg, char *initial, long ibytes) {
		open_callbacks(datasource, callbacks, ch_arg, initial, ibytes);
	}

	SoundProviderOgg::SoundProviderOgg(const unsigned char *data, int len, unsigned int ch_arg, char *initial, long ibytes) {
		FILE* memfile = fmemopen(const_cast<unsigned char*>(data), len, "r");
		if (unlikely(memfile == nullptr)) {
			std::system_error(errno);
		}
		open_file(memfile, ch_arg, initial, ibytes);
	}

	SoundProviderOgg::SoundProviderOgg(const char* file, unsigned int ch_arg, char *initial, long ibytes) {
		FILE* filed = fopen(file, "r");
		if (unlikely(filed == nullptr)) {
			std::system_error(errno);
		}
		open_file(filed, ch_arg, initial, ibytes);
	}

	SoundProviderOgg::~SoundProviderOgg() {
		std::unique_lock<std::mutex> lock(safeState);
		ov_clear(&vorbis_file);
	}

	void SoundProviderOgg::task_prestop() {
		safeState.lock();
	}

	void SoundProviderOgg::task_poststop() {
		safeState.unlock();
	}

	void SoundProviderOgg::provider_restart() {
		if (taskHandle != nullptr) {
			task_prestop();
			vTaskDelete(taskHandle);
			task_poststop();
		}
		try {
			seekPcm(0);
		} catch(OggExceptions::NotSeekable) {}; // It is probably ok.
		unconditionalStart();
	}

	void SoundProviderOgg::checkSeekable() {
		if (not seekable()) { throw OggExceptions::NotSeekable(); }
	}

	void SoundProviderOgg::task_code() {
		constexpr size_t bytes_per_sample = 2;
		const size_t buf_len = CONFIG_OGG_BUFFER_SIZE*chTotal;
		auto buf = std::make_unique<char[]>(buf_len); // buf_len bytes for us (buf_len/2 samples)
		assert(buf != nullptr);

		bool eof = false;
		while(not eof) {
			size_t buf_offset = 0;
			seeked = false;
			while(buf_offset < buf_len) {
				if (seeked) {
					buf_offset = 0;
					seeked = false;
				}
				int bitstream = -1;
				std::unique_lock<std::mutex> lock(safeState);
				long res = ov_read(&vorbis_file, buf.get()+buf_offset, buf_len - buf_offset, &bitstream);
				if (res < 0) { // Error
					postControl(FAILURE);
					return;
				} else if (res == 0) { // EOF
					eof = true;
					break; // Success
				} else {
					buf_offset += res;
				}
			}
			for (size_t i = 0; true; ++i) {
				if (seeked) {break;};
				size_t sampleoffset = ((chTotal*i)+(ch-1))*bytes_per_sample;
				if ((sampleoffset + 1) > buf_offset) break; // If we have reached end of buf
				unsigned char high = (unsigned char)buf[sampleoffset];
				unsigned char low = (unsigned char)buf[sampleoffset+1];
				int16_t shortSample = (short)( (low << 8) | high );
				SoundData sample = (((int)shortSample)-SHRT_MIN)/(USHRT_MAX/255);
				postSample(sample);
			}
		}
		waitQueueEmpty();
		postControl(END);
	}

	void SoundProviderOgg::seekRaw(long pos) {
		checkSeekable();
		std::unique_lock<std::mutex> lock(safeState);
		ov_raw_seek(&vorbis_file, pos);
	};

	void SoundProviderOgg::seekPcm(int64_t pos) {
		checkSeekable();
		std::unique_lock<std::mutex> lock(safeState);
		ov_pcm_seek(&vorbis_file, pos);
	};

	void SoundProviderOgg::seekPcmPage(int64_t pos) {
		checkSeekable();
		std::unique_lock<std::mutex> lock(safeState);
		ov_pcm_seek_page(&vorbis_file, pos);
	};

	void SoundProviderOgg::seekTime(int64_t pos) {
		checkSeekable();
		std::unique_lock<std::mutex> lock(safeState);
		ov_time_seek(&vorbis_file, pos);
	};

	void SoundProviderOgg::seekTimePage(int64_t pos) {
		checkSeekable();
		std::unique_lock<std::mutex> lock(safeState);
		ov_time_seek_page(&vorbis_file, pos);
	};

	OggInfo SoundProviderOgg::getInfo(int i) {
		OggInfo info;
		std::unique_lock<std::mutex> lock(safeState);
		vorbis_info* baseinfo = ov_info(&vorbis_file, i);
		// info. = baseinfo->
		info.version = baseinfo->version;
		info.channels = baseinfo->channels;
		info.rate = baseinfo->rate;

		info.bitrate_upper = baseinfo->bitrate_upper;
		info.bitrate_nominal = baseinfo->bitrate_nominal;
		info.bitrate_lower = baseinfo->bitrate_lower;
		lock.unlock();

		info.seekable = seekable();

		if (info.seekable) {
			lock.lock();
			info.raw_total = ov_raw_total(&vorbis_file, i);
			info.pcm_total = ov_pcm_total(&vorbis_file, i);
			info.time_total = ov_time_total(&vorbis_file, i);
			lock.unlock();
		}
		return info;
	};

	OggPosition SoundProviderOgg::getPosition() {
		OggPosition pos;
		std::unique_lock<std::mutex> lock(safeState);
		pos.raw = ov_raw_tell(&vorbis_file);
		pos.pcm = ov_pcm_tell(&vorbis_file);
		pos.time = ov_time_tell(&vorbis_file);
		return pos;
	};

	OggComment SoundProviderOgg::getComment(int i) {
		OggComment comment;
		std::unique_lock<std::mutex> lock(safeState);
		vorbis_comment* comment_struct = ov_comment(&vorbis_file,i);
		{
			for(size_t i = 0; i < comment_struct->comments; ++i) {
				int len = comment_struct->comment_lengths[i];
				comment.comments.emplace_back(comment_struct->user_comments[i], len);
			}
		}
		comment.vendor = comment_struct->vendor;
		return comment;
	};

};
