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

class plGLPipeline : public pl3DPipeline
{
protected:

    friend class plGLDevice;

public:
    plGLPipeline(hsWindowHndl display, hsWindowHndl window, const hsG3DDeviceModeRecord *devMode);
    virtual ~plGLPipeline();

    CLASSNAME_REGISTER(plGLPipeline);
    GETINTERFACE_ANY(plGLPipeline, pl3DPipeline);


    /* All of these virtual methods are not implemented by pl3DPipeline and
     * need to be re-implemented here!
     */

    /*** VIRTUAL METHODS ***/
    virtual bool PreRender(plDrawable* drawable, hsTArray<int16_t>& visList, plVisMgr* visMgr=nullptr);
    virtual bool PrepForRender(plDrawable* drawable, hsTArray<int16_t>& visList, plVisMgr* visMgr=nullptr);
    virtual plTextFont* MakeTextFont(char* face, uint16_t size);
    virtual void CheckVertexBufferRef(plGBufferGroup* owner, uint32_t idx);
    virtual void CheckIndexBufferRef(plGBufferGroup* owner, uint32_t idx);
    virtual bool OpenAccess(plAccessSpan& dst, plDrawableSpans* d, const plVertexSpan* span, bool readOnly);
    virtual bool CloseAccess(plAccessSpan& acc);
    virtual void CheckTextureRef(plLayerInterface* lay);
    virtual void PushRenderRequest(plRenderRequest* req);
    virtual void PopRenderRequest(plRenderRequest* req);
    virtual void ClearRenderTarget(plDrawable* d);
    virtual void ClearRenderTarget(const hsColorRGBA* col = nullptr, const float* depth = nullptr);
    virtual hsGDeviceRef* MakeRenderTargetRef(plRenderTarget* owner);
    virtual bool BeginRender();
    virtual bool EndRender();
    virtual void RenderScreenElements();
    virtual bool IsFullScreen() const;
    virtual void Resize(uint32_t width, uint32_t height);
    virtual bool CheckResources();
    virtual void LoadResources();
    virtual void SubmitClothingOutfit(plClothingOutfit* co);
    virtual bool SetGamma(float eR, float eG, float eB);
    virtual bool SetGamma(const uint16_t* const tabR, const uint16_t* const tabG, const uint16_t* const tabB);
    virtual bool CaptureScreen(plMipmap* dest, bool flipVertical = false, uint16_t desiredWidth = 0, uint16_t desiredHeight = 0);
    virtual plMipmap* ExtractMipMap(plRenderTarget* targ);
    virtual void GetSupportedDisplayModes(std::vector<plDisplayMode> *res, int ColorDepth = 32 );
    virtual int GetMaxAnisotropicSamples();
    virtual int GetMaxAntiAlias(int Width, int Height, int ColorDepth);
    virtual void ResetDisplayDevice(int Width, int Height, int ColorDepth, bool Windowed, int NumAASamples, int MaxAnisotropicSamples, bool vSync = false);
    virtual void RenderSpans(plDrawableSpans* ice, const hsTArray<int16_t>& visList);

protected:
    void ISetupTransforms(plDrawableSpans* drawable, const plSpan& span, hsMatrix44& lastL2W);
    void IRenderBufferSpan(const plIcicle& span, hsGDeviceRef* vb, hsGDeviceRef* ib, hsGMaterial* material, uint32_t vStart, uint32_t vLength, uint32_t iStart, uint32_t iLength);
};

#endif // _plGLPipeline_inc_
