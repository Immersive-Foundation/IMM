# Sets the minimum version of CMake required to build your native library.
# This ensures that a certain set of CMake features is available to
# your build.

cmake_minimum_required(VERSION 3.10.2)

# Specifies a library name, specifies whether the library is STATIC or
# SHARED, and provides relative paths to the source code. You can
# define multiple libraries by adding multiple add.library() commands,
# and CMake builds them for you. When you build your app, Gradle
# automatically packages shared libraries with your APK.
project("immplayer")

set(imm-root $ENV{IMMROOT})
set(imm-player ${imm-root}/code/libImmPlayer/src)
set(imm-android ${imm-root}/code/projects/android/immAndroid)
set(imm-third-party ${imm-root}/../../third_party_libs)
set(imm-code ${imm-root}/code)
#set(imm-tools ${imm-root}/tools)
add_library(
    # Specifies the name of the library.
    immplayer

    # Sets the library as a shared library.
    STATIC

    ${imm-player}/player.cpp
    ${imm-player}/document.cpp
    ${imm-player}/blue_noise.cpp
    ${imm-player}/mngrPlayer.cpp
    ${imm-player}/layerRenderers/layerRendererModel/layerRendererModel.cpp
    ${imm-player}/layerRenderers/layerRendererPicture/layerRendererPicture.cpp
    ${imm-player}/layerRenderers/layerRendererSound/layerRendererSound.cpp
    ${imm-player}/layerRenderers/layerRendererPaint/static/layerRendererPaintStatic.cpp
    ${imm-player}/layerRenderers/layerRendererPaint/pretessellated/layerRendererPaintPretessellated.cpp
)
#[[
# ----------- Shader transformation ---------------------
add_custom_target(
        ShadersStatic
        COMMAND ${CMAKE_COMMAND}
        -D INPUT=${imm-player}/layerRenderers/layerRendererPaint/static/
        -D OUTPUT=${imm-player}/layerRenderers/layerRendererPaint/static/tmp/shader_static_brush_GLES_glsl.h
        -D NAMESPACE=INVALID
        -P ${CMAKE_CURRENT_SOURCE_DIR}/create_shader.cmake
)

add_dependencies(immplayer ShadersStatic)

add_custom_target(
        ShadersPretessellated
        COMMAND ${CMAKE_COMMAND}
        -D INPUT=${imm-player}/layerRenderers/layerRendererPaint/pretessellated/
        -D OUTPUT=${imm-player}/layerRenderers/layerRendererPaint/pretessellated/tmp/shader_pretessellated_brush_GLES_glsl.h
        -D NAMESPACE=INVALID
        -P ${CMAKE_CURRENT_SOURCE_DIR}/create_shader.cmake
)

add_dependencies(immplayer ShadersPretessellated)

add_custom_target(
        ShadersModel
        COMMAND ${CMAKE_COMMAND}
        -D INPUT=${imm-player}/layerRenderers/layerRendererModel/gles
        -D OUTPUT=${imm-player}/layerRenderers/layerRendererModel/tmp/shader_model_GLES_glsl.h
        -D NAMESPACE=INVALID
        -P ${CMAKE_CURRENT_SOURCE_DIR}/create_shader.cmake
)

add_dependencies(immplayer ShadersModel)

add_custom_target(
        ShadersPicture
        COMMAND ${CMAKE_COMMAND}
        -D INPUT=${imm-player}/layerRenderers/layerRendererPicture/gles
        -D OUTPUT=${imm-player}/layerRenderers/layerRendererPicture/tmp/shader_layerRendererPicture_GLES_glsl.h
        -D NAMESPACE=INVALID
        -P ${CMAKE_CURRENT_SOURCE_DIR}/create_shader.cmake
)

add_dependencies(immplayer ShadersPicture)
]]

#[[
# Add immcore project dependency
set(imm-core-build ${imm-root}/code/projects/android/ImmAndroid/immcore/build)
file(MAKE_DIRECTORY ${imm-core-build})
include_directories(${imm-code})
add_subdirectory( ${imm-android}/immcore ${imm-core-build} )
add_library(immcore-lib STATIC IMPORTED )
]]

# Add immimporter project dependency
set(imm-importer-build ${imm-root}/code/projects/android/ImmAndroid/immimporter/build)
file(MAKE_DIRECTORY ${imm-importer-build})
include_directories(${imm-code})
add_subdirectory( ${imm-android}/immimporter ${imm-importer-build} )
add_library(immimporter-lib STATIC IMPORTED )
#[[
set_target_properties( immimporter-lib PROPERTIES IMPORTED_LOCATION
        ${imm-android}/immimporter/libs/libimmimporter.a
)
]]
#  Add audio 360 library dependencies
set( audio360sdk-dir ${CMAKE_SOURCE_DIR}/../../../../../../../../first-party-sdks/Audio360_SDK/1.7.10-5ed066141c02 )
include_directories(${imm-third-party}/audio360/include)
add_library( audio360-lib SHARED IMPORTED )
set_target_properties(
audio360-lib PROPERTIES IMPORTED_LOCATION
${imm-third-party}/audio360/lib/libAudio360.so
)


find_library( android-lib android )
find_library( log-lib log )
find_library( gl3-lib GLESv3 )
find_library( opensl-lib OpenSLES )

# ------------ Link libraries -------------

target_link_libraries(
    immplayer
    immimporter
    audio360-lib
    ${android-lib}
    ${gl3-lib}
    ${log-lib}
    ${opensl-lib} # Needed for linking libAudio360.a
)

set(imm-player-install-dir ${imm-root}/code/projects/android/ImmAndroid/immplayer/libs)
file(MAKE_DIRECTORY ${imm-player-install-dir})
set_target_properties(immplayer PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY  ${imm-player-install-dir}) # LIBRARY for .so, ARCHIVE for .a file
