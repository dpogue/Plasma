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

#include "hsTimer.h"
#include "plResMgr/plResManager.h"
#include "plResMgr/plKeyFinder.h"
#include "pnDispatch/plDispatch.h"

#include "plPipeline.h"
#include "plPipeline/plPipelineCreate.h"
#include "plProgressMgr/plProgressMgr.h"
#include "plPipeline/plPlateProgressMgr.h"

#include "pnMessage/plClientMsg.h"
#include "pnMessage/plTimeMsg.h"
#include "plMessage/plRenderMsg.h"
#include "plMessage/plTransitionMsg.h"

#include "pnSceneObject/plCoordinateInterface.h"

#include "plScene/plSceneNode.h"
#include "plScene/plPageTreeMgr.h"
#include "plScene/plVisMgr.h"

#include "plAgeDescription/plAgeDescription.h"
#include "plFile/plEncryptedStream.h"
#include "plGImage/plFontCache.h"

#include "plUnifiedTime/plClientUnifiedTime.h"

plClient* plClient::fInstance = nullptr;

plClient::plClient()
:   fPipeline(nullptr),
    fCurrentNode(nullptr),
    fPageMgr(nullptr),
    fFontCache(nullptr),
    fWindowHndl(nullptr),
    fDone(false),
    fProgressBar(nullptr),
    fHoldLoadRequests(false),
    fNumLoadingRooms(0),
    fNumPostLoadMsgs(0),
    fPostLoadMsgInc(0.f)
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

    hsStatusMessage("Shutting down client...\n");

    for (auto room : fRooms) {
        plSceneNode* sn = room.fNode;
        GetKey()->Release(sn->GetKey());
    }
    fRooms.clear();
    fRoomsLoading.clear();


    delete fPipeline;
    fPipeline = nullptr;

    if (fPageMgr) {
        fPageMgr->Reset();
    }

    IUnRegisterAs(fFontCache, kFontCache_KEY);

    delete fPageMgr;
    fPageMgr = nullptr;
    plGlobalVisMgr::DeInit();

    // This will destruct the client. Do it last.
    UnRegisterAs(kClient_KEY);

    return false;
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
    //from.Set(0.f, -20.f, 5.f);
    from.Set(-24.5391f, -22.1473f, 10.f);
    //at.Set(0.f, 5.f, 0.f);
    at.Set(-23.6462f, 2.2479f, 10.f);
    up.Set(0,0.f,1.f);
    hsMatrix44 cam;
    cam.MakeCamera(&from,&at,&up);

    float yon = 500.0f;

    pipe->SetFOV(60.f, int32_t(60.f * pipe->Height() / pipe->Width()));
    pipe->SetDepth(1.f, yon);

    hsMatrix44 id;
    cam.GetInverse(&id);

    pipe->SetWorldToCamera(cam, id);
    pipe->RefreshMatrices();

    // Do this so we're still black before we show progress bars, but the correct color coming out of 'em
    fClearColor.Set( 0.f, 0.f, 0.0f, 1.f );
    pipe->SetClear(&fClearColor);
    pipe->ClearRenderTarget();

    if (fPipeline)
        fPipeline->LoadResources();

    return false;
}


bool plClient::StartInit()
{
    hsStatusMessage("Init client\n");

    plPlateProgressMgr::DeclareThyself();

    // Set our callback for the progress manager so everybody else can use it
    fLastProgressUpdate = 0.f;
    plProgressMgr::GetInstance()->SetCallbackProc(IProgressMgrCallbackProc);

    // Check the registry, which deletes data files that are either corrupt or
    // have old version numbers.  If the file still exists on the file server
    // then it will be patched on-the-fly as needed (unless you're running with
    // local data of course).
    //((plResManager *)hsgResMgr::ResMgr())->VerifyPages();

    RegisterAs(kClient_KEY);

    plGlobalVisMgr::Init();
    fPageMgr = new plPageTreeMgr();


    /// Init the font cache
    fFontCache = new plFontCache();


    // Pulled from plClient::IOnAsyncInitComplete
    // Load our custom fonts from our current dat directory
    fFontCache->LoadCustomFonts("dat");

    // We'd like to do a SetHoldLoadRequests here, but the GUI stuff doesn't draw right
    // if you try to delay the loading for it.  To work around that, we allocate a
    // global loading bar in advance and set it to a big enough range that when the GUI's
    // are done loading about the right amount of it is filled.
    fNumLoadingRooms++;
    IStartProgress("Loading...", 0);

    SetHoldLoadRequests(true);
    fProgressBar->SetLength(fProgressBar->GetProgress());

    fNumLoadingRooms--;

    //((plResManager*)hsgResMgr::ResMgr())->PageInAge("GlobalAnimations");
    SetHoldLoadRequests(false);

    // Tell the transition manager to start faded out. This is so we don't
    // get a frame or two of non-faded drawing before we do our initial fade in
    //(void)(new plTransitionMsg( plTransitionMsg::kFadeOut, 0.0f, true ))->Send();

    //ILoadAge("ParadoxTestAge");
    ILoadAge("GuildPub-Writers");

    return true;
}

bool plClient::MainLoop()
{
    if (IUpdate())
    {
        return true;
    }

    if (IDraw())
    {
        return true;
    }

    return false;
}


bool plClient::MsgReceive(plMessage* msg)
{
    if (plGenRefMsg* genRefMsg = plGenRefMsg::ConvertNoRef(msg)) {
        // do nothing, we just use the client's key to ref vault image nodes.
        return true;
    }


    plClientRefMsg* pRefMsg = plClientRefMsg::ConvertNoRef(msg);
    if (pRefMsg)
    {
        switch (pRefMsg->fType)
        {
        case plClientRefMsg::kLoadRoom:
            if (hsCheckBits(pRefMsg->GetContext(), plRefMsg::kOnCreate))
            {
                IRoomLoaded(plSceneNode::Convert(pRefMsg->GetRef()), false);
            }
            break;

        case plClientRefMsg::kLoadRoomHold:
            if (hsCheckBits(pRefMsg->GetContext(), plRefMsg::kOnCreate))
            {
                IRoomLoaded(plSceneNode::Convert(pRefMsg->GetRef()), true);
            }
            break;
        }
        return true;
    }


    plClientMsg* pMsg = plClientMsg::ConvertNoRef(msg);
    if (pMsg)
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

            case plClientMsg::kLoadAgeKeys:
                ((plResManager*)hsgResMgr::ResMgr())->LoadAgeKeys(pMsg->GetAgeName());
                break;
        }
        return true;
    }

    return hsKeyedObject::MsgReceive(msg);
}






bool plClient::ILoadAge(const ST::string& ageName)
{
    plFileName filename = plFileName::Join("dat", ST::format("{}.age", ageName));
    hsStream* stream = plEncryptedStream::OpenEncryptedFile(filename);

    plAgeDescription ad;
    ad.Read(stream);
    ad.SetAgeName(ageName);
    stream->Close();
    delete stream;
    ad.SeekFirstPage();

    plAgePage* page;
    plKey clientKey = GetKey();

    plClientMsg* loadAgeKeysMsg = new plClientMsg(plClientMsg::kLoadAgeKeys);
    loadAgeKeysMsg->SetAgeName(ageName);
    loadAgeKeysMsg->Send(clientKey);

    plClientMsg* pMsg1 = new plClientMsg(plClientMsg::kLoadRoom);
    pMsg1->SetAgeName(ageName);

    while ((page = ad.GetNextPage()) != nullptr)
    {
        pMsg1->AddRoomLoc(ad.CalcPageLocation(page->GetName()));
    }

    pMsg1->Send(clientKey);

    plClientMsg* dumpAgeKeys = new plClientMsg(plClientMsg::kReleaseAgeKeys);
    dumpAgeKeys->SetAgeName(ageName);
    dumpAgeKeys->Send(clientKey);

    return true;
}



int plClient::IFindRoomByLoc(const plLocation& loc)
{
    int i = 0;
    for (auto it = fRooms.begin(); it != fRooms.end(); ++it)
    {
        if (it->fNode->GetKey()->GetUoid().GetLocation() == loc)
            return i;
        i++;
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
    for (auto it = fRooms.begin(); it != fRooms.end(); ++it)
    {
        if (it->fNode == fCurrentNode)
        {
            bAppend = false;
            break;
        }
    }
    if (bAppend)
    {
        if (hold)
        {
            fRooms.push_back(plRoomRec(fCurrentNode, plRoomRec::kHeld));
        }
        else
        {
            fRooms.push_back(plRoomRec(fCurrentNode, 0));
            fPageMgr->AddNode(fCurrentNode);
        }
    }

    fNumLoadingRooms--;

    hsRefCnt_SafeUnRef(fCurrentNode);
    plKey pRmKey = fCurrentNode->GetKey();
    //plAgeLoader::GetInstance()->FinishedPagingInRoom(&pRmKey, 1);
    // *** this used to call "ActivateNode" (in physics) which wasn't implemented.
    // *** we should make this "turn on" physics for the selected node
    // *** depending on what guarantees we can make about the load state -- anything useful?

    // now tell all those who are interested that a room was loaded
    /*if (!hold)
    {
        plRoomLoadNotifyMsg* loadmsg = new plRoomLoadNotifyMsg;
        loadmsg->SetRoom(pRmKey);
        loadmsg->SetWhatHappen(plRoomLoadNotifyMsg::kLoaded);
        plgDispatch::MsgSend(loadmsg);
    }*/

    if (hold)
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


bool plClient::IUpdate()
{
    // reset timer on first frame if realtime and not clamping, to avoid initial large delta
    if (hsTimer::GetSysSeconds()==0 && hsTimer::IsRealTime() && hsTimer::GetTimeClamp()==0)
    {
        hsTimer::SetRealTime(true);
    }

    plgDispatch::Dispatch()->MsgQueueProcess();

    hsTimer::IncSysSeconds();
    plClientUnifiedTime::SetSysTime(); // keep a unified time, based on sysSeconds
    // Time may have been clamped in IncSysSeconds, depending on hsTimer's current mode.

    double currTime = hsTimer::GetSysSeconds();
    float delSecs = hsTimer::GetDelSysSeconds();

    plTimeMsg* msg = new plTimeMsg(nullptr, nullptr, nullptr, nullptr);
    plgDispatch::MsgSend(msg);

    plEvalMsg* eval = new plEvalMsg(nullptr, nullptr, nullptr, nullptr);
    plgDispatch::MsgSend(eval);

    plTransformMsg* xform = new plTransformMsg(nullptr, nullptr, nullptr, nullptr);
    plgDispatch::MsgSend(xform);

    plCoordinateInterface::SetTransformPhase(plCoordinateInterface::kTransformPhaseDelayed);

    // At this point, we just register for a plDelayedTransformMsg when dirtied.
    if (!plCoordinateInterface::GetDelayedTransformsEnabled())
    {
        xform = new plTransformMsg(nullptr, nullptr, nullptr, nullptr);
        plgDispatch::MsgSend(xform);
    }
    else
    {
        xform = new plDelayedTransformMsg(nullptr, nullptr, nullptr, nullptr);
        plgDispatch::MsgSend(xform);
    }

    plCoordinateInterface::SetTransformPhase(plCoordinateInterface::kTransformPhaseNormal);


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

    plGlobalVisMgr::Instance()->Eval(fPipeline->GetViewPositionWorld());

    plRenderMsg* rendMsg = new plRenderMsg(fPipeline);
    plgDispatch::MsgSend(rendMsg);

    if (fPipeline->BeginRender())
    {
        return false;
    }

    fPipeline->ClearRenderTarget();

    fPageMgr->Render(fPipeline);

    plProgressMgr::GetInstance()->Draw(fPipeline);

    fLastProgressUpdate = hsTimer::GetSeconds();

    fPipeline->RenderScreenElements();

    fPipeline->EndRender();

    return false;
}

bool plClient::IDrawProgress() {
    if (fPipeline->BeginRender())
    {
        return false;
    }

    // Override the clear color to black.
    fPipeline->ClearRenderTarget(&hsColorRGBA().Set(0.f, 0.f, 0.f, 1.f));

    plProgressMgr::GetInstance()->Draw(fPipeline);
    fPipeline->RenderScreenElements();
    fPipeline->EndRender();

    return false;
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

    // Increments the taskbar progress [Windows 7+]
#ifdef HS_BUILD_FOR_WIN32
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

    //fInstance->fMessagePumpProc();

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
    if (/*false &&*/ fProgressBar)
    {
        plDispatch::SetMsgRecieveCallback(nullptr);
        ((plResManager*)hsgResMgr::ResMgr())->SetProgressBarProc(IReadKeyedObjCallback);
        delete fProgressBar;
        fProgressBar = nullptr;

        //plPipeResReq::Request();

        //fFlags.SetBit(kFlagGlobalDataLoaded);
        //if (fFlags.IsBitSet(kFlagAsyncInitComplete))
        //    ICompleteInit();
    }
}
