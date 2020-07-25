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

#include "plGLClient.h"

#include "plPipeResReq.h"
#include "plProfile.h"
#include "hsTimer.h"
#include "plTimerCallbackManager.h"

#include "pnDispatch/plDispatch.h"
#include "pnDispatch/plDispatchLogBase.h"

#include "pnMessage/plAudioSysMsg.h"
#include "pnMessage/plCameraMsg.h"
#include "pnMessage/plClientMsg.h"
#include "pnMessage/plCmdIfaceModMsg.h"
#include "pnMessage/plProxyDrawMsg.h"
#include "pnMessage/plTimeMsg.h"

#include "pnSceneObject/plCoordinateInterface.h"

#include "plAgeDescription/plAgeDescription.h"
#include "plAgeLoader/plAgeLoader.h"
#include "plAgeLoader/plResPatcher.h"

#include "plAudio/plAudioSystem.h"

#include "plAvatar/plArmatureMod.h"
#include "plAvatar/plAvatarClothing.h"
#include "plAvatar/plAvatarMgr.h"

#include "plDrawable/plAccessGeometry.h"
#include "plDrawable/plVisLOSMgr.h"

#include "plFile/plEncryptedStream.h"

#include "plGImage/plAVIWriter.h"
#include "plGImage/plFontCache.h"

#include "plInputCore/plInputManager.h"
#include "plInputCore/plInputInterfaceMgr.h"
#include "plInputCore/plInputDevice.h"

#include "plMessage/plInputEventMsg.h"
#include "plMessage/plMovieMsg.h"
#include "plMessage/plRenderMsg.h"
#include "plMessage/plResPatcherMsg.h"
#include "plMessage/plRoomLoadNotifyMsg.h"
#include "plMessage/plTransitionMsg.h"

#include "plNetClient/plLinkEffectsMgr.h"
#include "plNetClient/plNetClientMgr.h"
#include "plNetClient/plNetLinkingMgr.h"

#include "plPipeline.h"
#include "plPipeline/plPipelineCreate.h"
#include "plPipeline/plPlateProgressMgr.h"
#include "plPipeline/plTransitionMgr.h"

#include "plProgressMgr/plProgressMgr.h"

#include "plResMgr/plResManager.h"
#include "plResMgr/plKeyFinder.h"

#include "plScene/plSceneNode.h"
#include "plScene/plPageTreeMgr.h"
#include "plScene/plRelevanceMgr.h"
#include "plScene/plVisMgr.h"

#include "plSDL/plSDL.h"

#include "plStatGather/plProfileManagerFull.h"

#include "plStatusLog/plStatusLog.h"

#include "plUnifiedTime/plClientUnifiedTime.h"

#include "pfAudio/plListener.h"

#include "pfCamera/plVirtualCamNeu.h"

#include "pfCharacter/pfMarkerMgr.h"

#include "pfConsoleCore/pfConsoleEngine.h"
#include "pfConsole/pfConsole.h"
#include "pfConsole/pfConsoleDirSrc.h"

#include "pfGameGUIMgr/pfGameGUIMgr.h"
#include "pfGameGUIMgr/pfGUICtrlGenerator.h"

#include "pfJournalBook/pfJournalBook.h"

#include "pfLocalizationMgr/pfLocalizationMgr.h"

#include "pfMoviePlayer/plMoviePlayer.h"

#include "pfPatcher/plManifests.h"



plProfile_Extern(DrawTime);
plProfile_Extern(UpdateTime);
plProfile_CreateTimer("ResMgr", "Update", ResMgr);
plProfile_CreateTimer("DispatchQueue", "Update", DispatchQueue);
plProfile_CreateTimer("RenderSetup", "Update", RenderMsg);
plProfile_CreateTimer("Simulation", "Update", Simulation);
plProfile_CreateTimer("NetTime", "Update", UpdateNetTime);
plProfile_Extern(TimeMsg);
plProfile_Extern(EvalMsg);
plProfile_Extern(TransformMsg);
plProfile_Extern(CameraMsg);
plProfile_Extern(AnimatingPhysicals);
plProfile_Extern(StoppedAnimPhysicals);

plProfile_CreateTimer("BeginRender", "Render", BeginRender);
plProfile_CreateTimer("ClearRender", "Render", ClearRender);
plProfile_CreateTimer("PreRender", "Render", PreRender);
plProfile_CreateTimer("MainRender", "Render", MainRender);
plProfile_CreateTimer("PostRender", "Render", PostRender);
plProfile_CreateTimer("Movies", "Render", Movies);
plProfile_CreateTimer("Console", "Render", Console);
plProfile_CreateTimer("StatusLog", "Render", StatusLog);
plProfile_CreateTimer("ProgressMgr", "Render", ProgressMgr);
plProfile_CreateTimer("ScreenElem", "Render", ScreenElem);
plProfile_CreateTimer("EndRender", "Render", EndRender);

static plDispatchBase* gDisp = nil;
static plTimerCallbackManager* gTimerMgr = nil;
bool plClient::fDelayMS = false;

plClient* plClient::fInstance = nullptr;

plClient::plClient()
:   fInputManager(nullptr),
    fPipeline(nullptr),
    fCurrentNode(nullptr),
    fTransitionMgr(nullptr),
    fLinkEffectsMgr(nullptr),
    fPageMgr(nullptr),
    fFontCache(nullptr),
    fConsole(nullptr),
    fWindowHndl(nullptr),
    fDone(false),
    fWindowActive(false),
    fProgressBar(nullptr),
    fGameGUIMgr(nullptr),
    fHoldLoadRequests(false),
    fNumLoadingRooms(0),
    fNumPostLoadMsgs(0),
    fPostLoadMsgInc(0.f),
    fCamera(nullptr),
    fQuitIntro(false)
{
    hsStatusMessage("Constructing client\n");
    plClient::SetInstance(this);

    // Setup the timer. These can be overriden with console commands.
    hsTimer::SetRealTime(true);
#ifdef HS_DEBUGGING
    hsTimer::SetTimeClamp(0.1f);
#else // HS_DEBUGGING
    hsTimer::SetTimeClamp(0);
#endif // HS_DEBUGGING

    // need to do this before the console is created
    IDetectAudioVideoSettings();

    /// allow console commands to start working early
    // Create the console engine
    fConsoleEngine = new pfConsoleEngine();

    // create network mgr before console runs
    plNetClientMgr::SetInstance(new plNetClientMgr);
    plAgeLoader::SetInstance(new plAgeLoader);

    // Use it to parse the init directory
    plFileName initFolder = plFileSystem::GetInitPath();
    pfConsoleDirSrc dirSrc(fConsoleEngine, initFolder, "*.ini");

#ifndef PLASMA_EXTERNAL_RELEASE
    // internal builds also parse the local init folder
    dirSrc.ParseDirectory("init", "*.ini");
#endif
}

plClient::~plClient()
{
    hsStatusMessage("Destructing client\n");

    plClient::SetInstance(nullptr);

    delete fPageMgr;
}


template<typename T>
static void IUnRegisterAs(T*& ko, plFixedKeyId id)
{
    if (ko) {
        ko->UnRegisterAs(id);
        ko = nullptr;
    }
}

bool plClient::Shutdown()
{
    plSynchEnabler ps(false);   // disable dirty state tracking during shutdown 
    delete fProgressBar;

    // Just in case, clear this out (trying to fix a crash bug where this is still active at shutdown)
    plDispatch::SetMsgRecieveCallback(nullptr);

    // Let the resmanager know we're going to be shutting down.
    hsgResMgr::ResMgr()->BeginShutdown();

    // Must kill off all movies before shutting down audio.
    IKillMovies();

    plgAudioSys::Activate(false);

    // Get any proxies to commit suicide.
    plProxyDrawMsg* nuke = new plProxyDrawMsg(plProxyDrawMsg::kAllTypes | plProxyDrawMsg::kDestroy);
    plgDispatch::MsgSend(nuke);

    if (plAVIWriter::IsInitialized())
    {
        plAVIWriter::Instance().Shutdown();
    }

    hsStatusMessage("Shutting down client...\n");

    // First, before anybody else goes away, write out our key mappings
    if (plInputInterfaceMgr::GetInstance())
    {
        plInputInterfaceMgr::GetInstance()->WriteKeyMap();
    }

#ifndef MINIMAL_GL_BUILD
    // tell Python that its ok to shutdown
    PythonInterface::WeAreInShutdown(); 
#endif

    // Shutdown the journalBook API
    pfJournalBook::SingletonShutdown();

    /// Take down the KI
    if (fGameGUIMgr)
    {
        // unload the blackbar which will bootstrap in the rest of the KI dialogs
        fGameGUIMgr->UnloadDialog("KIBlackBar");
    }

    // Take down our GUI control generator
    pfGUICtrlGenerator::Instance().Shutdown();

    if (plNetClientMgr* nc = plNetClientMgr::GetInstance())
    {
        nc->Shutdown();
    }

    if (plAgeLoader* al = plAgeLoader::GetInstance())
    {
        al->Shutdown();
    }

    IUnRegisterAs(fInputManager, kInput_KEY);
    IUnRegisterAs(fGameGUIMgr, kGameGUIMgr_KEY);

    for (int i = 0; i < fRooms.Count(); i++)
    {
        plSceneNode *sn = fRooms[i].fNode;
        GetKey()->Release(sn->GetKey());
    }
    fRooms.Reset();
    fRoomsLoading.clear();

    plAccessGeometry::DeInit();

    delete fPipeline;
    fPipeline = nullptr;

#ifndef MINIMAL_GL_BUILD
    if (plSimulationMgr::GetInstance())
    {
        plSimulationMgr::Shutdown();
    }
#endif

    plAvatarMgr::ShutDown();
    plRelevanceMgr::DeInit();

    if (fPageMgr) {
        fPageMgr->Reset();
    }

    IUnRegisterAs(fTransitionMgr, kTransitionMgr_KEY);

    delete fConsoleEngine;
    fConsoleEngine = nullptr;

    IUnRegisterAs(fLinkEffectsMgr, kLinkEffectsMgr_KEY);

    plClothingMgr::DeInit();

    IUnRegisterAs(fFontCache, kFontCache_KEY);

    pfMarkerMgr::Shutdown();

    IUnRegisterAs(fConsole, kConsoleObject_KEY);

#ifndef MINIMAL_GL_BUILD
    PythonInterface::finiPython();
#endif

    IUnRegisterAs(fCamera, kVirtualCamera1_KEY);

    // mark the listener for death.
    // there's no need to keep this around...
    plUoid lu(kListenerMod_KEY);
    plKey pLKey = hsgResMgr::ResMgr()->FindKey(lu);
    if (pLKey)
    {
        plListener* pLMod = plListener::ConvertNoRef(pLKey->GetObjectPtr());
        if (pLMod)
            pLMod->UnRegisterAs(kListenerMod_KEY);
    }

    plgAudioSys::Shutdown();

    if (pfLocalizationMgr::InstanceValid())
    {
        pfLocalizationMgr::Shutdown();
    }

    plVisLOSMgr::DeInit();

    delete fPageMgr;
    fPageMgr = nullptr;
    plGlobalVisMgr::DeInit();

    // This will destruct the client. Do it last.
    UnRegisterAs(kClient_KEY);

    return false;
}


void plClient::InitInputs()
{
    hsStatusMessage("InitInputs client\n");

    fInputManager = new plInputManager(fWindowHndl);
    fInputManager->CreateInterfaceMod(fPipeline);
    fInputManager->RegisterAs(kInput_KEY);
    plgDispatch::Dispatch()->RegisterForExactType(plIMouseXEventMsg::Index(), fInputManager->GetKey());
    plgDispatch::Dispatch()->RegisterForExactType(plIMouseYEventMsg::Index(), fInputManager->GetKey());
    plgDispatch::Dispatch()->RegisterForExactType(plIMouseBEventMsg::Index(), fInputManager->GetKey());
    plgDispatch::Dispatch()->RegisterForExactType(plEvalMsg::Index(), fInputManager->GetKey());

    plInputDevice* pKeyboard = new plKeyboardDevice();
    fInputManager->AddInputDevice(pKeyboard);

    plInputDevice* pMouse = new plMouseDevice();
    fInputManager->AddInputDevice(pMouse);

    if (fWindowActive)
    {
        fInputManager->Activate(true);
    }
}


bool plClient::InitPipeline(hsWindowHndl display)
{
    hsStatusMessage("InitPipeline client\n");


    /* Totally fake device init stuff because we ignore most of it anyways */
    hsG3DDeviceRecord dev;
    dev.SetG3DDeviceType(hsG3DDeviceSelector::kDevTypeOpenGL);

    hsG3DDeviceMode mode;
    mode.SetColorDepth(32);
    mode.SetHeight(600);
    mode.SetWidth(800);

    hsG3DDeviceModeRecord devRec(dev, mode);


    /* Create the pipeline */
    plPipeline* pipe = plPipelineCreate::CreatePipeline(display, fWindowHndl, &devRec);
    if (pipe->GetErrorString() != nullptr)
    {
        hsStatusMessage(pipe->GetErrorString());
        return true;
    }
    fPipeline = pipe;


    hsVector3 up;
    hsPoint3 from, at;
    from.Set(0.f, 0.f, 10.f);
    at.Set(0.f, 20.f, 0.f);
    up.Set(0.f, 0.f, -1.f);
    hsMatrix44 cam;
    cam.MakeCamera(&from,&at,&up);

    float yon = 500.0f;

    pipe->SetFOV(60.f, int32_t(60.f * pipe->Height() / pipe->Width()));
    pipe->SetDepth(0.3f, yon);

    hsMatrix44 id;
    id.Reset();

    pipe->SetWorldToCamera(cam, id);
    pipe->RefreshMatrices();

    // Do this so we're still black before we show progress bars, but the correct color coming out of 'em
    fClearColor.Set( 0.f, 0.f, 0.0f, 1.f );
    pipe->SetClear(&fClearColor);
    pipe->ClearRenderTarget();

    plAccessGeometry::Init(pipe);

    if (fPipeline)
        fPipeline->LoadResources();

    return false;
}


bool plClient::StartInit()
{
    hsStatusMessage("Init client\n");
    fFlags.SetBit(kFlagIniting);

    pfLocalizationMgr::Initialize("dat");

    plPlateProgressMgr::DeclareThyself();

    // Set our callback for the progress manager so everybody else can use it
    fLastProgressUpdate = 0.f;
    plProgressMgr::GetInstance()->SetCallbackProc(IProgressMgrCallbackProc);

    // Check the registry, which deletes data files that are either corrupt or
    // have old version numbers.  If the file still exists on the file server
    // then it will be patched on-the-fly as needed (unless you're running with
    // local data of course).
#ifdef PLASMA_EXTERNAL_RELEASE
    ((plResManager *)hsgResMgr::ResMgr())->VerifyPages();
#endif

    plgAudioSys::Init();

    RegisterAs(kClient_KEY);

    plGlobalVisMgr::Init();
    fPageMgr = new plPageTreeMgr();

    plVisLOSMgr::Init(fPipeline, fPageMgr);

    // init globals
    plAvatarMgr::GetInstance();
    plRelevanceMgr::Init();

    gDisp = plgDispatch::Dispatch();
    gTimerMgr = plgTimerCallbackMgr::Mgr();

    //
    // initialize input system
    //
    InitInputs();

    /// Init the console object
    /// Note: this can be done last because the console engine was inited first, and
    /// everything in code that works with the console does so through the console engine
    fConsole = new pfConsole();
    pfConsole::SetPipeline(fPipeline);
    fConsole->RegisterAs(kConsoleObject_KEY);
    fConsole->Init(fConsoleEngine);

    // Init the font cache and load our custom fonts from our current dat directory
    // (This needs to happen before the loading screen is rendered)
    fFontCache = new plFontCache();
    fFontCache->LoadCustomFonts("dat");

    /// Init the transition manager
    fTransitionMgr = new plTransitionMgr();
    fTransitionMgr->RegisterAs(kTransitionMgr_KEY);
    fTransitionMgr->Init();

    // Init the Age Linking effects manager
    fLinkEffectsMgr = new plLinkEffectsMgr();
    fLinkEffectsMgr->RegisterAs(kLinkEffectsMgr_KEY);
    fLinkEffectsMgr->Init();

    /// Init the in-game GUI manager
    fGameGUIMgr = new pfGameGUIMgr();
    fGameGUIMgr->RegisterAs(kGameGUIMgr_KEY);
    fGameGUIMgr->Init();

    plgAudioSys::Activate(true);

    /// Init Net before loading things
    plgDispatch::Dispatch()->RegisterForExactType(plNetCommAuthMsg::Index(), GetKey());
    plNetClientMgr::GetInstance()->RegisterAs(kNetClientMgr_KEY);
    plAgeLoader::GetInstance()->Init();

    plCmdIfaceModMsg* pModMsg2 = new plCmdIfaceModMsg;
    pModMsg2->SetBCastFlag(plMessage::kBCastByExactType);
    pModMsg2->SetSender(fConsole->GetKey());
    pModMsg2->SetCmd(plCmdIfaceModMsg::kAdd);
    plgDispatch::MsgSend(pModMsg2);

    // create new virtual camera
    fCamera = new plVirtualCam1;
    fCamera->RegisterAs(kVirtualCamera1_KEY);
    fCamera->Init();
    fCamera->SetPipeline(GetPipeline());

    plVirtualCam1::Refresh();
    fGameGUIMgr->SetAspectRatio((float)fPipeline->Width() / (float)fPipeline->Height());
    plMouseDevice::Instance()->SetDisplayResolution((float)fPipeline->Width(), (float)fPipeline->Height());
    plInputManager::SetRecenterMouse(false);

    plgDispatch::Dispatch()->RegisterForExactType(plMovieMsg::Index(), GetKey());

    // create the listener for the audio system:
    plListener* pLMod = new plListener;
    pLMod->RegisterAs(kListenerMod_KEY);

    plgDispatch::Dispatch()->RegisterForExactType(plEvalMsg::Index(), pLMod->GetKey());
    plgDispatch::Dispatch()->RegisterForExactType(plAudioSysMsg::Index(), pLMod->GetKey());

    plSynchedObject::PushSynchDisabled(false); // enable dirty tracking
    return true;
}

bool plClient::BeginGame()
{
    plNetClientMgr::GetInstance()->Init();

    if (!fQuitIntro)
    {
        IPlayIntroMovie("avi/CyanWorlds.webm", 0.f, 0.f, 0.f, 1.f, 1.f, 0.75);
    }

    if (GetDone()) return false;

    /*
    if (NetCommGetStartupAge()->ageDatasetName.compare_i("StartUp") == 0) {
        // This is needed because there is no auth step in this case
        plNetCommAuthMsg* msg = new plNetCommAuthMsg();
        msg->result = kNetSuccess;
        msg->param = nullptr;
        msg->Send();
    }
    */
#ifdef MINIMAL_GL_BUILD
    // This came from plClient::IPatchGlobalAgeFiles
    plgDispatch::Dispatch()->RegisterForExactType(plResPatcherMsg::Index(), GetKey());

    plResPatcher* patcher = plResPatcher::GetInstance();
    patcher->Update(plManifest::EssentialGameManifests());
#endif

    return true;
}

bool plClient::MainLoop()
{
    if (plClient::fDelayMS)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    // Reset our stats
    plProfileManager::Instance().BeginFrame();

    if (IUpdate())
    {
        return true;
    }

    if (IDraw())
    {
        return true;
    }

    plProfileManagerFull::Instance().EndFrame();
    plProfileManager::Instance().EndFrame();

    // Draw the stats
    plProfileManagerFull::Instance().Update();

    return false;
}


bool plClient::MsgReceive(plMessage* msg)
{
    if (plGenRefMsg* genRefMsg = plGenRefMsg::ConvertNoRef(msg)) {
        // do nothing, we just use the client's key to ref vault image nodes.
        return true;
    }


    if (plClientRefMsg* pRefMsg = plClientRefMsg::ConvertNoRef(msg))
    {
        switch (pRefMsg->fType)
        {
        case plClientRefMsg::kLoadRoom:
            #ifndef PLASMA_EXTERNAL_RELEASE
            plStatusLog::AddLineSF( "pageouts.log", ".. ClientRefMsg received for room {}", pRefMsg->GetRef() ? pRefMsg->GetRef()->GetKey()->GetUoid().GetObjectName() : ST_LITERAL("nilref") );
            #endif

            if (hsCheckBits(pRefMsg->GetContext(), plRefMsg::kOnCreate))
            {
                // was it that the room was loaded?
                IRoomLoaded(plSceneNode::Convert(pRefMsg->GetRef()), false);
            }
            else if (hsCheckBits(pRefMsg->GetContext(), plRefMsg::kOnDestroy))
            {
                // or was it that the room was unloaded?
                IRoomUnloaded(plSceneNode::Convert(pRefMsg->GetRef()));
            }
            #ifndef PLASMA_EXTERNAL_RELEASE
            else
            {
                plStatusLog::AddLineS("pageouts.log", "..    refMsg is UNHANDLED");
            }
            #endif
            break;

        case plClientRefMsg::kLoadRoomHold:
            if (hsCheckBits(pRefMsg->GetContext(), plRefMsg::kOnCreate))
            {
                IRoomLoaded(plSceneNode::Convert(pRefMsg->GetRef()), true);
            }
            break;

        // Manually add room.
        // Add to pageMgr, but don't load the entire room.
        case plClientRefMsg::kManualRoom:
            {
                if (pRefMsg->GetContext() & plRefMsg::kOnCreate || pRefMsg->GetContext() & plRefMsg::kOnRequest)
                {
                    bool found = false;
                    plSceneNode* pNode = plSceneNode::ConvertNoRef(pRefMsg->GetRef());

                    for (size_t i = 0; i < fRooms.Count(); i++)
                    {
                        if (fRooms[i].fNode->GetKey() == pRefMsg->GetSender())
                        {
                            found=true;
                            break;
                        }
                    }

                    if (!found)
                    {
                        if (pNode)
                        {
                            fRooms.Append(plRoomRec(pNode, 0));
                            fPageMgr->AddNode(pNode);
                        }
                    }
                }
                else
                {
                    plSceneNode* node = plSceneNode::ConvertNoRef(pRefMsg->GetRef());
                    if (node)
                    {
                        for (size_t i = 0; i < fRooms.Count(); i++)
                        {
                            if (fRooms[i].fNode->GetKey() == node->GetKey())
                            {
                                fRooms.Remove(i);
                                break;
                            }
                        }
                        fPageMgr->RemoveNode(node);
                    }
                }
            }
            break;
        }
        return true;
    }

    if (plClientMsg* pMsg = plClientMsg::ConvertNoRef(msg))
    {
        switch (pMsg->GetClientMsgFlag())
        {
        case plClientMsg::kQuit:
            SetDone(true);
            break;

        case plClientMsg::kLoadRoom:
        case plClientMsg::kLoadRoomHold:
            IQueueRoomLoad(pMsg->GetRoomLocs(), (pMsg->GetClientMsgFlag() == plClientMsg::kLoadRoomHold));
            if (!fHoldLoadRequests)
            {
                ILoadNextRoom();
            }
            break;

        case plClientMsg::kUnloadRoom:
            IUnloadRooms(pMsg->GetRoomLocs());
            break;

        case plClientMsg::kLoadNextRoom:
            ILoadNextRoom();
            break;

        case plClientMsg::kLoadAgeKeys:
            ((plResManager*)hsgResMgr::ResMgr())->LoadAgeKeys(pMsg->GetAgeName());
            break;

        case plClientMsg::kReleaseAgeKeys:
            ((plResManager *)hsgResMgr::ResMgr())->DropAgeKeys(pMsg->GetAgeName());
            break;

        case plClientMsg::kDisableRenderScene:
            plClient::GetInstance()->SetFlag(plClient::kFlagDBGDisableRender, true);
            break;

        case plClientMsg::kEnableRenderScene:
            plClient::GetInstance()->SetFlag(plClient::kFlagDBGDisableRender, false);
            break;
        }
        return true;
    }

    if (plRenderRequestMsg* rendReq = plRenderRequestMsg::ConvertNoRef(msg))
    {
        IAddRenderRequest(rendReq->Request());
        return true;
    }

    if (plMovieMsg* mov = plMovieMsg::ConvertNoRef(msg))
    {
        return IHandleMovieMsg(mov);
    }

    if (plResPatcherMsg* resMsg = plResPatcherMsg::ConvertNoRef(msg))
    {
        IHandlePatcherMsg(resMsg);
        return true;
    }

    return hsKeyedObject::MsgReceive(msg);
}





void plClient::IDetectAudioVideoSettings()
{
#if defined(HS_DEBUGGING) || defined(DEBUG)
    plPipeline::fDefaultPipeParams.Windowed = true;
#else
    plPipeline::fDefaultPipeParams.Windowed = false;
#endif

    hsUNIXStream s;
    plFileName audioIniFile = plFileName::Join(plFileSystem::GetInitPath(), "audio.ini");
    plFileName graphicsIniFile = plFileName::Join(plFileSystem::GetInitPath(), "graphics.ini");

#ifndef PLASMA_EXTERNAL_RELEASE
    // internal builds can use the local dir
    if (plFileInfo("init/audio.ini").Exists())
    {
        audioIniFile = "init/audio.ini";
    }

    if (plFileInfo("init/graphics.ini").Exists())
    {
        graphicsIniFile = "init/graphics.ini";
    }
#endif

    //check to see if audio.ini exists
    if (s.Open(audioIniFile))
    {
        s.Close();
    }
    else
    {
        IWriteDefaultAudioSettings(audioIniFile);
    }

    // check to see if graphics.ini exists
    if (s.Open(graphicsIniFile))
    {
        s.Close();
    }
    else
    {
        IWriteDefaultGraphicsSettings(graphicsIniFile);
    }
}

void plClient::IWriteDefaultGraphicsSettings(const plFileName& destFile)
{
    hsStream* stream = plEncryptedStream::OpenEncryptedFileWrite(destFile);

    stream->WriteFmt("Graphics.Width {d}\n",                    plPipeline::fDefaultPipeParams.Width);
    stream->WriteFmt("Graphics.Height {d}\n",                   plPipeline::fDefaultPipeParams.Height);
    stream->WriteFmt("Graphics.ColorDepth {d}\n",               plPipeline::fDefaultPipeParams.ColorDepth);
    stream->WriteFmt("Graphics.Windowed {}\n",                  plPipeline::fDefaultPipeParams.Windowed);
    stream->WriteFmt("Graphics.AntiAliasAmount {d}\n",          plPipeline::fDefaultPipeParams.AntiAliasingAmount);
    stream->WriteFmt("Graphics.AnisotropicLevel {d}\n",         plPipeline::fDefaultPipeParams.AnisotropicLevel);
    stream->WriteFmt("Graphics.TextureQuality {d}\n",           plPipeline::fDefaultPipeParams.TextureQuality);
    stream->WriteFmt("Quality.Level {d}\n",                     plPipeline::fDefaultPipeParams.VideoQuality);
    stream->WriteFmt("Graphics.Shadow.Enable {d}\n",            plPipeline::fDefaultPipeParams.Shadows);
    stream->WriteFmt("Graphics.EnablePlanarReflections {d}\n",  plPipeline::fDefaultPipeParams.PlanarReflections);
    stream->WriteFmt("Graphics.EnableVSync {}\n",               plPipeline::fDefaultPipeParams.VSync);

    stream->Close();
    delete stream;
    stream = nullptr;
}

void plClient::IWriteDefaultAudioSettings(const plFileName& destFile)
{
    hsStream* stream = plEncryptedStream::OpenEncryptedFileWrite(destFile);

    stream->WriteFmt("Audio.Initialize {}\n",                   true);
    stream->WriteFmt("Audio.UseEAX {}\n",                       false);
    stream->WriteFmt("Audio.SetPriorityCutoff {d}\n",           6);
    stream->WriteFmt("Audio.MuteAll {}\n",                      false);
    stream->WriteFmt("Audio.SetChannelVolume SoundFX {d}\n",    1);
    stream->WriteFmt("Audio.SetChannelVolume BgndMusic {d}\n",  1);
    stream->WriteFmt("Audio.SetChannelVolume Ambience {d}\n",   1);
    stream->WriteFmt("Audio.SetChannelVolume NPCVoice {d}\n",   1);
    stream->WriteFmt("Audio.EnableVoiceRecording {d}\n",        1);

    stream->Close();
    delete stream;
    stream = nullptr;
}

void plClient::WindowActivate(bool active)
{
    if (fDone)
        return;

    if (fWindowActive != active)
    {
        if (fInputManager)
        {
            fInputManager->Activate(active);
        }

        plArmatureMod::WindowActivate(active);

#ifndef MINIMAL_GL_BUILD
        // Remember, we are no longer exclusive fullscreen, so we actually have to toggle the desktop resolution
        // whee? wait. WHEEE!
        if (fPipeline->IsFullScreen())
            IChangeResolution(active ? fPipeline->Width() : 0, active ? fPipeline->Height() : 0);
#endif
    }
    fWindowActive = active;
}



int plClient::IFindRoomByLoc(const plLocation& loc)
{
    int i = 0;
    for (int i = 0; i < fRooms.Count(); i++)
    {
        if (fRooms[i].fNode->GetKey()->GetUoid().GetLocation() == loc)
            return i;
    }

    return -1;
}

bool plClient::IIsRoomLoading(const plLocation& loc)
{
    for (int i = 0; i < fRoomsLoading.size(); i++)
    {
        if (fRoomsLoading[i] == loc)
            return true;
    }
    return false;
}

#include "plResMgr/plPageInfo.h"

void plClient::IQueueRoomLoad(const std::vector<plLocation>& locs, bool hold)
{
    bool allSameAge = true;
    ST::string lastAgeName;

    uint32_t numRooms = 0;
    for (int i = 0; i < locs.size(); i++)
    {
        const plLocation& loc = locs[i];

        const plPageInfo* info = plKeyFinder::Instance().GetLocationInfo(loc);
        bool alreadyLoaded = (IFindRoomByLoc(loc) != -1);
        bool isLoading = IIsRoomLoading(loc);

        if (!info || alreadyLoaded || isLoading)
        {
#ifdef HS_DEBUGGING
            if (!info)
                hsStatusMessageF("Ignoring LoadRoom request for location 0x%x because we can't find the location", loc.GetSequenceNumber());
            else if (alreadyLoaded)
                hsStatusMessageF("Ignoring LoadRoom request for %s-%s, since room is already loaded", info->GetAge().c_str(), info->GetPage().c_str());
            else if (isLoading)
                hsStatusMessageF("Ignoring LoadRoom request for %s-%s, since room is currently loading", info->GetAge().c_str(), info->GetPage().c_str());
#endif

            continue;
        }

        fLoadRooms.push_back(new LoadRequest(loc, hold));

        if (lastAgeName.empty() || info->GetAge() == lastAgeName)
            lastAgeName = info->GetAge();
        else
            allSameAge = false;

        hsStatusMessageF("+++ Loading room %s_%s", info->GetAge().c_str(), info->GetPage().c_str());
        numRooms++;
    }

    if (numRooms == 0)
        return;

    fNumLoadingRooms += numRooms;
}

void plClient::ILoadNextRoom()
{
    LoadRequest* req = nil;

    while (!fLoadRooms.empty())
    {
        req = fLoadRooms.front();
        fLoadRooms.pop_front();

        bool alreadyLoaded = (IFindRoomByLoc(req->loc) != -1);
        bool isLoading = IIsRoomLoading(req->loc);
        if (alreadyLoaded || isLoading)
        {
            delete req;
            req = nil;
            fNumLoadingRooms--;
        }
        else
            break;
    }

    if (req)
    {
        plClientRefMsg* pRefMsg = new plClientRefMsg(GetKey(),
            plRefMsg::kOnCreate, -1,
            req->hold ? plClientRefMsg::kLoadRoomHold : plClientRefMsg::kLoadRoom);

        fRoomsLoading.push_back(req->loc); // flag the location as currently loading

        // PageInPage is not guaranteed to finish synchronously, just FYI
        plResManager *mgr = (plResManager *)hsgResMgr::ResMgr();
        mgr->PageInRoom(req->loc, plSceneNode::Index(), pRefMsg);

        delete req;

        plClientMsg* nextRoom = new plClientMsg(plClientMsg::kLoadNextRoom);
        nextRoom->Send(GetKey());
    }
}

void plClient::IUnloadRooms(const std::vector<plLocation>& locs)
{
    for (int i = 0; i < locs.size(); i++)
    {
        const plLocation& loc = locs[i];

        if (!loc.IsValid())
        {
            continue;
        }

        plKey nodeKey = nullptr;

        // First, look in our room list. It *should* be there, which allows us to avoid a
        // potential nasty reload-find in the resMgr.
        int roomIdx = IFindRoomByLoc(loc);
        if (roomIdx != -1)
        {
            nodeKey = fRooms[roomIdx].fNode->GetKey();
        }

        if (nodeKey == nullptr)
        {
            nodeKey = plKeyFinder::Instance().FindSceneNodeKey(loc);
        }

        if (nodeKey != nullptr)
        {
            plSceneNode* node = plSceneNode::ConvertNoRef(nodeKey->ObjectIsLoaded());
            if (node)
            {
                #ifndef PLASMA_EXTERNAL_RELEASE
                plStatusLog::AddLineSF("pageouts.log", "SceneNode for {} loaded; Removing node", node->GetKey()->GetUoid().GetObjectName());
                #endif
                fPageMgr->RemoveNode(node);
            }
            else
            {
                #ifndef PLASMA_EXTERNAL_RELEASE
                plStatusLog::AddLineSF("pageouts.log", "SceneNode for {} NOT loaded", nodeKey->GetUoid().GetObjectName());
                #endif
            }
            GetKey()->Release(nodeKey);     // release notify interest in scene node

            uint32_t recFlags = 0;
            if (roomIdx != -1)
            {
                recFlags = fRooms[roomIdx].fFlags;
                fRooms.Remove(roomIdx);
            }

            if (node == fCurrentNode)
            {
                fCurrentNode = nullptr;
            }

            #ifndef PLASMA_EXTERNAL_RELEASE
            plStatusLog::AddLineSF("pageouts.log", "Telling netClientMgr about paging out {}", nodeKey->GetUoid().GetObjectName());
            #endif

            if (plNetClientMgr::GetInstance() != nullptr)
            {
                if (!hsCheckBits(recFlags, plRoomRec::kHeld))
                {
                    // Don't care really about the message that just came in, we care whether it was really held or not
                    plAgeLoader::GetInstance()->StartPagingOutRoom(&nodeKey, 1);
                }
                else
                {
                    // Tell NetClientManager not to expect any pageout info on this guy, since he was held
                    plAgeLoader::GetInstance()->IgnorePagingOutRoom(&nodeKey, 1);
                }
            }
        }
    }
}

void plClient::SetHoldLoadRequests(bool hold)
{
    fHoldLoadRequests = hold;

    if (!fHoldLoadRequests)
    {
        ILoadNextRoom();
    }
}


void plClient::IRoomLoaded(plSceneNode* node, bool hold)
{
    fCurrentNode = node;
    // make sure we don't already have this room in the list:
    bool bAppend = true;
    for (int i = 0; i < fRooms.Count(); i++)
    {
        if (fRooms[i].fNode == fCurrentNode)
        {
            bAppend = false;
            break;
        }
    }
    if (bAppend)
    {
        if (hold)
        {
            fRooms.Append(plRoomRec(fCurrentNode, plRoomRec::kHeld));
        }
        else
        {
            fRooms.Append(plRoomRec(fCurrentNode, 0));
            fPageMgr->AddNode(fCurrentNode);
        }
    }

    fNumLoadingRooms--;

    // Shut down the progress bar if that was the last room
    if (fProgressBar != nil && fNumLoadingRooms <= 0)
    {
#ifndef PLASMA_EXTERNAL_RELEASE
        if (!hold && plDispatchLogBase::IsLogging())
        {
            plDispatchLogBase::GetInstance()->LogStatusBarChange(fProgressBar->GetTitle().c_str(), "displaying messages");
        }
#endif // PLASMA_EXTERNAL_RELEASE
    }

    hsRefCnt_SafeUnRef(fCurrentNode);
    plKey pRmKey = fCurrentNode->GetKey();
    plAgeLoader::GetInstance()->FinishedPagingInRoom(&pRmKey, 1);
    // *** this used to call "ActivateNode" (in physics) which wasn't implemented.
    // *** we should make this "turn on" physics for the selected node
    // *** depending on what guarantees we can make about the load state -- anything useful?

    // now tell all those who are interested that a room was loaded
    if (!hold)
    {
        plRoomLoadNotifyMsg* loadmsg = new plRoomLoadNotifyMsg;
        loadmsg->SetRoom(pRmKey);
        loadmsg->SetWhatHappen(plRoomLoadNotifyMsg::kLoaded);
        plgDispatch::MsgSend(loadmsg);
    }
    else
    {
        hsStatusMessageF("Done loading hold room %s, t=%f\n", pRmKey->GetName().c_str(), hsTimer::GetSeconds());
    }

    plLocation loc = pRmKey->GetUoid().GetLocation();
    for (int i = 0; i < fRoomsLoading.size(); i++)
    {
        if (fRoomsLoading[i] == loc)
        {
            fRoomsLoading.erase(fRoomsLoading.begin() + i);
            break;
        }
    }

    if (!fNumLoadingRooms)
        IStopProgress();
}

void plClient::IRoomUnloaded(plSceneNode* node)
{
#ifndef PLASMA_EXTERNAL_RELEASE
    plStatusLog::AddLineS("pageouts.log", "..    refMsg is onDestroy");
#endif

    fCurrentNode = node;
    hsRefCnt_SafeUnRef(fCurrentNode);
    plKey pRmKey = fCurrentNode->GetKey();
    if (plAgeLoader::GetInstance())
    {
        plAgeLoader::GetInstance()->FinishedPagingOutRoom(&pRmKey, 1);
    }

    // tell all those who are interested that a room was unloaded
    plRoomLoadNotifyMsg* loadmsg = new plRoomLoadNotifyMsg;
    loadmsg->SetRoom(pRmKey);
    loadmsg->SetWhatHappen(plRoomLoadNotifyMsg::kUnloaded);
    plgDispatch::MsgSend(loadmsg);
}


bool plClient::IUpdate()
{
    plProfile_BeginTiming(UpdateTime);

    // reset timer on first frame if realtime and not clamping, to avoid initial large delta
    if (hsTimer::GetSysSeconds()==0 && hsTimer::IsRealTime() && hsTimer::GetTimeClamp()==0)
    {
        hsTimer::SetRealTime(true);
    }

    plProfile_BeginTiming(DispatchQueue);
    plgDispatch::Dispatch()->MsgQueueProcess();
    plProfile_EndTiming(DispatchQueue);

    hsTimer::IncSysSeconds();
    plClientUnifiedTime::SetSysTime(); // keep a unified time, based on sysSeconds
    // Time may have been clamped in IncSysSeconds, depending on hsTimer's current mode.

    double currTime = hsTimer::GetSysSeconds();
    float delSecs = hsTimer::GetDelSysSeconds();

    // do not change this ordering

    plProfile_BeginTiming(UpdateNetTime);
    plNetClientMgr::GetInstance()->Update(currTime);
    plProfile_EndTiming(UpdateNetTime);

    // This TimeMsg doesn't really do much, except somehow it flushes the dispatch
    // after the NetClientMgr updates, delivering any SelfDestruct messages in the
    // queue. This is important to prevent objects that are about to go away from
    // starting trouble during their update. So to get rid of this message, some
    // other way of flushing the dispatch after NegClientMgr's update is needed. mf 
    plProfile_BeginTiming(TimeMsg);
    plTimeMsg* msg = new plTimeMsg(nullptr, nullptr, nullptr, nullptr);
    plgDispatch::MsgSend(msg);
    plProfile_EndTiming(TimeMsg);

    plProfile_BeginTiming(EvalMsg);
    plEvalMsg* eval = new plEvalMsg(nullptr, nullptr, nullptr, nullptr);
    plgDispatch::MsgSend(eval);
    plProfile_EndTiming(EvalMsg);

    const char* xFormLap1 = "Main";
    plProfile_BeginLap(TransformMsg, xFormLap1);
    plTransformMsg* xform = new plTransformMsg(nullptr, nullptr, nullptr, nullptr);
    plgDispatch::MsgSend(xform);
    plProfile_EndLap(TransformMsg, xFormLap1);

    plCoordinateInterface::SetTransformPhase(plCoordinateInterface::kTransformPhaseDelayed);

    // At this point, we just register for a plDelayedTransformMsg when dirtied.
    if (!plCoordinateInterface::GetDelayedTransformsEnabled())
    {
        const char* xFormLap2 = "Simulation";
        plProfile_BeginLap(TransformMsg, xFormLap2);
        xform = new plTransformMsg(nullptr, nullptr, nullptr, nullptr);
        plgDispatch::MsgSend(xform);
        plProfile_EndLap(TransformMsg, xFormLap2);
    }
    else
    {
        const char* xFormLap3 = "Delayed";
        plProfile_BeginLap(TransformMsg, xFormLap3);
        xform = new plDelayedTransformMsg(nullptr, nullptr, nullptr, nullptr);
        plgDispatch::MsgSend(xform);
        plProfile_EndLap(TransformMsg, xFormLap3);
    }

    plCoordinateInterface::SetTransformPhase(plCoordinateInterface::kTransformPhaseNormal);

    plProfile_BeginTiming(CameraMsg);
    plCameraMsg* cameras = new plCameraMsg;
    cameras->SetCmd(plCameraMsg::kUpdateCameras);
    cameras->SetBCastFlag(plMessage::kBCastByExactType);
    plgDispatch::MsgSend(cameras);
    plProfile_EndTiming(CameraMsg);

    return false;
}


bool plClient::IDraw()
{
    // If we're shutting down, don't attempt to draw. Doing so
    // tends to cause a device reload each frame.
    if (fDone)
    {
        return true;
    }

    if (plProgressMgr::GetInstance()->IsActive())
    {
        return IDrawProgress();
    }

    plProfile_Extern(VisEval);
    plProfile_BeginTiming(VisEval);
    plGlobalVisMgr::Instance()->Eval(fPipeline->GetViewPositionWorld());
    plProfile_EndTiming(VisEval);

    plProfile_BeginTiming(RenderMsg);
    plRenderMsg* rendMsg = new plRenderMsg(fPipeline);
    plgDispatch::MsgSend(rendMsg);
    plProfile_EndTiming(RenderMsg);

    plPreResourceMsg* preMsg = new plPreResourceMsg(fPipeline);
    plgDispatch::MsgSend(preMsg);

    // This might not be the ideal place for this, but it
    // needs to be AFTER the plRenderMsg is sent, and
    // BEFORE BeginRender. (plRenderMsg causes construction of
    // Dynamic objects (e.g. RT's), BeginRender uses them (e.g. shadows).
    if (plPipeResReq::Check() || fPipeline->CheckResources())
    {
        fPipeline->LoadResources();
    }

    plProfile_EndTiming(UpdateTime);

    plProfile_BeginTiming(DrawTime);

    plProfile_BeginTiming(BeginRender);
    if (fPipeline->BeginRender())
    {
        plProfile_EndTiming(BeginRender);
        return IFlushRenderRequests();
    }
    plProfile_EndTiming(BeginRender);

    plProfile_BeginTiming(ClearRender);
    fPipeline->ClearRenderTarget();
    plProfile_EndTiming(ClearRender);

    plProfile_BeginTiming(PreRender);
    if (!fFlags.IsBitSet(kFlagDBGDisableRRequests))
    {
        IProcessPreRenderRequests();
    }
    plProfile_EndTiming(PreRender);

    plProfile_BeginTiming(MainRender);
    if (!fFlags.IsBitSet(kFlagDBGDisableRender))
    {
        fPageMgr->Render(fPipeline);
    }
    plProfile_EndTiming(MainRender);

    plProfile_BeginTiming(PostRender);
    if (!fFlags.IsBitSet(kFlagDBGDisableRRequests))
    {
        IProcessPostRenderRequests();
    }
    plProfile_EndTiming(PostRender);

    plProfile_BeginTiming(Movies);
    IServiceMovies();
    plProfile_EndTiming(Movies);

#ifndef MINIMAL_GL_BUILD
#ifndef PLASMA_EXTERNAL_RELEASE
    plProfile_BeginTiming(Console);
    fConsole->Draw(fPipeline);
    plProfile_EndTiming(Console);
#endif
#endif

    plProfile_BeginTiming(ProgressMgr);
    plProgressMgr::GetInstance()->Draw(fPipeline);
    plProfile_EndTiming(ProgressMgr);

    fLastProgressUpdate = hsTimer::GetSeconds();

    plProfile_BeginTiming(ScreenElem);
    fPipeline->RenderScreenElements();
    plProfile_EndTiming(ScreenElem);

    plProfile_BeginTiming(EndRender);
    fPipeline->EndRender();
    plProfile_EndTiming(EndRender);

    plProfile_EndTiming(DrawTime);

    return false;
}

bool plClient::IDrawProgress()
{
    // Reset our stats
    plProfileManager::Instance().BeginFrame();

    plProfile_BeginTiming(DrawTime);
    if (fPipeline->BeginRender())
    {
        return IFlushRenderRequests();
    }

    // Override the clear color to black.
    fPipeline->ClearRenderTarget(&hsColorRGBA().Set(0.f, 0.f, 0.f, 1.f));

#ifndef MINIMAL_GL_BUILD
#ifndef PLASMA_EXTERNAL_RELEASE
    fConsole->Draw(fPipeline);
#endif

    plStatusLogMgr::GetInstance().Draw();
#endif
    plProgressMgr::GetInstance()->Draw(fPipeline);
    fPipeline->RenderScreenElements();
    fPipeline->EndRender();
    plProfile_EndTiming(DrawTime);

    plProfileManager::Instance().EndFrame();

    return false;
}

bool plClient::IFlushRenderRequests()
{
    // For those requesting ack's, we could go through and send them
    // mail telling them their request was ill-timed. But hopefully,
    // the lack of an acknowledgement will serve as notice.
    int i;
    for (i = 0; i < fPreRenderRequests.GetCount(); i++)
    {
        hsRefCnt_SafeUnRef(fPreRenderRequests[i]);
    }
    fPreRenderRequests.Reset();

    for (i = 0; i < fPostRenderRequests.GetCount(); i++)
    {
        hsRefCnt_SafeUnRef(fPostRenderRequests[i]);
    }
    fPostRenderRequests.Reset();

    return false;
}

void plClient::IProcessRenderRequests(hsTArray<plRenderRequest*>& reqs)
{
    int i;
    for (i = 0; i < reqs.GetCount(); i++)
    {
        reqs[i]->Render(fPipeline, fPageMgr);
        hsRefCnt_SafeUnRef(reqs[i]);
    }
    reqs.SetCount(0);
}

void plClient::IProcessPreRenderRequests()
{
    IProcessRenderRequests(fPreRenderRequests);
}

void plClient::IProcessPostRenderRequests()
{
    IProcessRenderRequests(fPostRenderRequests);
}

void plClient::IAddRenderRequest(plRenderRequest* req)
{
    if (req->GetPriority() < 0)
    {
        int i;
        for (i = 0; i < fPreRenderRequests.GetCount(); i++)
        {
            if (req->GetPriority() < fPreRenderRequests[i]->GetPriority())
                break;
        }
        fPreRenderRequests.Insert(i, req);
        hsRefCnt_SafeRef(req);
    }
    else
    {
        int i;
        for (i = 0; i < fPostRenderRequests.GetCount(); i++)
        {
            if (req->GetPriority() < fPostRenderRequests[i]->GetPriority())
                break;
        }
        fPostRenderRequests.Insert(i, req);
        hsRefCnt_SafeRef(req);
    }
}



void plClient::IDispatchMsgReceiveCallback()
{
    if (fInstance->fProgressBar)
    {
        fInstance->fProgressBar->Increment(1);
    }

    static char buf[30];
    sprintf(buf, "Msg %d", fInstance->fNumPostLoadMsgs);
    fInstance->IIncProgress(fInstance->fPostLoadMsgInc, buf);

    fInstance->fNumPostLoadMsgs++;
}

void plClient::IReadKeyedObjCallback(plKey key)
{
    fInstance->IIncProgress(1, key->GetName().c_str());
}

void plClient::IProgressMgrCallbackProc(plOperationProgress* progress)
{
    if (!fInstance)
        return;

#ifdef HS_BUILD_FOR_WIN32
    // Increments the taskbar progress [Windows 7+]
    if (gTaskbarList && fInstance->GetWindowHandle())
    {
        static TBPFLAG lastState = TBPF_NOPROGRESS;
        TBPFLAG myState;

        // So, calling making these kernel calls is kind of SLOW. So, let's
        // hide that behind a userland check--this helps linking go faster!
        if (progress->IsAborting())
            myState = TBPF_ERROR;
        else if (progress->IsLastUpdate())
            myState = TBPF_NOPROGRESS;
        else if (progress->GetMax() == 0.f)
            myState = TBPF_INDETERMINATE;
        else
            myState = TBPF_NORMAL;

        if (myState == TBPF_NORMAL)
            // This sets us to TBPF_NORMAL
            gTaskbarList->SetProgressValue(fInstance->GetWindowHandle(), (ULONGLONG)progress->GetProgress(), (ULONGLONG)progress->GetMax());
        else if (myState != lastState)
            gTaskbarList->SetProgressState(fInstance->GetWindowHandle(), myState);
        lastState = myState;
    }
#endif

    fInstance->fMessagePumpProc();

    // HACK HACK HACK HACK!
    // Yes, this is the ORIGINAL, EVIL famerate limit from plClient::IDraw (except I bumped it to 60fps)
    // As it so happens, this callback is happening in the main resource loading thread
    // Without this NASTY ASS HACK, we draw after loading every KO, which starves the loader.
    // At some point, a better solution should be found... Like running the loader in a separate thread.
    static float lastDrawTime;
    static const float kMaxFrameRate = 1.f/60.f;
    float currTime = (float) hsTimer::GetSeconds();
    if ((currTime - lastDrawTime) > kMaxFrameRate)
    {
        fInstance->IDraw();
        lastDrawTime = currTime;
    }
}



void plClient::IIncProgress(float byHowMuch, const char * text)
{
    if (fProgressBar) {
#ifndef PLASMA_EXTERNAL_RELEASE
        fProgressBar->SetStatusText(text);
#endif
        fProgressBar->Increment(byHowMuch);
    }
}

void plClient::IStartProgress(const char *title, float len)
{
    if (fProgressBar)
    {
        fProgressBar->SetLength(fProgressBar->GetMax()+len);
    }
    else
    {
        fProgressBar = plProgressMgr::GetInstance()->RegisterOperation(len, title, plProgressMgr::kNone, false, true);
#ifndef PLASMA_EXTERNAL_RELEASE
        if (plDispatchLogBase::IsLogging())
        {
            plDispatchLogBase::GetInstance()->LogStatusBarChange(fProgressBar->GetTitle().c_str(), "starting");
        }
#endif // PLASMA_EXTERNAL_RELEASE

        ((plResManager*)hsgResMgr::ResMgr())->SetProgressBarProc(IReadKeyedObjCallback);
        plDispatch::SetMsgRecieveCallback(IDispatchMsgReceiveCallback);

        fLastProgressUpdate = 0.f;
    }

    if (fPipeline)
    {
        fPipeline->LoadResources();
    }
}

void plClient::IStopProgress()
{
    if (fProgressBar)
    {
        plDispatch::SetMsgRecieveCallback(nullptr);
        ((plResManager*)hsgResMgr::ResMgr())->SetProgressBarProc(IReadKeyedObjCallback);
        delete fProgressBar;
        fProgressBar = nullptr;

        plPipeResReq::Request();

        fFlags.SetBit(kFlagGlobalDataLoaded);
        if (fFlags.IsBitSet(kFlagAsyncInitComplete))
        {
            ICompleteInit();
        }
    }
}

bool plClient::IPlayIntroMovie(const char* movieName, float endDelay, float posX, float posY, float scaleX, float scaleY, float volume /* = 1.0 */)
{
    fQuitIntro = false;

    plMoviePlayer player;
    player.SetPosition(posX, posY);
    player.SetScale(scaleX, scaleY);
    player.SetFileName(movieName);
    player.SetFadeToTime(endDelay);
    player.SetFadeToColor(hsColorRGBA().Set(0, 0, 0, 1.f));
    player.SetVolume(volume);

    bool firstTry = true;  // flag to make sure that we don't quit before we even start

    if (player.Start())
    {
        while (true)
        {
            if (fInstance)
            {
                fInstance->fMessagePumpProc();
            }

            if (GetDone())
            {
                return true;
            }

            if (firstTry)
            {
                firstTry = false;
                fQuitIntro = false;
            }
            else
            {
                if (fQuitIntro)
                {
                    return true;
                }
            }

            bool done = false;
            if (!fPipeline->BeginRender())
            {
                fPipeline->ClearRenderTarget();
                done = !player.NextFrame();

                fPipeline->RenderScreenElements();
                fPipeline->EndRender();
            }

            if (done)
                return true;
        }
        return true;
    }
    return false;
}

bool plClient::IHandleMovieMsg(plMovieMsg* mov)
{
    if (mov->GetFileName().empty())
        return true;

    size_t i = fMovies.size();
    if (!(mov->GetCmd() & plMovieMsg::kMake))
    {
        for (i = 0; i < fMovies.size(); i++)
        {
            if (mov->GetFileName().compare_i(fMovies[i]->GetFileName().AsString()) == 0)
                break;
        }
    }
    if (i == fMovies.size())
    {
        fMovies.push_back(new plMoviePlayer());
        fMovies[i]->SetFileName(mov->GetFileName());
    }

    if (mov->GetCmd() & plMovieMsg::kAddCallbacks)
    {
        int j;
        for (j = 0; j < mov->GetNumCallbacks(); j++)
            fMovies[i]->AddCallback(mov->GetCallback(j));
    }
    if (mov->GetCmd() & plMovieMsg::kMove)
        fMovies[i]->SetPosition(mov->GetCenter());
    if (mov->GetCmd() & plMovieMsg::kScale)
        fMovies[i]->SetScale(mov->GetScale());
    if (mov->GetCmd() & plMovieMsg::kColorAndOpacity)
        fMovies[i]->SetColor(mov->GetColor());
    if (mov->GetCmd() & plMovieMsg::kColor)
    {
        hsColorRGBA c = fMovies[i]->GetColor();
        c.Set(mov->GetColor().r, mov->GetColor().g, mov->GetColor().b, c.a);
        fMovies[i]->SetColor(c);
    }
    if (mov->GetCmd() & plMovieMsg::kOpacity)
    {
        hsColorRGBA c = fMovies[i]->GetColor();
        c.a = mov->GetColor().a;
        fMovies[i]->SetColor(c);
    }
    if (mov->GetCmd() & plMovieMsg::kFadeIn)
    {
        fMovies[i]->SetFadeFromColor(mov->GetFadeInColor());
        fMovies[i]->SetFadeFromTime(mov->GetFadeInSecs());
    }
    if (mov->GetCmd() & plMovieMsg::kFadeOut)
    {
        fMovies[i]->SetFadeToColor(mov->GetFadeOutColor());
        fMovies[i]->SetFadeToTime(mov->GetFadeOutSecs());
    }
    if (mov->GetCmd() & plMovieMsg::kVolume)
        fMovies[i]->SetVolume(mov->GetVolume());

    if (mov->GetCmd() & plMovieMsg::kStart)
        fMovies[i]->Start();
    if (mov->GetCmd() & plMovieMsg::kPause)
        fMovies[i]->Pause(true);
    if (mov->GetCmd() & plMovieMsg::kResume)
        fMovies[i]->Pause(false);
    if (mov->GetCmd() & plMovieMsg::kStop)
        fMovies[i]->Stop();

    // If a movie has lost its filename, it means something went horribly wrong
    // with playing it and it has shutdown. Or we just stopped it. Either way,
    // we need to clear it out of our list.
    if (!fMovies[i]->GetFileName().IsValid())
    {
        delete fMovies[i];
        fMovies[i] = fMovies.back();
        fMovies.pop_back();
    }
    return true;
}

void plClient::IServiceMovies()
{
    for (size_t i = 0; i < fMovies.size(); i++)
    {
        if (!fMovies[i]->NextFrame())
        {
            delete fMovies[i];
            fMovies[i] = fMovies.back();
            fMovies.pop_back();
            i--;
        }
    }
}

void plClient::IKillMovies()
{
    for (size_t i = 0; i < fMovies.size(); i++)
        delete fMovies[i];
    fMovies.clear();
}

void plClient::IOnAsyncInitComplete()
{
    // Init State Desc Language (files should now be downloaded and in place)
    plSDLMgr::GetInstance()->SetNetApp(plNetClientMgr::GetInstance());
    plSDLMgr::GetInstance()->Init( plSDL::kDisallowTimeStamping );

#ifndef MINIMAL_GL_BUILD
    PythonInterface::initPython();
    // set the pipeline for the python cyMisc module so that it can do a screen capture
    cyMisc::SetPipeline(fPipeline);
#endif

    // We'd like to do a SetHoldLoadRequests here, but the GUI stuff doesn't draw right
    // if you try to delay the loading for it.  To work around that, we allocate a
    // global loading bar in advance and set it to a big enough range that when the GUI's
    // are done loading about the right amount of it is filled.
    fNumLoadingRooms++;
    IStartProgress("Loading Global...", 0);

    /// Init the KI
    // load the blackbar which will bootstrap in the rest of the KI dialogs
    fGameGUIMgr->LoadDialog("KIBlackBar");

    // Init the journal book API
    pfJournalBook::SingletonInit();

    SetHoldLoadRequests(true);
    fProgressBar->SetLength(fProgressBar->GetProgress());

    plClothingMgr::Init();
    // Load in any clothing data
    ((plResManager*)hsgResMgr::ResMgr())->PageInAge("GlobalClothing");

    pfMarkerMgr::Instance();

    /// Now parse final init files (*.fni). These are files just like ini files, only to be run
    /// after all hell has broken loose in the client.
    plFileName initFolder = plFileSystem::GetInitPath();
    pfConsoleDirSrc dirSrc(fConsoleEngine, initFolder, "net*.fni");  // connect to net first
#ifndef PLASMA_EXTERNAL_RELEASE
    // internal builds also parse the local init folder
    dirSrc.ParseDirectory("init", "net*.fni");
#endif

    dirSrc.ParseDirectory(initFolder, "*.fni");
#ifndef PLASMA_EXTERNAL_RELEASE
    // internal builds also parse the local init folder
    dirSrc.ParseDirectory("init", "*.fni");
#endif

    // run fni in the Aux Init dir
#if 0
    if (fpAuxInitDir)
    {
        dirSrc.ParseDirectory(fpAuxInitDir, "net*.fni");   // connect to net first
        dirSrc.ParseDirectory(fpAuxInitDir, "*.fni");
    }
#endif

    fNumLoadingRooms--;

    ((plResManager*)hsgResMgr::ResMgr())->PageInAge("GlobalAnimations");
    SetHoldLoadRequests(false);

#ifndef MINIMAL_GL_BUILD // The "hide" message never fires for this
    // Tell the transition manager to start faded out. This is so we don't
    // get a frame or two of non-faded drawing before we do our initial fade in
    (void)(new plTransitionMsg(plTransitionMsg::kFadeOut, 0.0f, true))->Send();
#endif

    fFlags.SetBit(kFlagAsyncInitComplete);
    if (fFlags.IsBitSet(kFlagGlobalDataLoaded))
    {
        ICompleteInit();
    }
}

void plClient::ICompleteInit () {
#ifndef MINIMAL_GL_BUILD
    plSimulationMgr::GetInstance()->Resume();               // start the sim at the last possible minute
#endif

    fFlags.SetBit(kFlagIniting, false);
    hsStatusMessage("Client init complete.");

    fCamera->SetCutNextTrans();

    // Tell everyone we're ready to rock.
    plClientMsg* clientMsg = new plClientMsg(plClientMsg::kInitComplete);
    clientMsg->SetBCastFlag(plMessage::kBCastByType);
    clientMsg->Send();

    if (fInitialAgeName.empty())
    {
        fInitialAgeName = "GuildPub-Writers";
    }

    plAgeLoader::GetInstance()->LoadAge(fInitialAgeName);
}

void plClient::IHandlePatcherMsg(plResPatcherMsg* msg)
{
    plgDispatch::Dispatch()->UnRegisterForExactType(plResPatcherMsg::Index(), GetKey());

    if (!msg->Success()) {
        plNetClientApp::GetInstance()->QueueDisableNet(true, msg->GetError().c_str());
        return;
    }

    IOnAsyncInitComplete();
}
