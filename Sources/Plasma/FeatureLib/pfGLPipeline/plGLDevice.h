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
#ifndef _plGLDevice_h_
#define _plGLDevice_h_

#include "hsMatrix44.h"
#include "plGLDeviceRef.h"

#include <string_theory/string>

class plBitmap;
class plCubicEnvironmap;
class plGLPipeline;
class plLayerInterface;
class plMipmap;
class plRenderTarget;

class plGLDeviceImpl
{
protected:
    hsWindowHndl    fWindow;
    hsWindowHndl    fDevice;

    plGLDeviceImpl(hsWindowHndl window, hsWindowHndl device)
        : fWindow(window), fDevice(device) {};

public:
    virtual ~plGLDeviceImpl() { };

    virtual void Shutdown() = 0;
    virtual bool BeginRender(ST::string& error) = 0;
    virtual bool EndRender(ST::string& error) = 0;
};

class plGLDevice
{
public:
    typedef plGLVertexBufferRef VertexBufferRef;
    typedef plGLIndexBufferRef  IndexBufferRef;
    typedef plGLTextureRef      TextureRef;

protected:
    ST::string fErrorMsg;
    plGLPipeline*       fPipeline;
    plGLDeviceImpl*     fImpl;
    hsWindowHndl        fWindow;
    hsWindowHndl        fDevice;
    size_t              fActiveThread;
    GLuint              fCurrentProgram;
    GLfloat             fMatrixL2W[16];
    GLfloat             fMatrixW2L[16];
    GLfloat             fMatrixW2C[16];
    GLfloat             fMatrixC2W[16];
    GLfloat             fMatrixProj[16];

public:
    plGLDevice();

    void Setup(plGLPipeline* pipe, hsWindowHndl window, hsWindowHndl device);
    void Shutdown();

    /**
     * Set rendering to the specified render target.
     *
     * Null rendertarget is the primary. Invalidates the state as required by
     * experience, not documentation.
     */
    void SetRenderTarget(plRenderTarget* target);

    /** Translate our viewport into a GL viewport. */
    void SetViewport();

    bool BeginRender();

    /**
     * Tell GL we're through rendering for this frame, and flip the back buffer
     * to front.
     */
    bool EndRender();

    /* Device Ref Functions **************************************************/
    void SetupVertexBufferRef(plGBufferGroup* owner, uint32_t idx, VertexBufferRef* vRef);
    void CheckStaticVertexBuffer(VertexBufferRef* vRef, plGBufferGroup* owner, uint32_t idx);
    void FillStaticVertexBufferRef(VertexBufferRef* ref, plGBufferGroup* group, uint32_t idx);
    void FillVolatileVertexBufferRef(VertexBufferRef* ref, plGBufferGroup* group, uint32_t idx);

    /**
     * Only software skinned objects, dynamic decals, and particle systems
     * currently use the dynamic vertex buffer.
     */
    void RefreshDynamicVertexBufferRef(VertexBufferRef* ref, plGBufferGroup* group);

    void FillGLVertexBuffer(VertexBufferRef* ref, GLsizeiptr size, const void* data, GLenum usage);
    void SetupIndexBufferRef(plGBufferGroup* owner, uint32_t idx, IndexBufferRef* iRef);
    void CheckIndexBuffer(IndexBufferRef* iRef);
    void FillIndexBufferRef(IndexBufferRef* iRef, plGBufferGroup* owner, uint32_t idx);
    void SetupTextureRef(plLayerInterface* layer, plBitmap* img, TextureRef* tRef);
    void CheckTexture(TextureRef* tRef);
    void MakeTextureRef(TextureRef* tRef, plLayerInterface* layer, plMipmap* img);
    void MakeCubicTextureRef(TextureRef* tRef, plLayerInterface* layer, plCubicEnvironmap* img);

    void SetProjectionMatrix(const hsMatrix44& src);
    void SetWorldToCameraMatrix(const hsMatrix44& src);
    void SetLocalToWorldMatrix(const hsMatrix44& src);

    void SetCurrentProgram(GLuint program) { fCurrentProgram = program; }

    ST::string GetErrorString() const { return fErrorMsg; }

    bool HasContext() const { return fImpl != nullptr; }

    const GLfloat* GetL2WMatrix() const { return fMatrixL2W; }
    const GLfloat* GetW2LMatrix() const { return fMatrixW2L; }
    const GLfloat* GetC2WMatrix() const { return fMatrixC2W; }
    const GLfloat* GetW2CMatrix() const { return fMatrixW2C; }
    const GLfloat* GetProjectionMatrix() const { return fMatrixProj; }

private:
    /**
     * Initializes the OpenGL rendering context.
     */
    bool InitDevice();

    void BindTexture(TextureRef* tRef, plMipmap* img, GLuint mapping);
};

#endif

