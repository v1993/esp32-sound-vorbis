#pragma once

#include <sound.h>
#include <soundProviderTask.h>

#include <mutex>
#include <atomic>
#include <exception>

extern "C" {
#include <tremor/ivorbiscodec.h>
#include <tremor/ivorbisfile.h>
#undef _OS_H
}

namespace SoundVorbis {
	USING_NS_SOUND;

	struct OggVorbisInfo { // Functions to fill:
			int version; // ov_info
			int channels;
			long rate;

			long bitrate_upper;
			long bitrate_nominal;
			long bitrate_lower;

			bool seekable; // ov_seekable
			long streams; // ov_streams

			int64_t raw_total; // ov_raw_total, only if seekable
			int64_t pcm_total; // ov_pcm_total
			int64_t time_total; // ov_time_total
	};

	struct OggVorbisPosition {
			int64_t raw;
			int64_t pcm;
			int64_t time;
	};

	struct OggVorbisComment {
			std::vector<std::string> comments;
			std::string vendor;
	};

	class VorbisException: public std::exception {
		public:
			virtual const char* what() const throw () {
				return "vorbis exeption";
			}
	};
	namespace VorbisExceptions {
		// class : public VorbisException { public: virtual const char* what() const throw() { return "vorbis: ";} };
		class Unknown: public VorbisException {
			public:
				virtual const char* what() const throw () {
					return "vorbis: unknown error";
				}
		};
		class NotSeekable: public VorbisException {
			public:
				virtual const char* what() const throw () {
					return "vorbis: trying to seek in unseekable stream";
				}
		};

		class ReadError: public VorbisException {
			public:
				virtual const char* what() const throw () {
					return "vorbis: read error";
				}
		};
		class NotVorbis: public VorbisException {
			public:
				virtual const char* what() const throw () {
					return "vorbis: data isn't vorbis data";
				}
		};
		class BadVersion: public VorbisException {
			public:
				virtual const char* what() const throw () {
					return "vorbis: vorbis version mismatch";
				}
		};
		class BadHeader: public VorbisException {
			public:
				virtual const char* what() const throw () {
					return "vorbis: invalid vorbis bitstream header";
				}
		};
		class Fault: public VorbisException {
			public:
				virtual const char* what() const throw () {
					return "vorbis: internal logic fault; indicates a bug or heap/stack corruption";
				}
		};
	}

	class SoundProviderVorbis: public Sound::SoundProviderTask {
#if CONFIG_VORBIS_INTROSPECTION
			public:
#else
		protected:
#endif
			OggVorbis_File vorbis_file;
			std::mutex safeState; // You must NOT preform any blocking operations in "safe" area

		protected:
			long frequency;
			virtual unsigned long int getFrequency() override {
				return frequency;
			}
			;
			unsigned int ch;
			unsigned int chTotal;
			virtual void task_prestop() override final;
			virtual void task_poststop() override final;
			virtual void task_code() override final;

			virtual void provider_restart() override; // Seek to start

			void vorbis_read();

			std::atomic<bool> seeked = {false}; // Read again
			std::atomic<bool> exit_now = {false}; // If true, exit task immediately
			std::mutex heapUsed; // Lock when deleting task

			void checkErr(int err);
			void setup(unsigned int ch_arg);
			void open_file(FILE *f, unsigned int ch_arg, char *initial, long ibytes);
			void open_callbacks(void *datasource, const ov_callbacks& callbacks, unsigned int ch_arg, char *initial, long ibytes);

			bool seekable() { // UNSAFE
				return (ov_seekable(&vorbis_file) == 0) ? false : true;
			}
			;

			void checkSeekable(); // UNSAFE
		public:
			explicit SoundProviderVorbis(FILE *f, unsigned int ch_arg, char *initial = nullptr, long ibytes = 0); // Calls ov_open directly
			explicit SoundProviderVorbis(void *datasource, const ov_callbacks& callbacks, unsigned int ch_arg, char *initial = nullptr, long ibytes = 0);

			explicit SoundProviderVorbis(const unsigned char *data, int len, unsigned int ch_arg, char *initial = nullptr, long ibytes = 0); // Uses fmemopen to open memory as file
			explicit SoundProviderVorbis(const char* file, unsigned int ch_arg, char *initial = nullptr, long ibytes = 0); // Uses fopen in read only mode

			virtual ~SoundProviderVorbis();

			void seekRaw(long pos); // Directly calls correspondenting functions
			void seekPcm(int64_t pos);
			void seekPcmPage(int64_t pos);
			void seekTime(int64_t pos);
			void seekTimePage(int64_t pos);

			OggVorbisInfo getInfo(int i = -1);
			OggVorbisPosition getPosition();
			OggVorbisComment getComment(int i = -1);
	};
}
;
