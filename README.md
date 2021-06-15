# Introduction

The immersive media (Imm) is an API-neutral runtime immersive media delivery format. Imm provides an efficient, extensible, interoperable format for the transmission and loading of immersive 3D content.

## Context

With the advent of VR/AR technology and platforms, new classes of 3d creation software in VR have emerged. The fundamental difference between 3d creation in VR and a traditional 3d software is the dimension of the input. On VR platforms, users have 6 degrees of freedom to control the input. As a result, it enables a new kind of art form - 3d drawings. An art form that the drawing is composed of strokes in 3d space. An innovative file format has to be built to support the art.

Beyond the need for efficient delivery of 3d drawings, many drawing applications and 3d content asset pipelines can benefit from a standard, interoperable format to enable sharing and reuse of immersive media between applications.The format combines an easily parsable scene graph, animation timeline and binary representation of images, geometry, audios. IMM is able to faithfully preserve scenes with 2D and 360 imagery, 3D polygon meshes, audio and animations, and also custom data such as stroke based 3D drawing, while enabling efficient delivery.

## IMM Basics

IMM files store binary data. Scenegraph and metadata is uncompressed binary for efficient parsing of large files. Asset data is binary compressed for efficient storage, and only read on demand as needed by the scenegraph. This enables loading very large animation files in sub-second times..

## Versioning

Any updates made to Imm in a minor version will be backwards and forwards compatible. Backwards compatibility will ensure that any client implementation that supports loading a Imm x.y asset will also be able to load a Imm x.0 asset. Forward compatibility will allow a client implementation that only supports Imm x.0 to load Imm x.y assets while gracefully ignoring any new features it does not understand.

A minor version update can introduce new features but will not change any previously existing behavior. Existing functionality can be deprecated in a minor version update, but it will not be removed.

Major version updates are not expected to be compatible with previous versions.

## File Extensions and MIME Types

IMM is flexible and allows for extension to custom data types bedong 2D images, 3D images and 3D meshes. For example, the VR animation tool &quot;Quill&quot; generates 3D stroke data that can be compressed inside IMM in its own special storage block.

# Project architecture

##
 Modules

The IMM project contains a set of libraries, binaries and extra files. You&#39;ll find them all at the root of the IMM/ folder:

Libraries

| libCore | OS services, containers, Rendering, Sound, etc |
| --- | --- |
| libImmExporter | Exporting a scene graph into IMM |
| libImmImporter | Reading an IMM file into memory |
| libImmPlayer | Rendering and playing back an IMM loaded with libImmImporter |

Binaries:

| appImmViewer | An IMM player, built on top of libImmPlayer |
| --- | --- |
| appImmUnity | A Unity IMM player plugin, build on top of libImmPlayer |
| appDX11ShaderCompiler | A command line utility to compile DX11 shader, needed by libImmPlayer |

Others:

| project | Project/Solution files for Windows and Android |
| --- | --- |
| documentation | This documentation, license notices, etc |

## Dependencies

This is the dependency hierarchy for IMM playback solutions

![fig1](/docs/fig1.png)

This is the dependency hierarchy for IMM import and export pipelines. Note, we do recommend NOT importing IMM files for further art authoring, since IMM has already been optimized for storage, transmission and playback. Think of the IMM as a JPG - if you want to modify its content, you probably want to do it in the source PSD or PNG file and re-export to JPG again.

![fig2](/docs/fig2.png)

# Building the Libraries

IMM can be built on Windows 10 both. Import/export libraries work on any Windows machine. The ImmViewer works both on Mono and in VR (either with Oculus RIFT or Oculus Quest + Link) or for monoscopic rendering.

## On Windows

Visual Studio instructions

## On Android (for Quest)

1. Download Android Studio (version \&gt; 4.1.0) from developer.android.com.

2. Open folder thirdparty/lpng1637/projects/androidstudio in Android Studio first and build the project.

2. Open projects/android/ in android studio and build libCore, libImmImporte, libImmPlayer, appImmViewer in order.

3. Connect your Quest or Quest2 Device to your computer and follow the Developer Guide to enable Developer mode on your device. There will be a Quest device showing up on your Android Studio.

4. Add a new Android App runnable in Configurations. Set the module as android.appImmViewer and click the play button on the menu bar.

#


# Concepts

## Chunks

The IMM format is composed of binary resource chunks and thus can be extended and modified while maintaining backward compatibility.

Each chunk starts with a 8 byte signature to indicate the type of the chunk. It follows with another 8 byte uint64 to indicate the size of the chunk.

#### IMM Header Chunk

Each IMM file must have a header at the beginning of the file to make it a valid imm file.

Besides the common header of the chunk. It should contain a version number stored as a 4 bytes integer.

The first 2 bytes indicate the major version.

The next 2 bytes indicate the minor version.

Example:

8 byte char Immersiv

8 byte int64 size 4

4 byte int64 version 0x00010001

#### Category Chunk

IMM files usually have a category chunk following the header chunk.

The size of the content is also 5 bytes.

The first byte is an integer value indicating the type of imm file.

0 means a static immersive media file.

1 means an animated immersive media file.

2 means a story based immersive media file.

The second byte is an integer value indicating the capabilities of immersive media.

Each bit of the byte is a binary mask. The integer is stored in little endian encoding.

The first bit indicates whether the immersive media is grabbable in the player.

The second bit indicates whether the immersive media has any audio in it.

The rest bits are reserved for future use.

The third, fourth, fifth bytes are reserved for future use.

Example:

8 byte char signature Category

8 byte uint64 size 5

1 byte uint8 type 0x01

1 byte uint8 capabilities 0x01

2 byte uint16 dummy 0x0000

1 byte uint8 dummy 0x00

#### Performance Chunk

This is a 32 bytes chunk.

The first byte indicates the performance requirement for playing the imm file.

Example:

8 byte char signature Performance

8 byte uint64 size 32

8 bytes uint64 Memory 1000

8 bytes uint64 Max Drawcall 1000

8 bytes uint64 Max Triangle 1000

8 bytes uint64 Max Sound Channel 1000

#### Coordinate System and Unit Chunk

IMM files must have a coordinate system chunk specifying the unit system and axis system.

This is a 2 bytes chunk.

The first byte indicates the measurement unit.

0 means meter.

1 means centimeter.

The second bytes indicates the axis direction.

Value 0 defines a right-handed coordinate system, that is, the cross product of +X and +Y yields +Z. It also defines +Y as up. The front of the imm faces +Z.

![](RackMultipart20210615-4-v21ml_html_8a27568c4d3e112e.png)

Other coordinate systems are not defined yet.

Example:

8 byte char signature CoordSys

8 byte uint64 size 2

1 byte uint8 units 0x00

1 byte uint8 axis 0x00

#### Sequence Chunk

Sequence stores the information of the root node of the IMM scene graph except information stored in the following layer chunk.

The chunk starts with defaultViewpoint&#39;s name as string.

String in IMM file is stored in the following way:

4 byte int32 n represents the length of the string, then it is followed by a n bytes char array representing the content of the string.

Then a vec of 3 float represents the rgb value of the color of the background.

The sequence chunk is followed by a layer chunk, which is the root node of the scene graph.

Example:

8 byte char signature Sequence

8 byte uint64 size 23

4 byte uint32 len 0x00000007

7 bytes string default

12 bytes 3 float color [1.0,1.0,1.0]

#### Layer Chunk

Layer represents a single node on the scene graph.

Example:

8 bytes char signature Layerdes

8 bytes uint64 size 23

8 bytes uint64 version 0x00000001

65 bytes transform …..

65 bytes pivotTransform …..

4 bytes float opacity 1.0

1 bytes hasTimeline true

8 bytes int64 TimelineDuration 100

4 bytes int32 maxRepeatCount 1

Variant bytes string name

4 byte uint32 len 0x00000007

7 bytes char array default

Variant bytes string type

4 byte uint32 len 0x00000014

14 bytes char array Immersiv.Group

4 bytes int32 assetId 1

\&lt;Timeline Chunk\&gt;

If type is group

4 bytes int32 number of children n

\&lt;n Children Layer Chunk - Recursive\&gt;

\&lt;Specific Layer Chunk According to layer type\&gt;

Layer chunk starts with an int64 version number. It is always 1 for the current version of IMM.

It also contains the transform information relative to its parent layer and the transform information of its pivot point.

Transforms in IMM are stored in this way:

Transform [

DOUBLE[4] Rotation

DOUBLE Scale

DOUBLE[3] Translation

UINT8 Flip (0=none, 1=x, 2=y, 3=z)

]

The first 4 double represents the rotation with a quaternion.

Then the next double value is the scale which applies to all 3 axises.

The third line is a 3 double translation in cartesian coordinates system.

The end line is a 1 byte integer representing whether the transform flips the node aligned to a certain axis.

The next float is the opacity representing the transparency of the layer.

hasTimeline boolean represents whether the layer has timeline animation.

Then it follows an integer value meaning the timeline duration in the unit of frames.

maxRepeatCount means how many times the animation repeats if the layer has repeatable timeline animation.

Then it follows with two strings. The name represents the name of a layer. The type represents which type of content the layer has. The type of the layer will be introduced in later sections of the documents.

The type name must be one of the following options:

- Immersiv.Group
- Quill.Paint
- Quill.Model
- Quill.Picture
- Quill.Sound
- Quill.Viewpoint

The assetId is the unique identifier of the resource of the layer. It matches the id of the asset in asset table chunks.

If the layer has a timeline, the following chunk should be timeline chunk.

After that, if the type of layer is a group layer, then it is followed by a 4 bytes int32 meaning the number of children layers the group layer contains.

After the integer, there are n children layer chunks, which are exactly like the parent layer and a children layer might have its own children layers.

For other types of layer, it is followed by a specific layer chunk according to the type, it will be introduced in later sections.

#### Timeline Chunk

Timeline animation uses keyframes in a chronological order to store animations.

For each animation property, the timeline has a separate track for each one of them containing all the keyframes of that animation property.

Example:

8 bytes char signature Timeline

8 bytes uint64 size 23

8 bytes uint64 version 0x00000002

Properties [

1 byte int8, animation property id 0x0

8 byte int64, number of key frames 20

KeyFrames [

8 bytes uint64 time in tick (1/12600 s) 0

1 byte uint8 Interpolation type 0

Variant byte keyframe value according to the animation property id

]

]

The current version of the timeline of 2.

After the version number, the chunk stores a list of tracks.

**Note:** Every animation property has a track in timeline chunk. So the number of tracks is as same as the total type of animation properties in the current version.

For each track, it starts with 1 byte integer the animation property id.

| Id | Property type | KeyFrame type |
 |
| --- | --- | --- | --- |
| 0 | Visibility | Boolean | Whether the layer is displayed after the key |
| 1 | Opacity | Float | The opacity of the layer |
| 2 | Position | Vec3 Double | Deprecated in version 2 |
| 3 | Rotation | Vec4 Double | Deprecated in version 2 |
| 4 | Scale | Double | Deprecated in version 2 |
| 5 | DrawInTime | Double |
 |
| 6 | Action | Int32 |
 |
| 7 | Loop | Boolean |
 |
| 8 | Offset | Int32 |
 |
| 9 | Transform | 65 bytes structure | The transform of the layer |

Then it follows with a 8 bytes int64 to indicate how many keyframes are in the track.

Every keyframe has a interpolation type indicating how the value of the property is defined between this frame and the next keyframe. Interpolation type should be one of the following. (0=STEPPED, 1=LINEAR, 2=SMOOTHSTEP, 3=EASEIN, 3=EASEOUT)

The last part of a keyframe is its value, the size of the value is defined in the animation property above.

#### Resource Table Chunk




8 byte int64

### Scenes

## Layers

### Image Layer

### 3d Image

### 360 Image

### Audio Layer

### Paint Layer

### Spawn Layer

## Transformations

## Timeline and Animations

### Geometry Animation

### Layer Animation

## Binary Data Storage

### Paint

### Texture

### Sound

### Model

### Video

## Geometry

### Drawings

### Strokes

## Shading

### Unlit Shader

# Acknowledgements


