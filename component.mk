COMPONENT_SRCDIRS := . tremor ogg/src
#COMPONENT_ADD_INCLUDEDIRS := . # Refer to https://github.com/espressif/esp-idf/issues/2265
#COMPONENT_PRIV_INCLUDEDIRS := config tremor ogg/include
#OGG_FLAGS = -include config/tremor/os.h -Wno-error=char-subscripts 
#CFLAGS += $(OGG_FLAGS)
#CXXFLAGS += $(OGG_FLAGS) -std=gnu++14

COMPONENT_ADD_INCLUDEDIRS := . config ogg/include
#COMPONENT_PRIV_INCLUDEDIRS := config tremor ogg/include
OGG_VORBIS_FLAGS = -include config/tremor/os.h -Wno-error=char-subscripts 
CFLAGS += $(OGG_VORBIS_FLAGS)
CXXFLAGS += $(OGG_VORBIS_FLAGS) -std=gnu++14
