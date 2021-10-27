# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.10.2)

# Declares and names the project.

project("immcore")
set(imm-root $ENV{IMMROOT})
set(imm-core-src ${imm-root}/code/libImmCore/src)
set(imm-third-party ${imm-root}/../../third_party_libs)

add_library( # Sets the name of the library.
             immcore

             # Sets the library as a shared library.
             STATIC

             # Provides a relative path to your source file(s).
             ${imm-core-src}/libBasics/android/piCStr.cpp
             ${imm-core-src}/libBasics/android/piFileOS.cpp
             ${imm-core-src}/libBasics/android/piTimer.cpp
             ${imm-core-src}/libBasics/android/piMutex.cpp
             ${imm-core-src}/libBasics/android/piLog.cpp
             ${imm-core-src}/libBasics/piArray.cpp
             ${imm-core-src}/libBasics/piPool.cpp
             ${imm-core-src}/libBasics/piString.cpp
             ${imm-core-src}/libBasics/piImage.cpp
             ${imm-core-src}/libBasics/piFileName.cpp
             ${imm-core-src}/libBasics/piTypes.cpp
             ${imm-core-src}/libBasics/piTick.cpp
             ${imm-core-src}/libBasics/piFile.cpp
             ${imm-core-src}/libBasics/piStr.cpp
             ${imm-core-src}/libCompression/basic/piBitInterlacer.cpp
             ${imm-core-src}/libCompression/basic/piPredictors.cpp
             ${imm-core-src}/libCompression/basic/piTransforms.cpp
             ${imm-core-src}/libMesh/piMesh.cpp
             ${imm-core-src}/libMesh/piRenderMesh.cpp
             ${imm-core-src}/libRender/piRenderer.cpp
             ${imm-core-src}/libRender/opengles/piGLES_Renderer.cpp
             ${imm-core-src}/libWave/piWave.cpp
             ${imm-core-src}/libWave/formats/piWaveOGG.cpp
             ${imm-core-src}/libWave/formats/piWaveWAV.cpp
             ${imm-core-src}/libSound/piSoundEngineNUll.cpp
             ${imm-core-src}/libSound/piSound.cpp
             ${imm-core-src}/libSound/windows/piSoundEngineAudioSDKBackend.cpp
        )

include_directories(
        ${imm-third-party}/libjpeg-turbo/libjpeg-turbo
        ${imm-third-party}/libjpeg-turbo/include/android
        ${imm-third-party}/png/include
        ${imm-third-party}/audio360/include
        #${imm-third-party}/opus/include
)

# Add OGG dependency needed for loading IMM files which will convert anything over 128kb to OGG.
set( ogg-third-party ${imm-third-party}/ogg/android )
add_subdirectory( ${ogg-third-party} ${CMAKE_BINARY_DIR}/ogg-build )

include_directories(
        ${ogg-third-party}/include
        ${ogg-third-party}/libvorbis-1.3.5/include
        ${ogg-third-party}/libvorbis-1.3.5/lib
)

# Add audio 360 dependency
add_library(audio360-lib SHARED IMPORTED)
set_target_properties(
        audio360-lib PROPERTIES IMPORTED_LOCATION
        ${imm-third-party}/audio360/lib/libAudio360.so
)

# Add jpeg dependency
add_library( jpeg-lib STATIC IMPORTED )
set_target_properties(
        jpeg-lib PROPERTIES IMPORTED_LOCATION
        ${imm-third-party}/libjpeg-turbo/libturbojpeg.a )

# Add png dependency
add_library( png-lib STATIC IMPORTED )
set_target_properties(
        png-lib PROPERTIES IMPORTED_LOCATION
        ${imm-third-party}/png/lib/Release/libpng16.a
)

find_library( android-lib android )
find_library( log-lib log )
find_library( egl-lib EGL )
find_library( gl3-lib GLESv3 )
find_library( opensl-lib OpenSLES )
find_library( z-lib z)

target_link_libraries(
        immcore
        audio360-lib
        vorbis # from third-party subdirectory dependency
        jpeg-lib
        png-lib
        z
        ${log-lib}
        ${egl-lib}
        ${gl3-lib}
        ${android-lib} # Needed for linking libAudio360.a
        ${opensl-lib} # Needed for linking libAudio360.a
)

set(imm-core-install-dir ${imm-root}/code/projects/android/ImmAndroid/immcore/libs)
file(MAKE_DIRECTORY ${imm-core-install-dir})
set_target_properties(immcore PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY  ${imm-core-install-dir}) # LIBRARY for .so, ARCHIVE for .a file
