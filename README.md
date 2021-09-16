
# Introduction

The immersive media (IMM) is an API-neutral runtime immersive media delivery format. IMM provides an efficient, extensible, interoperable format for the transmission and loading of immersive 3D and 2D animated content of mixed media types (geometry, pictures, 360 panoramas, stroke based paintings, etc).

## Context

With the advent of VR/AR technology and platforms, filmic and animated storytelling can happen in 6 degrees of freedom, in that the viewer is located in the same space where the story is being told. Unlike traditional film or 3D animation where the final delivery format is 2D pixels, for VR storytelling the content needs to stay 3D until the very moment it is presented to the user. This requires the equivalent of a new video file format that can handle true immersion. Unlike depth-based 360 stereo video or light-fields based video, IMM is designed to transmit a true, full 3D description of the film.

This is achieved by honoring the original 3D nature of the content. 3D models, 3D paint strokes, voxel data, and other 3D content all have special containers inside IMM so that the playback engine can produce a true immersive rendition. In addition, IMM has containers for more traditional pieces of information such as 2D pictures (positioned in 3D space), audio, 360 backgrounds, etc. IMM comes also with a scenegraph and an animation timeline, so the playback engine can reproduce the film appropriately. All data types are heavily compressed for quick streaming of the data from the internet to the user's device.

The current IMM repository contains the IMM exporter and importer, as well as a reference playback engine.

IMM has been used to deliver a few dozen films, including the Tribeca film festival nominated "Rebels", the "Tale of Soda Island" series,  “The Remedy”, "Goodbye Mr. Octopus", "4 Stories" and many more.


## IMM Basics

IMM files store binary data. Scenegraph and animation metadata is uncompressed binary for easy streaming and rapid parsing of large files. This usually represents a negligible fraction of a film's file size. On the other hand, all asset data is binary compressed for efficient storage, with a specific compression tailored for each container type. The asset data is readable in random order, and it is recommended the playback engine loads it on demand and streamlined in and out of memory as needed by the scenegraph and animation timeline. The reference player in the Imm repository shows how to do this.

## Versioning

IMM is a living format, and it's expected to evolve rapidly together with the VR animation industry. Because of that, each data container comes with a versioning schema that can be used to keep backwards compatibility as needed.


# Project architecture

## Modules

The IMM project contains a set of libraries, binaries and extra files. You&#39;ll find them all at the root of the IMM/ folder:


|Name|Type|Description|
| --- | --- |--- |
|libImmExporter/ | library| Exporting a scene graph into IMM |
|libImmImporter/ | library| Reading an IMM file into memory |
|libImmPlayer/ | library| Reference player capable of plating IMM files (GL and DX renderers) |
|libCore/ | library| OS services, containers, Rendering, Sound, etc |
|appImmViewer/ | binary | An native IMM player for Windows, base on libImmPlayer |
|appImmUnity/ | binary | A Unity IMM player plugin, based on libImmPlayer |
|appDX11ShaderCompiler/ |binary | A command line utility to compile DX11 shader, needed by libImmPlayer |
|ImmUnitySampleProject/ | project | A Unity project showing how to use appImmUnity
|projects/| project| Contains all the Visual Studio and Android project files to build Imm


## Dependencies

This is the dependency hierarchy for IMM playback solutions

![fig1](/docs/fig1.png)

This is the dependency hierarchy for IMM import and export pipelines. Note, we do recommend NOT importing IMM files for further art authoring, since IMM has already been optimized for storage, transmission and playback. Think of the IMM as a JPG - if you want to modify its content, you probably want to do it in the source PSD or PNG file and re-export to JPG again.

![fig2](/docs/fig2.png)

# Building the Libraries

The ImmViewer works both on Mono and in VR (either with Oculus RIFT or Oculus Quest + Link) or for monoscopic rendering.

## On Android (for Quest)

1. Download Android Studio (version 4.1.0) from developer.android.com.

2. Open folder thirdparty/lpng1637/projects/androidstudio in Android Studio first and build the project.

2. Open projects/android/ in android studio and build libCore, libImmImporte, libImmPlayer, appImmViewer in order.

3. Connect your Quest or Quest 2 Device to your computer and follow the Developer Guide to enable Developer mode on your device. There will be a Quest device showing up on your Android Studio.

4. Add a new Android App runnable in Configurations. Set the module as android.appImmViewer and click the play button on the menu bar.




