# PLEASE, UPDATE THIS FILE WHEN ADDING ANY NEW C SOURCES

set(COMPONENT_SRCS
	# OGG
	"ogg/src/bitwise.c"
	"ogg/src/framing.c"

	# Tremor
	"tremor/block.c"
	"tremor/codebook.c"
	"tremor/floor0.c"
	"tremor/floor1.c"
	"tremor/info.c"
	"tremor/mapping0.c"
	"tremor/mdct.c"
	"tremor/registry.c"
	"tremor/res012.c"
	"tremor/sharedbook.c"
	"tremor/synthesis.c"
	"tremor/vorbisfile.c"
	"tremor/window.c"

	# Main code
	"soundProviderVorbis.cpp"
	)

set(COMPONENT_REQUIRES "esp32-sound")
set(COMPONENT_ADD_INCLUDEDIRS "." "config" "ogg/include")

register_component()

#target_compile_options(${COMPONENT_TARGET} PUBLIC "-include ${COMPONENT_PATH}/config/tremor/os.h")
#target_compile_features(${COMPONENT_TARGET} PUBLIC cxx_std_14)
#set_property(TARGET ${COMPONENT_TARGET} PROPERTY CXX_STANDARD 14)
#target_compile_options(${COMPONENT_TARGET} PUBLIC "-std=gnu++14")
