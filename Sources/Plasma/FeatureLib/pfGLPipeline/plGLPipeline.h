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
#ifndef _plGLPipeline_inc_
#define _plGLPipeline_inc_

#include "plGLDevice.h"
#include "plPipeline/pl3DPipeline.h"
#include "plPipeline/hsG3DDeviceSelector.h"

class plIcicle;
class plGLMaterialShaderRef;
class plGLVertexBufferRef;
class plPlate;

class plGLEnumerate
{
public:
    plGLEnumerate() {
        hsG3DDeviceSelector::AddDeviceEnumerator(&plGLEnumerate::Enumerate);
    }

private:
    static void Enumerate(std::vector<hsG3DDeviceRecord>& records);
};

class plGLPipeline : public pl3DPipeline<plGLDevice>
{
    friend class plGLPlateManager;
    friend class plGLDevice;

protected:
    typedef void(*blend_vert_buffer_ptr)(plSpan*, hsMatrix44*, int, const uint8_t*, uint8_t , uint32_t, uint8_t*, uint32_t, uint32_t, uint16_t);
    static hsCpuFunctionDispatcher<blend_vert_buffer_ptr> blend_vert_buffer;

    plGLMaterialShaderRef*  fMatRefList;
    plGLRenderTargetRef*    fRenderTargetRefList;

public:
    plGLPipeline(hsWindowHndl display, hsWindowHndl window, const hsG3DDeviceModeRecord *devMode);
    virtual ~plGLPipeline();

    CLASSNAME_REGISTER(plGLPipeline);
    GETINTERFACE_ANY(plGLPipeline, plPipeline);


    /* All of these virtual methods are not implemented by pl3DPipeline and
     * need to be re-implemented here!
     */

    /*** VIRTUAL METHODS ***/
    bool PreRender(plDrawable* drawable, std::vector<int16_t>& visList, plVisMgr* visMgr=nullptr) override;
    bool PrepForRender(plDrawable* drawable, std::vector<int16_t>& visList, plVisMgr* visMgr=nullptr) override;
    plTextFont* MakeTextFont(ST::string face, uint16_t size) override;
    bool OpenAccess(plAccessSpan& dst, plDrawableSpans* d, const plVertexSpan* span, bool readOnly) override;
    bool CloseAccess(plAccessSpan& acc) override;
    void PushRenderRequest(plRenderRequest* req) override;
    void PopRenderRequest(plRenderRequest* req) override;
    void ClearRenderTarget(plDrawable* d) override;
    void ClearRenderTarget(const hsColorRGBA* col = nullptr, const float* depth = nullptr) override;
    hsGDeviceRef* MakeRenderTargetRef(plRenderTarget* owner) override;
    bool BeginRender() override;
    bool EndRender() override;
    void RenderScreenElements() override;
    bool IsFullScreen() const override;
    void Resize(uint32_t width, uint32_t height) override;
    void LoadResources() override;
    bool SetGamma(float eR, float eG, float eB) override;
    bool SetGamma(const uint16_t* const tabR, const uint16_t* const tabG, const uint16_t* const tabB) override;
    bool CaptureScreen(plMipmap* dest, bool flipVertical = false, uint16_t desiredWidth = 0, uint16_t desiredHeight = 0) override;
    plMipmap* ExtractMipMap(plRenderTarget* targ) override;
    void GetSupportedDisplayModes(std::vector<plDisplayMode> *res, int ColorDepth = 32 ) override;
    int GetMaxAnisotropicSamples() override;
    int GetMaxAntiAlias(int Width, int Height, int ColorDepth) override;
    void ResetDisplayDevice(int Width, int Height, int ColorDepth, bool Windowed, int NumAASamples, int MaxAnisotropicSamples, bool vSync = false  ) override;
    void RenderSpans(plDrawableSpans* ice, const std::vector<int16_t>& visList) override;

protected:
    void ISetupTransforms(plDrawableSpans* drawable, const plSpan& span, plGLMaterialShaderRef* mRef, hsMatrix44& lastL2W);
    void IRenderBufferSpan(const plIcicle& span, hsGDeviceRef* vb, hsGDeviceRef* ib, hsGMaterial* material, uint32_t vStart, uint32_t vLength, uint32_t iStart, uint32_t iLength);

    /**
     * Only software skinned objects, dynamic decals, and particle systems
     * currently use the dynamic vertex buffer.
     */
    bool IRefreshDynVertices(plGBufferGroup* group, plGLVertexBufferRef* vRef);

    /**
     * Make sure the buffers underlying this span are ready to be rendered.
     * Meaning that the underlying GL buffers are in sync with the plasma
     * buffers.
     */
    bool ICheckDynBuffers(plDrawableSpans* drawable, plGBufferGroup* group, const plSpan* span);

    void IHandleZMode(hsGMatState flags);
    void IHandleBlendMode(hsGMatState flags);

    /**
     * Tests and sets the current winding order cull mode (CW, CCW, or none).
     * Will reverse the cull mode as necessary for left handed camera or local
     * to world transforms.
     */
    void ISetCullMode();

    void ICalcLighting(plGLMaterialShaderRef* mRef, const plLayerInterface* currLayer, const plSpan* currSpan);
    void ISelectLights(const plSpan* span, plGLMaterialShaderRef* mRef, bool proj = false);
    void IEnableLight(plGLMaterialShaderRef* mRef, size_t i, plLightInfo* light);
    void IDisableLight(plGLMaterialShaderRef* mRef, size_t i);
    void IScaleLight(plGLMaterialShaderRef* mRef, size_t i, float scale);
    void IDrawPlate(plPlate* plate);

    /**
     * Emulate matrix palette operations in software.
     *
     * The big difference between the hardware and software versions is we only
     * want to lock the vertex buffer once and blend all the verts we're going
     * to in software, so the vertex blend happens once for an entire drawable.
     * In hardware, we want the opposite, to break it into managable chunks,
     * manageable meaning few enough matrices to fit into hardware registers.
     * So for hardware version, we set up our palette, draw a span or few,
     * setup our matrix palette with new matrices, draw, repeat.
     */
    bool ISoftwareVertexBlend(plDrawableSpans* drawable, const std::vector<int16_t>& visList);

    /**
     * Given a pointer into a buffer of verts that have blending data in the
     * plVertCoder format, blends them into the destination buffer given
     * without the blending info.
     */
    void IBlendVertsIntoBuffer(plSpan* span, hsMatrix44* matrixPalette, int numMatrices, const uint8_t* src, uint8_t format, uint32_t srcStride, uint8_t* dest, uint32_t destStride, uint32_t count, uint16_t localUVWChans) {
        blend_vert_buffer.call(span, matrixPalette, numMatrices, src, format, srcStride, dest, destStride, count, localUVWChans);
    };

private:
    static plGLEnumerate enumerator;
};

#endif // _plGLPipeline_inc_
