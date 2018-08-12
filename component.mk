COMPONENT_SRCDIRS := . tremor ogg/src
COMPONENT_ADD_INCLUDEDIRS := . config ogg/include
#COMPONENT_PRIV_INCLUDEDIRS := config tremor ogg/include
OGG_VORBIS_FLAGS = -include config/tremor/os.h -Wno-error=char-subscripts 
CFLAGS += $(OGG_VORBIS_FLAGS)
CXXFLAGS += $(OGG_VORBIS_FLAGS) -std=gnu++14
