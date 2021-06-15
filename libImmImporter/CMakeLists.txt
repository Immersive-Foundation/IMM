# Sets the minimum version of CMake required to build your native library.
# This ensures that a certain set of CMake features is available to
# your build.

cmake_minimum_required(VERSION 3.4.1)

# Specifies a library name, specifies whether the library is STATIC or
# SHARED, and provides relative paths to the source code. You can
# define multiple libraries by adding multiple add.library() commands,
# and CMake builds them for you. When you build your app, Gradle
# automatically packages shared libraries with your APK.

set( libImm-dir ${CMAKE_CURRENT_SOURCE_DIR})

add_library(
    # Specifies the name of the library.
    libImmImporter

    # Sets the library as a shared library.
    STATIC

    ${libImm-dir}/src/document/layerPaint/element.cpp
    ${libImm-dir}/src/document/layerPaint/drawingPretessellated.cpp
    ${libImm-dir}/src/document/layerPaint/drawingStatic.cpp
    ${libImm-dir}/src/document/sequence.cpp
    ${libImm-dir}/src/document/layer.cpp
    ${libImm-dir}/src/document/behaviors/keepalive.cpp
    ${libImm-dir}/src/document/layerModel3d.cpp
    ${libImm-dir}/src/document/layerPicture.cpp
    ${libImm-dir}/src/document/layerSound.cpp
    ${libImm-dir}/src/document/layerInstance.cpp
    ${libImm-dir}/src/document/layerPaintStatic.cpp
    ${libImm-dir}/src/document/layerPaintPretessellated.cpp
    ${libImm-dir}/src/document/layerSpawnArea.cpp
    ${libImm-dir}/src/fromImmersive/fromImmersive.cpp
    ${libImm-dir}/src/fromImmersive/fromImmersiveLayer.cpp
    ${libImm-dir}/src/fromImmersive/fromImmersiveLayerPaint.cpp
    ${libImm-dir}/src/fromImmersive/fromImmersiveLayerModel.cpp
    ${libImm-dir}/src/fromImmersive/fromImmersiveLayerSound.cpp
    ${libImm-dir}/src/fromImmersive/fromImmersiveLayerPicture.cpp
    ${libImm-dir}/src/fromImmersive/fromImmersiveLayerSpawnArea.cpp
    ${libImm-dir}/src/fromImmersive/fromImmersiveUtils.cpp
)
# ------------ Add Core project dependency -------------

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


# ------------ Add prebuilt library dependencies -------------

find_library( android-lib android )
find_library( log-lib log )
find_library( gl3-lib GLESv3 )
find_library( opensl-lib OpenSLES )
find_library( z-lib z )


set( audio360sdk-dir ${CMAKE_SOURCE_DIR}/../../../../../../../../first-party-sdks/Audio360_SDK/1.7.10-5ed066141c02 )
include_directories(
    ${audio360sdk-dir}/Audio360/include/
)
add_library( audio360-lib SHARED IMPORTED )
set_target_properties(
    audio360-lib PROPERTIES IMPORTED_LOCATION
    ${audio360sdk-dir}/Audio360/Android/${ANDROID_ABI}/libAudio360.so )


# ------------ Link libraries -------------

set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

target_link_libraries(
    libImmImporter
    libCore
    audio360-lib
    ${android-lib}
    ${gl3-lib}
    ${log-lib}
    ${opensl-lib} # Needed for linking libAudio360.a
    ${z-lib} # fromImmersiveLayerPaint uses zlib
)


file( MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/libs/${ANDROID_ABI} )
set_target_properties(libImmImporter PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY  ${CMAKE_CURRENT_SOURCE_DIR}/build/libs/${ANDROID_ABI})
