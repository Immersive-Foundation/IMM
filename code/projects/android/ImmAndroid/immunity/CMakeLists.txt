# Sets the minimum version of CMake required to build your native library.
# This ensures that a certain set of CMake features is available to
# your build.

cmake_minimum_required(VERSION 3.10.2)

# Specifies a library name, specifies whether the library is STATIC or
# SHARED, and provides relative paths to the source code. You can
# define multiple libraries by adding multiple add.library() commands,
# and CMake builds them for you. When you build your app, Gradle
# automatically packages shared libraries with your APK.
project("immunity")

set(imm-root $ENV{IMMROOT})
set(imm-unity ${imm-root}/code/appImmUnity/src)
set(imm-android ${imm-root}/code/projects/android/immAndroid)
set(imm-third-party ${imm-root}/../../third_party_libs)
set(imm-code ${imm-root}/code)

add_library(
    # Specifies the name of the library.
    ImmUnityPlugin

    # Sets the library as a shared library.
    SHARED

    ${imm-unity}/main.cpp

)

# Add immplayer dependency
set(imm-player-build ${imm-root}/code/projects/android/ImmAndroid/immplayer/build)
file(MAKE_DIRECTORY ${imm-player-build})
include_directories(${imm-code})
add_subdirectory( ${imm-android}/immplayer ${imm-player-build} )
add_library(immplayer-lib STATIC IMPORTED )


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
        ImmUnityPlugin
        immplayer
        audio360-lib
        ${android-lib}
        ${gl3-lib}
        ${log-lib}
        ${opensl-lib} # Needed for linking libAudio360.a
)

set(imm-unity-install-dir ${imm-root}/code/projects/android/ImmAndroid/immunity/libs)
file(MAKE_DIRECTORY ${imm-unity-install-dir})
set_target_properties(ImmUnityPlugin PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY  ${imm-unity-install-dir}) # LIBRARY for .so, ARCHIVE for .a file
