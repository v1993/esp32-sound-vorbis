#include <soundProviderOgg.h>

void SoundProviderOgg::checkErr(int *errp, bool fatal) { // Check for errors
	enum STBVorbisError e;
	if (errp == NULL) { 
		e = stb_vorbis_get_error(vorbis_state);
	} else {
		e = *errp;
	}
	if (e != VORBIS__no_error) {
		if (fatal) {abort();}
		else {
			if (e == VORBIS_outofmem) {
				throw std::bad_alloc();
			} else {
				throw std::runtime_error();
			}
		}
	}
};

void SoundProviderOgg::setup(unsigned int ch_arg) {
	stb_vorbis_info = stb_vorbis_get_info(vorbis_state);
	assert(ch_arg <= stb_vorbis_info.channels); // We have requested channel

	ch = ch_arg;
	safeState = xSemaphoreCreateMutex();
}

SoundProviderOgg::SoundProviderOgg(const unsigned char *data, int len, unsigned int ch_arg) {
	int err;
	vorbis_state = stb_vorbis_open_memory(data, len, &err, NULL);
	checkErr(&err, false);
	setup();
}

#if CONFIG_OGG_SND_STDIO
	SoundProviderOgg::SoundProviderOgg(const char *file, unsigned int ch_arg) {
		int err;
		vorbis_state = stb_vorbis_open_filename(file, &err, NULL);
		checkErr(&err, false);
		setup();
	}

	SoundProviderOgg::SoundProviderOgg(FILE *f, unsigned int ch_arg) {
		int err;
		vorbis_state = stb_vorbis_open_filename(f, &err, NULL);
		checkErr(&err, false);
		setup();
	}

	SoundProviderOgg::SoundProviderOgg(FILE *f, unsigned int len, unsigned int ch_arg) {
		int err;
		vorbis_state = stb_vorbis_open_file_section(f, false, &err, NULL, len);
		checkErr(&err, false);
		setup();
	}
#endif

SoundProviderOgg::~SoundProviderOgg() {
	stb_vorbis_close(vorbis_state);
	vSemaphoreDelete(safeState);
}

void SoundProviderOgg::task_prestop() {
	xSemaphoreTake(safeState, portMAX_DELAY);
}

void SoundProviderOgg::task_poststop() {
	xSemaphoreGive(safeState);
}

void SoundProviderOgg::task_code() {
	xSemaphoreTake(safeState, portMAX_DELAY);
	stb_vorbis_seek_start(vorbis_state);
	checkErr();
	xSemaphoreGive(safeState);

	while(true) {
		float **buf;
		xSemaphoreTake(safeState, portMAX_DELAY);
		int samplecnt = stb_vorbis_get_frame_float(vorbis_state, NULL, &buf_pointer);
		xSemaphoreGive(safeState);
		if (samplecnt == 0) {break;};
		for (int i = 0; i < samplecnt; i++) {
			postSample((255*(buf[ch][i]+FLT_MAX))/(2*FLT_MAX));
		}
	}
}
