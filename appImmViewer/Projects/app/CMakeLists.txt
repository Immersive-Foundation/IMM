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
set( ovrsource-imm ${ovrsource-root}/arvr/apps/Quill/IMM2 )

set( IMMViewer-dir ${ovrsource-imm}/exeViewer )
set( ovrmobilesdk-dir ${ovrsource-root}/first-party-sdks/OculusMobileSDK )
set( ovrplatformsdk-dir ${ovrsource-root}/first-party-sdks/OculusPlatformSDK )
set( shaderUtil-dir ${ovrsource-root}/third-party/utils/Shaders )

# ------------- End Shaders ------------------------

add_library(
    IMMViewer SHARED

    ${IMMViewer-dir}/src/Engine/android/OvrApp.cpp
    ${IMMViewer-dir}/src/Engine/android/GlHelpers.cpp

    ${IMMViewer-dir}/src/Engine/settings.cpp
    ${IMMViewer-dir}/src/Engine/viewer/viewer.cpp
    ${IMMViewer-dir}/src/Engine/viewer/ui/uiProgress.cpp
    ${IMMViewer-dir}/src/Engine/android/ui/controllers.cpp
    ${IMMViewer-dir}/src/Engine/android/ui/performancePanel.cpp
    ${IMMViewer-dir}/src/Engine/android/ui/viewpointsPanel.cpp
    ${IMMViewer-dir}/src/Engine/android/ui/upNextPanel.cpp
    ${IMMViewer-dir}/src/Engine/android/ui/messagePanel.cpp
    ${IMMViewer-dir}/src/Engine/android/ui/downloadComponent.cpp
)

#  ------------ Add Oculus Mobile SDK includes -------------

include_directories(
    ${ovrmobilesdk-dir}/1.35.0/VrApi/Include
    ${ovrmobilesdk-dir}/1.35.0/1stParty/OVR/Include
)

# ------------ Add libImmPlayer project dependency -------------

set( libImmPlayer-dir ${ovrsource-imm}/libImmPlayer )
set( libImmPlayer-app-dir ${libImmPlayer-dir}/Projects/Android/app )
set( libImmPlayer-build-dir ${libImmPlayer-dir}/Projects/Android/app/build )
file( MAKE_DIRECTORY ${libImmPlayer-build-dir} )

add_subdirectory( ${libImmPlayer-app-dir} ${libImmPlayer-build-dir} )
add_library( libImmPlayer SHARED IMPORTED )
include_directories(
    ${libImmPlayer-dir}/src
)
#set_target_properties( libImmPlayer PROPERTIES IMPORTED_LOCATION
#${libImmPlayer-build-dir}/${ANDROID_ABI}/libImmPlayer.so )


#  ------------ Add libQuillAudio360Wrapper includes -------------

set( libquillaudio360-dir ${ovrsource-imm}/libQuillAudio360Wrapper )
include_directories(
    ${libquillaudio360-dir}/src
)

#  ------------ Add piLibs includes -------------

set( piLibs-dir ${ovrsource-root}/third-party/piLibs )
include_directories(
    ${piLibs-dir}/dist/include
)

find_library( android-lib android )
find_library( log-lib log )
find_library( egl-lib EGL )
find_library( gl3-lib GLESv3 )

#  ------------ Add pre-compiled libvrapi.so -------------

add_library( vrapi SHARED IMPORTED )
set_target_properties(
    vrapi PROPERTIES
    IMPORTED_LOCATION
    ${ovrmobilesdk-dir}/1.35.0/VrApi/Libs/Android/${ANDROID_ABI}/Release/libvrapi.so )

# Need to manually copy libvrapi.so to CMake output directory so that it gets packaged with
# libDemoQuillPlayer and we can load it with System.loadLibrary("vrapi").
add_custom_command(TARGET IMMViewer POST_BUILD
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
add_custom_command(TARGET IMMViewer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${ovrplatformsdk-dir}/13.0.0/Android/libs/${ANDROID_ABI}/libovrplatformloader.so
        ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libovrplatformloader.so
)

#  ------------ Add pre-compiled libAudio360.so -------------

# Need to manually copy libAudio360.so to CMake output directory so that it gets packaged with
# libDemoQuillPlayer.
set( audio360-dir ${ovrsource-root}/first-party-sdks/Audio360_SDK/1.7.10-5ed066141c02/Audio360 )

add_custom_command(TARGET IMMViewer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${audio360-dir}/Android/${ANDROID_ABI}/libAudio360.so
        ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libAudio360.so
)

# ----------- Shader transformation ---------------------

add_custom_target(
        Shaders
        COMMAND ${CMAKE_COMMAND}
        -D INPUT=${IMMViewer-dir}/src/Engine/android/shaders
        -D OUTPUT=${IMMViewer-dir}/src/Engine/android/tmp/shaders.h
        -D NAMESPACE=INVALID
        -P ${shaderUtil-dir}/create_shader.cmake
)

add_dependencies(IMMViewer Shaders)

# ------------ Link libraries -------------

target_include_directories( IMMViewer PRIVATE
        ${ANDROID_NDK}/sources/android/native_app_glue )

target_link_libraries( IMMViewer
        vrapi
        ovrplatformloader
        ${egl-lib}
        ${gl3-lib}
        ImmPlayer
)

#  ------------ Add pre-compiled libpanelappsdk.so -------------

add_library( panelappsdk SHARED IMPORTED )
set_target_properties(
        panelappsdk PROPERTIES
        IMPORTED_LOCATION ${ovrsource-root}/arvr/first-party/sdks/internal/panelappsdk/libs/${ANDROID_ABI}/libpanelappsdk.so
)

add_library(
        QuillPanel SHARED
        ${ovrsource-root}/arvr/apps/Quill/Quill/exeQuillPlayer/Projects/Android/app/src/main/cpp/QuillPanelApp.cpp
)

target_include_directories(
        QuillPanel PRIVATE
        ${ovrsource-root}/arvr/first-party/sdks/internal/panelappsdk/headers
        ${ovrsource-root}/arvr/first-party/sdks/internal/panelappsdk/libs/${ANDROID_ABI}
)

target_compile_options(QuillPanel PRIVATE -fno-rtti)

target_link_libraries( QuillPanel
        panelappsdk
        ${android-lib}
        ${log-lib}
)
