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

#include "plPipeline/pl3DPipeline.h"
#include "plPipeline/hsG3DDeviceSelector.h"
#include "plGLDevice.h"

class plIcicle;
class plGLMaterialShaderRef;
class plPlate;

class plGLPipeline : public pl3DPipeline
{
protected:

    friend class plGLDevice;
    friend class plGLPlateManager;

    plGLMaterialShaderRef* fMatRefList;

public:
    plGLPipeline(hsWindowHndl display, hsWindowHndl window, const hsG3DDeviceModeRecord *devMode);
    virtual ~plGLPipeline();

    CLASSNAME_REGISTER(plGLPipeline);
    GETINTERFACE_ANY(plGLPipeline, pl3DPipeline);


    /* All of these virtual methods are not implemented by pl3DPipeline and
     * need to be re-implemented here!
     */

    /*** VIRTUAL METHODS ***/
    bool PreRender(plDrawable* drawable, hsTArray<int16_t>& visList, plVisMgr* visMgr=nullptr) HS_OVERRIDE;
    bool PrepForRender(plDrawable* drawable, hsTArray<int16_t>& visList, plVisMgr* visMgr=nullptr) HS_OVERRIDE;
    plTextFont* MakeTextFont(char* face, uint16_t size) HS_OVERRIDE;
    bool OpenAccess(plAccessSpan& dst, plDrawableSpans* d, const plVertexSpan* span, bool readOnly) HS_OVERRIDE;
    bool CloseAccess(plAccessSpan& acc) HS_OVERRIDE;
    void PushRenderRequest(plRenderRequest* req) HS_OVERRIDE;
    void PopRenderRequest(plRenderRequest* req) HS_OVERRIDE;
    void ClearRenderTarget(plDrawable* d) HS_OVERRIDE;
    void ClearRenderTarget(const hsColorRGBA* col = nullptr, const float* depth = nullptr) HS_OVERRIDE;
    hsGDeviceRef* MakeRenderTargetRef(plRenderTarget* owner) HS_OVERRIDE;
    bool BeginRender() HS_OVERRIDE;
    bool EndRender() HS_OVERRIDE;
    void RenderScreenElements() HS_OVERRIDE;
    bool IsFullScreen() const HS_OVERRIDE;
    void Resize(uint32_t width, uint32_t height) HS_OVERRIDE;
    bool CheckResources() HS_OVERRIDE;
    void LoadResources() HS_OVERRIDE;
    void SubmitClothingOutfit(plClothingOutfit* co) HS_OVERRIDE;
    bool SetGamma(float eR, float eG, float eB) HS_OVERRIDE;
    bool SetGamma(const uint16_t* const tabR, const uint16_t* const tabG, const uint16_t* const tabB) HS_OVERRIDE;
    bool CaptureScreen(plMipmap* dest, bool flipVertical = false, uint16_t desiredWidth = 0, uint16_t desiredHeight = 0) HS_OVERRIDE;
    plMipmap* ExtractMipMap(plRenderTarget* targ) HS_OVERRIDE;
    void GetSupportedDisplayModes(std::vector<plDisplayMode> *res, int ColorDepth = 32 ) HS_OVERRIDE;
    int GetMaxAnisotropicSamples() HS_OVERRIDE;
    int GetMaxAntiAlias(int Width, int Height, int ColorDepth) HS_OVERRIDE;
    void ResetDisplayDevice(int Width, int Height, int ColorDepth, bool Windowed, int NumAASamples, int MaxAnisotropicSamples, bool vSync = false) HS_OVERRIDE;
    void RenderSpans(plDrawableSpans* ice, const hsTArray<int16_t>& visList) HS_OVERRIDE;

protected:
    void ISetupTransforms(plDrawableSpans* drawable, const plSpan& span, hsMatrix44& lastL2W);
    void IRenderBufferSpan(const plIcicle& span, hsGDeviceRef* vb, hsGDeviceRef* ib, hsGMaterial* material, uint32_t vStart, uint32_t vLength, uint32_t iStart, uint32_t iLength);

    void IHandleZMode(hsGMatState flags);
    void IHandleBlendMode(hsGMatState flags);
    void ICalcLighting(plGLMaterialShaderRef* mRef, const plLayerInterface* currLayer, const plSpan* currSpan);

    void ISelectLights(const plSpan* span, bool proj = false);
    void IEnableLight(size_t i, plLightInfo* light);
    void IDisableLight(size_t i);
    void IScaleLight(size_t i, float scale);

    void IDrawPlate(plPlate* plate);
};

#endif // _plGLPipeline_inc_
