COMPONENT_SRCDIRS := . tremor ogg/src
COMPONENT_ADD_INCLUDEDIRS := . config tremor ogg/include
COMPONENT_PRIV_INCLUDEDIRS := config tremor ogg/include
MY_FLAGS = -include os.h -Wno-error=char-subscripts 
CFLAGS += $(MY_FLAGS)
CXXFLAGS += $(MY_FLAGS) -std=gnu++14
