#ifndef SOUND_PROVIDER_PCM_H
#define SOUND_PROVIDER_PCM_H

#define STB_VORBIS_NO_STDIO !(CONFIG_OGG_SND_STDIO)
#define STB_VORBIS_NO_PUSHDATA_API // We don't use it

#include <sound.h>
#include <soundProviderTask.h>
#include "std_vorbis/std_vorbis.h"

#include <float.h>

class SoundProviderOgg: public SoundProviderTask {
	protected:
		unsigned long int getFrequency();
		unsigned int ch;
		void task_prestop();
		void task_poststop();
		void task_code();
		SemaphoreHandle_t safeState = NULL; // You must NOT preform any blocking operations in "safe" area

//		QueueHandle_t queue = NULL;

		stb_vorbis* vorbis_state = NULL;

		void checkErr(int *errp = NULL, bool fatal = true);
		void setup();
	public:
		explicit SoundProviderOgg(const unsigned char *data, int len, unsigned int ch_arg); // stb_vorbis_open_memory

		#if CONFIG_OGG_SND_STDIO
			explicit SoundProviderOgg(const char *file, unsigned int ch_arg); // stb_vorbis_open_filename
			explicit SoundProviderOgg(FILE *f, unsigned int ch_arg); // stb_vorbis_open_file
			explicit SoundProviderOgg(FILE *f, unsigned int len, unsigned int ch_arg); // stb_vorbis_open_file_section
		#endif

		~SoundProviderOgg();

		seek();
}
#endif
