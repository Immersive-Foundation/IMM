//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once

#include "../libBasics/piVecTypes.h"
#include "../libBasics/piTArray.h"
#include "../libBasics/piFile.h"
#include "../libBasics/piStreamI.h"
#include "../libBasics/piStreamO.h"

namespace ImmCore {


#define piMesh_MAXELEMS    8
#define piMesh_MAXINDEXARRAYS 48
#define piMesh_MAXVERTEXARRAYS 8

class piMesh
{
public:
    enum class Type : uint32_t
    {
        Polys = 0,
        Points = 1
    };

    enum class VertexElemDataType : uint32_t
    {
        UByte = 0,
        Float = 1,
        Int = 2,
        Double = 3,
        Int16 = 4,
        Half = 5
    };

    typedef struct
    {
        unsigned int        mNumComponents;
        VertexElemDataType  mType;
        bool                mNormalize;
        unsigned int        mOffset;
    }VertexElemInfo;

    typedef struct
    {
        int                       mStride;    // in bytes
        int                       mNumElems;
        int                       mDivisor;
        VertexElemInfo      mElems[piMesh_MAXELEMS];
    }VertexFormat;


    typedef struct
    {
        unsigned int	          mMax;
        unsigned int	          mNum;
        void                     *mBuffer;
        VertexFormat        mFormat;
    }VertexArray;

    typedef struct
    {
        int               mNumVertexArrays;
        VertexArray mVertexArray[piMesh_MAXVERTEXARRAYS];
    }VertexData;

    typedef struct
    {
        int mNum;
        uint32_t mIndex[4];  // 3 or 4... // might want to do this 64 bits in the future
    }Face32;

    typedef struct
    {
        uint32_t  mMax; // might want to do this 64 bits in the future
        uint32_t  mNum; // might want to do this 64 bits in the future
        Face32   *mBuffer;
    }IndexArray32;

    typedef struct
    {
        enum class Depth : uint32_t
        {
            Bits16 = 0,
            Bits32 = 1,
            Bits64 = 2,
        };

        Type             mType;
        Depth            mDepth;  // Bits32 for now
        int              mNumIndexArrays;
        IndexArray32     mIndexArray[piMesh_MAXINDEXARRAYS];
    }FaceData;


    piMesh();
    ~piMesh();

	void Init(void);
    void InitMove(const piMesh *src);
    bool Init( int numVertexStreams, int nv, const VertexFormat *vf, Type type, int numElementsArrays, int numElements );
    void DeInit( void );
    bool Expand( int nv, int nf );
    bool Clone( piMesh *dst ) const;

    void *GetVertexData(uint32_t streamID, uint32_t vertexID, uint32_t elementID ) const;
    Face32 *GetFaceData(uint32_t indexArrayID, uint32_t indexID ) const;
    bool AddVertexStream( const int nv, const VertexFormat *vf );
    void Normalize( int stream, int pPos, int npos );
    void CalcBBox( int stream, int pPos );

    bool ReadFromDisk( const wchar_t *name );
    bool ReadFromFile(piFile * file);
    bool ReadFromMemory(piTArray<uint8_t> *data);

    bool WriteToDisk(const wchar_t *name) const;
    bool WriteToFile(piFile * file) const;
    bool WriteToMemory(piTArray<uint8_t> *data) const;

    //-----------------------
    // Dynamic Construction
    int  GetVertexSize(uint32_t streamID);
    void GetVertex(uint32_t streamID, uint32_t vertexID, void *data);
    void SetVertex(uint32_t streamID, uint32_t vertexID, void *data);
	void SetTriangle(uint32_t streamID, uint32_t triangleID, uint32_t a, uint32_t b, uint32_t c);
	void SetQuad(uint32_t streamID, uint32_t triangleID, uint32_t a, uint32_t b, uint32_t c, uint32_t d);

private:
    bool iWrite(piOStream & fp) const;
    bool iRead(piIStream & fp);

public:
    bound3     mBBox;
    VertexData mVertexData;
    FaceData   mFaceData;
};



} // namespace ImmCore
