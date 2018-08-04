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

namespace SoundOgg {
	USING_NS_SOUND;

	struct OggInfo { // Functions to fill: 
		int version; // ov_info
		int channels;
		long rate;

		long bitrate_upper;
		long bitrate_nominal;
		long bitrate_lower;

		bool seekable; // ov_seekable

		int64_t raw_total; // ov_raw_total, only if seekable
		int64_t pcm_total; // ov_pcm_total
		int64_t time_total; // ov_time_total
	};

	struct OggPosition {
		int64_t raw;
		int64_t pcm;
		int64_t time;
	};

	struct OggComment {
		std::vector<std::string> comments;
		std::string vendor;
	};

	class OggException: public std::exception { public: virtual const char* what() const throw() { return "ogg exeption";} };
	namespace OggExceptions {
		// class : public OggException { public: virtual const char* what() const throw() { return "ogg: ";} };
		class Unknown: public OggException { public: virtual const char* what() const throw() { return "ogg: unknown error";} };
		class NotSeekable: public OggException { public: virtual const char* what() const throw() { return "ogg: trying to seek in unseekable stream";} };
	}

	class SoundProviderOgg: public Sound::SoundProviderTask {
		#if CONFIG_OGG_INTROSPECTION
		public:
		#else
		protected:
		#endif
			OggVorbis_File vorbis_file;
			std::mutex safeState; // You must NOT preform any blocking operations in "safe" area

		protected:
			long frequency;
			virtual unsigned long int getFrequency() override { return frequency; };
			unsigned int ch;
			unsigned int chTotal;
			virtual void task_prestop() override final;
			virtual void task_poststop() override final;
			virtual void task_code() override final;

			virtual void provider_restart() override; // Seek to start

			std::atomic<bool> seeked = {false}; // Read again

			void checkErr(int err);
			void setup(unsigned int ch_arg);
			void open_file(FILE *f, unsigned int ch_arg, char *initial, long ibytes);
			void open_callbacks(void *datasource, const ov_callbacks& callbacks, unsigned int ch_arg, char *initial, long ibytes);

			bool seekable() { std::unique_lock<std::mutex> lock(safeState); return (ov_seekable(&vorbis_file) == 0) ? false : true; };

			void checkSeekable();
		public:
			explicit SoundProviderOgg(FILE *f, unsigned int ch_arg, char *initial = nullptr, long ibytes = 0); // Calls ov_open directly
			// TODO: implement ov_open_callbacks version
			explicit SoundProviderOgg(void *datasource, const ov_callbacks& callbacks, unsigned int ch_arg, char *initial = nullptr, long ibytes = 0);

			explicit SoundProviderOgg(const unsigned char *data, int len, unsigned int ch_arg, char *initial = nullptr, long ibytes = 0); // Uses fmemopen to open memory as file
			explicit SoundProviderOgg(const char* file, unsigned int ch_arg, char *initial = nullptr, long ibytes = 0); // Uses fopen in read only mode

			virtual ~SoundProviderOgg();

			void seekRaw(long pos); // Directly calls correspondenting functions
			void seekPcm(int64_t pos);
			void seekPcmPage(int64_t pos);
			void seekTime(int64_t pos);
			void seekTimePage(int64_t pos);

			OggInfo getInfo(int i = -1);
			OggPosition getPosition();
			OggComment getComment(int i = -1);
	};
};
