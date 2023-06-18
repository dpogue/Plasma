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

#include "pmDebugMgr.h"

#include <cstdio>
#include <string_theory/format>
#include "hsBounds.h"
#include "hsResMgr.h"
#include "plgDispatch.h"
#include "plClassIndexMacros.h"
#include "plCreatableIndex.h"
#include "pnFactory/plFactory.h"
#include "pnKeyedObject/plUoid.h"
#include "pnMessage/plAttachMsg.h"
#include "pnMessage/plClientMsg.h"
#include "pnMessage/plIntRefMsg.h"
#include "pnMessage/plNodeRefMsg.h"
#include "pnMessage/plObjRefMsg.h"
#include "pnMessage/plRefMsg.h"
#include "pnSceneObject/plCoordinateInterface.h"
#include "pnSceneObject/plDrawInterface.h"
#include "pnSceneObject/plSceneObject.h"
#include "plDrawable/plDrawableSpans.h"
#include "plDrawable/plGeometrySpan.h"
#include "plMessage/plMatRefMsg.h"
#include "plScene/plPostEffectMod.h"
#include "plScene/plSceneNode.h"
#include "plSurface/hsGMaterial.h"
#include "plSurface/plLayer.h"
#include "pfGameGUIMgr/pfGUIButtonMod.h"
#include "pfGameGUIMgr/pfGUIDialogMod.h"
#include "pfMessage/pfGameGUIMsg.h"

#define FACTORY_NEW(cls) static_cast<cls*>(plFactory::Create(CLASS_INDEX_SCOPED(cls)));

pmDebugMgr* pmDebugMgr::fInstance = nullptr;

pmDebugMgr* pmDebugMgr::Instance()
{
    if (!pmDebugMgr::fInstance) {
        pmDebugMgr::fInstance = new pmDebugMgr();
        pmDebugMgr::fInstance->IInit();
    }

    return pmDebugMgr::fInstance;
}

pmDebugMgr::pmDebugMgr()
    : fDialog()
{
    plUoid uoid(plLocation::kGlobalFixedLoc, ClassIndex(), "_PlasmaModuleDebugMgr");
    fMyKey = hsgResMgr::ResMgr()->NewKey(uoid, this);
}

pmDebugMgr::~pmDebugMgr()
{
}

void pmDebugMgr::IInit()
{
    fprintf(stderr, "Initialized the Debug Manager from ModDLL\n");

    plgDispatch::Dispatch()->RegisterForExactType(plClientMsg::Index(), fMyKey);
}

plKey pmDebugMgr::IAddKey(hsKeyedObject* ko, const ST::string& prefix)
{
    static uint32_t keyCount = 0;

    ST::string keyName = ST::format("{}{}", prefix, keyCount++);
    return hsgResMgr::ResMgr()->NewKey(keyName, ko, plLocation::kGlobalFixedLoc);
}

static plGenRefMsg* IMakeGenRefMsg(const plKey& r, uint8_t c, int32_t w, uint8_t t)
{
    plGenRefMsg* refMsg = FACTORY_NEW(plGenRefMsg);
    refMsg->AddReceiver(r);
    refMsg->SetContext(c);
    refMsg->fWhich = w;
    refMsg->fType = t;
    return refMsg;
}

static plObjRefMsg* IMakeObjRefMsg(const plKey& r, uint8_t c, int8_t w, uint8_t t)
{
    plObjRefMsg* refMsg = FACTORY_NEW(plObjRefMsg);
    refMsg->AddReceiver(r);
    refMsg->SetContext(c);
    refMsg->fWhich = w;
    refMsg->fType = t;
    return refMsg;
}

static plNodeRefMsg* IMakeNodeRefMsg(const plKey& r, uint8_t c, int8_t w, int8_t t)
{
    plNodeRefMsg* refMsg = FACTORY_NEW(plNodeRefMsg);
    refMsg->AddReceiver(r);
    refMsg->SetContext(c);
    refMsg->fWhich = w;
    refMsg->fType = t;
    return refMsg;
}

static plMatRefMsg* IMakeMatRefMsg(const plKey& r, uint8_t c, int32_t w, uint8_t t)
{
    plMatRefMsg* refMsg = FACTORY_NEW(plMatRefMsg);
    refMsg->AddReceiver(r);
    refMsg->SetContext(c);
    refMsg->fWhich = w;
    refMsg->fType = t;
    return refMsg;
}

static plIntRefMsg* IMakeIntRefMsg(const plKey& r, uint8_t c, int32_t w, uint8_t t)
{
    plIntRefMsg* refMsg = FACTORY_NEW(plIntRefMsg);
    refMsg->AddReceiver(r);
    refMsg->SetContext(c);
    refMsg->fWhich = w;
    refMsg->fType = t;
    return refMsg;
}

void pmDebugMgr::ICreateDebugButton()
{
    fprintf(stderr, "Initializing the Debug Manager Dialog\n");

    plKey gameGUIMgrKey = hsgResMgr::ResMgr()->FindKey(plUoid(plFixedKeyId::kGameGUIMgr_KEY));
    float fov = atan(20.f / 200.f) * 2.f;
    hsColorRGBA black, white, grey;

    black.Set(0.f, 0.f, 0.f, 1.f);
    white.Set(1.f, 1.f, 1.f, 1.f);
    grey.Set(0.5f, 0.5f, 0.5f, 1.f);

    //
    // Set up the dialog -----------------------------------------------------
    //

    plPostEffectMod* renderMod = FACTORY_NEW(plPostEffectMod);
    IAddKey(renderMod, ST_LITERAL("DebugGUIRenderMod"));
    renderMod->SetHither(0.5f);
    renderMod->SetYon(200.0f);
    renderMod->SetFovX(hsRadiansToDegrees(fov));
    renderMod->SetFovY(hsRadiansToDegrees(fov));

    plSceneNode* node = FACTORY_NEW(plSceneNode);
    IAddKey(node, ST_LITERAL("DebugMgrSceneNode"));
    node->GetKey()->RefObject();

    hsgResMgr::ResMgr()->AddViaNotify(node->GetKey(), IMakeGenRefMsg(renderMod->GetKey(), plRefMsg::kOnCreate, 0, plPostEffectMod::kNodeRef), plRefFlags::kPassiveRef);

    fDialog = FACTORY_NEW(pfGUIDialogMod);
    IAddKey(fDialog, ST_LITERAL("DebugGUIDialog"));
    fDialog->SetRenderMod(renderMod);
    fDialog->SetName(ST_LITERAL("DebugGUIDialog"));

    plSceneObject* dlgSO = FACTORY_NEW(plSceneObject);
    IAddKey(dlgSO, ST_LITERAL("DebugGUISceneObject"));

    plCoordinateInterface* dlgCI = FACTORY_NEW(plCoordinateInterface);
    IAddKey(dlgCI, ST_LITERAL("DebugGUICoordIFace"));

    hsMatrix44 l2w, w2l;
    l2w.Reset();
    l2w.GetInverse(&w2l);

    hsgResMgr::ResMgr()->SendRef(fDialog->GetKey(), IMakeObjRefMsg(dlgSO->GetKey(), plRefMsg::kOnCreate, 0, plObjRefMsg::kModifier), plRefFlags::kActiveRef);
    hsgResMgr::ResMgr()->AddViaNotify(dlgCI->GetKey(), IMakeObjRefMsg(dlgSO->GetKey(), plRefMsg::kOnCreate, 0, plObjRefMsg::kInterface), plRefFlags::kActiveRef);
    hsgResMgr::ResMgr()->AddViaNotify(renderMod->GetKey(), IMakeObjRefMsg(dlgSO->GetKey(), plRefMsg::kOnCreate, 0, plObjRefMsg::kModifier), plRefFlags::kActiveRef);

    // Register the dialog with the Game GUI Manager
    hsgResMgr::ResMgr()->AddViaNotify(fDialog->GetKey(), IMakeGenRefMsg(gameGUIMgrKey, plRefMsg::kOnCreate, 0, pfGameGUIMgr::kDlgModRef), plRefFlags::kActiveRef);

    dlgSO->SetSceneNode(node->GetKey());
    dlgCI->SetLocalToParent(l2w, w2l);

    //
    // Now set up the button -------------------------------------------------
    //

    hsGMaterial* btnMat = FACTORY_NEW(hsGMaterial);
    IAddKey(btnMat, ST_LITERAL("DebugGUIMaterial"));

    plLayer* btnLayerBase = FACTORY_NEW(plLayer);
    IAddKey(btnLayerBase, ST_LITERAL("DebugGUIButtonLayer"));
    btnLayerBase->SetRuntimeColor(black);
    btnLayerBase->SetPreshadeColor(black);
    btnLayerBase->SetAmbientColor(grey);
    btnLayerBase->SetOpacity(1.f);

    plLayer* btnLayerText = FACTORY_NEW(plLayer);
    IAddKey(btnLayerText, ST_LITERAL("DebugGUIButtonLayer"));
    btnLayerText->SetRuntimeColor(black);
    btnLayerText->SetPreshadeColor(black);
    btnLayerText->SetAmbientColor(white);
    btnLayerText->SetOpacity(1.f);

    hsgResMgr::ResMgr()->SendRef(btnLayerBase->GetKey(), IMakeMatRefMsg(btnMat->GetKey(), plRefMsg::kOnRequest, 0, plMatRefMsg::kLayer), plRefFlags::kActiveRef);
    //hsgResMgr::ResMgr()->SendRef(btnLayerText->GetKey(), IMakeMatRefMsg(btnMat->GetKey(), plRefMsg::kOnRequest, 1, plMatRefMsg::kLayer), plRefFlags::kActiveRef);

    std::vector<plGeometrySpan*> spanArray { new plGeometrySpan() };
    plGeometrySpan* btnGeoSpan = spanArray[0];
    btnGeoSpan->fMaterial = btnMat;
    btnGeoSpan->fLocalToWorld = l2w;
    btnGeoSpan->fWorldToLocal = w2l;
    btnGeoSpan->fFormat = 1;

    float x = (0.0f - 0.5f) * 20.f;
    float y = (0.0f - 0.5f) * 20.f;
    float width = 1.0f * 20.f;
    float height = 1.0f * 20.f;

    hsPoint3 corner(x, -y, -100.f);
    hsVector3 xVec(width, 0.f, 0.f);
    hsVector3 yVec(0.f, height, 0.f);
    hsVector3 normal = (xVec) % (yVec);
    normal.Normalize();

    // Okay, so this is hacky but hopefully efficient...
    // The format for each vertex is position, normal, UVW
    btnGeoSpan->fVertexData = new uint8_t[sizeof(float) * 9 * 4];
    hsPoint3* data = (hsPoint3*)btnGeoSpan->fVertexData;
    data[0]  = corner;
    data[1]  = hsPoint3(normal);
    data[2]  = hsPoint3(0.f, 1.f, 0.f);
    data[3]  = corner + xVec;
    data[4]  = hsPoint3(normal);
    data[5]  = hsPoint3(1.f, 1.f, 0.f);
    data[6]  = corner + xVec + yVec;
    data[7]  = hsPoint3(normal);
    data[8]  = hsPoint3(1.f, 0.f, 0.f);
    data[9]  = corner + yVec;
    data[10] = hsPoint3(normal);
    data[11] = hsPoint3(0.f, 0.f, 0.f);

    hsBounds3Ext bounds;
    bounds.MakeEmpty();
    bounds.Union(&data[0]);
    bounds.Union(&data[6]);

    btnGeoSpan->fLocalBounds = bounds;
    btnGeoSpan->fWorldBounds = bounds;

    btnGeoSpan->fDiffuseRGBA = new uint32_t[4];
    btnGeoSpan->fDiffuseRGBA[0] = 0xffffffff;
    btnGeoSpan->fDiffuseRGBA[1] = 0xffffffff;
    btnGeoSpan->fDiffuseRGBA[2] = 0xffffffff;
    btnGeoSpan->fDiffuseRGBA[3] = 0xffffffff;

    btnGeoSpan->fSpecularRGBA = new uint32_t[4];
    btnGeoSpan->fSpecularRGBA[0] = 0;
    btnGeoSpan->fSpecularRGBA[1] = 0;
    btnGeoSpan->fSpecularRGBA[2] = 0;
    btnGeoSpan->fSpecularRGBA[3] = 0;

    btnGeoSpan->fMultColor = new hsColorRGBA[4];
    btnGeoSpan->fMultColor[0] = black;
    btnGeoSpan->fMultColor[1] = black;
    btnGeoSpan->fMultColor[2] = black;
    btnGeoSpan->fMultColor[3] = black;

    btnGeoSpan->fAddColor = new hsColorRGBA[4];
    btnGeoSpan->fAddColor[0] = black;
    btnGeoSpan->fAddColor[1] = black;
    btnGeoSpan->fAddColor[2] = black;
    btnGeoSpan->fAddColor[3] = black;

    btnGeoSpan->fIndexData = new uint16_t[6];
    btnGeoSpan->fIndexData[0] = 0;
    btnGeoSpan->fIndexData[1] = 1;
    btnGeoSpan->fIndexData[2] = 2;
    btnGeoSpan->fIndexData[3] = 0;
    btnGeoSpan->fIndexData[4] = 2;
    btnGeoSpan->fIndexData[5] = 3;

    plDrawableSpans* btnSpan = FACTORY_NEW(plDrawableSpans);
    IAddKey(btnSpan, ST_LITERAL("DebugGUISpans"));

    uint32_t idx = btnSpan->AppendDISpans(spanArray, uint32_t(-1), false);

    hsgResMgr::ResMgr()->SendRef(btnSpan->GetKey(), IMakeNodeRefMsg(node->GetKey(), plRefMsg::kOnCreate, 0, plNodeRefMsg::kDrawable), plRefFlags::kActiveRef);

    plDrawInterface* btnDI = FACTORY_NEW(plDrawInterface);
    IAddKey(btnDI, ST_LITERAL("DebugGUIDrawIFace"));

    plCoordinateInterface* btnCI = FACTORY_NEW(plCoordinateInterface);
    IAddKey(btnCI, ST_LITERAL("DebugGUICoordIFace"));

    plSceneObject* btnSO = FACTORY_NEW(plSceneObject);
    IAddKey(btnSO, ST_LITERAL("DebugGUISceneObject"));

    hsgResMgr::ResMgr()->SendRef(btnCI->GetKey(), IMakeObjRefMsg(btnSO->GetKey(), plRefMsg::kOnCreate, 0, plObjRefMsg::kInterface), plRefFlags::kActiveRef);
    hsgResMgr::ResMgr()->SendRef(btnDI->GetKey(), IMakeObjRefMsg(btnSO->GetKey(), plRefMsg::kOnCreate, 0, plObjRefMsg::kInterface), plRefFlags::kActiveRef);
    hsgResMgr::ResMgr()->SendRef(btnSpan->GetKey(), IMakeIntRefMsg(btnDI->GetKey(), plRefMsg::kOnCreate, 0, plIntRefMsg::kDrawable), plRefFlags::kActiveRef);

    plAttachMsg* attachMsg = FACTORY_NEW(plAttachMsg);
    attachMsg->AddReceiver(dlgSO->GetKey());
    attachMsg->SetContext(plRefMsg::kOnRequest);
    hsgResMgr::ResMgr()->SendRef(btnCI->GetKey(), attachMsg, plRefFlags::kActiveRef);

    btnSO->SetSceneNode(node->GetKey());

    pfGUIButtonMod* btnMod = FACTORY_NEW(pfGUIButtonMod);
    IAddKey(btnMod, ST_LITERAL("DebugGUIButton"));
    hsgResMgr::ResMgr()->SendRef(btnMod->GetKey(), IMakeObjRefMsg(btnSO->GetKey(), plRefMsg::kOnCreate, 0, plObjRefMsg::kModifier), plRefFlags::kActiveRef);

    fDialog->AddControlOnExport(btnMod);

    fprintf(stderr, "Making visible the Debug Manager Dialog\n");

    // Tell the Game GUI Manager to show the dialog
    pfGameGUIMsg* showMsg = FACTORY_NEW(pfGameGUIMsg);
    showMsg->AddReceiver(gameGUIMgrKey);
    showMsg->SetString(fDialog->GetName());
    plgDispatch::MsgSend(showMsg);
}

bool pmDebugMgr::MsgReceive(plMessage* msg)
{
    plClientMsg* clientMsg = plClientMsg::ConvertNoRef(msg);
    if (clientMsg) {
        if (clientMsg->GetClientMsgFlag() == plClientMsg::kInitComplete) {
            ICreateDebugButton();
        }

        return true;
    }

    return hsKeyedObject::MsgReceive(msg);
}


// Duplicating IClearMembers here to avoid linking with plDrawable
plGeometrySpan::plGeometrySpan()
{
    fVertexData = nullptr;
    fIndexData = nullptr;
    fMaterial = nullptr;
    fNumVerts = fNumIndices = 0; 
    fBaseMatrix = fNumMatrices = 0;
    fLocalUVWChans = 0;
    fMaxBoneIdx = 0;
    fPenBoneIdx = 0;
    fCreating = false;
    fFogEnviron = nullptr;
    fProps = 0;
    fMinDist = fMaxDist = -1.f;
    fWaterHeight = 0;
    fMultColor = nullptr;
    fAddColor = nullptr;
    fDiffuseRGBA = nullptr;
    fSpecularRGBA = nullptr;
    fInstanceRefs = nullptr;
    fInstanceGroupID = kNoGroupID;
    fSpanRefIndex = (uint32_t)-1;
    fLocalToOBB.Reset();
    fOBBToLocal.Reset();
    fDecalLevel = 0;
    fMaxOwner = ST::string();
}

void pfGUIDialogMod::AddControlOnExport(pfGUIControlMod *ctrl)
{
    fControls.emplace_back(ctrl);
    hsgResMgr::ResMgr()->AddViaNotify(ctrl->GetKey(), IMakeGenRefMsg(GetKey(), plRefMsg::kOnCreate, fControls.size() - 1, pfGUIDialogMod::kControlRef), plRefFlags::kActiveRef);
}
