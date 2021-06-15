//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <float.h>

#include "../libBasics/piFile.h"
#include "../libBasics/piVecTypes.h"
#include "piRenderMesh.h"

namespace ImmCore {


static piRArrayDataType dt2al( piMesh::VertexElemDataType dt )
{
    if( dt== piMesh::VertexElemDataType::UByte  ) return piRArrayType_UByte;
    if( dt== piMesh::VertexElemDataType::Float  ) return piRArrayType_Float;
    if( dt== piMesh::VertexElemDataType::Int    ) return piRArrayType_Int;
    if( dt== piMesh::VertexElemDataType::Half ) return piRArrayType_Half;
    if (dt == piMesh::VertexElemDataType::Int16) return piRArrayType_Short;
    return piRArrayType_Float;
}


piRenderMesh::piRenderMesh()
{
}

piRenderMesh::~piRenderMesh()
{
}

const bound3 & piRenderMesh::GetBBox( void ) const
{
    return mBBox;
}

const int piRenderMesh::GetNumVertices( void ) const
{
    return mVertexData.mStream[0].mLength;
}

const piBuffer piRenderMesh::GetVertexBuffer( int stream ) const
{
    return mVertexData.mStream[stream].mVBO;
}


void piRenderMesh::End(piRenderer *renderer)
{
    for (unsigned int i = 0; i < mVertexData.mNumStreams; i++)
    {
        piRenderMeshVertexStream *stream = mVertexData.mStream + i;
        renderer->DestroyBuffer(stream->mVBO);
    }

    if (mIndexData.mNumElementArrays == 0)
    {
        //renderer->DestroyVertexArray(mIndexData.mVertexArrayData.mVAO);
    }
    else
    {
        for (unsigned int i = 0; i < mIndexData.mNumElementArrays; i++)
        {
            if (mIndexData.mElementArray[i].mNum != 0)
            {
                for (int j = 0; j < mIndexData.mElementArray[i].mVertexArray.mNum; j++)
                {
                    renderer->DestroyVertexArray(mIndexData.mElementArray[i].mVertexArray.mVAO[i]);
                }
                renderer->DestroyBuffer(mIndexData.mElementArray[i].mIBO);
            }
        }
    }


}

bool piRenderMesh::InitFromMesh(piRenderer *renderer, const piMesh *mesh, piRenderer::PrimitiveType patchNum, bool *staticVertexStreams)
{
    if (!iInitBuffers(renderer, mesh, patchNum, staticVertexStreams))
        return false;

    if (mIndexData.mNumElementArrays == 0)
    {
        //mIndexData.mVAO = renderer->CreateVertexArray( mVertexData.mNumStreams, mVertexData.mStream[0].mVBO, &mVertexData.mStream[0].mLayout,
        //                                                                    mVertexData.mStream[1].mVBO, &mVertexData.mStream[1].mLayout,
        //                                                                  nullptr, piRenderer::IndexArrayFormat::UINT_32);
        //if (!mIndexData.mVAO)
        //  return false;
    }
    else
    {
        for (unsigned int i = 0; i<mIndexData.mNumElementArrays; i++)
        {
            mIndexData.mElementArray[i].mVertexArray.mNum = 1;
            for (int j = 0; j <mIndexData.mElementArray[i].mVertexArray.mNum; j++)
            {
                mIndexData.mElementArray[i].mVertexArray.mVAO[j] = renderer->CreateVertexArray(mVertexData.mNumStreams,
                    mVertexData.mStream[0].mVBO, &mVertexData.mStream[0].mLayout,
                    mVertexData.mStream[1].mVBO, &mVertexData.mStream[1].mLayout,
                    mIndexData.mElementArray[i].mIBO, piRenderer::IndexArrayFormat::UINT_32);
                if (!mIndexData.mElementArray[i].mVertexArray.mVAO[j])
                    return false;
            }
        }
    }

    return true;
}

static void al_to_al2(piRenderer::ArrayLayout2 *dst, const piRArrayLayout *src)
{
    dst->mNumElements = src->mNumElements;
    for (int i = 0; i < src->mNumElements; i++)
    {
        // name
        dst->mEntry[i].mName[0] = 'C';
        dst->mEntry[i].mName[1] = 'H';
        dst->mEntry[i].mName[2] = 'A';
        dst->mEntry[i].mName[3] = 'N';
        dst->mEntry[i].mName[4] = 'A' + i;
        dst->mEntry[i].mName[5] = 0;
        dst->mEntry[i].mFormat = piRenderer::Format::C3_32_FLOAT;
        // type
        if (src->mEntry[i].mType == piRArrayType_Float)
        {
            if (src->mEntry[i].mNumComponents == 1) dst->mEntry[i].mFormat = piRenderer::Format::C1_32_FLOAT;
            if (src->mEntry[i].mNumComponents == 2) dst->mEntry[i].mFormat = piRenderer::Format::C2_32_FLOAT;
            if (src->mEntry[i].mNumComponents == 3) dst->mEntry[i].mFormat = piRenderer::Format::C3_32_FLOAT;
            if (src->mEntry[i].mNumComponents == 4) dst->mEntry[i].mFormat = piRenderer::Format::C4_32_FLOAT;
        }
        dst->mEntry[i].mPerInstance = false;
    }
}

bool piRenderMesh::InitFromMeshWithShader(piRenderer *renderer, const piMesh *mesh, piRenderer::PrimitiveType patchNum,
    int numShader, const void **shaders, const int *shaderSizes, piLog *log)
{
    if (!iInitBuffers(renderer, mesh, patchNum, nullptr))
        return false;

    if (mIndexData.mNumElementArrays == 0)
    {
        //mIndexData.mVAO = renderer->CreateVertexArray( mVertexData.mNumStreams, mVertexData.mStream[0].mVBO, &mVertexData.mStream[0].mLayout,
        //                                                                    mVertexData.mStream[1].mVBO, &mVertexData.mStream[1].mLayout,
        //                                                                  nullptr, piRenderer::IndexArrayFormat::UINT_32);
        //if (!mIndexData.mVAO)
        //  return false;
    }
    else
    {
        for (unsigned int i = 0; i<mIndexData.mNumElementArrays; i++)
        {
            mIndexData.mElementArray[i].mVertexArray.mNum = numShader;
            for (int j = 0; j <mIndexData.mElementArray[i].mVertexArray.mNum; j++)
            {
                piRenderer::ArrayLayout2 ly0, ly1;
                al_to_al2(&ly0, &mVertexData.mStream[0].mLayout);
                if(mVertexData.mNumStreams>1)
                al_to_al2(&ly1, &mVertexData.mStream[1].mLayout);

                mIndexData.mElementArray[i].mVertexArray.mVAO[j] = renderer->CreateVertexArray2(mVertexData.mNumStreams,
                    mVertexData.mStream[0].mVBO, &ly0,
                    mVertexData.mStream[1].mVBO, &ly1,
                    shaders[j], shaderSizes[j],
                    mIndexData.mElementArray[i].mIBO,
                    piRenderer::IndexArrayFormat::UINT_32);

                if (!mIndexData.mElementArray[i].mVertexArray.mVAO[j])
                    return false;
            }
        }
    }

    return true;
}

bool piRenderMesh::iInitBuffers(piRenderer *renderer, const piMesh *mesh, piRenderer::PrimitiveType patchNum, bool *staticVertexStreams)
{
    memset( this, 0, sizeof(piRenderMesh) );
    mVertexData.mNumStreams = 0;
    mIndexData.mNumElementArrays = 0;

    //------------------------------------------------------------

    if (mesh->mFaceData.mType == piMesh::Type::Points && patchNum != piRenderer::PrimitiveType::Point) return false;
    //if (mesh->mFaceData.mType == piRMVEDT_Polys  && patchNum != piPT_Triangle) return false;


    mBBox = mesh->mBBox;

    mVertexData.mNumStreams = mesh->mVertexData.mNumVertexArrays;

    for( unsigned int k=0; k<mVertexData.mNumStreams; k++ )
    {
        const piMesh::VertexArray *mva = mesh->mVertexData.mVertexArray + k;
        piRenderMeshVertexStream *stream = mVertexData.mStream + k;

        stream->mLength = mva->mNum;
        stream->mLayout.mStride = mva->mFormat.mStride;
        stream->mLayout.mNumElements = mva->mFormat.mNumElems;
        stream->mLayout.mDivisor = mva->mFormat.mDivisor;
        stream->mVBO = 0;

        for( int j=0; j<mva->mFormat.mNumElems; j++ )
        {
            stream->mLayout.mEntry[j].mNumComponents = mva->mFormat.mElems[j].mNumComponents;
            stream->mLayout.mEntry[j].mType = dt2al( mva->mFormat.mElems[j].mType );
            stream->mLayout.mEntry[j].mNormalize = mva->mFormat.mElems[j].mNormalize;
        }

        stream->mBuffer = malloc(stream->mLength*stream->mLayout.mStride);
        if( !stream->mBuffer )
            return false;

        memcpy(stream->mBuffer, mesh->mVertexData.mVertexArray[k].mBuffer, stream->mLength*stream->mLayout.mStride);
    }

    //------------------------------------------------------------

    if (mesh->mFaceData.mType == piMesh::Type::Points)
    {
        mIndexData.mType = piRenderer::PrimitiveType::Point;
        mIndexData.mNumElementArrays = mesh->mFaceData.mNumIndexArrays;
        for( int i=0; i<mesh->mFaceData.mNumIndexArrays; i++ )
        {
            const int num = mesh->mFaceData.mIndexArray[i].mNum;
            mIndexData.mElementArray[i].mNum = num;
            if( num>0 )
            {
                mIndexData.mElementArray[i].mBuffer = (unsigned int*)malloc( num*sizeof(unsigned int) );
                if( !mIndexData.mElementArray[i].mBuffer )
                    return false;
                memcpy( mIndexData.mElementArray[i].mBuffer, mesh->mFaceData.mIndexArray[i].mBuffer, num*sizeof(unsigned int) );
            }
        }
        return true;
    }

    //------------------------------------------------------------
    mIndexData.mType = patchNum;
    mIndexData.mNumElementArrays = mesh->mFaceData.mNumIndexArrays;
    for( int i=0; i<mesh->mFaceData.mNumIndexArrays; i++ )
    {
        mIndexData.mElementArray[i].mNum = 0;
        if( mesh->mFaceData.mIndexArray[i].mNum>0 )
        {
            if (patchNum == piRenderer::PrimitiveType::QuadPatch)
            {
                const unsigned int nf = mesh->mFaceData.mIndexArray[i].mNum;
                mIndexData.mElementArray[i].mNum = nf * 4;
                mIndexData.mElementArray[i].mBuffer = (unsigned int*)malloc( mIndexData.mElementArray[i].mNum*sizeof(unsigned int) );
                if( !mIndexData.mElementArray[i].mBuffer )
                    return false;
                unsigned int *ind = mIndexData.mElementArray[i].mBuffer;
                for( unsigned int j=0; j<nf; j++ )
                {
                    ind[0] = mesh->mFaceData.mIndexArray[i].mBuffer[j].mIndex[0];
                    ind[1] = mesh->mFaceData.mIndexArray[i].mBuffer[j].mIndex[1];
                    ind[2] = mesh->mFaceData.mIndexArray[i].mBuffer[j].mIndex[2];
                    ind[3] = mesh->mFaceData.mIndexArray[i].mBuffer[j].mIndex[3];
                    ind += 4;
                }
            }
            else
            {
                const unsigned int nf = mesh->mFaceData.mIndexArray[i].mNum;
                int  nt = 0;
                for( unsigned int j=0; j<nf; j++ )
                {
                    const int fn = mesh->mFaceData.mIndexArray[i].mBuffer[j].mNum;
                    nt += (fn-2);
                }

                mIndexData.mElementArray[i].mNum = nt * 3;
                mIndexData.mElementArray[i].mBuffer = (unsigned int*)malloc( mIndexData.mElementArray[i].mNum*sizeof(unsigned int) );
                if( !mIndexData.mElementArray[i].mBuffer )
                    return false;

                unsigned int *ind = mIndexData.mElementArray[i].mBuffer;

                for( unsigned int j=0; j<nf; j++ )
                {
                    const int fn = mesh->mFaceData.mIndexArray[i].mBuffer[j].mNum;
                    if( fn==3 )
                    {
                        ind[0] = mesh->mFaceData.mIndexArray[i].mBuffer[j].mIndex[0];
                        ind[1] = mesh->mFaceData.mIndexArray[i].mBuffer[j].mIndex[1];
                        ind[2] = mesh->mFaceData.mIndexArray[i].mBuffer[j].mIndex[2];
                        ind += 3;
                    }
                    else
                    {
                        ind[0] = mesh->mFaceData.mIndexArray[i].mBuffer[j].mIndex[0];
                        ind[1] = mesh->mFaceData.mIndexArray[i].mBuffer[j].mIndex[1];
                        ind[2] = mesh->mFaceData.mIndexArray[i].mBuffer[j].mIndex[2];
                        ind += 3;
                        ind[0] = mesh->mFaceData.mIndexArray[i].mBuffer[j].mIndex[0];
                        ind[1] = mesh->mFaceData.mIndexArray[i].mBuffer[j].mIndex[2];
                        ind[2] = mesh->mFaceData.mIndexArray[i].mBuffer[j].mIndex[3];
                        ind += 3;
                    }
                }
            }
        }
    }


    //-----------------------------------------------------------------------------------------
    for( unsigned int k=0; k<mVertexData.mNumStreams; k++ )
    {
        piRenderMeshVertexStream * stream = mVertexData.mStream + k;

        const bool isStatic = (staticVertexStreams==nullptr) ? true : staticVertexStreams[k];
        stream->mVBO = renderer->CreateBuffer(stream->mBuffer, stream->mLayout.mStride*stream->mLength, (isStatic) ? piRenderer::BufferType::Static : piRenderer::BufferType::Dynamic, piRenderer::BufferUse::Vertex);
        if( !stream->mVBO )
            return false;
    }

    if(mIndexData.mNumElementArrays==0 )
    {
    }
    else
    {
        for( unsigned int i=0; i<mIndexData.mNumElementArrays; i++ )
        {
            if( mIndexData.mElementArray[i].mNum==0 )
            {
                mIndexData.mElementArray[i].mIBO = 0;
            }
            else
            {
                mIndexData.mElementArray[i].mIBO = renderer->CreateBuffer( mIndexData.mElementArray[i].mBuffer, mIndexData.mElementArray[i].mNum*sizeof(unsigned int), piRenderer::BufferType::Static, piRenderer::BufferUse::Index);
                if( !mIndexData.mElementArray[i].mIBO )
                    return false;
            }
        }
    }

    return true;
}


bool piRenderMesh::UpdateFromMesh(piRenderer *renderer, const piMesh *mesh, int vStream  )
{
    piRenderMeshVertexStream *dstStream = mVertexData.mStream+vStream;

    const uint32_t numBytes = dstStream->mLayout.mStride*dstStream->mLength;

    memcpy(dstStream->mBuffer, mesh->mVertexData.mVertexArray[vStream].mBuffer, numBytes);

    renderer->UpdateBuffer(mVertexData.mStream[vStream].mVBO, mesh->mVertexData.mVertexArray[vStream].mBuffer, 0, numBytes );


    return true;
}

void piRenderMesh::Render( piRenderer *renderer, int elementArrayID, int vertexArrayID, int numInstances ) const
{
    if( mIndexData.mNumElementArrays==0 )
    {
        //renderer->AttachVertexArray( mIndexData.mVAO );
        renderer->DrawPrimitiveNotIndexed(mIndexData.mType, 0, mVertexData.mStream[0].mLength, numInstances);
        //renderer->DettachVertexArray();
    }
    else
    {
        if( renderer->GetAPI()==piRenderer::API::GL || renderer->GetAPI()==piRenderer::API::GLES )
        renderer->AttachVertexArray(mIndexData.mElementArray[elementArrayID].mVertexArray.mVAO[vertexArrayID] );
        else
            renderer->AttachVertexArray2(mIndexData.mElementArray[elementArrayID].mVertexArray.mVAO[vertexArrayID]);
        renderer->DrawPrimitiveIndexed(mIndexData.mType, mIndexData.mElementArray[elementArrayID].mNum, numInstances, 0, 0, 0);
        renderer->DettachVertexArray();
    }
}


}
