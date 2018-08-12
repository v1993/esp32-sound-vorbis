#include <soundProviderVorbis.h>
#include <iostream>
#include <cmath>
#include <cstring>
#include <memory>
#include <limits.h>
extern "C" {
#include <stdio.h>
#include "esp_log.h"
}

static const char* TAG = "Vorbis";

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define checkExit() if (unlikely(exit_now.load())) return

#define assume(x, y) __builtin_expect((x), (y))

namespace SoundVorbis {
	void SoundProviderVorbis::checkErr(int e) {
		switch(assume(e, 0)) {
			// case : throw VorbisExceptions::(); break
			case 0:
				break; // No error
			case OV_EREAD:
				throw VorbisExceptions::ReadError();
				break;
			case OV_ENOTVORBIS:
				throw VorbisExceptions::NotVorbis();
				break;
			case OV_EVERSION:
				throw VorbisExceptions::BadVersion();
				break;
			case OV_EBADHEADER:
				throw VorbisExceptions::BadHeader();
				break;
			case OV_EFAULT:
				throw VorbisExceptions::Fault();
				break;
			default:
				throw VorbisExceptions::Unknown();
				break;
		};
	}

	void SoundProviderVorbis::setup(unsigned int ch_arg) {
		vorbis_info* info = ov_info(&vorbis_file, -1);
		assert(ch_arg > 0);
		assert(ch_arg <= info->channels); // We have requested channel (count from one)

		ch = ch_arg;
		chTotal = info->channels;
		frequency = info->rate;

		stackSize = CONFIG_VORBIS_STACK_SIZE;
	}

	void SoundProviderVorbis::open_file(FILE *f, unsigned int ch_arg, char *initial, long ibytes) {
		checkErr(ov_open(f, &vorbis_file, initial, ibytes));
		setup(ch_arg);
	}

	void SoundProviderVorbis::open_callbacks(void *datasource, const ov_callbacks& callbacks, unsigned int ch_arg, char *initial, long ibytes) {
		checkErr(ov_open_callbacks(datasource, &vorbis_file, initial, ibytes, callbacks));
		setup(ch_arg);
	}

	SoundProviderVorbis::SoundProviderVorbis(FILE *f, unsigned int ch_arg, char *initial, long ibytes) {
		open_file(f, ch_arg, initial, ibytes);
	}

	SoundProviderVorbis::SoundProviderVorbis(void *datasource, const ov_callbacks& callbacks, unsigned int ch_arg, char *initial, long ibytes) {
		open_callbacks(datasource, callbacks, ch_arg, initial, ibytes);
	}

	SoundProviderVorbis::SoundProviderVorbis(const unsigned char *data, int len, unsigned int ch_arg, char *initial, long ibytes) {
		FILE* memfile = fmemopen(const_cast<unsigned char*>(data), len, "r");
		if (unlikely(memfile == nullptr)) {
			std::system_error(errno);
		}
		open_file(memfile, ch_arg, initial, ibytes);
	}

	SoundProviderVorbis::SoundProviderVorbis(const char* file, unsigned int ch_arg, char *initial, long ibytes) {
		FILE* filed = fopen(file, "r");
		if (unlikely(filed == nullptr)) {
			std::system_error(errno);
		}
		try {
			open_file(filed, ch_arg, initial, ibytes);
		} catch(...) { // FILE* isn't RAII thing, so I'm forced to this ugly hack
			fclose(filed);
			throw;
		}
	}

	SoundProviderVorbis::~SoundProviderVorbis() {
		std::unique_lock<std::mutex> lock(safeState);
		ov_clear(&vorbis_file);
	}

	void SoundProviderVorbis::task_prestop() {
		exit_now = true;
		heapUsed.lock();
	}

	void SoundProviderVorbis::task_poststop() {
		heapUsed.unlock();
	}

	void SoundProviderVorbis::provider_restart() {
		if (taskHandle != nullptr) {
			task_prestop();
			vTaskDelete(taskHandle);
			task_poststop();
		}
		try {
			seekPcm(0);
		} catch(VorbisExceptions::NotSeekable&) {
		}; // It is probably ok.
		unconditionalStart();
	}

	void SoundProviderVorbis::checkSeekable() {
		if (not seekable()) {
			throw VorbisExceptions::NotSeekable();
		}
	}

	void SoundProviderVorbis::task_code() {
		std::unique_lock<std::mutex> heapLock(heapUsed);
		vorbis_read();
		exit_now = false;
	}

	void SoundProviderVorbis::vorbis_read() {
		constexpr size_t bytes_per_sample = 2;
		const size_t buf_len = CONFIG_VORBIS_BUFFER_SIZE * chTotal;
		auto buf = std::make_unique<char[]>(buf_len); // buf_len bytes for us (buf_len/2 samples)
		assert(buf != nullptr);

		bool eof = false;
		while(not eof) {
			checkExit();
			size_t buf_offset = 0;
			seeked = false;
			long changeRate = -1; // No changes
			while(buf_offset < buf_len) {
				int bitstream = -1;
				checkExit();
				std::unique_lock<std::mutex> lock(safeState);
				if (seeked.load()) {
					ESP_LOGD(TAG, "Seek in read loop");
					buf_offset = 0;
					seeked = false;
				}
				checkExit(); // Check another one time before time consuming operation
				long res = ov_read(&vorbis_file, buf.get()+buf_offset, buf_len - buf_offset, &bitstream);
				if (unlikely(res < 0)) { // Error
					const char* msg;
					switch(res) {
						case OV_HOLE:
						msg = "interruption in the data";
						break;
						case OV_EBADLINK:
						msg = "invalid stream section";
						break;
						default:
						msg = "unknown error";
						break;
					}
					ESP_LOGE(TAG, "Ogg vorbis read error: %s", msg);
					postControl(FAILURE);
					return;
				} else if (res == 0) { // EOF
					eof = true;
					break;// Success
				} else {
					vorbis_info* info = ov_info(&vorbis_file, bitstream);
					if (unlikely(frequency != info->rate)) { // Who on this planet need it? Take him to me!
						frequency = info->rate;
						changeRate = buf_offset;
					}
					buf_offset += res;
					if (unlikely(changeRate != -1)) {
						break; // Only one change per buffer to prevent checks hell
					}
				}
			}
			for (size_t i = 0; true; ++i) {
				checkExit();
				if (seeked.load()) {ESP_LOGD(TAG, "seek in post loop"); queueReset(); break;};
				size_t sampleoffset = ((chTotal*i)+(ch-1))*bytes_per_sample;
				if ((sampleoffset + 1) > buf_offset) break; // If we have reached end of buf
				if (unlikely((changeRate != -1) and (changeRate <= sampleoffset))) {
					waitQueueEmpty();
					postControl(FREQUENCY_UPDATE);
					buf_offset = -1;
				}
				unsigned char high = (unsigned char)buf[sampleoffset];
				unsigned char low = (unsigned char)buf[sampleoffset+1];
				int16_t shortSample = (short)( (low << 8) | high );
				SoundData sample = (((int)shortSample)-SHRT_MIN)/(USHRT_MAX/255);
				postSample(sample);
			}
			if (eof) {
				while (uxQueueMessagesWaiting(queue) > 0) {
					vTaskDelay(1);
					if(seeked) {
						eof = false;
						break;
					}
				}
			}
		}
		postControl(END); // Wait is in loop
	}

	void SoundProviderVorbis::seekRaw(long pos) {
		std::unique_lock<std::mutex> lock(safeState);
		checkSeekable();
		ov_raw_seek(&vorbis_file, pos);
		seeked = true;
		queueReset();
	}

	void SoundProviderVorbis::seekPcm(int64_t pos) {
		std::unique_lock<std::mutex> lock(safeState);
		checkSeekable();
		ov_pcm_seek(&vorbis_file, pos);
		seeked = true;
		queueReset();
	}

	void SoundProviderVorbis::seekPcmPage(int64_t pos) {
		std::unique_lock<std::mutex> lock(safeState);
		checkSeekable();
		ov_pcm_seek_page(&vorbis_file, pos);
		seeked = true;
		queueReset();
	}

	void SoundProviderVorbis::seekTime(int64_t pos) {
		std::unique_lock<std::mutex> lock(safeState);
		checkSeekable();
		ov_time_seek(&vorbis_file, pos);
		seeked = true;
		queueReset();
	}

	void SoundProviderVorbis::seekTimePage(int64_t pos) {
		std::unique_lock<std::mutex> lock(safeState);
		checkSeekable();
		ov_time_seek_page(&vorbis_file, pos);
		seeked = true;
		queueReset();
	}

	OggVorbisInfo SoundProviderVorbis::getInfo(int i) {
		OggVorbisInfo info;
		std::unique_lock<std::mutex> lock(safeState);
		vorbis_info* baseinfo = ov_info(&vorbis_file, i);
		// info. = baseinfo->
		info.version = baseinfo->version;
		info.channels = baseinfo->channels;
		info.rate = baseinfo->rate;

		info.bitrate_upper = baseinfo->bitrate_upper;
		info.bitrate_nominal = baseinfo->bitrate_nominal;
		info.bitrate_lower = baseinfo->bitrate_lower;

		info.seekable = seekable();
		info.streams = ov_streams(&vorbis_file);

		if (info.seekable) {
			info.raw_total = ov_raw_total(&vorbis_file, i);
			info.pcm_total = ov_pcm_total(&vorbis_file, i);
			info.time_total = ov_time_total(&vorbis_file, i);
		}
		return info;
	}

	OggVorbisPosition SoundProviderVorbis::getPosition() {
		OggVorbisPosition pos;
		std::unique_lock<std::mutex> lock(safeState);
		pos.raw = ov_raw_tell(&vorbis_file);
		pos.pcm = ov_pcm_tell(&vorbis_file);
		pos.time = ov_time_tell(&vorbis_file);
		return pos;
	}

	OggVorbisComment SoundProviderVorbis::getComment(int i) {
		OggVorbisComment comment;
		std::unique_lock<std::mutex> lock(safeState);
		vorbis_comment* comment_struct = ov_comment(&vorbis_file, i);
		{
			for (size_t i = 0; i < comment_struct->comments; ++i) {
				int len = comment_struct->comment_lengths[i];
				comment.comments.emplace_back(comment_struct->user_comments[i], len);
			}
		}
		comment.vendor = comment_struct->vendor;
		return comment;
	}

}
