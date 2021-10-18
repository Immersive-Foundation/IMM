//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#include <string.h>
#include <malloc.h>

#include "../libBasics/piFile.h"
#include "../libBasics/piArray.h"
#include "../libBasics/piTArray.h"
#include "../libBasics/piVecTypes.h"
#include "../libBasics/piStreamI.h"
#include "../libBasics/piStreamFileI.h"
#include "../libBasics/piStreamArrayI.h"
#include "../libBasics/piStreamO.h"
#include "../libBasics/piStreamFileO.h"
#include "../libBasics/piStreamArrayO.h"
#include "piMesh.h"

namespace ImmCore {

static const unsigned int typeSizeof[] = {
    1, // UByte
    4, // Float
    4, // Int
    8, // Double
    2, // Int16
    2, // Half
};

piMesh::piMesh()
{
}

piMesh::~piMesh()
{
}

void piMesh::Init(void)
{
	mVertexData.mNumVertexArrays = 0;
	mFaceData.mNumIndexArrays = 0;
}

void piMesh::InitMove(const piMesh *src)
{
    mBBox = src->mBBox;
    mVertexData = src->mVertexData; // this copies the pointers too
    mFaceData = src->mFaceData; // this copies the pointers too
}

bool piMesh::Init( int numVertexStreams, int nv, const VertexFormat *vfs, Type type, int numElementsArrays, int numElements )
{
    if( (numElements<0) || (vfs[0].mNumElems>piMesh_MAXELEMS) ) return false;
    if( (type==Type::Polys) && (nv<3) ) return false;

    memset( this, 0, sizeof(piMesh) );


    mVertexData.mNumVertexArrays = 0;//numVertexStreams;
    for( int j=0; j<numVertexStreams; j++ )
    {
        if( !this->AddVertexStream( nv, vfs+j ) )
            return false;
    }

    //--------------------
    mFaceData.mNumIndexArrays = numElementsArrays;
    mFaceData.mType = type;
    mFaceData.mDepth = FaceData::Depth::Bits32;
    for( int i=0; i<numElementsArrays; i++ )
    {
		mFaceData.mIndexArray[i].mMax  = numElements;
		mFaceData.mIndexArray[i].mNum  = numElements;
		if( numElements>0 )
		{
            const int bufferSize = numElements * ( (mFaceData.mType==Type::Polys) ? sizeof(Face32) : sizeof(unsigned int) );
			mFaceData.mIndexArray[i].mBuffer = (Face32*)malloc( bufferSize );
			if( !mFaceData.mIndexArray[i].mBuffer )
			{
				return false;
			}
			memset( mFaceData.mIndexArray[i].mBuffer, 0, bufferSize );
		}
    }
    return true;
}

bool piMesh::AddVertexStream( const int nv, const VertexFormat *vf )
{
    const int id = mVertexData.mNumVertexArrays;

    mVertexData.mVertexArray[id].mMax = nv;
    mVertexData.mVertexArray[id].mNum = nv;
    mVertexData.mVertexArray[id].mFormat = *vf;

    mVertexData.mVertexArray[id].mBuffer = malloc( nv*vf->mStride );
    if( !mVertexData.mVertexArray[id].mBuffer )
        return false;
    memset( mVertexData.mVertexArray[id].mBuffer, 0, nv*vf->mStride );

    int off = 0;
    for( int i=0; i<vf->mNumElems; i++ )
    {
        mVertexData.mVertexArray[id].mFormat.mElems[i].mOffset = off;
        off  += vf->mElems[i].mNumComponents*typeSizeof[ static_cast<int>(vf->mElems[i].mType) ];
    }

    mVertexData.mNumVertexArrays = id + 1;

    return true;
}

bool piMesh::Clone(piMesh *dst ) const
{
    const int nv = mVertexData.mVertexArray[0].mNum;
    const int numElements = mFaceData.mIndexArray[0].mNum;

    const VertexFormat *vf = &mVertexData.mVertexArray[0].mFormat;

    if( !dst->Init( 1, nv, vf, mFaceData.mType, mFaceData.mNumIndexArrays, numElements ) )
        return false;

    const int bufferSize = numElements * ((mFaceData.mType == Type::Polys) ? sizeof(Face32) : sizeof(unsigned int));

    memcpy(dst->mVertexData.mVertexArray[0].mBuffer, mVertexData.mVertexArray[0].mBuffer, nv*vf->mStride);
    memcpy(dst->mFaceData.mIndexArray[0].mBuffer, mFaceData.mIndexArray[0].mBuffer, bufferSize );
    return true;
}

void piMesh::DeInit( void )
{
    for( int i=0; i<mVertexData.mNumVertexArrays; i++ )
    {
        if( mVertexData.mVertexArray[i].mMax ) 
            free( mVertexData.mVertexArray[i].mBuffer );
    }
    for( int i=0; i<mFaceData.mNumIndexArrays; i++ )
    {
        if( mFaceData.mIndexArray[i].mMax )   
           free( mFaceData.mIndexArray[i].mBuffer );
    }
}

void piMesh::GetStats( int *numIndices, int *numVertices, int *vertexFormatSize ) const
{
    *numIndices = mFaceData.mIndexArray->mNum;
    *numVertices = mVertexData.mVertexArray[0].mNum;
    *vertexFormatSize = mVertexData.mVertexArray[0].mFormat.mStride;
}

bool piMesh::iWrite(piOStream & fp) const
{
    fp.WriteUInt32(0); // version

    fp.WriteFloatarray((float*)&mBBox, 6);

    fp.WriteUInt32(mVertexData.mNumVertexArrays);

    for (int j = 0; j<mVertexData.mNumVertexArrays; j++)
    {
        const VertexArray *va = mVertexData.mVertexArray + j;
        fp.WriteUInt32(va->mNum);
        fp.WriteUInt32(va->mFormat.mStride);
        fp.WriteUInt32(va->mFormat.mDivisor);
        fp.WriteUInt32(va->mFormat.mNumElems);

        for (int i = 0; i<va->mFormat.mNumElems; i++)
        {
            fp.WriteUInt32((int)va->mFormat.mElems[i].mType);
            fp.WriteUInt32(va->mFormat.mElems[i].mNumComponents);
            fp.WriteUInt32(va->mFormat.mElems[i].mNormalize ? 1 : 0);
            fp.WriteUInt32(va->mFormat.mElems[i].mOffset);
        }

        if (va->mNum>0)
        {
            fp.WriteFloatarray((float*)va->mBuffer, va->mNum*va->mFormat.mStride / 4);
        }
    }

    const int esize = (mFaceData.mType == Type::Polys) ? sizeof(Face32) : sizeof(unsigned int);
    fp.WriteUInt32((mFaceData.mType == Type::Polys) ? 0 : 1);
    fp.WriteUInt32( static_cast<uint32_t>(mFaceData.mDepth));
    fp.WriteUInt32(mFaceData.mNumIndexArrays);
    for (int i = 0; i<mFaceData.mNumIndexArrays; i++)
    {
        fp.WriteUInt32(mFaceData.mIndexArray[i].mNum);

        if (mFaceData.mIndexArray[i].mNum>0)
        {
            fp.WriteUInt32array((unsigned int*)mFaceData.mIndexArray[i].mBuffer, mFaceData.mIndexArray[i].mNum*esize / 4);
        }
    }

    return true;
}

bool piMesh::iRead(piIStream &fp)
{
    const uint32_t version = fp.ReadUInt32();
    if( version!=0 ) return false;

    fp.ReadFloatarray((float*)&mBBox, 6);

    mVertexData.mNumVertexArrays = fp.ReadUInt32();

    for (int j = 0; j<mVertexData.mNumVertexArrays; j++)
    {
        VertexArray *va = mVertexData.mVertexArray + j;

        va->mNum = fp.ReadUInt32();
        va->mFormat.mStride = fp.ReadUInt32();
        va->mFormat.mDivisor = fp.ReadUInt32();
        va->mFormat.mNumElems = fp.ReadUInt32();
        va->mMax = va->mNum;

        for (int i = 0; i<va->mFormat.mNumElems; i++)
        {
            va->mFormat.mElems[i].mType = static_cast<piMesh::VertexElemDataType>(fp.ReadUInt32());
            va->mFormat.mElems[i].mNumComponents = fp.ReadUInt32();
            va->mFormat.mElems[i].mNormalize = (fp.ReadUInt32() == 1);
            va->mFormat.mElems[i].mOffset = fp.ReadUInt32();
        }

        if (va->mNum>0)
        {
            va->mBuffer = malloc(va->mNum*va->mFormat.mStride);
            if (!va->mBuffer)
                return false;

            fp.ReadFloatarray((float*)va->mBuffer, va->mNum*va->mFormat.mStride / 4);
        }

    }


    mFaceData.mType = static_cast<Type>(fp.ReadUInt32());
    mFaceData.mDepth = static_cast<FaceData::Depth>(fp.ReadUInt32());
    if(mFaceData.mDepth!= FaceData::Depth::Bits32) return false;

    mFaceData.mNumIndexArrays = fp.ReadUInt32();
    const int esize = (mFaceData.mType == Type::Polys) ? sizeof(Face32) : sizeof(unsigned int);
    for (int i = 0; i<mFaceData.mNumIndexArrays; i++)
    {
        mFaceData.mIndexArray[i].mNum = fp.ReadUInt32();
        mFaceData.mIndexArray[i].mMax = mFaceData.mIndexArray[i].mNum;
        if (mFaceData.mIndexArray[i].mNum>0)
        {
            mFaceData.mIndexArray[i].mBuffer = (Face32*)malloc(mFaceData.mIndexArray[i].mNum*esize);
            if (!mFaceData.mIndexArray[i].mBuffer)
                return false;

            fp.ReadUInt32array((unsigned int*)mFaceData.mIndexArray[i].mBuffer, mFaceData.mIndexArray[i].mNum*esize / 4);
        }
    }

    return true;
}

bool piMesh::WriteToFile(piFile * fp) const
{
    piOStreamFile st(fp);
    return iWrite(st);
}

bool piMesh::WriteToDisk( const wchar_t *name ) const
{
    piFile fp;
    if (!fp.Open(name, L"wb"))
        return false;

    piOStreamFile st(&fp);

    bool res = iWrite(st);

    fp.Close();

    return res;
}

bool piMesh::WriteToMemory(piTArray<uint8_t> *data) const
{
    piOStreamArray st(data);
    return iWrite(st);
}

bool piMesh::ReadFromDisk( const wchar_t *name )
{
    piFile fp;
    if( !fp.Open(name, L"rb") )
		return false;
    piIStreamFile st(&fp, piIStreamFile::kDefaultFileSize);
    bool res = iRead(st);
	fp.Close();
	return res;
}

bool piMesh::ReadFromFile(piFile * fp)
{
    piIStreamFile st(fp, piIStreamFile::kDefaultFileSize);
    return iRead(st);
}

bool piMesh::ReadFromMemory(piTArray<uint8_t> *data)
{
    piIStreamArray st(data);
    return iRead(st);
}

void piMesh::Normalize( int stream, int ppos, int npos )
{
    const int numv = mVertexData.mVertexArray[stream].mNum;
    const int numt = mFaceData.mIndexArray[0].mNum;

    for( int i=0; i<numv; i++ )
    {
        float *v = (float*)GetVertexData( stream, i, npos );
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 0.0f;
    }

    for( int i=0; i<numt; i++ )
    {
        Face32 *face = mFaceData.mIndexArray[0].mBuffer + i;

        const int ft = face->mNum;
        
        vec3 nor = vec3( 0.0f, 0.0f, 0.0f );
        for( int j=0; j<ft; j++ )
        {
            const vec3 va = *((vec3*)GetVertexData( stream, face->mIndex[ j      ], ppos ));
            const vec3 vb = *((vec3*)GetVertexData( stream, face->mIndex[(j+1)%ft], ppos ));

            nor += cross( va, vb );
        }

        for( int j=0; j<ft; j++ )
        {
            vec3 *n = (vec3*)GetVertexData( stream, face->mIndex[j], npos );
            n->x += nor.x;
            n->y += nor.y;
            n->z += nor.z;
        }
    }

    for( int i=0; i<numv; i++ )
    {
        vec3 *v = (vec3*)GetVertexData( stream, i, npos );
        *v = normalize( *v );
    }
}

void piMesh::CalcBBox( int stream, int pPos )
{
    mBBox = bound3(1e20f, -1e20f, 1e20f, -1e20f, 1e20f, -1e20f );

    const int num = mVertexData.mVertexArray[stream].mNum;
    for( int i=0; i<num; i++ )
    {
        float *v = (float*)GetVertexData( stream, i, pPos );
        if (v[0]<mBBox.mMinX) mBBox.mMinX = v[0];
        if (v[1]<mBBox.mMinY) mBBox.mMinY = v[1];
        if (v[2]<mBBox.mMinZ) mBBox.mMinZ = v[2];
        if (v[0]>mBBox.mMaxX) mBBox.mMaxX = v[0];
        if (v[1]>mBBox.mMaxY) mBBox.mMaxY = v[1];
        if (v[2]>mBBox.mMaxZ) mBBox.mMaxZ = v[2];
    }
}

bool piMesh::Expand( int nv, int nf )
{
    for( int j=0; j<mVertexData.mNumVertexArrays; j++ )
    {
        VertexArray *va = mVertexData.mVertexArray + j;

        const unsigned int newNum = va->mNum + nv;
        if( (va->mNum + nv) >= va->mMax )
        {
            const unsigned int newMax = va->mMax + ((nv<64)?64:nv);
            va->mBuffer = realloc( va->mBuffer, newMax*va->mFormat.mStride );
            if( !va->mBuffer ) 
                return false;
            va->mMax = newMax;
            va->mNum = newNum;
        }
        else
        {
            va->mNum = newNum;
        }
    }

    {
        const unsigned int newNum = mFaceData.mIndexArray[0].mNum + nf;
        if ((mFaceData.mIndexArray[0].mNum + nf) >= mFaceData.mIndexArray[0].mMax)
        {
            const unsigned int newMax = mFaceData.mIndexArray[0].mMax + ((nf<64) ? 64 : nf);
            mFaceData.mIndexArray[0].mBuffer = (Face32*)realloc(mFaceData.mIndexArray[0].mBuffer, newMax * sizeof(Face32));
            if (!mFaceData.mIndexArray[0].mBuffer)
                return false;
            mFaceData.mIndexArray[0].mMax = newMax;
            mFaceData.mIndexArray[0].mNum = newNum;
        }
        else
        {
            mFaceData.mIndexArray[0].mNum = newNum;
        }
    }

    return true;
}

int piMesh::GetVertexSize(uint32_t streamID)
{
    return mVertexData.mVertexArray[streamID].mFormat.mStride;
}

void piMesh::GetVertex(uint32_t streamID, uint32_t vertexID, void *data)
{
    piAssert(vertexID <mVertexData.mVertexArray[streamID].mMax);
    memcpy(data, (char*)mVertexData.mVertexArray[streamID].mBuffer + mVertexData.mVertexArray[streamID].mFormat.mStride*vertexID, mVertexData.mVertexArray[streamID].mFormat.mStride);
}

void piMesh::SetVertex(uint32_t streamID, uint32_t vertexID, void *data)
{
    piAssert( vertexID <mVertexData.mVertexArray[streamID].mMax);
    memcpy((char*)mVertexData.mVertexArray[streamID].mBuffer + mVertexData.mVertexArray[streamID].mFormat.mStride*vertexID, data, mVertexData.mVertexArray[streamID].mFormat.mStride );
}

void piMesh::SetTriangle(uint32_t streamID, uint32_t triangleID, uint32_t a, uint32_t b, uint32_t c )
{
    piAssert(triangleID < mFaceData.mIndexArray[streamID].mMax);
    piAssert(a <mVertexData.mVertexArray[0].mNum);
    piAssert(b <mVertexData.mVertexArray[0].mNum);
    piAssert(c <mVertexData.mVertexArray[0].mNum);

    Face32 *face = mFaceData.mIndexArray[streamID].mBuffer + triangleID;
    face->mNum = 3;
    face->mIndex[0] = a;
    face->mIndex[1] = b;
    face->mIndex[2] = c;
}

void piMesh::SetQuad(uint32_t streamID, uint32_t triangleID, uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    piAssert(triangleID < mFaceData.mIndexArray[streamID].mMax);
    piAssert(a <mVertexData.mVertexArray[0].mNum);
    piAssert(b <mVertexData.mVertexArray[0].mNum);
    piAssert(c <mVertexData.mVertexArray[0].mNum);
    piAssert(d <mVertexData.mVertexArray[0].mNum);

    Face32 *face = mFaceData.mIndexArray[streamID].mBuffer + triangleID;
	face->mNum = 4;
	face->mIndex[0] = a;
	face->mIndex[1] = b;
	face->mIndex[2] = c;
	face->mIndex[3] = d;
}


void *piMesh::GetVertexData(uint32_t streamID, uint32_t vertexID, uint32_t elementID ) const
{
    piAssert(vertexID <mVertexData.mVertexArray[streamID].mMax);
    return (void*)((char*)mVertexData.mVertexArray[streamID].mBuffer + vertexID*mVertexData.mVertexArray[streamID].mFormat.mStride + mVertexData.mVertexArray[streamID].mFormat.mElems[elementID].mOffset );
}

piMesh::Face32 *piMesh::GetFaceData(uint32_t indexArrayID, uint32_t indexID) const
{
    piAssert(indexID <mFaceData.mIndexArray[indexArrayID].mNum);
    return mFaceData.mIndexArray[indexArrayID].mBuffer + indexID;
}

}
