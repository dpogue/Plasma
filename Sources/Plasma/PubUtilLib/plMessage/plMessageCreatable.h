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

#ifndef plMessageCreatable_inc
#define plMessageCreatable_inc

#include "pnFactory/plCreator.h"

#ifndef MINIMAL_GL_BUILD
#include "plAccountUpdateMsg.h"
REGISTER_CREATABLE(plAccountUpdateMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plActivatorMsg.h"
REGISTER_CREATABLE(plActivatorMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plAgeLoadedMsg.h"
REGISTER_CREATABLE(plAgeBeginLoadingMsg);
REGISTER_CREATABLE(plAgeLoadedMsg);
REGISTER_CREATABLE(plAgeLoaded2Msg);
REGISTER_CREATABLE(plInitialAgeStateLoadedMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plAngularVelocityMsg.h"
REGISTER_CREATABLE(plAngularVelocityMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plAnimCmdMsg.h"
REGISTER_CREATABLE(plAGCmdMsg);
REGISTER_CREATABLE(plAGDetachCallbackMsg);
REGISTER_CREATABLE(plAGInstanceCallbackMsg);
REGISTER_CREATABLE(plAnimCmdMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plAvatarFootMsg.h"
REGISTER_CREATABLE(plAvatarFootMsg);
#endif

#include "plAvatarMsg.h"
#ifndef MINIMAL_GL_BUILD
REGISTER_CREATABLE(plArmatureUpdateMsg);
REGISTER_CREATABLE(plAvatarBehaviorNotifyMsg);
REGISTER_CREATABLE(plAvatarOpacityCallbackMsg);
REGISTER_CREATABLE(plAvatarMsg);
REGISTER_CREATABLE(plAvatarPhysicsEnableCallbackMsg);
REGISTER_CREATABLE(plAvatarSetTypeMsg);
REGISTER_CREATABLE(plAvatarSpawnNotifyMsg);
REGISTER_CREATABLE(plAvatarStealthModeMsg);
REGISTER_CREATABLE(plAvBrainGenericMsg);
REGISTER_CREATABLE(plAvOneShotMsg);
REGISTER_CREATABLE(plAvSeekMsg);
REGISTER_CREATABLE(plAvTaskMsg);
REGISTER_CREATABLE(plAvTaskSeekDoneMsg);

REGISTER_CREATABLE(plAvPopBrainMsg);
REGISTER_CREATABLE(plAvPushBrainMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plBulletMsg.h"
REGISTER_CREATABLE(plBulletMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plCaptureRenderMsg.h"
REGISTER_CREATABLE(plCaptureRenderMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plCCRMessageCreatable.h"  // kept separately for selective server include 
#endif

#ifndef MINIMAL_GL_BUILD
#include "plClimbEventMsg.h"
REGISTER_CREATABLE(plClimbEventMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plClimbMsg.h"
REGISTER_CREATABLE(plClimbMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plCollideMsg.h"
REGISTER_CREATABLE(plCollideMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plCondRefMsg.h"
REGISTER_CREATABLE(plCondRefMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plConnectedToVaultMsg.h"
REGISTER_CREATABLE(plConnectedToVaultMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plConsoleMsg.h"
REGISTER_CREATABLE(plConsoleMsg);
#endif

#include "plDeviceRecreateMsg.h"
REGISTER_CREATABLE(plDeviceRecreateMsg);

#ifndef MINIMAL_GL_BUILD
#include "plDynaDecalEnableMsg.h"
REGISTER_CREATABLE(plDynaDecalEnableMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plDynamicEnvMapMsg.h"
REGISTER_CREATABLE(plDynamicEnvMapMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plDynamicTextMsg.h"
REGISTER_CREATABLE(plDynamicTextMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plExcludeRegionMsg.h"
REGISTER_CREATABLE(plExcludeRegionMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plInputEventMsg.h"
REGISTER_CREATABLE(plAvatarInputStateMsg);
REGISTER_CREATABLE(plControlEventMsg);
REGISTER_CREATABLE(plDebugKeyEventMsg);
REGISTER_CREATABLE(plIMouseBEventMsg);
REGISTER_CREATABLE(plIMouseXEventMsg);
REGISTER_CREATABLE(plIMouseYEventMsg);
REGISTER_CREATABLE(plInputEventMsg);
REGISTER_CREATABLE(plKeyEventMsg);
REGISTER_CREATABLE(plMouseEventMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plInputIfaceMgrMsg.h"
REGISTER_CREATABLE(plInputIfaceMgrMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plInterestingPing.h"
REGISTER_CREATABLE(plInterestingModMsg);
REGISTER_CREATABLE(plInterestingPing);
#endif

#include "plLayRefMsg.h"
REGISTER_CREATABLE(plLayRefMsg);

#include "plLightRefMsg.h"
REGISTER_CREATABLE(plLightRefMsg);

#ifndef MINIMAL_GL_BUILD
#include "plLinearVelocityMsg.h"
REGISTER_CREATABLE(plLinearVelocityMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plLinkToAgeMsg.h"
REGISTER_CREATABLE(plLinkCallbackMsg);
REGISTER_CREATABLE(plLinkEffectBCMsg);
REGISTER_CREATABLE(plLinkEffectPrepBCMsg);
REGISTER_CREATABLE(plLinkEffectsTriggerMsg);
REGISTER_CREATABLE(plLinkEffectsTriggerPrepMsg);
REGISTER_CREATABLE(plLinkingMgrMsg);
REGISTER_CREATABLE(plLinkToAgeMsg);
REGISTER_CREATABLE(plPseudoLinkAnimCallbackMsg);
REGISTER_CREATABLE(plPseudoLinkAnimTriggerMsg);
REGISTER_CREATABLE(plPseudoLinkEffectMsg);
#endif

#include "plListenerMsg.h"
REGISTER_CREATABLE(plListenerMsg);
REGISTER_CREATABLE(plSetListenerMsg);

#ifndef MINIMAL_GL_BUILD
#include "plLoadAgeMsg.h"
REGISTER_CREATABLE(plLoadAgeMsg);
REGISTER_CREATABLE(plLinkInDoneMsg)
REGISTER_CREATABLE(plLinkOutUnloadMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plLOSHitMsg.h"
REGISTER_CREATABLE(plLOSHitMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plLOSRequestMsg.h"
REGISTER_CREATABLE(plLOSRequestMsg);
#endif

#include "plMatRefMsg.h"
REGISTER_CREATABLE(plMatRefMsg);

#include "plMatrixUpdateMsg.h"
REGISTER_CREATABLE(plMatrixUpdateMsg);

#ifndef MINIMAL_GL_BUILD
#include "plMemberUpdateMsg.h"
REGISTER_CREATABLE(plMemberUpdateMsg);
#endif

#include "plMeshRefMsg.h"
REGISTER_CREATABLE(plMeshRefMsg);

#ifndef MINIMAL_GL_BUILD
#include "plMovieMsg.h"
REGISTER_CREATABLE(plMovieMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plMultistageMsg.h"
REGISTER_CREATABLE(plMultistageModMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plNetClientMgrMsg.h"
REGISTER_CREATABLE(plNetClientMgrMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plNetCommMsgs.h"
REGISTER_CREATABLE(plNetCommActivePlayerMsg);
REGISTER_CREATABLE(plNetCommAuthConnectedMsg);
REGISTER_CREATABLE(plNetCommAuthMsg);
REGISTER_CREATABLE(plNetCommCreatePlayerMsg);
REGISTER_CREATABLE(plNetCommDeletePlayerMsg);
REGISTER_CREATABLE(plNetCommFileDownloadMsg);
REGISTER_CREATABLE(plNetCommFileListMsg);
REGISTER_CREATABLE(plNetCommLinkToAgeMsg);
REGISTER_CREATABLE(plNetCommPlayerListMsg);
REGISTER_CREATABLE(plNetCommPublicAgeListMsg);
REGISTER_CREATABLE(plNetCommPublicAgeMsg);
REGISTER_CREATABLE(plNetCommRegisterAgeMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plNetOwnershipMsg.h"
REGISTER_CREATABLE(plNetOwnershipMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plNetVoiceListMsg.h"
REGISTER_CREATABLE(plNetVoiceListMsg);
#endif

#include "plNodeCleanupMsg.h"
REGISTER_CREATABLE(plNodeCleanupMsg);

#ifndef MINIMAL_GL_BUILD
#include "plOneShotMsg.h"
REGISTER_CREATABLE(plOneShotMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plParticleUpdateMsg.h"
REGISTER_CREATABLE(plParticleFlockMsg);
REGISTER_CREATABLE(plParticleKillMsg);
REGISTER_CREATABLE(plParticleTransferMsg);
REGISTER_CREATABLE(plParticleUpdateMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plPickedMsg.h"
REGISTER_CREATABLE(plPickedMsg);
#endif

#include "plRenderMsg.h"
REGISTER_CREATABLE(plPreResourceMsg);
REGISTER_CREATABLE(plRenderMsg);

#ifndef MINIMAL_GL_BUILD
#include "plRenderRequestMsg.h"
REGISTER_CREATABLE(plRenderRequestAck);
REGISTER_CREATABLE(plRenderRequestMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plReplaceGeometryMsg.h"
REGISTER_CREATABLE(plReplaceGeometryMsg);
REGISTER_CREATABLE(plSwapSpansRefMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plResMgrHelperMsg.h"
REGISTER_CREATABLE(plResMgrHelperMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plResPatcherMsg.h"
REGISTER_CREATABLE(plResPatcherMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plResponderMsg.h"
REGISTER_CREATABLE(plResponderMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plRideAnimatedPhysMsg.h"
REGISTER_CREATABLE(plRideAnimatedPhysMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plRippleShapeMsg.h"
REGISTER_CREATABLE(plRippleShapeMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plRoomLoadNotifyMsg.h"
REGISTER_CREATABLE(plRoomLoadNotifyMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plShadowCastMsg.h"
REGISTER_CREATABLE(plShadowCastMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plSimStateMsg.h"
// REGISTER_CREATABLE(plSimStateMsg);
// REGISTER_CREATABLE(plFreezeMsg);
// REGISTER_CREATABLE(plEventGroupMsg);
// REGISTER_CREATABLE(plEventGroupEnableMsg);
// REGISTER_CREATABLE(plSuspendEventMsg);
REGISTER_CREATABLE(plSubWorldMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plSpawnModMsg.h"
REGISTER_CREATABLE(plSpawnModMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plSpawnRequestMsg.h"
REGISTER_CREATABLE(plSpawnRequestMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plSwimMsg.h"
REGISTER_CREATABLE(plSwimMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plSynchEnableMsg.h"
REGISTER_CREATABLE(plSynchEnableMsg);
#endif

#include "plTimerCallbackMsg.h"
REGISTER_CREATABLE(plTimerCallbackMsg);

#ifndef MINIMAL_GL_BUILD
#include "plTransitionMsg.h"
REGISTER_CREATABLE(plTransitionMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plTriggerMsg.h"
REGISTER_CREATABLE(plTriggerMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plVaultNotifyMsg.h"
REGISTER_CREATABLE(plVaultNotifyMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plAIMsg.h"
REGISTER_CREATABLE(plAIArrivedAtGoalMsg);
REGISTER_CREATABLE(plAIBrainCreatedMsg);
REGISTER_CREATABLE(plAIMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plAvCoopMsg.h"
REGISTER_CREATABLE(plAvCoopMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plLoadAvatarMsg.h"
REGISTER_CREATABLE(plLoadAvatarMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plLoadCloneMsg.h"
REGISTER_CREATABLE(plLoadCloneMsg);
#endif

#ifndef MINIMAL_GL_BUILD
#include "plLoadClothingMsg.h"
REGISTER_CREATABLE(plLoadClothingMsg);
#endif

#endif // plMessageCreatable_inc
