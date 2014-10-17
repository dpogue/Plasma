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
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//  plGLPipeline Class Functions                                             //
//  plPipeline derivative for OpenGL                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "HeadSpin.h"

#include "hsTemplates.h"
#include "plGLPipeline.h"

#include <GL/gl.h>
#include <GL/glext.h>

#include "hsTimer.h"

#include "plPipeline/plPipelineCreate.h"
#include "plDrawable/plDrawableSpans.h"
#include "plDrawable/plGBufferGroup.h"
#include "plScene/plRenderRequest.h"
#include "plSurface/hsGMaterial.h"

#ifdef HS_SIMD_INCLUDE
#   include HS_SIMD_INCLUDE
#endif

plGLPipeline::plGLPipeline(hsWindowHndl display, hsWindowHndl window, const hsG3DDeviceModeRecord* devModeRec)
:   pl3DPipeline(devModeRec)
{
    fDevice.fDevice = display;
    fDevice.fWindow = window;
    fDevice.fPipeline = this;

    fDevice.InitDevice();

    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClearDepthf(1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

plGLPipeline::~plGLPipeline()
{
}


































/*****************************************************************************
 * STUBS STUBS STUBS
 *****************************************************************************/

bool plGLPipeline::PreRender(plDrawable* drawable, hsTArray<int16_t>& visList, plVisMgr* visMgr)
{
    plDrawableSpans *ds = plDrawableSpans::ConvertNoRef(drawable);
    if (!ds)
    {
        return false;
    }

    if ((ds->GetType() & fView.GetDrawableTypeMask()) == 0)
    {
        return false;
    }

    fView.GetVisibleSpans(ds, visList, visMgr);

    return visList.GetCount() > 0;
}

bool plGLPipeline::PrepForRender(plDrawable* drawable, hsTArray<int16_t>& visList, plVisMgr* visMgr)
{
    plDrawableSpans* ice = plDrawableSpans::ConvertNoRef(drawable);
    if (!ice)
    {
        return false;
    }

    // Find our lights
    ICheckLighting(ice, visList, visMgr);

    // Sort our faces
    if (ice->GetNativeProperty(plDrawable::kPropSortFaces))
    {
        ice->SortVisibleSpans(visList, this);
    }

    // Prep for render. This is gives the drawable a chance to
    // do any last minute updates for its buffers, including
    // generating particle tri lists.
    ice->PrepForRender(this);

    // Other stuff that we're ignoring for now...

    return true;
}

plTextFont* plGLPipeline::MakeTextFont(char* face, uint16_t size) { return nullptr; }

void plGLPipeline::CheckVertexBufferRef(plGBufferGroup* owner, uint32_t idx)
{
    // First, do we have a device ref at this index?
    DeviceType::VertexBufferRef* vRef = (DeviceType::VertexBufferRef*)owner->GetVertexBufferRef(idx);

    // If not
    if (!vRef)
    {
        // Make the blank ref
        vRef = new DeviceType::VertexBufferRef();

        fDevice.SetupVertexBufferRef(owner, idx, vRef);
    }

    if (!vRef->IsLinked())
    {
        vRef->Link(&fVtxBuffRefList);
    }

    // One way or another, we now have a vbufferref[idx] in owner.
    // Now, does it need to be (re)filled?
    // If the owner is volatile, then we hold off. It might not
    // be visible, and we might need to refill it again if we
    // have an overrun of our dynamic buffer.
    if (!vRef->Volatile())
    {
        // If it's a static buffer, allocate a vertex buffer for it.
        fDevice.CheckStaticVertexBuffer(vRef, owner, idx);

        // Might want to remove this assert, and replace it with a dirty check
        // if we have static buffers that change very seldom rather than never.
        hsAssert(!vRef->IsDirty(), "Non-volatile vertex buffers should never get dirty");
    }
    else
    {
        // Make sure we're going to be ready to fill it.

#if 0
        if (!vRef->fData && (vRef->fFormat != owner->GetVertexFormat()))
        {
            vRef->fData = new uint8_t[vRef->fCount * vRef->fVertexSize];
            fDevice.FillVolatileVertexBufferRef(vRef, owner, idx);
        }
#endif
    }
}

void plGLPipeline::CheckIndexBufferRef(plGBufferGroup* owner, uint32_t idx)
{
    DeviceType::IndexBufferRef* iRef = (DeviceType::IndexBufferRef*)owner->GetIndexBufferRef(idx);

    if (!iRef)
    {
        // Create one from scratch.
        iRef = new DeviceType::IndexBufferRef();

        fDevice.SetupIndexBufferRef(owner, idx, iRef);
    }

    if (!iRef->IsLinked())
    {
        iRef->Link(&fIdxBuffRefList);
    }

    // Make sure it has all resources created.
    fDevice.CheckIndexBuffer(iRef);

    // If it's dirty, refill it.
    if (iRef->IsDirty())
    {
        fDevice.FillIndexBufferRef(iRef, owner, idx);
    }
}

bool plGLPipeline::OpenAccess(plAccessSpan& dst, plDrawableSpans* d, const plVertexSpan* span, bool readOnly) { return false; }

bool plGLPipeline::CloseAccess(plAccessSpan& acc) { return false; }

void plGLPipeline::CheckTextureRef(plLayerInterface* lay) {}

void plGLPipeline::PushRenderRequest(plRenderRequest* req)
{
    // Save these, since we want to copy them to our current view
    hsMatrix44 l2w = fView.GetLocalToWorld();
    hsMatrix44 w2l = fView.GetWorldToLocal();

    plFogEnvironment defFog = fView.GetDefaultFog();

    fViewStack.push(fView);

    SetViewTransform(req->GetViewTransform());

    PushRenderTarget(req->GetRenderTarget());
    fView.fRenderState = req->GetRenderState();

    fView.fRenderRequest = req;
    hsRefCnt_SafeRef(fView.fRenderRequest);

    SetDrawableTypeMask(req->GetDrawableMask());
    SetSubDrawableTypeMask(req->GetSubDrawableMask());

    float depth = req->GetClearDepth();
    fView.SetClear(&req->GetClearColor(), &depth);

#if 0
    if (req->GetFogStart() < 0)
    {
        fView.SetDefaultFog(defFog);
    }
    else
    {
        defFog.Set(req->GetYon() * (1.f - req->GetFogStart()), req->GetYon(), 1.f, &req->GetClearColor());
        fView.SetDefaultFog(defFog);
        fCurrFog.fEnvPtr = nullptr;
    }
#endif

    if (req->GetOverrideMat())
    {
        PushOverrideMaterial(req->GetOverrideMat());
    }

    // Set from our saved ones...
    fView.SetWorldToLocal(w2l);
    fView.SetLocalToWorld(l2w);

    RefreshMatrices();

    if (req->GetIgnoreOccluders())
    {
        fView.SetMaxCullNodes(0);
    }

    fView.fCullTreeDirty = true;
}

void plGLPipeline::PopRenderRequest(plRenderRequest* req)
{
    if (req->GetOverrideMat())
    {
        PopOverrideMaterial(nil);
    }

    hsRefCnt_SafeUnRef(fView.fRenderRequest);
    fView = fViewStack.top();
    fViewStack.pop();

#if 0
    // Force the next thing drawn to update the fog settings.
    fD3DDevice->SetRenderState(D3DRS_FOGENABLE, FALSE);
    fCurrFog.fEnvPtr = nullptr;
#endif

    PopRenderTarget();
    fView.fXformResetFlags = fView.kResetProjection | fView.kResetCamera;
}

void plGLPipeline::ClearRenderTarget(plDrawable* d) {}

void plGLPipeline::ClearRenderTarget(const hsColorRGBA* col, const float* depth)
{
    if (fView.fRenderState & (kRenderClearColor | kRenderClearDepth))
    {
        hsColorRGBA clearColor = col ? *col : GetClearColor();
        float clearDepth = depth ? *depth : fView.GetClearDepth();

        GLuint masks = 0;
        if (fView.fRenderState & kRenderClearColor)
            masks |= GL_COLOR_BUFFER_BIT;
        if (fView.fRenderState & kRenderClearDepth)
            masks |= GL_DEPTH_BUFFER_BIT;

        glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        glClearDepthf(clearDepth);

        glClear(masks);
    }
}

hsGDeviceRef* plGLPipeline::MakeRenderTargetRef(plRenderTarget* owner) { return nullptr; }

bool plGLPipeline::BeginRender()
{
    // offset transform
    RefreshScreenMatrices();

    // If this is the primary BeginRender, make sure we're really ready.
    if (fInSceneDepth++ == 0)
    {
        fDevice.BeginRender();
    }

    fRenderCnt++;

    // Would probably rather this be an input.
    fTime = hsTimer::GetSysSeconds();

    return false;
}

bool plGLPipeline::EndRender() {
    bool retVal = false;

    /// Actually end the scene
    if (--fInSceneDepth == 0)
    {
        retVal = fDevice.EndRender();

        //IClearShadowSlaves();
    }

    // Do this last, after we've drawn everything
    // Just letting go of things we're done with for the frame.
    hsRefCnt_SafeUnRef(fCurrMaterial);
    fCurrMaterial = nullptr;

    for (int i = 0; i < 8; i++)
    {
        if (fLayerRef[i])
        {
            hsRefCnt_SafeUnRef(fLayerRef[i]);
            fLayerRef[i] = nullptr;
        }
    }

    return retVal;
}

void plGLPipeline::RenderScreenElements() {}

bool plGLPipeline::IsFullScreen() const { return false; }

void plGLPipeline::Resize(uint32_t width, uint32_t height) {}

bool plGLPipeline::CheckResources() { return false; }

void plGLPipeline::LoadResources() {}

void plGLPipeline::SubmitClothingOutfit(plClothingOutfit* co) {}

bool plGLPipeline::SetGamma(float eR, float eG, float eB) { return false; }

bool plGLPipeline::SetGamma(const uint16_t* const tabR, const uint16_t* const tabG, const uint16_t* const tabB) { return false; }

bool plGLPipeline::CaptureScreen(plMipmap* dest, bool flipVertical, uint16_t desiredWidth, uint16_t desiredHeight) { return false; }

plMipmap* plGLPipeline::ExtractMipMap(plRenderTarget* targ) { return nullptr; }

void plGLPipeline::GetSupportedDisplayModes(std::vector<plDisplayMode> *res, int ColorDepth) {}

int plGLPipeline::GetMaxAnisotropicSamples() { return 0; }

int plGLPipeline::GetMaxAntiAlias(int Width, int Height, int ColorDepth) { return 0; }

void plGLPipeline::ResetDisplayDevice(int Width, int Height, int ColorDepth, bool Windowed, int NumAASamples, int MaxAnisotropicSamples, bool vSync) {}

void plGLPipeline::RenderSpans(plDrawableSpans* ice, const hsTArray<int16_t>& visList)
{
    //plProfile_BeginTiming(RenderSpan);

    hsMatrix44 lastL2W;
    size_t i, j;
    hsGMaterial* material;
    const hsTArray<plSpan*>& spans = ice->GetSpanArray();

    //plProfile_IncCount(EmptyList, !visList.GetCount());

    /// Set this (*before* we do our TestVisibleWorld stuff...)
    lastL2W.Reset();
    ISetLocalToWorld( lastL2W, lastL2W );   // This is necessary; otherwise, we have to test for
                                            // the first transform set, since this'll be identity
                                            // but the actual device transform won't be (unless
                                            // we do this)


    /// Loop through our spans, combining them when possible
    for (i = 0; i < visList.GetCount(); )
    {
        if (GetOverrideMaterial() != nullptr)
        {
            material = GetOverrideMaterial();
        }
        else
        {
            material = ice->GetMaterial(spans[visList[i]]->fMaterialIdx);
        }

        /// It's an icicle--do our icicle merge loop
        plIcicle tempIce(*((plIcicle*)spans[visList[i]]));

        // Start at i + 1, look for as many spans as we can add to tempIce
        for (j = i + 1; j < visList.GetCount(); j++)
        {
            if (GetOverrideMaterial())
            {
                tempIce.fMaterialIdx = spans[visList[j]]->fMaterialIdx;
            }

            //plProfile_BeginTiming(MergeCheck);
            if (!spans[visList[j]]->CanMergeInto(&tempIce))
            {
                //plProfile_EndTiming(MergeCheck);
                break;
            }
            //plProfile_EndTiming(MergeCheck);
            //plProfile_Inc(SpanMerge);

            //plProfile_BeginTiming(MergeSpan);
            spans[visList[j]]->MergeInto(&tempIce);
            //plProfile_EndTiming(MergeSpan);
        }

        if (material != nullptr)
        {
            // What do we change?

            //plProfile_BeginTiming(SpanTransforms);
            ISetupTransforms(ice, tempIce, lastL2W);
            //plProfile_EndTiming(SpanTransforms);

            // Turn on this spans lights and turn off the rest.
            //IEnableLights( &tempIce );

            // Check that the underlying buffers are ready to go.
            //plProfile_BeginTiming(CheckDyn);
            //ICheckDynBuffers(drawable, drawable->GetBufferGroup(tempIce.fGroupIdx), &tempIce);
            //plProfile_EndTiming(CheckDyn);

            //plProfile_BeginTiming(CheckStat);
            CheckVertexBufferRef(ice->GetBufferGroup(tempIce.fGroupIdx), tempIce.fVBufferIdx);
            CheckIndexBufferRef(ice->GetBufferGroup(tempIce.fGroupIdx), tempIce.fIBufferIdx);
            //plProfile_EndTiming(CheckStat);

            // Draw this span now
            IRenderBufferSpan( tempIce,
                                ice->GetVertexRef( tempIce.fGroupIdx, tempIce.fVBufferIdx ),
                                ice->GetIndexRef( tempIce.fGroupIdx, tempIce.fIBufferIdx ),
                                material,
                                tempIce.fVStartIdx, tempIce.fVLength,   // These are used as our accumulated range
                                tempIce.fIPackedIdx, tempIce.fILength );
        }

        // Restart our search...
        i = j;
    }

    //plProfile_EndTiming(RenderSpan);
    /// All done!
}
















void plGLPipeline::ISetupTransforms(plDrawableSpans* drawable, const plSpan& span, hsMatrix44& lastL2W)
{
    if( span.fNumMatrices )
    {
        if( span.fNumMatrices <= 2 )
        {
            ISetLocalToWorld( span.fLocalToWorld, span.fWorldToLocal );
            lastL2W = span.fLocalToWorld;
        }
        else
        {
            lastL2W.Reset();
            ISetLocalToWorld( lastL2W, lastL2W );
            fView.fLocalToWorldLeftHanded = span.fLocalToWorld.GetParity();
        }
    }
    else
    if( lastL2W != span.fLocalToWorld )
    {
        ISetLocalToWorld( span.fLocalToWorld, span.fWorldToLocal );
        lastL2W = span.fLocalToWorld;
    }
    else
    {
        fView.fLocalToWorldLeftHanded = lastL2W.GetParity();
    }

#if 0
    if( span.fNumMatrices == 2 )
    {
        D3DXMATRIX  mat;
        IMatrix44ToD3DMatrix(mat, drawable->GetPaletteMatrix(span.fBaseMatrix+1));
        fD3DDevice->SetTransform(D3DTS_WORLDMATRIX(1), &mat);
        fD3DDevice->SetRenderState(D3DRS_VERTEXBLEND, D3DVBF_1WEIGHTS);
    }
    else
    {
        fD3DDevice->SetRenderState(D3DRS_VERTEXBLEND, D3DVBF_DISABLE);
    }
#endif
}


void plGLPipeline::IRenderBufferSpan(const plIcicle& span, hsGDeviceRef *vb, hsGDeviceRef *ib, hsGMaterial *material, uint32_t vStart, uint32_t vLength, uint32_t iStart, uint32_t iLength)
{
    DeviceType::VertexBufferRef* vRef = (DeviceType::VertexBufferRef*)vb;
    DeviceType::IndexBufferRef* iRef = (DeviceType::IndexBufferRef*)ib;

    if (!vRef->fRef || !iRef->fRef)
    {
        hsAssert( false, "Trying to render a nil buffer pair!" );
        return;
    }

#if 1
    /* Vertex Buffer stuff */
    glBindBuffer(GL_ARRAY_BUFFER, vRef->fRef);

    GLint posAttrib = glGetAttribLocation(fDevice.fProgram, "position");
    GLint colAttrib = glGetAttribLocation(fDevice.fProgram, "color");
    glEnableVertexAttribArray(posAttrib);
    glEnableVertexAttribArray(colAttrib);

    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, vRef->fVertexSize, 0);
    glVertexAttribPointer(colAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, vRef->fVertexSize, (void*)(sizeof(float) * 3 * 2));

    /* Index Buffer stuff and drawing */
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iRef->fRef);

    glDrawElements(GL_TRIANGLES, iRef->fCount, GL_UNSIGNED_SHORT, 0);

#else
    /* Hardcoded fake data for now */
    GLuint handles[2];
    glGenBuffers(2, handles);

    uint8_t* vbuf = new uint8_t[32 * 3];
    float* p = (float*)vbuf;
    p[0] = -1.f;
    p[1] = 1.f;
    p[2] = -1.f;
    p[3] = 1.f;
    p[4] = 1.f;
    p[5] = 1.f;
    ((uint32_t*)p)[6] = 0xFFFF0000;
    p[7] = 0.f;

    p[8] = 0.f;
    p[9] = 5.f;
    p[10] = 1.f;
    p[11] = 1.f;
    p[12] = 1.f;
    p[13] = 1.f;
    ((uint32_t*)p)[14] = 0xFF00FF00;
    p[15] = 0.f;

    p[16] = 1.f;
    p[17] = 1.f;
    p[18] = -1.f;
    p[19] = 1.f;
    p[20] = 1.f;
    p[21] = 1.f;
    ((uint32_t*)p)[22] = 0xFF0000FF;
    p[23] = 0.f;

    uint16_t* inds = new uint16_t[3];
    inds[0] = 0;
    inds[1] = 1;
    inds[2] = 2;

    glBindBuffer(GL_ARRAY_BUFFER, handles[0]);
    glBufferData(GL_ARRAY_BUFFER, 32 * 3, vbuf, GL_STATIC_DRAW);

    GLint posAttrib = glGetAttribLocation(fDevice.fProgram, "position");
    GLint colAttrib = glGetAttribLocation(fDevice.fProgram, "color");
    glEnableVertexAttribArray(posAttrib);
    glEnableVertexAttribArray(colAttrib);

    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 32, 0);
    glVertexAttribPointer(colAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, 32, (void*)24);


    /* Index Buffer stuff and drawing */
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handles[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * 3, inds, GL_STATIC_DRAW);

    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, 0);
#endif
}
