#pragma once

#include <sound.h>
#include <soundProviderTask.h>
#include "stb_vorbis/stb_vorbis.h"

#include <mutex>
#include <atomic>
#include <exception>

namespace SoundOgg {
	USING_NS_SOUND;

	class OggException: public std::exception { public: virtual const char* what() const throw() { return "ogg exeption";} };
	namespace OggExceptions {
		// class : public OggException { public: virtual const char* what() const throw() { return "ogg: ";} };
		class NeedMoreData: public OggException { public: virtual const char* what() const throw() { return "ogg: need more data (how you got it in PULLAPI?!)";} };

		class InvalidApiMixing: public OggException { public: virtual const char* what() const throw() { return "ogg: invalid api mixing (how you got it in PULLAPI?!)";} };
		class OutOfMem: public OggException { public: virtual const char* what() const throw() { return "ogg: out of memory";} };
		class FeatureNotSupported: public OggException { public: virtual const char* what() const throw() { return "ogg: feature not supported (used floor 0)";} };
		class TooManyChannels: public OggException { public: virtual const char* what() const throw() { return "ogg: too many channels (increace in menuconfig)";} };
		class FileOpenFailure: public OggException { public: virtual const char* what() const throw() { return "ogg: file open failure";} };
		class SeekWithoutLength: public OggException { public: virtual const char* what() const throw() { return "ogg: seek without length (how you got it in PULLAPI?!)";} };

		class UnexpectedEOF: public OggException { public: virtual const char* what() const throw() { return "ogg: unexpected end-of-file";} };
		class SeekInvalid: public OggException { public: virtual const char* what() const throw() { return "ogg: seek invalid (past EOF)";} };

		class InvalidSetup: public OggException { public: virtual const char* what() const throw() { return "ogg: vorbis invalid setup";} };
		class InvalidStream: public OggException { public: virtual const char* what() const throw() { return "ogg: vorbis invalid stream";} };

		class MissingCapturePattern: public OggException { public: virtual const char* what() const throw() { return "ogg: missing capture pattern";} };
		class InvalidStreamStructureVersion: public OggException { public: virtual const char* what() const throw() { return "ogg: invalid stream structure version";} };
		class ContinuedPacketFlagInvalid: public OggException { public: virtual const char* what() const throw() { return "ogg: continued packet flag invalid";} };
		class IncorrectStreamSerialNumber: public OggException { public: virtual const char* what() const throw() { return "ogg: incorrect stream serial number";} };
		class InvalidFirstPage: public OggException { public: virtual const char* what() const throw() { return "ogg: invalid first page";} };
		class BadPacketType: public OggException { public: virtual const char* what() const throw() { return "ogg: bad packet type";} };
		class CantFindLastPage: public OggException { public: virtual const char* what() const throw() { return "ogg: can't find last page";} };
		class SeekFailed: public OggException { public: virtual const char* what() const throw() { return "ogg: seek failed";} };
	}

	class SoundProviderOgg: public Sound::SoundProviderTask {
		protected:
			virtual unsigned long int getFrequency() override;
			unsigned int ch;
			virtual void task_prestop() override final;
			virtual void task_poststop() override final;
			virtual void task_code() override final;
			//std::mutex safeState; // You must NOT preform any blocking operations in "safe" area
			std::atomic<bool> seeked = {false};

			stb_vorbis* vorbis_state = nullptr;

			void checkErr(int* errp = nullptr);
			void setup(unsigned int ch_arg);

			unsigned int frequency;
		public:
			explicit SoundProviderOgg(const unsigned char *data, int len, unsigned int ch_arg); // stb_vorbis_open_memory

			#if CONFIG_OGG_SND_STDIO
				explicit SoundProviderOgg(const char *file, unsigned int ch_arg); // stb_vorbis_open_filename
				explicit SoundProviderOgg(FILE *f, unsigned int ch_arg); // stb_vorbis_open_file
				explicit SoundProviderOgg(FILE *f, unsigned int len, unsigned int ch_arg); // stb_vorbis_open_file_section
			#endif

			virtual ~SoundProviderOgg();

			void seek(unsigned int sample_number);
			void seek_frame(unsigned int sample_number);
			void seek_start();

			void seek_second(float time);

			unsigned int length_samples();
			float length_seconds();

			unsigned long int samplesPerSecond();
	};
};
