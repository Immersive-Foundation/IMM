# Sets the minimum version of CMake required to build your native library.
# This ensures that a certain set of CMake features is available to
# your build.

cmake_minimum_required(VERSION 3.4.1)

# Specifies a library name, specifies whether the library is STATIC or
# SHARED, and provides relative paths to the source code. You can
# define multiple libraries by adding multiple add.library() commands,
# and CMake builds them for you. When you build your app, Gradle
# automatically packages shared libraries with your APK.

add_library(
    # Specifies the name of the library.
    libCore

    # Sets the library as a static library.
    STATIC

    # Provides a relative path to your source file(s).
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libBasics/android/piLog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libBasics/piArray.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libBasics/piPool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libBasics/piString.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libBasics/piTick.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libBasics/piImage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libCompression/basic/piBitInterlacer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libCompression/basic/piPredictors.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libCompression/basic/piQuantize.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libCompression/basic/piTransforms.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libMesh/piMesh.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libMesh/piRenderMesh.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libRender/piRenderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libRender/opengles/piGLES_Renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libBasics/android/piFileOS.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libBasics/android/piTimer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libBasics/android/piMutex.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libBasics/android/piCStr.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libBasics/piStr.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libBasics/piFile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libBasics/piFileName.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libBasics/piTypes.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libWave/piWave.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libWave/formats/piWaveWAV.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libWave/formats/piWaveOGG.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libSound/piSoundEngineNull.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libSound/piSound.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libSound/windows/piSoundEngineAudioSDKBackend.cpp
)


#include_directories(
#    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../piLibs/dist/include
#    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../piLibs/src/libBasics/libWave/formats/
#    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../libjpeg-turbo/libjpeg-turbo/
#    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../libjpeg-turbo/include/android
#)

# audio 360
set( audio360sdk-dir ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../first-party-sdks/Audio360_SDK/1.7.10-5ed066141c02 )
include_directories(
    ${audio360sdk-dir}/Audio360/include/
)
add_library( audio360-lib SHARED IMPORTED )
set_target_properties(
    audio360-lib PROPERTIES IMPORTED_LOCATION
    ${audio360sdk-dir}/Audio360/Android/${ANDROID_ABI}/libAudio360.so )

# Add OGG dependency needed for loading IMM files which will convert anything over 128kb to OGG.
set( ogg-third-party ${CMAKE_SOURCE_DIR}/../thirdparty/ogg )
add_subdirectory( ${ogg-third-party} ${CMAKE_BINARY_DIR}/ogg-build )

include_directories(
    ${ogg-third-party}/include
    ${ogg-third-party}/libvorbis-1.3.5/include
    ${ogg-third-party}/libvorbis-1.3.5/lib
)

find_library( android-lib android )
find_library( log-lib log )
find_library( egl-lib EGL )
find_library( gl3-lib GLESv3 )
find_library( opensl-lib OpenSLES )



include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/lpng1637
)

add_library( png-lib STATIC IMPORTED )
set_target_properties(
    png-lib PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/../thirdparty/lpng1637/projects/androidstudio/pngandroid/build/intermediates/cmake/debug/obj/${ANDROID_ABI}/libpng16d.a
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/libjpeg-turbo/libjpeg-turbo/
    ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/libjpeg-turbo/include/android
)

add_library( jpeg-lib STATIC IMPORTED )
set_target_properties(
    jpeg-lib PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/../thirdparty/libjpeg-turbo/lib/${ANDROID_ABI}/libturbojpeg.a )

target_link_libraries(
    libCore
    vorbis # ogg - from third-party subdirectory dependency
    png-lib
    jpeg-lib
    audio360-lib
    ${log-lib}
    ${egl-lib}
    ${gl3-lib}
    ${android-lib} # Needed for linking libAudio360.a
    ${opensl-lib} # Needed for linking libAudio360.a
)

file( MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/libs/${ANDROID_ABI} )
set_target_properties(libCore PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY  ${CMAKE_CURRENT_SOURCE_DIR}/build/libs/${ANDROID_ABI})
