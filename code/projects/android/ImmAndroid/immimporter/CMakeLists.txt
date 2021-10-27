# Sets the minimum version of CMake required to build your native library.
# This ensures that a certain set of CMake features is available to
# your build.

cmake_minimum_required(VERSION 3.10.2)

# Specifies a library name, specifies whether the library is STATIC or
# SHARED, and provides relative paths to the source code. You can
# define multiple libraries by adding multiple add.library() commands,
# and CMake builds them for you. When you build your app, Gradle
# automatically packages shared libraries with your APK.
project("immimporter")

set(imm-root $ENV{IMMROOT})
set(imm-importer ${imm-root}/code/libImmImporter/src)
set(imm-android ${imm-root}/code/projects/android/immAndroid)
set(imm-third-party ${imm-root}/../../third_party_libs)

add_library(
    # Specifies the name of the library.
    immimporter

    # Sets the library as a shared library.
    STATIC

    ${imm-importer}/document/layer.cpp
    ${imm-importer}/document/layerEffect.cpp
    ${imm-importer}/document/layerInstance.cpp
    ${imm-importer}/document/layerModel3d.cpp
    ${imm-importer}/document/layerPaintStatic.cpp
    ${imm-importer}/document/layerPaintPretessellated.cpp
    ${imm-importer}/document/layerPicture.cpp
    ${imm-importer}/document/layerSound.cpp
    ${imm-importer}/document/layerSpawnArea.cpp
    ${imm-importer}/document/sequence.cpp
    ${imm-importer}/document/behaviors/keepalive.cpp
    ${imm-importer}/document/layerPaint/drawingStatic.cpp
    ${imm-importer}/document/layerPaint/drawingPretessellated.cpp
    ${imm-importer}/document/layerPaint/element.cpp
    ${imm-importer}/fromImmersive/fromImmersive.cpp
    ${imm-importer}/fromImmersive/fromImmersiveLayer.cpp
    ${imm-importer}/fromImmersive/fromImmersiveLayerModel.cpp
    ${imm-importer}/fromImmersive/fromImmersiveLayerPaint.cpp
    ${imm-importer}/fromImmersive/fromImmersiveLayerPicture.cpp
    ${imm-importer}/fromImmersive/fromImmersiveLayerSound.cpp
    ${imm-importer}/fromImmersive/fromImmersiveLayerSpawnArea.cpp
    ${imm-importer}/fromImmersive/fromImmersiveUtils.cpp
)
# Add immCore project dependency

set(imm-code ${imm-root}/code)
set(imm-core-build ${imm-root}/code/projects/android/ImmAndroid/immcore/build)
file(MAKE_DIRECTORY ${imm-core-build})
include_directories(${imm-code})
add_subdirectory( ${imm-android}/immcore ${imm-core-build} )
add_library(immcore-lib STATIC IMPORTED )
#set_target_properties( immcore-lib PROPERTIES IMPORTED_LOCATION
#        ${imm-android}/immcore/libs/libimmcore.a
        #${imm-core-output}/${ANDROID_ABI}/libimmcore.so
#)


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

#set(CMAKE_SHARED_LINKER_FLAGS
#   "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

target_link_libraries(
        immimporter
        immcore
        audio360-lib
        ${android-lib}
        ${gl3-lib}
        ${log-lib}
        ${opensl-lib} # Needed for linking libAudio360.a

)

set(imm-importer-install-dir ${imm-root}/code/projects/android/ImmAndroid/immimporter/libs)
file(MAKE_DIRECTORY ${imm-importer-install-dir})
set_target_properties(immimporter PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY  ${imm-importer-install-dir}) # LIBRARY for .so, ARCHIVE for .a file
