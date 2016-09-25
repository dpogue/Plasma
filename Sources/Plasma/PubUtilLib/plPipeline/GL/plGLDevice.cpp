/*==LICENSE==*

CyanWorlds.com Engine - MMOG client, server and tools
Copyright (C) 2011  Cyan Worlds, Inc.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Additional permissions under GNU GPL version 3 section 7

If you modify this Program, or any covered work, by linking or
combining it with any of RAD Game Tools Bink SDK, Autodesk 3ds Max SDK,
NVIDIA PhysX SDK, Microsoft DirectX SDK, OpenSSL library, Independent
JPEG Group JPEG library, Microsoft Windows Media SDK, or Apple QuickTime SDK
(or a modified version of those libraries),
containing parts covered by the terms of the Bink SDK EULA, 3ds Max EULA,
PhysX SDK EULA, DirectX SDK EULA, OpenSSL and SSLeay licenses, IJG
JPEG Library README, Windows Media SDK EULA, or QuickTime SDK EULA, the
licensors of this Program grant you additional
permission to convey the resulting work. Corresponding Source for a
non-source form of such a combination shall include the source code for
the parts of OpenSSL and IJG JPEG Library used as well as that of the covered
work.

You can contact Cyan Worlds, Inc. by email legal@cyan.com
 or by snail mail at:
      Cyan Worlds, Inc.
      14617 N Newport Hwy
      Mead, WA   99021

*==LICENSE==*/

#include "plGLDevice.h"
#include "plGLPipeline.h"

#if HS_BUILD_FOR_OSX
#    include <OpenGL/gl3.h>
#    include <OpenGL/gl3ext.h>
#else
#    include <GL/gl.h>
#    include <GL/glext.h>
#endif

#include "plDrawable/plGBufferGroup.h"
#include "plGImage/plMipmap.h"
#include "plGImage/plCubicEnvironmap.h"

#define HS_DEBUGGING 1

static float kIdentityMatrix[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

GLfloat* hsMatrix2GL(const hsMatrix44& src, GLfloat* dst)
{
    if (src.fFlags & hsMatrix44::kIsIdent)
    {
        memcpy(dst, kIdentityMatrix, sizeof(GLfloat) * 16);
    }
    else
    {
        memcpy(dst, src.fMap, sizeof(GLfloat) * 16);
    }

    return dst;
}

plGLDevice::plGLDevice()
:   fErrorMsg(nullptr),
    fDisplay(EGL_NO_DISPLAY),
    fSurface(EGL_NO_SURFACE),
    fContext(EGL_NO_CONTEXT),
    fCurrentProgram(0)
{
    memcpy(fMatrixL2W, kIdentityMatrix, sizeof(GLfloat) * 16);
    memcpy(fMatrixW2C, kIdentityMatrix, sizeof(GLfloat) * 16);
    memcpy(fMatrixProj, kIdentityMatrix, sizeof(GLfloat) * 16);
}


bool plGLDevice::InitDevice()
{
    if (!eglBindAPI(EGL_OPENGL_API))
    {
        fErrorMsg = "Could not bind to correct API";
        return false;
    }

    /* Set up the display */
    fDisplay = eglGetDisplay((EGLNativeDisplayType)fDevice);
    if (fDisplay == EGL_NO_DISPLAY)
    {
        fErrorMsg = "Could not get the display";
        return false;
    }

    if (!eglInitialize(fDisplay, nullptr, nullptr))
    {
        fErrorMsg = "Could not initialize the display";
        return false;
    }


    /* Set up the config attributes for EGL */
    EGLConfig  config;
    EGLint     config_count;
    EGLint config_attrs[] = {
        EGL_BUFFER_SIZE, 16,
        EGL_DEPTH_SIZE, 16,
#ifndef HS_BUILD_FOR_WIN32
        // These don't work with ANGLE :(
        EGL_SAMPLE_BUFFERS, 1,
        EGL_SAMPLES, 4,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
#endif
        EGL_NONE
    };

    if (!eglChooseConfig(fDisplay, config_attrs, &config, 1, &config_count) || config_count != 1)
    {
        fErrorMsg = "Could not choose appropriate config";
        return false;
    }


    /* Set up the GL context */
    EGLint ctx_attrs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 4,
        EGL_NONE
    };

    fContext = eglCreateContext(fDisplay, config, EGL_NO_CONTEXT, ctx_attrs);
    if (fContext == EGL_NO_CONTEXT)
    {
        fErrorMsg = "Unable to create rendering context";
        return false;
    }


    /* Set up the rendering surface */
    fSurface = eglCreateWindowSurface(fDisplay, config, (EGLNativeWindowType)fWindow, nullptr);
    if (fSurface == EGL_NO_SURFACE)
    {
        fErrorMsg = "Unable to create rendering surface";
        return false;
    }


    /* Associate everything */
    if (eglMakeCurrent(fDisplay, fSurface, fSurface, fContext) == EGL_FALSE) {
        fErrorMsg = "Failed to attach EGL context to surface";
        return false;
    }

    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_POLYGON_OFFSET_FILL);

    return true;
}


void plGLDevice::Shutdown()
{
    if (fSurface != EGL_NO_SURFACE) {
        eglDestroySurface(fDisplay, fSurface);
        fSurface = EGL_NO_SURFACE;
    }

    if (fContext != EGL_NO_CONTEXT) {
        eglDestroySurface(fDisplay, fContext);
        fContext = EGL_NO_CONTEXT;
    }

    if (fDisplay != EGL_NO_DISPLAY) {
        eglTerminate(fDisplay);
        fDisplay = EGL_NO_DISPLAY;
    }
}

void plGLDevice::SetRenderTarget(plRenderTarget* target)
{
    SetViewport();
}

void plGLDevice::SetViewport()
{
    glViewport(fPipeline->GetViewTransform().GetViewPortLeft(),
               fPipeline->GetViewTransform().GetViewPortTop(),
               fPipeline->GetViewTransform().GetViewPortWidth(),
               fPipeline->GetViewTransform().GetViewPortHeight());
}


bool plGLDevice::BeginRender()
{
    return true;
}

bool plGLDevice::EndRender()
{
    if (fPipeline->fCurrRenderTarget == nullptr)
    {
        if (eglSwapBuffers(fDisplay, fSurface) == EGL_FALSE) {
            hsStatusMessage("Failed to swap buffers");
            return false;
        }
    }
    return true;
}


void plGLDevice::SetupVertexBufferRef(plGBufferGroup* owner, uint32_t idx, VertexBufferRef* vRef)
{
    uint8_t format = owner->GetVertexFormat();

    // All indexed skinning is currently done on CPU, so the source data
    // will have indices, but we strip them out for the D3D buffer.
    if( format & plGBufferGroup::kSkinIndices )
    {
        hsStatusMessage("Have to deal with skinning :(");
#if 0
        format &= ~(plGBufferGroup::kSkinWeightMask | plGBufferGroup::kSkinIndices);
        format |= plGBufferGroup::kSkinNoWeights;       // Should do nothing, but just in case...
        vRef->SetSkinned(true);
        vRef->SetVolatile(true);
#endif
    }


    uint32_t vertSize = owner->GetVertexSize(); // vertex stride
    uint32_t numVerts = owner->GetVertBufferCount(idx);

    vRef->fOwner = owner;
    vRef->fCount = numVerts;
    vRef->fVertexSize = vertSize;
    vRef->fFormat = format;
    vRef->fRefTime = 0;

    vRef->SetDirty(true);
    vRef->SetRebuiltSinceUsed(true);
    vRef->fData = nullptr;

    vRef->SetVolatile(vRef->Volatile() || owner->AreVertsVolatile());

    vRef->fIndex = idx;

    owner->SetVertexBufferRef(idx, vRef);
    hsRefCnt_SafeUnRef(vRef);
}

void plGLDevice::CheckStaticVertexBuffer(VertexBufferRef* vRef, plGBufferGroup* owner, uint32_t idx)
{
    hsAssert(!vRef->Volatile(), "Creating a managed vertex buffer for a volatile buffer ref");

    if (!vRef->fRef)
    {
        glGenBuffers(1, &vRef->fRef);

        // Fill in the vertex data.
        FillStaticVertexBufferRef(vRef, owner, idx);

        // This is currently a no op, but this would let the buffer know it can
        // unload the system memory copy, since we have a managed version now.
        owner->PurgeVertBuffer(idx);
    }
}

void plGLDevice::FillStaticVertexBufferRef(VertexBufferRef* ref, plGBufferGroup* group, uint32_t idx)
{
    if (!ref->fRef)
    {
        // We most likely already warned about this earlier, best to just quietly return now
        return;
    }

    const uint32_t vertSize = ref->fVertexSize;
    const uint32_t vertStart = group->GetVertBufferStart(idx) * vertSize;
    const uint32_t size = group->GetVertBufferEnd(idx) * vertSize - vertStart;
    if (!size)
    {
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, ref->fRef);

    if (ref->fData)
    {
        glBufferData(GL_ARRAY_BUFFER, size, ref->fData + vertStart, GL_STATIC_DRAW);
    }
    else
    {
        hsAssert(0 == vertStart, "Offsets on non-interleaved data not supported");
        hsAssert(group->GetVertBufferCount(idx) * vertSize == size, "Trailing dead space on non-interleaved data not supported");

        uint8_t* buffer = new uint8_t[size];
        uint8_t* ptr = buffer;

        const uint32_t vertSmallSize = group->GetVertexLiteStride() - sizeof(hsPoint3) * 2;
        uint8_t* srcVPtr = group->GetVertBufferData(idx);
        plGBufferColor* const srcCPtr = group->GetColorBufferData(idx);

        const int numCells = group->GetNumCells(idx);
        int i;
        for (i = 0; i < numCells; i++)
        {
            plGBufferCell* cell = group->GetCell(idx, i);

            if (cell->fColorStart == uint32_t(-1))
            {
                /// Interleaved, do straight copy
                memcpy(ptr, srcVPtr + cell->fVtxStart, cell->fLength * vertSize);
                ptr += cell->fLength * vertSize;
            }
            else
            {
                hsStatusMessage("Non interleaved data");

                /// Separated, gotta interleave
                uint8_t* tempVPtr = srcVPtr + cell->fVtxStart;
                plGBufferColor* tempCPtr = srcCPtr + cell->fColorStart;
                int j;
                for( j = 0; j < cell->fLength; j++ )
                {
                    memcpy( ptr, tempVPtr, sizeof( hsPoint3 ) * 2 );
                    ptr += sizeof( hsPoint3 ) * 2;
                    tempVPtr += sizeof( hsPoint3 ) * 2;

                    memcpy( ptr, &tempCPtr->fDiffuse, sizeof( uint32_t ) );
                    ptr += sizeof( uint32_t );
                    memcpy( ptr, &tempCPtr->fSpecular, sizeof( uint32_t ) );
                    ptr += sizeof( uint32_t );

                    memcpy( ptr, tempVPtr, vertSmallSize );
                    ptr += vertSmallSize;
                    tempVPtr += vertSmallSize;
                    tempCPtr++;
                }
            }
        }

        hsAssert((ptr - buffer) == size, "Didn't fill the buffer?");
        glBufferData(GL_ARRAY_BUFFER, size, buffer, GL_STATIC_DRAW);

        delete[] buffer;
    }

    /// Unlock and clean up
    ref->SetRebuiltSinceUsed(true);
    ref->SetDirty(false);
}

void plGLDevice::FillVolatileVertexBufferRef(VertexBufferRef* ref, plGBufferGroup* group, uint32_t idx)
{
    hsStatusMessage("Trying to fill volatile vertex buffer ref!");
}

void plGLDevice::SetupIndexBufferRef(plGBufferGroup* owner, uint32_t idx, IndexBufferRef* iRef)
{
    uint32_t numIndices = owner->GetIndexBufferCount(idx);
    iRef->fCount = numIndices;
    iRef->fOwner = owner;
    iRef->fIndex = idx;
    iRef->fRefTime = 0;

    iRef->SetDirty(true);
    iRef->SetRebuiltSinceUsed(true);

    owner->SetIndexBufferRef(idx, iRef);
    hsRefCnt_SafeUnRef(iRef);

    iRef->SetVolatile(owner->AreIdxVolatile());
}

void plGLDevice::CheckIndexBuffer(IndexBufferRef* iRef)
{
    if (!iRef->fRef && iRef->fCount)
    {
        iRef->SetVolatile(false);

        glGenBuffers(1, &iRef->fRef);

        iRef->SetDirty(true);
        iRef->SetRebuiltSinceUsed(true);
    }
}

void plGLDevice::FillIndexBufferRef(IndexBufferRef* iRef, plGBufferGroup* owner, uint32_t idx)
{
    uint32_t startIdx = owner->GetIndexBufferStart(idx);
    uint32_t size = (owner->GetIndexBufferEnd(idx) - startIdx) * sizeof(uint16_t);

    if (!size)
    {
        return;
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iRef->fRef);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, owner->GetIndexBufferData(idx) + startIdx, GL_STATIC_DRAW);

    iRef->SetDirty(false);
}



void plGLDevice::SetupTextureRef(plLayerInterface* layer, plBitmap* img, TextureRef* tRef)
{
    tRef->fOwner = img;

    if (img->IsCompressed()) {
        switch (img->fDirectXInfo.fCompressionType) {
        case plBitmap::DirectXInfo::kDXT1:
            tRef->fFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            break;
        case plBitmap::DirectXInfo::kDXT5:
            tRef->fFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            break;
        }
    } else {
        switch (img->fUncompressedInfo.fType) {
        case plBitmap::UncompressedInfo::kRGB8888:
            tRef->fFormat = GL_RGBA;
            tRef->fDataType = GL_UNSIGNED_BYTE;
            tRef->fDataFormat = GL_BGRA;
            break;
        case plBitmap::UncompressedInfo::kRGB4444:
            tRef->fFormat = GL_RGBA;
            tRef->fDataType = GL_UNSIGNED_SHORT_4_4_4_4;
            tRef->fDataFormat = GL_BGRA;
            break;
        case plBitmap::UncompressedInfo::kRGB1555:
            tRef->fFormat = GL_RGBA;
            tRef->fDataType = GL_UNSIGNED_SHORT_5_5_5_1;
            tRef->fDataFormat = GL_BGRA;
            break;
        case plBitmap::UncompressedInfo::kInten8:
            tRef->fFormat = GL_LUMINANCE;
            tRef->fDataType = GL_UNSIGNED_BYTE;
            tRef->fDataFormat = GL_LUMINANCE;
            break;
        case plBitmap::UncompressedInfo::kAInten88:
            tRef->fFormat = GL_LUMINANCE_ALPHA;
            tRef->fDataType = GL_UNSIGNED_BYTE;
            tRef->fDataFormat = GL_LUMINANCE_ALPHA;
            break;
        }
    }

    tRef->SetDirty(true);

    img->SetDeviceRef(tRef);
    hsRefCnt_SafeUnRef(tRef);
}

void plGLDevice::CheckTexture(TextureRef* tRef)
{
    if (!tRef->fRef)
    {
        glGenTextures(1, &tRef->fRef);

        tRef->SetDirty(true);
    }
}


void plGLDevice::BindTexture(TextureRef* tRef, plMipmap* img, GLuint mapping)
{
#ifdef HS_DEBUGGING
    GLuint e = GL_NO_ERROR;
    if ((e = glGetError()) != GL_NO_ERROR) {
        hsStatusMessage(plFormat("Bind Texture failed {}", uint32_t(e)).c_str());
    }
#endif

    tRef->fLevels = img->GetNumLevels() - 1;

    if (img->IsCompressed()) {
        // Hack around the smallest levels being unusable
        img->SetCurrLevel(tRef->fLevels);
        while ((img->GetCurrWidth() | img->GetCurrHeight()) & 0x03) {
            tRef->fLevels--;
            hsAssert(tRef->fLevels >= 0, "How was this ever compressed?" );
            img->SetCurrLevel(tRef->fLevels);
        }

        for (GLuint lvl = 0; lvl <= tRef->fLevels; lvl++) {
            img->SetCurrLevel(lvl);

            glCompressedTexImage2D(mapping, lvl, tRef->fFormat, img->GetCurrWidth(), img->GetCurrHeight(), 0, img->GetCurrLevelSize(), img->GetCurrLevelPtr());

#ifdef HS_DEBUGGING
            if ((e = glGetError()) != GL_NO_ERROR) {
                hsStatusMessage(plFormat("Texture Image failed {} at level {}", uint32_t(e), lvl).c_str());
            }
#endif
        }
    } else {
        for (GLuint lvl = 0; lvl <= tRef->fLevels; lvl++) {
            img->SetCurrLevel(lvl);

            glTexImage2D(mapping, lvl, tRef->fFormat, img->GetCurrWidth(), img->GetCurrHeight(), 0, tRef->fDataFormat, tRef->fDataType, img->GetCurrLevelPtr());

#ifdef HS_DEBUGGING
            if ((e = glGetError()) != GL_NO_ERROR) {
                hsStatusMessage(plFormat("NonDXT Texture Image failed {} at level {}", uint32_t(e), lvl).c_str());

                if (img->GetKey()) {
                    hsStatusMessage(plFormat("\t{}", img->GetKeyName()).c_str());
                }
            }
#endif
        }
    }
}

void plGLDevice::MakeTextureRef(TextureRef* tRef, plLayerInterface* layer, plMipmap* img)
{
    tRef->fMapping = GL_TEXTURE_2D;

    if (!img->GetImage()) {
        glBindTexture(tRef->fMapping, 0);
        return;
    }

    glBindTexture(tRef->fMapping, tRef->fRef);

    BindTexture(tRef, img, tRef->fMapping);

    glTexParameteri(tRef->fMapping, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (tRef->fLevels) {
        glTexParameteri(tRef->fMapping, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(tRef->fMapping, GL_TEXTURE_MAX_LEVEL, tRef->fLevels);
    } else {
        glTexParameteri(tRef->fMapping, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

#ifdef HS_DEBUGGING
    GLuint e;
    if ((e = glGetError()) != GL_NO_ERROR) {
        hsStatusMessage(plFormat("Mipmap Texture failed {} (Texture {})", uint32_t(e), img->GetKeyName()).c_str());
    }
#endif
}


void plGLDevice::MakeCubicTextureRef(TextureRef* tRef, plLayerInterface* layer, plCubicEnvironmap* img)
{
    static const GLenum kFaceMapping[] = {
        GL_TEXTURE_CUBE_MAP_NEGATIVE_X, // kLeftFace
        GL_TEXTURE_CUBE_MAP_POSITIVE_X, // kRightFace
        GL_TEXTURE_CUBE_MAP_POSITIVE_Z, // kFrontFace
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, // kBackFace
        GL_TEXTURE_CUBE_MAP_POSITIVE_Y, // kTopFace
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Y  // kBottomFace
    };

    tRef->fMapping = GL_TEXTURE_CUBE_MAP;
    glBindTexture(tRef->fMapping, tRef->fRef);

    for (size_t i = 0; i < 6; i++) {
        BindTexture(tRef, img->GetFace(i), kFaceMapping[i]);
    }

    glTexParameteri(tRef->fMapping, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(tRef->fMapping, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(tRef->fMapping, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glTexParameteri(tRef->fMapping, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (tRef->fLevels) {
        glTexParameteri(tRef->fMapping, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(tRef->fMapping, GL_TEXTURE_MAX_LEVEL, tRef->fLevels);
    } else {
        glTexParameteri(tRef->fMapping, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

#ifdef HS_DEBUGGING
    GLuint e;
    if ((e = glGetError()) != GL_NO_ERROR) {
        hsStatusMessage(plFormat("Cubic Texture failed {} (Texture {})", uint32_t(e), img->GetKeyName()).c_str());
    }
#endif
}



void plGLDevice::SetProjectionMatrix(const hsMatrix44& src)
{
    hsMatrix2GL(src, fMatrixProj);
}

void plGLDevice::SetWorldToCameraMatrix(const hsMatrix44& src)
{
    hsMatrix44 inv;
    src.GetInverse(&inv);

    hsMatrix2GL(src, fMatrixW2C);
    hsMatrix2GL(inv, fMatrixC2W);
}

void plGLDevice::SetLocalToWorldMatrix(const hsMatrix44& src)
{
    hsMatrix44 inv;
    src.GetInverse(&inv);

    hsMatrix2GL(src, fMatrixL2W);
    hsMatrix2GL(inv, fMatrixW2L);
}
