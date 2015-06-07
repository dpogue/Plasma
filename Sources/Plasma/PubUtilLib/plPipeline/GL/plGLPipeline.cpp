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
#include "plGLMaterialShaderRef.h"

#include <GL/gl.h>
#include <GL/glext.h>

#include "hsTimer.h"

#include "plPipeline/plPipelineCreate.h"
#include "plPipeDebugFlags.h"
#include "plDrawable/plDrawableSpans.h"
#include "plDrawable/plGBufferGroup.h"
#include "plScene/plRenderRequest.h"
#include "plSurface/hsGMaterial.h"

#ifdef HS_SIMD_INCLUDE
#   include HS_SIMD_INCLUDE
#endif


#include "plProfile.h"

plProfile_CreateCounter("Feed Triangles", "Draw", DrawFeedTriangles);
plProfile_CreateCounter("Polys", "General", DrawTriangles);
plProfile_CreateCounter("Draw Prim Static", "Draw", DrawPrimStatic);
plProfile_CreateMemCounter("Total Texture Size", "Draw", TotalTexSize);
plProfile_CreateCounter("Material Change", "Draw", MatChange);
plProfile_CreateCounter("Layer Change", "Draw", LayChange);

plProfile_CreateTimer("RenderSpan", "PipeT", RenderSpan);
plProfile_CreateTimer("  MergeCheck", "PipeT", MergeCheck);
plProfile_CreateTimer("  MergeSpan", "PipeT", MergeSpan);
plProfile_CreateTimer("  SpanTransforms", "PipeT", SpanTransforms);
plProfile_CreateTimer("  SpanFog", "PipeT", SpanFog);
plProfile_CreateTimer("  SelectLights", "PipeT", SelectLights);
plProfile_CreateTimer("  SelectProj", "PipeT", SelectProj);
plProfile_CreateTimer("  CheckDyn", "PipeT", CheckDyn);
plProfile_CreateTimer("  CheckStat", "PipeT", CheckStat);
plProfile_CreateTimer("  RenderBuff", "PipeT", RenderBuff);
plProfile_CreateTimer("  RenderPrim", "PipeT", RenderPrim);

static plRenderNilFunc sRenderNil;

bool plRenderTriListFunc::RenderPrims() const
{
    plProfile_IncCount(DrawFeedTriangles, fNumTris);
    plProfile_IncCount(DrawTriangles, fNumTris);
    plProfile_Inc(DrawPrimStatic);

    glDrawElements(GL_TRIANGLES, fNumTris, GL_UNSIGNED_SHORT, (GLvoid*)(sizeof(uint16_t) * fIStart));
    return true; // TODO: Check for GL Error
}



plGLPipeline::plGLPipeline(hsWindowHndl display, hsWindowHndl window, const hsG3DDeviceModeRecord* devModeRec)
:   pl3DPipeline(devModeRec), fMatRefList(nullptr)
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
    if (!ds) {
        return false;
    }

    if ((ds->GetType() & fView.GetDrawableTypeMask()) == 0) {
        return false;
    }

    fView.GetVisibleSpans(ds, visList, visMgr);

    return visList.GetCount() > 0;
}

bool plGLPipeline::PrepForRender(plDrawable* drawable, hsTArray<int16_t>& visList, plVisMgr* visMgr)
{
    plDrawableSpans* ice = plDrawableSpans::ConvertNoRef(drawable);
    if (!ice) {
        return false;
    }

    // Find our lights
    ICheckLighting(ice, visList, visMgr);

    // Sort our faces
    if (ice->GetNativeProperty(plDrawable::kPropSortFaces)) {
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
    if (!vRef) {
        // Make the blank ref
        vRef = new DeviceType::VertexBufferRef();

        fDevice.SetupVertexBufferRef(owner, idx, vRef);
    }

    if (!vRef->IsLinked()) {
        vRef->Link(&fVtxBuffRefList);
    }

    // One way or another, we now have a vbufferref[idx] in owner.
    // Now, does it need to be (re)filled?
    // If the owner is volatile, then we hold off. It might not
    // be visible, and we might need to refill it again if we
    // have an overrun of our dynamic buffer.
    if (!vRef->Volatile()) {
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

    if (!iRef) {
        // Create one from scratch.
        iRef = new DeviceType::IndexBufferRef();

        fDevice.SetupIndexBufferRef(owner, idx, iRef);
    }

    if (!iRef->IsLinked()) {
        iRef->Link(&fIdxBuffRefList);
    }

    // Make sure it has all resources created.
    fDevice.CheckIndexBuffer(iRef);

    // If it's dirty, refill it.
    if (iRef->IsDirty()) {
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

    if (req->GetOverrideMat()) {
        PushOverrideMaterial(req->GetOverrideMat());
    }

    // Set from our saved ones...
    fView.SetWorldToLocal(w2l);
    fView.SetLocalToWorld(l2w);

    RefreshMatrices();

    if (req->GetIgnoreOccluders()) {
        fView.SetMaxCullNodes(0);
    }

    fView.fCullTreeDirty = true;
}

void plGLPipeline::PopRenderRequest(plRenderRequest* req)
{
    if (req->GetOverrideMat()) {
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
    if (fView.fRenderState & (kRenderClearColor | kRenderClearDepth)) {
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
    if (fInSceneDepth++ == 0) {
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
    if (--fInSceneDepth == 0) {
        retVal = fDevice.EndRender();

        //IClearShadowSlaves();
    }

    // Do this last, after we've drawn everything
    // Just letting go of things we're done with for the frame.
    hsRefCnt_SafeUnRef(fCurrMaterial);
    fCurrMaterial = nullptr;

    for (int i = 0; i < 8; i++) {
        if (fLayerRef[i]) {
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
    plProfile_BeginTiming(RenderSpan);

    hsMatrix44 lastL2W;
    size_t i, j;
    hsGMaterial* material;
    const hsTArray<plSpan*>& spans = ice->GetSpanArray();

    //plProfile_IncCount(EmptyList, !visList.GetCount());

    /// Set this (*before* we do our TestVisibleWorld stuff...)
    lastL2W.Reset();
    ISetLocalToWorld(lastL2W, lastL2W);     // This is necessary; otherwise, we have to test for
                                            // the first transform set, since this'll be identity
                                            // but the actual device transform won't be (unless
                                            // we do this)


    /// Loop through our spans, combining them when possible
    for (i = 0; i < visList.GetCount(); ) {
        if (GetOverrideMaterial() != nullptr) {
            material = GetOverrideMaterial();
        } else {
            material = ice->GetMaterial(spans[visList[i]]->fMaterialIdx);
        }

        /// It's an icicle--do our icicle merge loop
        plIcicle tempIce(*((plIcicle*)spans[visList[i]]));

        // Start at i + 1, look for as many spans as we can add to tempIce
        for (j = i + 1; j < visList.GetCount(); j++) {
            if (GetOverrideMaterial()) {
                tempIce.fMaterialIdx = spans[visList[j]]->fMaterialIdx;
            }

            plProfile_BeginTiming(MergeCheck);
            if (!spans[visList[j]]->CanMergeInto(&tempIce)) {
                plProfile_EndTiming(MergeCheck);
                break;
            }
            plProfile_EndTiming(MergeCheck);
            //plProfile_Inc(SpanMerge);

            plProfile_BeginTiming(MergeSpan);
            spans[visList[j]]->MergeInto(&tempIce);
            plProfile_EndTiming(MergeSpan);
        }

        if (material != nullptr) {
            // First, do we have a device ref at this index?
            plGLMaterialShaderRef* mRef = (plGLMaterialShaderRef*)material->GetDeviceRef();

            if (mRef == nullptr) {
                mRef = new plGLMaterialShaderRef(material);
                material->SetDeviceRef(mRef);

                //glUseProgram(mRef->fRef);
                //fDevice.fCurrentProgram = mRef->fRef;
            }

            if (!mRef->IsLinked()) {
                mRef->Link(&fMatRefList);
            }

            glUseProgram(mRef->fRef);
            fDevice.fCurrentProgram = mRef->fRef;

#ifdef HS_DEBUGGING
    GLenum e;
    if ((e = glGetError()) != GL_NO_ERROR) {
        hsStatusMessage(plFormat("Use Program failed {}", uint32_t(e)).c_str());
    }
#endif

            // What do we change?

            plProfile_BeginTiming(SpanTransforms);
            ISetupTransforms(ice, tempIce, lastL2W);
            plProfile_EndTiming(SpanTransforms);

            // Turn on this spans lights and turn off the rest.
            //IEnableLights( &tempIce );

            // Check that the underlying buffers are ready to go.
            //plProfile_BeginTiming(CheckDyn);
            //ICheckDynBuffers(drawable, drawable->GetBufferGroup(tempIce.fGroupIdx), &tempIce);
            //plProfile_EndTiming(CheckDyn);

            plProfile_BeginTiming(CheckStat);
            plGBufferGroup* grp = ice->GetBufferGroup(tempIce.fGroupIdx);
            CheckVertexBufferRef(grp, tempIce.fVBufferIdx);
            CheckIndexBufferRef(grp, tempIce.fIBufferIdx);
            plProfile_EndTiming(CheckStat);

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

    plProfile_EndTiming(RenderSpan);
    /// All done!
}
















void plGLPipeline::ISetupTransforms(plDrawableSpans* drawable, const plSpan& span, hsMatrix44& lastL2W)
{
    if (span.fNumMatrices) {
        if (span.fNumMatrices <= 2) {
            ISetLocalToWorld( span.fLocalToWorld, span.fWorldToLocal );
            lastL2W = span.fLocalToWorld;
        } else {
            lastL2W.Reset();
            ISetLocalToWorld( lastL2W, lastL2W );
            fView.fLocalToWorldLeftHanded = span.fLocalToWorld.GetParity();
        }
    } else if (lastL2W != span.fLocalToWorld) {
        ISetLocalToWorld( span.fLocalToWorld, span.fWorldToLocal );
        lastL2W = span.fLocalToWorld;
    } else {
        fView.fLocalToWorldLeftHanded = lastL2W.GetParity();
    }

#if 0 // Skinning
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


    if (fDevice.fCurrentProgram) {
        /* Push the matrices into the GLSL shader now */
        GLint uniform = glGetUniformLocation(fDevice.fCurrentProgram, "uMatrixProj");
        glUniformMatrix4fv(uniform, 1, GL_TRUE, fDevice.fMatrixProj);

        uniform = glGetUniformLocation(fDevice.fCurrentProgram, "uMatrixW2C");
        glUniformMatrix4fv(uniform, 1, GL_TRUE, fDevice.fMatrixW2C);

        uniform = glGetUniformLocation(fDevice.fCurrentProgram, "uMatrixL2W");
        glUniformMatrix4fv(uniform, 1, GL_TRUE, fDevice.fMatrixL2W);
    }
}


void plGLPipeline::IRenderBufferSpan(const plIcicle& span, hsGDeviceRef* vb,
                                     hsGDeviceRef* ib, hsGMaterial* material,
                                     uint32_t vStart, uint32_t vLength,
                                     uint32_t iStart, uint32_t iLength)
{
    plProfile_BeginTiming(RenderBuff);

    DeviceType::VertexBufferRef* vRef = (DeviceType::VertexBufferRef*)vb;
    DeviceType::IndexBufferRef* iRef = (DeviceType::IndexBufferRef*)ib;
    plGLMaterialShaderRef* mRef = (plGLMaterialShaderRef*)material->GetDeviceRef();

    if (!vRef->fRef || !iRef->fRef) {
        plProfile_EndTiming(RenderBuff);

        hsAssert( false, "Trying to render a nil buffer pair!" );
        return;
    }

#ifdef HS_DEBUGGING
    GLenum e;
    if ((e = glGetError()) != GL_NO_ERROR) {
        hsStatusMessage(plFormat("PRE Render failed {}", uint32_t(e)).c_str());
    }
#endif

    mRef->SetupTextureRefs();

    /* Vertex Buffer stuff */
    glBindBuffer(GL_ARRAY_BUFFER, vRef->fRef);

    GLint posAttrib = glGetAttribLocation(fDevice.fCurrentProgram, "aVtxPosition");
    if (posAttrib != -1) {
        glEnableVertexAttribArray(posAttrib);
        glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, vRef->fVertexSize, 0);
    }

    GLint norAttrib = glGetAttribLocation(fDevice.fCurrentProgram, "aVtxNormal");
    if (norAttrib != -1) {
        glEnableVertexAttribArray(norAttrib);
        glVertexAttribPointer(norAttrib, 3, GL_FLOAT, GL_FALSE, vRef->fVertexSize, (void*)(sizeof(float) * 3));
    }

    GLint colAttrib = glGetAttribLocation(fDevice.fCurrentProgram, "aVtxColor");
    if (colAttrib != -1) {
        glEnableVertexAttribArray(colAttrib);
        glVertexAttribPointer(colAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, vRef->fVertexSize, (void*)(sizeof(float) * 3 * 2));
    }

#ifdef HS_DEBUGGING
    if ((e = glGetError()) != GL_NO_ERROR) {
        hsStatusMessage(plFormat("Vertex Attributes failed {}", uint32_t(e)).c_str());
    }
#endif

    for (int i = 0; i < mRef->GetNumUVs(); i++) {
        plString name = plFormat("aVtxUVWSrc{}", i);

        GLint uvwAttrib = glGetAttribLocation(fDevice.fCurrentProgram, name.c_str());

        if (uvwAttrib != -1) {
            glEnableVertexAttribArray(uvwAttrib);

            glVertexAttribPointer(uvwAttrib, 3, GL_FLOAT, GL_FALSE, vRef->fVertexSize, (void*)((sizeof(float) * 3 * 2) + (sizeof(uint32_t) * 2) + (sizeof(float) * 3 * i)));
        }

#ifdef HS_DEBUGGING
        if ((e = glGetError()) != GL_NO_ERROR) {
            hsStatusMessage(plFormat("UVW Attributes failed {}", uint32_t(e)).c_str());
        }
#endif
    }

    /* Index Buffer stuff and drawing */
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iRef->fRef);

    plRenderTriListFunc render(&fDevice, 0, vStart, vLength, iStart, iLength);

    plProfile_EndTiming(RenderBuff);

    // TEMP
    render.RenderPrims();

#ifdef HS_DEBUGGING
    if ((e = glGetError()) != GL_NO_ERROR) {
        hsStatusMessage(plFormat("Render failed {}", uint32_t(e)).c_str());
    }
#endif
}
