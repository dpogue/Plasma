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

#include <GL/gl.h>
#include <GL/glext.h>

#include "plDrawable/plGBufferGroup.h"

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
        return dst;
    }
    else
    {
        return (GLfloat*)(src.fMap);
    }
}

plGLDevice::plGLDevice()
:   fErrorMsg(nullptr),
    fDisplay(EGL_NO_DISPLAY),
    fSurface(EGL_NO_SURFACE),
    fContext(EGL_NO_CONTEXT)
{
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
        EGL_RENDERABLE_TYPE,
        EGL_OPENGL_BIT,
        EGL_NONE
    };

    if (!eglChooseConfig(fDisplay, config_attrs, &config, 1, &config_count) || config_count != 1)
    {
        fErrorMsg = "Could not choose appropriate config";
        return false;
    }


    /* Set up the rendering surface */
    fSurface = eglCreateWindowSurface(fDisplay, config, (EGLNativeWindowType)fWindow, nullptr);
    if (fSurface == EGL_NO_SURFACE)
    {
        fErrorMsg = "Unable to create rendering surface";
        return false;
    }


    /* Set up the GL context */
    EGLint ctx_attrs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    fContext = eglCreateContext(fDisplay, config, EGL_NO_CONTEXT, ctx_attrs);
    if (fContext == EGL_NO_CONTEXT)
    {
        fErrorMsg = "Unable to create rendering context";
        return false;
    }


    /* Associate everything */
    eglMakeCurrent(fDisplay, fSurface, fSurface, fContext);

    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, 0, GL_TRUE);


    /* TEMP: Shader init stuff */
    const char* vs_src = "#version 130"
                     "\n"
                     "\n" "attribute vec3 position;"
                     "\n" "attribute vec4 color;"
                     "\n"
                     "\n" "uniform mat4 matrix_l2w;"
                     "\n" "uniform mat4 matrix_w2c;"
                     "\n" "uniform mat4 matrix_proj;"
                     "\n"
                     "\n" "varying vec4 v_color;"
                     "\n"
                     "\n" "void main() {"
                     "\n" "    vec4 pos = matrix_l2w * vec4(position, 1.0);"
                     "\n" "         pos = matrix_w2c * pos;"
                     "\n" "         pos = matrix_proj * pos;"
                     "\n"
                     "\n" "    gl_Position = pos;"
                     "\n" "    v_color = color;"
                     "\n" "}";

    const char* fs_src = "#version 130"
                     "\n"
                     "\n" "varying mediump vec4 v_color;"
                     "\n"
                     "\n" "void main() {"
                     "\n" "    gl_FragColor = v_color;"
                     "\n" "}";

    GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vshader, 1, &vs_src, nullptr);
    glCompileShader(vshader);

    GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader, 1, &fs_src, nullptr);
    glCompileShader(fshader);

    fProgram = glCreateProgram();
    glAttachShader(fProgram, vshader);
    glAttachShader(fProgram, fshader);

    glLinkProgram(fProgram);
    glUseProgram(fProgram);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    return true;
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
        eglSwapBuffers(fDisplay, fSurface);
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


    uint32_t vertSize = owner->GetVertexSize(); //IGetBufferFormatSize(format); // vertex stride
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
        const uint32_t vertSmallSize = group->GetVertexLiteStride() - sizeof( hsPoint3 ) * 2;
        uint8_t* srcVPtr = group->GetVertBufferData(idx);
        plGBufferColor* const srcCPtr = group->GetColorBufferData( idx );

        const int numCells = group->GetNumCells(idx);
        int i;
        for( i = 0; i < numCells; i++ )
        {
            plGBufferCell   *cell = group->GetCell( idx, i );

            if( cell->fColorStart == uint32_t(-1))
            {
                /// Interleaved, do straight copy
                memcpy( ptr, srcVPtr + cell->fVtxStart, cell->fLength * vertSize );
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

        glBufferData(GL_ARRAY_BUFFER, size, buffer, GL_STATIC_DRAW);
    }

    /// Unlock and clean up
    ref->SetRebuiltSinceUsed(true);
    ref->SetDirty(false);
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



void plGLDevice::SetProjectionMatrix(const hsMatrix44& src)
{
    static bool printed = false;
    if (!printed) {
        hsStatusMessage(plFormat("Proj: {}", src).c_str());
        printed = true;
    }

    GLfloat mat[16];
    GLint uniform = glGetUniformLocation(fProgram, "matrix_proj");
    glUniformMatrix4fv(uniform, 1, GL_TRUE, hsMatrix2GL(src, mat));
}

void plGLDevice::SetWorldToCameraMatrix(const hsMatrix44& src)
{
    GLfloat mat[16];
    GLint uniform = glGetUniformLocation(fProgram, "matrix_w2c");
    glUniformMatrix4fv(uniform, 1, GL_TRUE, hsMatrix2GL(src, mat));
}

void plGLDevice::SetLocalToWorldMatrix(const hsMatrix44& src)
{
    GLfloat mat[16];
    GLint uniform = glGetUniformLocation(fProgram, "matrix_l2w");
    glUniformMatrix4fv(uniform, 1, GL_TRUE, hsMatrix2GL(src, mat));
}
