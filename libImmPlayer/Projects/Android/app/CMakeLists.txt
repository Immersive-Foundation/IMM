# Sets the minimum version of CMake required to build your native library.
# This ensures that a certain set of CMake features is available to
# your build.

cmake_minimum_required(VERSION 3.4.1)

# Specifies a library name, specifies whether the library is STATIC or
# SHARED, and provides relative paths to the source code. You can
# define multiple libraries by adding multiple add.library() commands,
# and CMake builds them for you. When you build your app, Gradle
# automatically packages shared libraries with your APK.

set( libImmPlayer-dir ${CMAKE_CURRENT_SOURCE_DIR}/../../../../libImmPlayer )
set( shaderUtil-dir ${ovrsource-root}/third-party/utils/Shaders )

add_library(
    # Specifies the name of the library.
    immplayer

    # Sets the library as a shared library.
    SHARED

    src/main/cpp/native-lib.cpp # for android_native_app_glue android_main implementation
    ${libImmPlayer-dir}/src/player.cpp
    ${libImmPlayer-dir}/src/document.cpp
    ${libImmPlayer-dir}/src/blue_noise.cpp
    ${libImmPlayer-dir}/src/mngrPlayer.cpp
    ${libImmPlayer-dir}/src/document/layerPaint/element.cpp
    ${libImmPlayer-dir}/src/document/layerPaint/drawingPretessellated.cpp
    ${libImmPlayer-dir}/src/document/layerPaint/drawingStatic.cpp
    ${libImmPlayer-dir}/src/document/layerPaint/drawingStaticTexture.cpp
    ${libImmPlayer-dir}/src/document/sequence.cpp
    ${libImmPlayer-dir}/src/document/layer.cpp
    ${libImmPlayer-dir}/src/document/behaviors/keepalive.cpp
    ${libImmPlayer-dir}/src/document/layerModel3d.cpp
    ${libImmPlayer-dir}/src/document/layerPicture.cpp
    ${libImmPlayer-dir}/src/document/layerSound.cpp
    ${libImmPlayer-dir}/src/document/layerInstance.cpp
    ${libImmPlayer-dir}/src/document/layerReference.cpp
    ${libImmPlayer-dir}/src/document/layerPaintStatic.cpp
    ${libImmPlayer-dir}/src/document/layerPaintPretessellated.cpp
    ${libImmPlayer-dir}/src/document/layerPaintStaticTexture.cpp
    ${libImmPlayer-dir}/src/document/layerViewpoint.cpp
    ${libImmPlayer-dir}/src/document/visibility.cpp
    ${libImmPlayer-dir}/src/layerRenderers/layerRendererPaint/static/layerRendererPaintStatic.cpp
    ${libImmPlayer-dir}/src/layerRenderers/layerRendererPaint/staticTexture/layerRendererPaintStaticTexture.cpp
    ${libImmPlayer-dir}/src/layerRenderers/layerRendererPaint/pretessellated/layerRendererPaintPretessellated.cpp
    ${libImmPlayer-dir}/src/layerRenderers/layerRendererPaint/tesselation/layerRendererPaintTesselation.cpp
    ${libImmPlayer-dir}/src/layerRenderers/layerRendererPaint/tesselation/mngrBrushes.cpp
    ${libImmPlayer-dir}/src/layerRenderers/layerRendererPaint/tesselation/mngrArrays.cpp
    ${libImmPlayer-dir}/src/layerRenderers/layerRendererPicture/layerRendererPicture.cpp
    ${libImmPlayer-dir}/src/layerRenderers/layerRendererSound/layerRendererSound.cpp
    ${libImmPlayer-dir}/src/layerRenderers/layerRendererModel/layerRendererModel.cpp
    ${libImmPlayer-dir}/src/readin/fromQuill/fromQuill.cpp
    ${libImmPlayer-dir}/src/readin/fromQuill/fromQuillLayer.cpp
    ${libImmPlayer-dir}/src/readin/fromQuill/fromQuillLayerPaint.cpp
    ${libImmPlayer-dir}/src/readin/fromQuill/fromQuillLayerModel.cpp
    ${libImmPlayer-dir}/src/readin/fromQuill/fromQuillLayerSound.cpp
    ${libImmPlayer-dir}/src/readin/fromQuill/fromQuillLayerPicture.cpp
    ${libImmPlayer-dir}/src/readin/fromQuill/fromQuillLayerViewpoint.cpp
    ${libImmPlayer-dir}/src/readin/fromImmersive/fromImmersive.cpp
    ${libImmPlayer-dir}/src/readin/fromImmersive/fromImmersiveLayer.cpp
    ${libImmPlayer-dir}/src/readin/fromImmersive/fromImmersiveLayerPaint.cpp
    ${libImmPlayer-dir}/src/readin/fromImmersive/fromImmersiveLayerModel.cpp
    ${libImmPlayer-dir}/src/readin/fromImmersive/fromImmersiveLayerSound.cpp
    ${libImmPlayer-dir}/src/readin/fromImmersive/fromImmersiveLayerPicture.cpp
    ${libImmPlayer-dir}/src/readin/fromImmersive/fromImmersiveLayerViewpoint.cpp
    ${libImmPlayer-dir}/src/readin/fromImmersive/fromImmersiveUtils.cpp
    ${libquillaudio360wrapper-dir}/src/piSoundEngineAudioSDKBackend.cpp
    )
    
# ----------- Shader transformation ---------------------

add_custom_target(
        ShadersStatic
        COMMAND ${CMAKE_COMMAND} 
        -D INPUT=${libImmPlayer-dir}/src/layerRenderers/layerRendererPaint/static/gles/
        -D OUTPUT=${libImmPlayer-dir}/src/layerRenderers/layerRendererPaint/static/tmp/shader_static_brush_GLES_glsl.h
        -D NAMESPACE=INVALID
        -P ${shaderUtil-dir}/create_shader.cmake
)

add_dependencies(quillplayer ShadersStatic)

add_custom_target(
        ShadersPretessellated
        COMMAND ${CMAKE_COMMAND} 
        -D INPUT=${libImmPlayer-dir}/src/layerRenderers/layerRendererPaint/pretessellated/gles/
        -D OUTPUT=${libImmPlayer-dir}/src/layerRenderers/layerRendererPaint/pretessellated/tmp/shader_pretessellated_brush_GLES_glsl.h
        -D NAMESPACE=INVALID
        -P ${shaderUtil-dir}/create_shader.cmake
)

add_dependencies(quillplayer ShadersPretessellated)

add_custom_target(
        ShadersStaticTexture
        COMMAND ${CMAKE_COMMAND} 
        -D INPUT=${libImmPlayer-dir}/src/layerRenderers/layerRendererPaint/staticTexture/gles/
        -D OUTPUT=${libImmPlayer-dir}/src/layerRenderers/layerRendererPaint/staticTexture/tmp/shader_static_texture_brush_GLES_glsl.h
        -D NAMESPACE=INVALID
        -P ${shaderUtil-dir}/create_shader.cmake
)

add_dependencies(quillplayer ShadersStaticTexture)

add_custom_target(
        ShadersModel
        COMMAND ${CMAKE_COMMAND} 
        -D INPUT=${libImmPlayer-dir}/src/layerRenderers/layerRendererModel/gles/
        -D OUTPUT=${libImmPlayer-dir}/src/layerRenderers/layerRendererModel/tmp/shader_model_GLES_glsl.h
        -D NAMESPACE=INVALID
        -P ${shaderUtil-dir}/create_shader.cmake
)

add_dependencies(quillplayer ShadersModel)

add_custom_target(
        ShadersPicture
        COMMAND ${CMAKE_COMMAND} 
        -D INPUT=${libImmPlayer-dir}/src/layerRenderers/layerRendererPicture/gles/
        -D OUTPUT=${libImmPlayer-dir}/src/layerRenderers/layerRendererPicture/tmp/shader_layerRendererPicture_GLES_glsl.h
        -D NAMESPACE=INVALID
        -P ${shaderUtil-dir}/create_shader.cmake
)

add_dependencies(quillplayer ShadersPicture)

# ------------ Add prebuilt library dependencies -------------

find_library( android-lib android )
find_library( log-lib log )
find_library( gl3-lib GLESv3 )
find_library( opensl-lib OpenSLES )
find_library( z-lib z )

add_library( jpeg-lib STATIC IMPORTED )
set_target_properties(
    jpeg-lib PROPERTIES IMPORTED_LOCATION
    ${piLibs-app-dir}/src/main/jniLibs/${ANDROID_ABI}/libturbojpeg.a )

set( audio360sdk-dir ${CMAKE_SOURCE_DIR}/../../../../../../../../first-party-sdks/Audio360_SDK/1.7.10-5ed066141c02 )
include_directories(
    ${audio360sdk-dir}/Audio360/include/
)
add_library( audio360-lib SHARED IMPORTED )
set_target_properties(
    audio360-lib PROPERTIES IMPORTED_LOCATION
    ${audio360sdk-dir}/Audio360/Android/${ANDROID_ABI}/libAudio360.so )

# Need native_app_glue to be loaded as dependency in Android native activity
add_library(native_app_glue STATIC
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

target_include_directories(quillplayer PRIVATE
        ${ANDROID_NDK}/sources/android/native_app_glue)

# ------------ Link libraries -------------

set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

target_link_libraries(
    immplayer
     "-Wl,--whole-archive"
    native_app_glue # Required for loading in Android native activity OVR app
    imm
    jpeg-lib
    "-Wl,--no-whole-archive"
    audio360-lib
    ${android-lib}
    ${gl3-lib}
    ${log-lib}
    ${opensl-lib} # Needed for linking libAudio360.a
    ${z-lib} # fromImmersiveLayerPaint uses zlib
)

# TODO: Get install of libImmPlayer.so to work

file( MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/outputs/libs )
install(TARGETS quillplayer
    LIBRARY DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/build/outputs/libs
)
