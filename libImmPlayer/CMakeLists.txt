# Sets the minimum version of CMake required to build your native library.
# This ensures that a certain set of CMake features is available to
# your build.

cmake_minimum_required(VERSION 3.4.1)

# Specifies a library name, specifies whether the library is STATIC or
# SHARED, and provides relative paths to the source code. You can
# define multiple libraries by adding multiple add.library() commands,
# and CMake builds them for you. When you build your app, Gradle
# automatically packages shared libraries with your APK.

set( libImmPlayer-dir ${CMAKE_CURRENT_SOURCE_DIR})
set( shaderUtil-dir ${CMAKE_CURRENT_SOURCE_DIR} )

add_library(
    # Specifies the name of the library.
    libImmPlayer

    # Sets the library as a shared library.
    STATIC

    # src/main/cpp/native-lib.cpp # for android_native_app_glue android_main implementation
    ${libImmPlayer-dir}/src/player.cpp
    ${libImmPlayer-dir}/src/document.cpp
    ${libImmPlayer-dir}/src/blue_noise.cpp
    ${libImmPlayer-dir}/src/mngrPlayer.cpp
    ${libImmPlayer-dir}/src/layerRenderers/layerRendererPaint/static/layerRendererPaintStatic.cpp
    ${libImmPlayer-dir}/src/layerRenderers/layerRendererPaint/pretessellated/layerRendererPaintPretessellated.cpp
    ${libImmPlayer-dir}/src/layerRenderers/layerRendererPicture/layerRendererPicture.cpp
    ${libImmPlayer-dir}/src/layerRenderers/layerRendererSound/layerRendererSound.cpp
    ${libImmPlayer-dir}/src/layerRenderers/layerRendererModel/layerRendererModel.cpp
    # ${libquillaudio360wrapper-dir}/src/piSoundEngineAudioSDKBackend.cpp
)

# ----------- Shader transformation ---------------------

add_custom_target(
    ShadersStatic
    COMMAND ${CMAKE_COMMAND}
    -D INPUT=${libImmPlayer-dir}/src/layerRenderers/layerRendererPaint/static/
    -D OUTPUT=${libImmPlayer-dir}/src/layerRenderers/layerRendererPaint/static/tmp/shader_static_brush_GLES_glsl.h
    -D NAMESPACE=INVALID
    -P ${shaderUtil-dir}/create_shader.cmake
)

add_dependencies(libImmPlayer ShadersStatic)

add_custom_target(
    ShadersPretessellated
    COMMAND ${CMAKE_COMMAND}
    -D INPUT=${libImmPlayer-dir}/src/layerRenderers/layerRendererPaint/pretessellated/
    -D OUTPUT=${libImmPlayer-dir}/src/layerRenderers/layerRendererPaint/pretessellated/tmp/shader_pretessellated_brush_GLES_glsl.h
    -D NAMESPACE=INVALID
    -P ${shaderUtil-dir}/create_shader.cmake
)

add_dependencies(libImmPlayer ShadersPretessellated)


add_custom_target(
    ShadersModel
    COMMAND ${CMAKE_COMMAND}
    -D INPUT=${libImmPlayer-dir}/src/layerRenderers/layerRendererModel/gles
    -D OUTPUT=${libImmPlayer-dir}/src/layerRenderers/layerRendererModel/tmp/shader_model_GLES_glsl.h
    -D NAMESPACE=INVALID
    -P ${shaderUtil-dir}/create_shader.cmake
)

add_dependencies(libImmPlayer ShadersModel)

add_custom_target(
    ShadersPicture
    COMMAND ${CMAKE_COMMAND}
    -D INPUT=${libImmPlayer-dir}/src/layerRenderers/layerRendererPicture/gles
    -D OUTPUT=${libImmPlayer-dir}/src/layerRenderers/layerRendererPicture/tmp/shader_layerRendererPicture_GLES_glsl.h
    -D NAMESPACE=INVALID
    -P ${shaderUtil-dir}/create_shader.cmake
)

add_dependencies(libImmPlayer ShadersPicture)

# ------------ Add Core project dependency -------------

set( core-dir ${CMAKE_CURRENT_SOURCE_DIR}/../libCore )
set( core-build-dir ${core-dir}/build )
file( MAKE_DIRECTORY ${core-build-dir} )


# TODO: need to use real path
include_directories(
    ${core-dir}/..
)

#add_subdirectory( ${core-app-dir} ${core-build-dir} )

add_library( libCore STATIC IMPORTED )
set_target_properties( libCore PROPERTIES IMPORTED_LOCATION
    ${core-build-dir}/libs/${ANDROID_ABI}/liblibCore.a )

# ------------ Add ImmImportor project dependency -------------

set( imm-importer-dir ${CMAKE_CURRENT_SOURCE_DIR}/../libImmImporter )
set( imm-importer-build-dir ${imm-importer-dir}/build )
file( MAKE_DIRECTORY ${imm-importer-build-dir} )


# TODO: need to use real path

#add_subdirectory( ${core-app-dir} ${core-build-dir} )

add_library( libImmImporter STATIC IMPORTED )
set_target_properties( libImmImporter PROPERTIES IMPORTED_LOCATION
    ${imm-importer-build-dir}/libs/${ANDROID_ABI}/liblibImmImporter.a )

# ------------ Add prebuilt library dependencies -------------

find_library( android-lib android )
find_library( log-lib log )
find_library( gl3-lib GLESv3 )
find_library( opensl-lib OpenSLES )
find_library( z-lib z )

set( audio360sdk-dir ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../first-party-sdks/Audio360_SDK/1.7.10-5ed066141c02 )
include_directories(
    ${audio360sdk-dir}/Audio360/include/
)
add_library( audio360-lib SHARED IMPORTED )
set_target_properties(
    audio360-lib PROPERTIES IMPORTED_LOCATION
    ${audio360sdk-dir}/Audio360/Android/${ANDROID_ABI}/libAudio360.so )

# ------------ Link libraries -------------

target_link_libraries(
    libImmPlayer
    libImmImporter
    libCore
    #audio360-lib
    ${android-lib}
    ${gl3-lib}
    ${log-lib}
    ${opensl-lib} # Needed for linking libAudio360.a
    ${z-lib} # fromImmersiveLayerPaint uses zlib
)

file( MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/libs/${ANDROID_ABI} )
set_target_properties(libImmPlayer PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY  ${CMAKE_CURRENT_SOURCE_DIR}/build/libs/${ANDROID_ABI})
