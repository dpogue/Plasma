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
#include <string_theory/format>

#include "hsTemplates.h"
#include "plGLConstants.h"
#include "plGLPipeline.h"
#include "plGLMaterialShaderRef.h"
#include "plGLPlateManager.h"

#if HS_BUILD_FOR_MACOS
#    include <OpenGL/gl3.h>
#    include <OpenGL/gl3ext.h>
#else
#    include <GL/gl.h>
#    include <GL/glext.h>
#endif

#include "hsTimer.h"

#include "plPipeline/plPipelineCreate.h"
#include "plPipeline/plDebugText.h"
#include "plPipeDebugFlags.h"
#include "plDrawable/plDrawableSpans.h"
#include "plDrawable/plGBufferGroup.h"
#include "plGLight/plLightInfo.h"
#include "plScene/plRenderRequest.h"
#include "plSurface/hsGMaterial.h"

#include "hsGMatState.inl"

#ifdef HS_SIMD_INCLUDE
#    include HS_SIMD_INCLUDE
#endif


#include "plProfile.h"

plProfile_CreateCounter("Feed Triangles", "Draw", DrawFeedTriangles);
plProfile_CreateCounter("Polys", "General", DrawTriangles);
plProfile_CreateCounter("Draw Prim Static", "Draw", DrawPrimStatic);
plProfile_CreateMemCounter("Total Texture Size", "Draw", TotalTexSize);
plProfile_CreateCounter("Material Change", "Draw", MatChange);
plProfile_CreateCounter("Layer Change", "Draw", LayChange);

plProfile_CreateTimer("PlateMgr", "PipeT", PlateMgr);
plProfile_CreateTimer("DebugText", "PipeT", DebugText);
plProfile_CreateTimer("Reset", "PipeT", Reset);

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

    if (fDevice.InitDevice()) {
        hsColorRGBA clearColor = GetClearColor();

        glDepthMask(GL_TRUE);
        glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        glClearDepth(1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    fPlateMgr = new plGLPlateManager(this);
}

plGLPipeline::~plGLPipeline()
{
    if (plGLPlateManager* pm = static_cast<plGLPlateManager*>(fPlateMgr))
    {
        pm->IReleaseGeometry();
    }

    fDevice.Shutdown();
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

bool plGLPipeline::OpenAccess(plAccessSpan& dst, plDrawableSpans* d, const plVertexSpan* span, bool readOnly) { return false; }

bool plGLPipeline::CloseAccess(plAccessSpan& acc) { return false; }

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

        glDepthMask(GL_TRUE);
        glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        glClearDepth(1.0);

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

void plGLPipeline::RenderScreenElements()
{
    bool reset = false;

    if (fView.HasCullProxy())
    {
        Draw(fView.GetCullProxy());
    }


    hsGMatState tHack = PushMaterialOverride(hsGMatState::kMisc, hsGMatState::kMiscWireFrame, false);
    hsGMatState ambHack = PushMaterialOverride(hsGMatState::kShade, hsGMatState::kShadeWhite, true);

    plProfile_BeginTiming(PlateMgr);
    // Plates
    if (fPlateMgr)
    {
        fPlateMgr->DrawToDevice(this);
        reset = true;
    }
    plProfile_EndTiming(PlateMgr);

    PopMaterialOverride(ambHack, true);
    PopMaterialOverride(tHack, false);

    plProfile_BeginTiming(DebugText);
    /// Debug text
    if (fDebugTextMgr && plDebugText::Instance().IsEnabled())
    {
        fDebugTextMgr->DrawToDevice(this);
        reset = true;
    }
    plProfile_EndTiming(DebugText);

    plProfile_BeginTiming(Reset);
    if (reset)
    {
        fView.fXformResetFlags = fView.kResetAll; // Text destroys view transforms
    }
    plProfile_EndTiming(Reset);
}

bool plGLPipeline::IsFullScreen() const { return false; }

void plGLPipeline::Resize(uint32_t width, uint32_t height) {}

bool plGLPipeline::CheckResources() { return false; }

void plGLPipeline::LoadResources()
{
    if (plGLPlateManager* pm = static_cast<plGLPlateManager*>(fPlateMgr))
    {
        pm->IReleaseGeometry();
        pm->ICreateGeometry();
    }
}

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

#ifdef HS_DEBUGGING
            GLenum e_pre;
            if ((e_pre = glGetError()) != GL_NO_ERROR) {
                hsStatusMessage(ST::format("RenderSpans pre-material failed {}", uint32_t(e_pre)).c_str());
            }
#endif

        if (material != nullptr) {
            // First, do we have a device ref at this index?
            plGLMaterialShaderRef* mRef = (plGLMaterialShaderRef*)material->GetDeviceRef();

            if (mRef == nullptr) {
                mRef = new plGLMaterialShaderRef(material, this);
                material->SetDeviceRef(mRef);
            }

            if (!mRef->IsLinked()) {
                mRef->Link(&fMatRefList);
            }

            glUseProgram(mRef->fRef);
            fDevice.fCurrentProgram = mRef->fRef;

#ifdef HS_DEBUGGING
            GLenum e;
            if ((e = glGetError()) != GL_NO_ERROR) {
                hsStatusMessage(ST::format("Use Program failed {} (Material {})", uint32_t(e), material->GetKeyName()).c_str());
            }
#endif

            // TODO: Figure out how to use VAOs properly :(
            GLuint vao;
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);

            // What do we change?

            plProfile_BeginTiming(SpanTransforms);
            ISetupTransforms(ice, tempIce, lastL2W);
            plProfile_EndTiming(SpanTransforms);

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
            glDeleteVertexArrays(1, &vao);
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

        uniform = glGetUniformLocation(fDevice.fCurrentProgram, "uMatrixC2W");
        if (uniform != -1) {
            glUniformMatrix4fv(uniform, 1, GL_TRUE, fDevice.fMatrixC2W);
        }

        uniform = glGetUniformLocation(fDevice.fCurrentProgram, "uMatrixL2W");
        glUniformMatrix4fv(uniform, 1, GL_TRUE, fDevice.fMatrixL2W);

        uniform = glGetUniformLocation(fDevice.fCurrentProgram, "uMatrixW2L");
        if (uniform != -1) {
            glUniformMatrix4fv(uniform, 1, GL_TRUE, fDevice.fMatrixW2L);
        }
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

        hsAssert( false, ST::format("Trying to render a nil buffer pair! (Mat: {})", material->GetKeyName()).c_str() );
        return;
    }

#ifdef HS_DEBUGGING
    GLenum e;
    if ((e = glGetError()) != GL_NO_ERROR) {
        hsStatusMessage(ST::format("PRE Render failed {}", uint32_t(e)).c_str());
    }
#endif

    mRef->SetupTextureRefs();

    /* Vertex Buffer stuff */
    glBindBuffer(GL_ARRAY_BUFFER, vRef->fRef);

    glEnableVertexAttribArray(kVtxPosition);
    glVertexAttribPointer(kVtxPosition, 3, GL_FLOAT, GL_FALSE, vRef->fVertexSize, 0);

    glEnableVertexAttribArray(kVtxNormal);
    glVertexAttribPointer(kVtxNormal, 3, GL_FLOAT, GL_FALSE, vRef->fVertexSize, (void*)(sizeof(float) * 3));

    glEnableVertexAttribArray(kVtxColor);
    glVertexAttribPointer(kVtxColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, vRef->fVertexSize, (void*)(sizeof(float) * 3 * 2));

    int numUVs = vRef->fOwner->GetNumUVs();
    for (int i = 0; i < numUVs; i++) {
        glEnableVertexAttribArray(kVtxUVWSrc + i);
        glVertexAttribPointer(kVtxUVWSrc + i, 3, GL_FLOAT, GL_FALSE, vRef->fVertexSize, (void*)((sizeof(float) * 3 * 2) + (sizeof(uint32_t) * 2) + (sizeof(float) * 3 * i)));
    }

#ifdef HS_DEBUGGING
    if ((e = glGetError()) != GL_NO_ERROR) {
        hsStatusMessage(ST::format("Vertex Attributes failed {}", uint32_t(e)).c_str());
    }
#endif

    /* Index Buffer stuff and drawing */
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iRef->fRef);

    plRenderTriListFunc render(&fDevice, 0, vStart, vLength, iStart, iLength);

    plProfile_EndTiming(RenderBuff);

#if 1
    // Enable this for LayerAnimations, but the timing/speed seems wrong
    for (size_t i = 0; i < material->GetNumLayers(); i++) {
        plLayerInterface* lay = material->GetLayer(i);
        if (lay) {
            lay->Eval(fTime, fFrame, 0);
        }
    }
#endif

    // Turn on this spans lights and turn off the rest.
    ISelectLights(&span);

    for (size_t pass = 0; pass < mRef->GetNumPasses(); pass++) {
        // Set uniform to current pass
        if (mRef->uPassNumber != -1) {
            glUniform1i(mRef->uPassNumber, pass);
        }

        glUniform1f(mRef->uInvertVtxAlpha, 0.f);

        plLayerInterface* lay = material->GetLayer(mRef->GetPassIndex(pass));

        ICalcLighting(mRef, lay, &span);

        hsGMatState s;
        s.Composite(lay->GetState(), fMatOverOn, fMatOverOff);

        IHandleZMode(s);
        IHandleBlendMode(s);

        if (s.fBlendFlags & (hsGMatState::kBlendTest | hsGMatState::kBlendAlpha | hsGMatState::kBlendAddColorTimesAlpha) &&
            !(s.fBlendFlags & hsGMatState::kBlendAlphaAlways))
        {
            // AlphaTestHigh is used for reducing sort artifacts on textures that
            // are mostly opaque or transparent, but have regions of translucency
            // in transition. Like a texture for a bush billboard. It lets there be
            // some transparency falloff, but quit drawing before it gets so
            // transparent that draw order problems (halos) become apparent.
            if (s.fBlendFlags & hsGMatState::kBlendAlphaTestHigh) {
                glUniform1f(mRef->uAlphaThreshold, 64.f/255.f);
            } else {
                glUniform1f(mRef->uAlphaThreshold, 0.f);
            }
        } else {
            glUniform1f(mRef->uAlphaThreshold, 0.f);
        }

        if (s.fMiscFlags & hsGMatState::kMiscTwoSided) {
            glDisable(GL_CULL_FACE);
        } else {
            glEnable(GL_CULL_FACE);
        }

        // TEMP
        render.RenderPrims();
    }


#ifdef HS_DEBUGGING
    if ((e = glGetError()) != GL_NO_ERROR) {
        hsStatusMessage(ST::format("Render failed {}", uint32_t(e)).c_str());
    }
#endif
}


void plGLPipeline::IHandleZMode(hsGMatState flags)
{
    switch (flags.fZFlags & hsGMatState::kZMask)
    {
        case hsGMatState::kZClearZ:
            glDepthFunc(GL_ALWAYS);
            glDepthMask(GL_TRUE);
            break;
        case hsGMatState::kZNoZRead:
            glDepthFunc(GL_ALWAYS);
            glDepthMask(GL_TRUE);
            break;
        case hsGMatState::kZNoZWrite:
            glDepthFunc(GL_LEQUAL);
            glDepthMask(GL_FALSE);
            break;
        case hsGMatState::kZNoZRead | hsGMatState::kZClearZ:
            glDepthFunc(GL_ALWAYS);
            glDepthMask(GL_TRUE);
            break;
        case hsGMatState::kZNoZRead | hsGMatState::kZNoZWrite:
            glDepthFunc(GL_ALWAYS);
            glDepthMask(GL_FALSE);
            break;
        case 0:
            glDepthFunc(GL_LEQUAL);
            glDepthMask(GL_TRUE);
            break;
        case hsGMatState::kZClearZ | hsGMatState::kZNoZWrite:
        case hsGMatState::kZClearZ | hsGMatState::kZNoZWrite | hsGMatState::kZNoZRead:
            hsAssert(false, "Illegal combination of Z Buffer modes (Clear but don't write)");
            break;
    }

    if (flags.fZFlags & hsGMatState::kZIncLayer) {
        int32_t int_value = 8;
        float value = *(float*)(&int_value);

        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-1.f, 1.f);
    } else {
        glPolygonOffset(0.f, 0.f);
        glDisable(GL_POLYGON_OFFSET_FILL);
    }
}

void plGLPipeline::IHandleBlendMode(hsGMatState flags)
{
    // No color, just writing out Z values.
    if (flags.fBlendFlags & hsGMatState::kBlendNoColor) {
        glBlendFunc(GL_ZERO, GL_ONE);
        flags.fBlendFlags |= 0x80000000;
    } else {
        switch (flags.fBlendFlags & hsGMatState::kBlendMask)
        {
            // Detail is just a special case of alpha, handled in construction of the texture
            // mip chain by making higher levels of the chain more transparent.
            case hsGMatState::kBlendDetail:
            case hsGMatState::kBlendAlpha:
                if (flags.fBlendFlags & hsGMatState::kBlendInvertFinalAlpha) {
                    if (flags.fBlendFlags & hsGMatState::kBlendAlphaPremultiplied) {
                        glBlendFunc(GL_ONE, GL_SRC_ALPHA);
                    } else {
                        glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
                    }
                } else {
                    if (flags.fBlendFlags & hsGMatState::kBlendAlphaPremultiplied) {
                        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                    } else {
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    }
                }
                break;

            // Multiply the final color onto the frame buffer.
            case hsGMatState::kBlendMult:
                if (flags.fBlendFlags & hsGMatState::kBlendInvertFinalColor) {
                    glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
                } else {
                    glBlendFunc(GL_ZERO, GL_SRC_COLOR);
                }
                break;

            // Add final color to FB.
            case hsGMatState::kBlendAdd:
                glBlendFunc(GL_ONE, GL_ONE);
                break;

            // Multiply final color by FB color and add it into the FB.
            case hsGMatState::kBlendMADD:
                glBlendFunc(GL_DST_COLOR, GL_ONE);
                break;

            // Final color times final alpha, added into the FB.
            case hsGMatState::kBlendAddColorTimesAlpha:
                if (flags.fBlendFlags & hsGMatState::kBlendInvertFinalAlpha) {
                    glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_ONE);
                } else {
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                }
                break;

            // Overwrite final color onto FB
            case 0:
                glBlendFunc(GL_ONE, GL_ZERO);
                break;

            default:
                {
                    hsAssert(false, "Too many blend modes specified in material");

#if 0
                    plLayer* lay = plLayer::ConvertNoRef(fCurrMaterial->GetLayer(fCurrLayerIdx)->BottomOfStack());
                    if( lay )
                    {
                        if( lay->GetBlendFlags() & hsGMatState::kBlendAlpha )
                        {
                            lay->SetBlendFlags((lay->GetBlendFlags() & ~hsGMatState::kBlendMask) | hsGMatState::kBlendAlpha);
                        }
                        else
                        {
                            lay->SetBlendFlags((lay->GetBlendFlags() & ~hsGMatState::kBlendMask) | hsGMatState::kBlendAdd);
                        }
                    }
#endif
                }
                break;
        }
    }
}

void plGLPipeline::ICalcLighting(plGLMaterialShaderRef* mRef, const plLayerInterface* currLayer, const plSpan* currSpan)
{
    //plProfile_Inc(MatLightState);

    GLint e;

    if (IsDebugFlagSet(plPipeDbg::kFlagAllBright))
    {
        glUniform4f(mRef->uGlobalAmbient,  1.0, 1.0, 1.0, 1.0);

        glUniform4f(mRef->uMatAmbientCol,  1.0, 1.0, 1.0, 1.0);
        glUniform4f(mRef->uMatDiffuseCol,  1.0, 1.0, 1.0, 1.0);
        glUniform4f(mRef->uMatEmissiveCol, 1.0, 1.0, 1.0, 1.0);
        glUniform4f(mRef->uMatSpecularCol, 1.0, 1.0, 1.0, 1.0);

        glUniform1f(mRef->uMatAmbientSrc,  1.0);
        glUniform1f(mRef->uMatDiffuseSrc,  1.0);
        glUniform1f(mRef->uMatEmissiveSrc, 1.0);
        glUniform1f(mRef->uMatSpecularSrc, 1.0);

        return;
    }

    hsGMatState state;
    state.Composite(currLayer->GetState(), fMatOverOn, fMatOverOff);

    uint32_t mode = (currSpan != nullptr) ? (currSpan->fProps & plSpan::kLiteMask) : plSpan::kLiteMaterial;

    if (state.fMiscFlags & hsGMatState::kMiscBumpChans) {
        mode = plSpan::kLiteMaterial;
        state.fShadeFlags |= hsGMatState::kShadeNoShade | hsGMatState::kShadeWhite;
    }

    /// Select one of our three lighting methods
    switch (mode) {
        case plSpan::kLiteMaterial:     // Material shading
        {
            if (state.fShadeFlags & hsGMatState::kShadeWhite) {
                glUniform4f(mRef->uGlobalAmbient, 1.0, 1.0, 1.0, 1.0);
                glUniform4f(mRef->uMatAmbientCol, 1.0, 1.0, 1.0, 1.0);
            } else if (IsDebugFlagSet(plPipeDbg::kFlagNoPreShade)) {
                glUniform4f(mRef->uGlobalAmbient, 0.0, 0.0, 0.0, 1.0);
                glUniform4f(mRef->uMatAmbientCol, 0.0, 0.0, 0.0, 1.0);
            } else {
                hsColorRGBA amb = currLayer->GetPreshadeColor();
                glUniform4f(mRef->uGlobalAmbient, amb.r, amb.g, amb.b, 1.0);
                glUniform4f(mRef->uMatAmbientCol, amb.r, amb.g, amb.b, 1.0);
            }

            hsColorRGBA dif = currLayer->GetRuntimeColor();
            glUniform4f(mRef->uMatDiffuseCol, dif.r, dif.g, dif.b, currLayer->GetOpacity());

            hsColorRGBA em = currLayer->GetAmbientColor();
            glUniform4f(mRef->uMatEmissiveCol, em.r, em.g, em.b, 1.0);

            // Set specular properties
            if (state.fShadeFlags & hsGMatState::kShadeSpecular) {
                hsColorRGBA spec = currLayer->GetSpecularColor();
                glUniform4f(mRef->uMatSpecularCol, spec.r, spec.g, spec.b, 1.0);
#if 0
                mat.Power = currLayer->GetSpecularPower();
#endif
            } else {
                glUniform4f(mRef->uMatSpecularCol, 0.0, 0.0, 0.0, 0.0);
            }

            glUniform1f(mRef->uMatDiffuseSrc,  1.0);
            glUniform1f(mRef->uMatEmissiveSrc, 1.0);
            glUniform1f(mRef->uMatSpecularSrc, 1.0);

            if (state.fShadeFlags & hsGMatState::kShadeNoShade) {
                glUniform1f(mRef->uMatAmbientSrc, 1.0);
            } else {
                glUniform1f(mRef->uMatAmbientSrc, 0.0);
            }

            break;
        }

        case plSpan::kLiteVtxPreshaded:  // Vtx preshaded
        {
            glUniform4f(mRef->uGlobalAmbient, 0.0, 0.0, 0.0, 0.0);
            glUniform4f(mRef->uMatAmbientCol, 0.0, 0.0, 0.0, 0.0);
            glUniform4f(mRef->uMatDiffuseCol, 0.0, 0.0, 0.0, 0.0);
            glUniform4f(mRef->uMatEmissiveCol, 0.0, 0.0, 0.0, 0.0);
            glUniform4f(mRef->uMatSpecularCol, 0.0, 0.0, 0.0, 0.0);

            glUniform1f(mRef->uMatDiffuseSrc,  0.0);
            glUniform1f(mRef->uMatAmbientSrc,  1.0);
            glUniform1f(mRef->uMatSpecularSrc, 1.0);

            if (state.fShadeFlags & hsGMatState::kShadeEmissive) {
                glUniform1f(mRef->uMatEmissiveSrc, 0.0);
            } else {
                glUniform1f(mRef->uMatEmissiveSrc, 1.0);
            }
            break;
        }

        case plSpan::kLiteVtxNonPreshaded:      // Vtx non-preshaded
        {
            glUniform4f(mRef->uMatAmbientCol, 0.0, 0.0, 0.0, 0.0);
            glUniform4f(mRef->uMatDiffuseCol, 0.0, 0.0, 0.0, 0.0);

            hsColorRGBA em = currLayer->GetAmbientColor();
            glUniform4f(mRef->uMatEmissiveCol, em.r, em.g, em.b, 1.0);

            // Set specular properties
            if (state.fShadeFlags & hsGMatState::kShadeSpecular) {
                hsColorRGBA spec = currLayer->GetSpecularColor();
                glUniform4f(mRef->uMatSpecularCol, spec.r, spec.g, spec.b, 1.0);
#if 0
                mat.Power = currLayer->GetSpecularPower();
#endif
            } else {
                glUniform4f(mRef->uMatSpecularCol, 0.0, 0.0, 0.0, 0.0);
            }

            hsColorRGBA amb = currLayer->GetPreshadeColor();
            glUniform4f(mRef->uGlobalAmbient, amb.r, amb.g, amb.b, amb.a);

            glUniform1f(mRef->uMatAmbientSrc,  0.0);
            glUniform1f(mRef->uMatDiffuseSrc,  0.0);
            glUniform1f(mRef->uMatEmissiveSrc, 1.0);
            glUniform1f(mRef->uMatSpecularSrc, 1.0);
            break;
        }
    }



    // Piggy-back some temporary fog stuff on the lighting...
    const plFogEnvironment* fog = (currSpan ? (currSpan->fFogEnvironment ? currSpan->fFogEnvironment : &fView.GetDefaultFog()) : nullptr);
    uint8_t type = fog ? fog->GetType() : plFogEnvironment::kNoFog;
    hsColorRGBA color;

    switch (type) {
        case plFogEnvironment::kLinearFog:
        {
            float start, end;
            fog->GetPipelineParams(&start, &end, &color);

            if (mRef->uFogExponential != -1) {
                glUniform1i(mRef->uFogExponential, 0);
            }
            if (mRef->uFogValues != -1) {
                glUniform2f(mRef->uFogValues, start, end);
            }
            if (mRef->uFogColor != -1) {
                glUniform3f(mRef->uFogColor, color.r, color.g, color.b);
            }
            break;
        }
        case plFogEnvironment::kExpFog:
        case plFogEnvironment::kExp2Fog:
        {
            float density;
            float power = (type == plFogEnvironment::kExp2Fog) ? 2.0f : 1.0f;
            fog->GetPipelineParams(&density, &color);

            if (mRef->uFogExponential != -1) {
                glUniform1i(mRef->uFogExponential, 1);
            }
            if (mRef->uFogValues != -1) {
                glUniform2f(mRef->uFogValues, power, density);
            }
            if (mRef->uFogColor != -1) {
                glUniform3f(mRef->uFogColor, color.r, color.g, color.b);
            }
            break;
        }
        default:
            if (mRef->uFogExponential != -1) {
                glUniform1i(mRef->uFogExponential, 0);
            }
            if (mRef->uFogValues != -1) {
                glUniform2f(mRef->uFogValues, 0.0, 0.0);
            }
            if (mRef->uFogColor != -1) {
                glUniform3f(mRef->uFogColor, 0.0, 0.0, 0.0);
            }
            break;
    }
}


void plGLPipeline::ISelectLights(const plSpan* span, bool proj)
{
    const size_t numLights = 8;
    size_t i = 0;
    int32_t startScale;
    float threshhold;
    float overHold = 0.3;
    float scale;

    if  (!IsDebugFlagSet(plPipeDbg::kFlagNoRuntimeLights) &&
        !(IsDebugFlagSet(plPipeDbg::kFlagNoApplyProjLights) && proj) &&
        !(IsDebugFlagSet(plPipeDbg::kFlagOnlyApplyProjLights) && !proj))
    {
        hsTArray<plLightInfo*>& spanLights = span->GetLightList(proj);

        for (i = 0; i < spanLights.GetCount() && i < numLights; i++) {
            IEnableLight(i, spanLights[i]);
        }
        startScale = i;

        /// Attempt #2: Take some of the n strongest lights (below a given threshhold) and
        /// fade them out to nothing as they get closer to the bottom. This way, they fade
        /// out of existence instead of pop out.

        if (i < spanLights.GetCount() - 1 && i > 0) {
            threshhold = span->GetLightStrength(i, proj);
            i--;
            overHold = threshhold * 1.5f;

            if (overHold > span->GetLightStrength(0, proj)) {
                overHold = span->GetLightStrength(0, proj);
            }

            for (; i > 0 && span->GetLightStrength(i, proj) < overHold; i--) {
                scale = (overHold - span->GetLightStrength(i, proj)) / (overHold - threshhold);

                IScaleLight(i, (1 - scale) * span->GetLightScale(i, proj));
            }
            startScale = i + 1;
        }


        /// Make sure those lights that aren't scaled....aren't
        for (i = 0; i < startScale; i++) {
            IScaleLight(i, span->GetLightScale(i, proj));
        }
    }

    for (; i < numLights; i++) {
        IDisableLight(i);
    }
}

void plGLPipeline::IEnableLight(size_t i, plLightInfo* light)
{
    GLuint position     = glGetUniformLocation(fDevice.fCurrentProgram, ST::format("uLampSources[{}].position", i).c_str());
    GLuint ambient      = glGetUniformLocation(fDevice.fCurrentProgram, ST::format("uLampSources[{}].ambient", i).c_str());
    GLuint diffuse      = glGetUniformLocation(fDevice.fCurrentProgram, ST::format("uLampSources[{}].diffuse", i).c_str());
    GLuint specular     = glGetUniformLocation(fDevice.fCurrentProgram, ST::format("uLampSources[{}].specular", i).c_str());
    GLuint constAtten   = glGetUniformLocation(fDevice.fCurrentProgram, ST::format("uLampSources[{}].constAtten", i).c_str());
    GLuint linAtten     = glGetUniformLocation(fDevice.fCurrentProgram, ST::format("uLampSources[{}].linAtten", i).c_str());
    GLuint quadAtten    = glGetUniformLocation(fDevice.fCurrentProgram, ST::format("uLampSources[{}].quadAtten", i).c_str());
    GLuint scale        = glGetUniformLocation(fDevice.fCurrentProgram, ST::format("uLampSources[{}].scale", i).c_str());

    hsColorRGBA amb = light->GetAmbient();
    glUniform4f(ambient, amb.r, amb.g, amb.b, amb.a);

    hsColorRGBA diff = light->GetDiffuse();
    glUniform4f(diffuse, diff.r, diff.g, diff.b, diff.a);

    hsColorRGBA spec = light->GetSpecular();
    glUniform4f(specular, spec.r, spec.g, spec.b, spec.a);

    plDirectionalLightInfo* dirLight = nullptr;
    plOmniLightInfo* omniLight = nullptr;

    if ((dirLight = plDirectionalLightInfo::ConvertNoRef(light)) != nullptr)
    {
        hsVector3 direction = dirLight->GetWorldDirection();
        glUniform4f(position, direction.fX, direction.fY, direction.fZ, 0.0);

        glUniform1f(constAtten, 1.0f);
        glUniform1f(linAtten, 0.0f);
        glUniform1f(quadAtten, 0.0f);
    }
    else if ((omniLight = plOmniLightInfo::ConvertNoRef(light)) != nullptr)
    {
        hsPoint3 pos = omniLight->GetWorldPosition();
        glUniform4f(position, pos.fX, pos.fY, pos.fZ, 1.0);

        // TODO: Maximum Range

        glUniform1f(constAtten, omniLight->GetConstantAttenuation());
        glUniform1f(linAtten, omniLight->GetLinearAttenuation());
        glUniform1f(quadAtten, omniLight->GetQuadraticAttenuation());
    }
    else {
        IDisableLight(i);
    }
}

void plGLPipeline::IDisableLight(size_t i)
{
    GLuint position     = glGetUniformLocation(fDevice.fCurrentProgram, ST::format("uLampSources[{}].position", i).c_str());
    GLuint ambient      = glGetUniformLocation(fDevice.fCurrentProgram, ST::format("uLampSources[{}].ambient", i).c_str());
    GLuint diffuse      = glGetUniformLocation(fDevice.fCurrentProgram, ST::format("uLampSources[{}].diffuse", i).c_str());
    GLuint specular     = glGetUniformLocation(fDevice.fCurrentProgram, ST::format("uLampSources[{}].specular", i).c_str());
    GLuint constAtten   = glGetUniformLocation(fDevice.fCurrentProgram, ST::format("uLampSources[{}].constAtten", i).c_str());
    GLuint linAtten     = glGetUniformLocation(fDevice.fCurrentProgram, ST::format("uLampSources[{}].linAtten", i).c_str());
    GLuint quadAtten    = glGetUniformLocation(fDevice.fCurrentProgram, ST::format("uLampSources[{}].quadAtten", i).c_str());
    GLuint scale        = glGetUniformLocation(fDevice.fCurrentProgram, ST::format("uLampSources[{}].scale", i).c_str());

    if (position != -1)     glUniform4f(position, 0.0f, 0.0f, 0.0f, 0.0f);
    if (ambient != -1)      glUniform4f(ambient, 0.0f, 0.0f, 0.0f, 0.0f);
    if (diffuse != -1)      glUniform4f(diffuse, 0.0f, 0.0f, 0.0f, 0.0f);
    if (specular != -1)     glUniform4f(specular, 0.0f, 0.0f, 0.0f, 0.0f);
    if (constAtten != -1)   glUniform1f(constAtten, 1.0f);
    if (linAtten != -1)     glUniform1f(linAtten, 0.0f);
    if (quadAtten != -1)    glUniform1f(quadAtten, 0.0f);
    if (scale != -1)        glUniform1f(scale, 0.0f);
}

void plGLPipeline::IScaleLight(size_t i, float scale)
{
    GLuint uniform = glGetUniformLocation(fDevice.fCurrentProgram, ST::format("uLampSources[{}].scale", i).c_str());
    if (uniform != -1) glUniform1f(uniform, scale);
}

void plGLPipeline::IDrawPlate(plPlate* plate)
{
    hsGMaterial* material = plate->GetMaterial();

    // To override the transform done by the z-bias
    static float projMat[16] = {
         1.0f,  0.0f,  0.0f,  0.0f,
         0.0f, -1.0f,  0.0f,  0.0f,
         0.0f,  0.0f,  2.0f, -2.0f,
         0.0f,  0.0f,  1.0f,  0.0f
    };

    /// Set up the transform directly
    fDevice.SetLocalToWorldMatrix(plate->GetTransform());

    //IPushPiggyBacks(material);

    // First, do we have a device ref at this index?
    plGLMaterialShaderRef* mRef = (plGLMaterialShaderRef*)material->GetDeviceRef();

    if (mRef == nullptr) {
        mRef = new plGLMaterialShaderRef(material, this);
        material->SetDeviceRef(mRef);
    }

    if (!mRef->IsLinked()) {
        mRef->Link(&fMatRefList);
    }

    glUseProgram(mRef->fRef);
    fDevice.fCurrentProgram = mRef->fRef;

    mRef->SetupTextureRefs();

    /* Push the matrices into the GLSL shader now */
    GLint uniform = glGetUniformLocation(fDevice.fCurrentProgram, "uMatrixProj");
    glUniformMatrix4fv(uniform, 1, GL_TRUE, projMat);

    uniform = glGetUniformLocation(fDevice.fCurrentProgram, "uMatrixW2C");
    glUniformMatrix4fv(uniform, 1, GL_TRUE, fDevice.fMatrixW2C);

    uniform = glGetUniformLocation(fDevice.fCurrentProgram, "uMatrixC2W");
    if (uniform != -1) {
        glUniformMatrix4fv(uniform, 1, GL_TRUE, fDevice.fMatrixC2W);
    }

    uniform = glGetUniformLocation(fDevice.fCurrentProgram, "uMatrixL2W");
    glUniformMatrix4fv(uniform, 1, GL_TRUE, fDevice.fMatrixL2W);

    uniform = glGetUniformLocation(fDevice.fCurrentProgram, "uMatrixW2L");
    if (uniform != -1) {
        glUniformMatrix4fv(uniform, 1, GL_TRUE, fDevice.fMatrixW2L);
    }

    glUniform4f(mRef->uGlobalAmbient,  1.0, 1.0, 1.0, 1.0);

    glUniform4f(mRef->uMatAmbientCol,  1.0, 1.0, 1.0, 1.0);
    glUniform4f(mRef->uMatDiffuseCol,  1.0, 1.0, 1.0, 1.0);
    glUniform4f(mRef->uMatEmissiveCol, 1.0, 1.0, 1.0, 1.0);
    glUniform4f(mRef->uMatSpecularCol, 1.0, 1.0, 1.0, 1.0);

    glUniform1f(mRef->uMatAmbientSrc,  1.0);
    glUniform1f(mRef->uMatDiffuseSrc,  1.0);
    glUniform1f(mRef->uMatEmissiveSrc, 1.0);
    glUniform1f(mRef->uMatSpecularSrc, 1.0);

    glDrawElements(GL_TRIANGLE_STRIP, 6, GL_UNSIGNED_SHORT, (GLvoid*)(sizeof(uint16_t) * 0));

    //IPopPiggyBacks();
}




///////////////////////////////////////////////////////////////////////////////
//// Functions from Other Classes That Need to Be Here to Compile Right ///////
///////////////////////////////////////////////////////////////////////////////

plPipeline* plPipelineCreate::ICreateGLPipeline(hsWindowHndl disp, hsWindowHndl hWnd, const hsG3DDeviceModeRecord* devMode)
{
    plGLPipeline* pipe = new plGLPipeline(disp, hWnd, devMode);
    return pipe;
}
