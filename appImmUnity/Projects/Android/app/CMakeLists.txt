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
set( ovrsource-quill ${ovrsource-root}/arvr/apps/Quill/IMM2 )
set( appImmUnity-dir ${ovrsource-quill}/appImmUnity)

add_library(
    # Specifies the name of the library.
    ImmViewerUnityPlugin

    # Sets the library as a shared library.
    SHARED

    ${appImmUnity-dir}/src/main.cpp
)

# ------------ Add libImmPlayer project dependency -------------

set( libImmPlayer-dir ${ovrsource-quill}/libImmPlayer )
set( libImmPlayer-app-dir ${libImmPlayer-dir}/Projects/Android/app )
set( libImmPlayer-build-dir ${libImmPlayer-dir}/Projects/Android/app/build )
file( MAKE_DIRECTORY ${libImmPlayer-build-dir} )

add_subdirectory( ${libImmPlayer-app-dir} ${libImmPlayer-build-dir} )
add_library( libImmPlayer SHARED IMPORTED )
include_directories(${libImmPlayer-dir}/src)
#set_target_properties( libImmPlayer PROPERTIES IMPORTED_LOCATION
#${libImmPlayer-build-dir}/${ANDROID_ABI}/libImmPlayer.so )

# ------------ Link libraries -------------

target_link_libraries(ImmViewerUnityPlugin
    ImmUnity
)
