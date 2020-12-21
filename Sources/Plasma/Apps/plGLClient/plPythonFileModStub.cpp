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
//////////////////////////////////////////////////////////////////////
//
// plPythonFileMod   - the 'special' Python File modifier.
//
// This modifier will handle the interface to python code that has been file-ized.
//
//////////////////////////////////////////////////////////////////////////

#include <locale>
#include "HeadSpin.h"
#include "plgDispatch.h"
#include "hsResMgr.h"
#include "hsStream.h"

#include "plPythonFileModStub.h"

#include "plResMgr/plKeyFinder.h"
#include "pnKeyedObject/plKeyImp.h"
#include "pnKeyedObject/plUoid.h"
#include "plModifier/plSDLModifier.h"

#include "pnSceneObject/plSceneObject.h"
#include "pnSceneObject/plCoordinateInterface.h"
#include "pnKeyedObject/plKey.h"
#include "pnMessage/plTimeMsg.h"
#include "pnMessage/plCmdIfaceModMsg.h"
#include "plMessage/plInputEventMsg.h"
#include "plModifier/plLogicModifier.h"
#include "pfMessage/pfGUINotifyMsg.h"
#include "plMessage/plRoomLoadNotifyMsg.h"
#include "pfMessage/plClothingMsg.h"
#include "pfMessage/pfKIMsg.h"
#include "plMessage/plMemberUpdateMsg.h"
#include "plMessage/plAgeLoadedMsg.h"
#include "pnMessage/plRemoteAvatarInfoMsg.h"
#include "pnMessage/plPlayerPageMsg.h"
#include "plNetClient/plNetClientMgr.h"
#include "plNetTransport/plNetTransportMember.h"
#include "pnMessage/plSDLNotificationMsg.h"
#include "plMessage/plNetOwnershipMsg.h"
#include "plSDL/plSDL.h"
#include "plVault/plVault.h"
#include "plMessage/plCCRMsg.h"
#include "plMessage/plVaultNotifyMsg.h"
#include "plInputCore/plInputInterfaceMgr.h"
#include "plInputCore/plInputDevice.h"
#include "pfMessage/pfMarkerMsg.h"
#include "pfMessage/pfBackdoorMsg.h"
#include "plMessage/plAvatarMsg.h"
#include "plMessage/plLOSHitMsg.h"
#include "plMessage/plRenderMsg.h"
#include "pfMessage/pfMovieEventMsg.h"
#include "plMessage/plClimbEventMsg.h"
#include "plMessage/plCaptureRenderMsg.h"
#include "plGImage/plMipmap.h"
#include "plMessage/plAccountUpdateMsg.h"
#include "plAgeLoader/plAgeLoader.h"
#include "plMessage/plAIMsg.h"
#include "plAvatar/plAvBrainCritter.h"
#include "pfMessage/pfGameScoreMsg.h"

#include "plProfile.h"

//#include "plPythonSDLModifier.h"

#include "plMessage/plTimerCallbackMsg.h"

plProfile_CreateTimer("Update", "Python", PythonUpdate);

/////////////////////////////////////////////////////////////////////////////
//
// fFunctionNames    - the actual names of the functions for On[event] types
//
const char* plPythonFileMod::fFunctionNames[] = 
{
    "OnFirstUpdate",        // kfunc_FirstUpdate
    "OnUpdate",             // kfunc_Update
    "OnNotify",             // kfunc_Notify
    "OnTimer",              // kfunc_AtTimer
    "OnControlKeyEvent",    // kfunc_OnKeyEvent
    "Load",                 // kfunc_Load
    "Save",                 // kfunc_Save
    "OnGUINotify",          // kfunc_GUINotify
    "OnPageLoad",           // kfunc_PageLoad
    "OnClothingUpdate",     // kfunc_ClothingUpdate
    "OnKIMsg",              // kfunc_KIMsg,
    "OnMemberUpdate",       // kfunc_MemberUpdate,
    "OnRemoteAvatarInfo",   // kfunc_RemoteAvatarInfo,
    "OnRTChat",             // kfunc_RTChat,
    "OnVaultEvent",         // kfunc_VaultEvent,
    "AvatarPage",           // kfunc_AvatarPage,
    "OnSDLNotify",          // kfunc_SDLNotify
    "OnOwnershipChanged",   // kfunc_OwnershipNotify
    "OnAgeVaultEvent",      // kfunc_AgeVaultEvent
    "OnInit",               // kfunc_Init,
    "OnCCRMsg",             // kfunc_OnCCRMsg,
    "OnServerInitComplete", // kfunc_OnServerInitComplete
    "OnVaultNotify",        // kfunc_OnVaultNotify
    "OnDefaultKeyCaught",   // kfunc_OnDefaultKeyCaught
    "OnMarkerMsg",          // kfunc_OnMarkerMsg,
    "OnBackdoorMsg",        // kfunc_OnBackdoorMsg,
    "OnBehaviorNotify",     // kfunc_OnBehaviorNotify,
    "OnLOSNotify",          // kfunc_OnLOSNotify,
    "BeginAgeUnLoad",       // kfunc_OnBeginAgeLoad,
    "OnMovieEvent",         // kfunc_OnMovieEvent,
    "OnScreenCaptureDone",  // kfunc_OnScreenCaptureDone,
    "OnClimbingBlockerEvent",// kFunc_OnClimbingBlockerEvent,
    "OnAvatarSpawn",        // kFunc_OnAvatarSpawn
    "OnAccountUpdate",      // kFunc_OnAccountUpdate
    "gotPublicAgeList",     // kfunc_gotPublicAgeList
    "OnAIMsg",              // kfunc_OnAIMsg
    "OnGameScoreMsg",       // kfunc_OnGameScoreMsg
    nullptr
};

bool plPythonFileMod::fAtConvertTime = false;

/////////////////////////////////////////////////////////////////////////////
//
//  Function   : plPythonFileMod and ~plPythonFileMod
//  PARAMETERS : none
//
//  PURPOSE    : Constructor and destructor
//
plPythonFileMod::plPythonFileMod()
{
    fModule = nil;
    fLocalNotify= true;
    fIsFirstTimeEval = true;
    fVaultCallback = nil;
    fSDLMod = nil;
    fSelfKey = nil;
    fInstance = nil;
    fKeyCatcher = nil;
    fPipe = nil;
    fAmIAttachedToClone = false;
}

plPythonFileMod::~plPythonFileMod()
{
}


/////////////////////////////////////////////////////////////////////////////
//
//  Function   : AddTarget
//  PARAMETERS : sobj  - object to add as our target
//
//  PURPOSE    : Get the Key of our target
//
// NOTE: This modifier wasn't intended to have multiple targets
//
void plPythonFileMod::AddTarget(plSceneObject* sobj)
{
    plMultiModifier::AddTarget(sobj);
    plgDispatch::Dispatch()->RegisterForExactType(plEvalMsg::Index(), GetKey());
    plgDispatch::Dispatch()->RegisterForExactType(plPlayerPageMsg::Index(), GetKey());
    plgDispatch::Dispatch()->RegisterForExactType(plAgeBeginLoadingMsg::Index(), GetKey());
    plgDispatch::Dispatch()->RegisterForExactType(plInitialAgeStateLoadedMsg::Index(), GetKey());
}

void plPythonFileMod::RemoveTarget(plSceneObject* so)
{
    // remove sdl modifier
    if (fSDLMod) {
        if (GetNumTargets()) {
            plSceneObject* sceneObj = plSceneObject::ConvertNoRef(GetTarget(0)->GetKey()->ObjectIsLoaded());
            if (sceneObj && fSDLMod)
                sceneObj->RemoveModifier(fSDLMod);
        }
        delete fSDLMod;
        fSDLMod = nullptr;
    }

    plMultiModifier::RemoveTarget(so);
}

/////////////////////////////////////////////////////////////////////////////
//
//  Function   : IEval
//  PARAMETERS : secs
//               del
//               dirty
//
//  PURPOSE    : This is where the main update work is done
//    Tasks:
//      - Call the Python code's Update function (if there)
//
bool plPythonFileMod::IEval(double secs, float del, uint32_t dirty)
{
    return true;
}


/////////////////////////////////////////////////////////////////////////////
//
//  Function   : MsgReceive
//  PARAMETERS : msg   - the message that came to us.
//
//  PURPOSE    : Handle all the different types of messages that we recv
//
bool plPythonFileMod::MsgReceive(plMessage* msg)
{
    plAgeLoadedMsg* ageLoadedMsg = plAgeLoadedMsg::ConvertNoRef(msg);
    if (ageLoadedMsg && ageLoadedMsg->fLoaded) {
        plgDispatch::Dispatch()->UnRegisterForExactType(plAgeLoadedMsg::Index(), GetKey());
    }

    // if this is a render message, then we are just trying to get a pointer to the Pipeline
    plRenderMsg* rMsg = plRenderMsg::ConvertNoRef(msg);
    if (rMsg) {
        fPipe = rMsg->Pipeline();
        plgDispatch::Dispatch()->UnRegisterForExactType(plRenderMsg::Index(), GetKey());
        return true;
    }

    return plModifier::MsgReceive(msg);
}

void plPythonFileMod::Read(hsStream* stream, hsResMgr* mgr)
{
    plMultiModifier::Read(stream, mgr);

    // read in the name of the python script
    fPythonFile = stream->ReadSafeString();

    // then read in the list of receivers that want to be notified
    uint32_t nRcvs = stream->ReadLE32();
    fReceivers.reserve(nRcvs);
    for (size_t i = 0; i < nRcvs; i++)
        fReceivers.push_back(mgr->ReadKey(stream));

    // then read in the list of parameters
    uint32_t nParms = stream->ReadLE32();
    fParameters.resize(nParms);
    for (size_t i = 0; i < nParms; i++)
        fParameters[i].Read(stream, mgr);
}

void plPythonFileMod::Write(hsStream* stream, hsResMgr* mgr)
{
    plMultiModifier::Write(stream, mgr);

    stream->WriteSafeString(fPythonFile);

    // then write out the list of receivers that want to be notified
    stream->WriteLE32((uint32_t)fReceivers.size());
    for (size_t i = 0; i < fReceivers.size(); i++)
        mgr->WriteKey(stream, fReceivers[i]);

    // then write out the list of parameters
    stream->WriteLE32((uint32_t)fParameters.size());
    for (size_t i = 0; i < fParameters.size(); i++)
        fParameters[i].Write(stream, mgr);
}

//// kGlobalNameKonstant /////////////////////////////////////////////////
//  My continued attempt to spread the CORRECT way to spell konstant. -mcn

ST::string plPythonFileMod::kGlobalNameKonstant(ST_LITERAL("VeryVerySpecialPythonFileMod"));

