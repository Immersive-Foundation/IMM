cmake_minimum_required(VERSION 3.6)


set(OGG_SOURCES
    src/bitwise.c
    src/framing.c
    )

set(VORBIS_DIR libvorbis-1.3.5)
set(VORBIS_SOURCES
    ${VORBIS_DIR}/lib/analysis.c
    ${VORBIS_DIR}/lib/mdct.c
    ${VORBIS_DIR}/lib/psy.c
    ${VORBIS_DIR}/lib/bitrate.c
    ${VORBIS_DIR}/lib/block.c
    ${VORBIS_DIR}/lib/registry.c
    ${VORBIS_DIR}/lib/codebook.c	
    ${VORBIS_DIR}/lib/res0.c
    ${VORBIS_DIR}/lib/envelope.c	
    ${VORBIS_DIR}/lib/sharedbook.c
    ${VORBIS_DIR}/lib/floor0.c		
    ${VORBIS_DIR}/lib/smallft.c
    ${VORBIS_DIR}/lib/floor1.c		
    ${VORBIS_DIR}/lib/synthesis.c
    ${VORBIS_DIR}/lib/info.c
    ${VORBIS_DIR}/lib/lookup.c		
    ${VORBIS_DIR}/lib/vorbisenc.c
    ${VORBIS_DIR}/lib/lpc.c		
    ${VORBIS_DIR}/lib/vorbisfile.c
    ${VORBIS_DIR}/lib/lsp.c		
    ${VORBIS_DIR}/lib/window.c
    ${VORBIS_DIR}/lib/mapping0.c
    )
set(VORBIS_PRIVATE_INCLUDES ${VORBIS_DIR}/lib)
set(VORBIS_INCLUDES ${VORBIS_DIR}/include)

add_library(ogg ${OGG_SOURCES})
target_include_directories(ogg PUBLIC include)

add_library(vorbis ${VORBIS_SOURCES})
target_include_directories(vorbis
    PRIVATE ${VORBIS_PRIVATE_INCLUDES}
    PUBLIC ${VORBIS_INCLUDES})
target_link_libraries(vorbis PUBLIC ogg)
