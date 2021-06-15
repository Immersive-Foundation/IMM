# Sets the minimum version of CMake required to build your native library.
# This ensures that a certain set of CMake features is available to
# your build.

cmake_minimum_required(VERSION 3.4.1)

# Specifies a library name, specifies whether the library is STATIC or
# SHARED, and provides relative paths to the source code. You can
# define multiple libraries by adding multiple add.library() commands,
# and CMake builds them for you. When you build your app, Gradle
# automatically packages shared libraries with your APK.

set( ovrsource-root $ENV{OVRSOURCE})
set( appImmPlayer-dir ${CMAKE_CURRENT_SOURCE_DIR})

set( ovrmobilesdk-dir ${ovrsource-root}/first-party-sdks/OculusMobileSDK )
set( ovrplatformsdk-dir ${ovrsource-root}/first-party-sdks/OculusPlatformSDK )
set( shaderUtil-dir ${ovrsource-root}/third-party/utils/Shaders )

# ------------- End Shaders ------------------------

add_library(
    appImmPlayer SHARED

    ${appImmPlayer-dir}/src/android/cpp/OvrApp.cpp
    ${appImmPlayer-dir}/src/android/cpp/GlHelpers.cpp
    ${appImmPlayer-dir}/src/android/cpp/Haptics.cpp

    ${appImmPlayer-dir}/src/simpleJSON/JSON.cpp
    ${appImmPlayer-dir}/src/simpleJSON/JSONValue.cpp

    ${appImmPlayer-dir}/src/settings.cpp
    ${appImmPlayer-dir}/src/viewer/viewer.cpp
    ${appImmPlayer-dir}/src/viewer/piCameraD.cpp
)

#  ------------ Add Oculus Mobile SDK includes -------------

include_directories(
    ${ovrmobilesdk-dir}/1.35.0/VrApi/Include
    ${ovrmobilesdk-dir}/1.35.0/1stParty/OVR/Include
)

# ------------ Add core project dependency -------------

set( core-dir ${CMAKE_CURRENT_SOURCE_DIR}/../libCore )
set( core-build-dir ${core-dir}/build )
file( MAKE_DIRECTORY ${core-build-dir} )

# TODO: need to use real path
include_directories(
    ${core-dir}/..
)

#add_subdirectory( ${core-app-dir} ${core-build-dir} )

add_library(libCore STATIC IMPORTED )
set_target_properties( libCore PROPERTIES IMPORTED_LOCATION
    ${core-build-dir}/libs/${ANDROID_ABI}/liblibCore.a )

# ------------ Add libImmImporter project dependency -------------

set( imm-importer-dir ${CMAKE_CURRENT_SOURCE_DIR}/../libImmImporter )
set( imm-importer-build-dir ${imm-importer-dir}/build )
file( MAKE_DIRECTORY ${imm-importer-build-dir} )

# TODO: need to use real path

#add_subdirectory( ${core-app-dir} ${core-build-dir} )

add_library( libImmImporter STATIC IMPORTED )
set_target_properties( libImmImporter PROPERTIES IMPORTED_LOCATION
    ${imm-importer-build-dir}/libs/${ANDROID_ABI}/liblibImmImporter.a )

# ------------ Add libImmImporter project dependency -------------

set( imm-player-dir ${CMAKE_CURRENT_SOURCE_DIR}/../libImmPlayer )
set( imm-player-build-dir ${imm-player-dir}/build )
file( MAKE_DIRECTORY ${imm-player-build-dir} )

add_library( libImmPlayer STATIC IMPORTED )
set_target_properties( libImmPlayer PROPERTIES IMPORTED_LOCATION
    ${imm-player-build-dir}/libs/${ANDROID_ABI}/liblibImmPlayer.a )

#  ------------ Add pre-compiled libvrapi.so -------------

add_library( vrapi SHARED IMPORTED )
set_target_properties(
    vrapi PROPERTIES
    IMPORTED_LOCATION
    ${ovrmobilesdk-dir}/1.35.0/VrApi/Libs/Android/${ANDROID_ABI}/Release/libvrapi.so )

# Need to manually copy libvrapi.so to CMake output directory so that it gets packaged with
# libDemoQuillPlayer and we can load it with System.loadLibrary("vrapi").
add_custom_command(TARGET appImmPlayer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${ovrmobilesdk-dir}/1.35.0/VrApi/Libs/Android/${ANDROID_ABI}/Release/libvrapi.so
        ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libvrapi.so
)

#  ------------ Add pre-compiled libovrplatformloader.so -------------

include_directories(
    ${ovrplatformsdk-dir}/13.0.0/Include
)

add_library( ovrplatformloader SHARED IMPORTED )
set_target_properties(
    ovrplatformloader PROPERTIES
    IMPORTED_LOCATION
    ${ovrplatformsdk-dir}/13.0.0/Android/libs/${ANDROID_ABI}/libovrplatformloader.so )

# Need to manually copy libovrplatformloader.so to CMake output directory so that it gets packaged with
# libDemoQuillPlayer
add_custom_command(TARGET appImmPlayer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${ovrplatformsdk-dir}/13.0.0/Android/libs/${ANDROID_ABI}/libovrplatformloader.so
        ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libovrplatformloader.so
)

#  ------------ Add pre-compiled libAudio360.so -------------

# Need to manually copy libAudio360.so to CMake output directory so that it gets packaged with
# libDemoQuillPlayer.
set( audio360-dir ${ovrsource-root}/first-party-sdks/Audio360_SDK/1.7.10-5ed066141c02/Audio360 )

add_custom_command(TARGET appImmPlayer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${audio360-dir}/Android/${ANDROID_ABI}/libAudio360.so
    ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libAudio360.so
    )
# ------------ Link libraries -------------

find_library( android-lib android )
find_library( log-lib log )
find_library( egl-lib EGL )
find_library( gl3-lib GLESv3 )
find_library( z-lib z )


# Add OGG dependency needed for loading IMM files which will convert anything over 128kb to OGG.
set( ogg-third-party ${CMAKE_SOURCE_DIR}/../thirdparty/ogg )
add_subdirectory( ${ogg-third-party} ${CMAKE_BINARY_DIR}/ogg-build )

add_library( png-lib STATIC IMPORTED )
set_target_properties(
    png-lib PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/../thirdparty/lpng1637/projects/androidstudio/pngandroid/build/intermediates/cmake/debug/obj/${ANDROID_ABI}/libpng16d.a
)

add_library( jpeg-lib STATIC IMPORTED )
set_target_properties(
    jpeg-lib PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/../thirdparty/libjpeg-turbo/lib/${ANDROID_ABI}/libturbojpeg.a )

set( audio360sdk-dir ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../first-party-sdks/Audio360_SDK/1.7.10-5ed066141c02 )

add_library( audio360-lib SHARED IMPORTED )
set_target_properties(
    audio360-lib PROPERTIES IMPORTED_LOCATION
    ${audio360sdk-dir}/Audio360/Android/${ANDROID_ABI}/libAudio360.so )

# Need native_app_glue to be loaded as dependency in Android native activity
add_library(native_app_glue STATIC
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

target_include_directories(appImmPlayer PRIVATE
    ${ANDROID_NDK}/sources/android/native_app_glue)

# ------------ Link libraries -------------

set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

target_link_libraries( appImmPlayer
        native_app_glue
        vrapi
        ovrplatformloader
        libImmPlayer
        libImmImporter
        libCore
        vorbis
        png-lib
        jpeg-lib
        audio360-lib
        ${egl-lib}
        ${gl3-lib}
        ${android-lib}
        ${log-lib}
        ${z-lib}
)
