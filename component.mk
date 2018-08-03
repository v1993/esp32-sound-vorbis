COMPONENT_SRCDIRS:=. ./stb_vorbis
COMPONENT_ADD_INCLUDEDIRS:=.
CXXFLAGS += -std=c++14
#COMPONENT_OWNBUILDTARGET = true

ifeq ($(CONFIG_OGG_SND_STDIO), n)
	CPPFLAGS += -DSTB_VORBIS_NO_STDIO
endif

CPPFLAGS += -DSTB_VORBIS_NO_PUSHDATA_API

CFLAGS += -Wno-error=unused-value -Wno-error=maybe-uninitialized

#build:
#	@echo $(CFLAGS)
