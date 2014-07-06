#include "plGLClient.h"

#include "hsTimer.h"
#include "plResMgr/plResManager.h"
#include "plResMgr/plKeyFinder.h"
#include "pnDispatch/plDispatch.h"

#include "plPipeline/GL/plGLPipeline.h"

#include "pnMessage/plClientMsg.h"
#include "plMessage/plRenderMsg.h"

#include "plScene/plSceneNode.h"

#include "plScene/plPageTreeMgr.h"
#include "plScene/plVisMgr.h"

#include "plAgeDescription/plAgeDescription.h"
#include "plFile/plEncryptedStream.h"

#include <X11/Xlib.h>

plClient* plClient::fInstance = nullptr;

plClient::plClient()
:   fPipeline(nullptr),
    fCurrentNode(nullptr),
    fPageMgr(nullptr),
    fWindowHndl(nullptr),
    fDone(false),
    fHoldLoadRequests(false),
    fNumLoadingRooms(0)
{
    hsStatusMessage("Constructing client\n");
    plClient::SetInstance(this);

    hsTimer::SetRealTime(true);
    hsTimer::SetTimeClamp(0.f);
}

plClient::~plClient()
{
    hsStatusMessage("Destructing client\n");

    plClient::SetInstance(nullptr);

    delete fPageMgr;
}


bool plClient::Shutdown()
{
    hsgResMgr::ResMgr()->BeginShutdown();

    hsStatusMessage("Shutting down client...\n");

    delete fPipeline;
    fPipeline = nullptr;

    if (fPageMgr)
    {
        fPageMgr->Reset();
    }


    delete fPageMgr;
    fPageMgr = nullptr;
    plGlobalVisMgr::DeInit();

    // This will destruct the client. Do it last.
    UnRegisterAs(kClient_KEY);

    return false;
}


bool plClient::InitPipeline()
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


    Display* display = XOpenDisplay(nullptr);


    /* Create the pipeline */
    plPipeline* pipe = new plGLPipeline((hsWindowHndl)display, (hsWindowHndl)fWindowHndl, &devRec);
    if (pipe->GetErrorString() != nullptr)
    {
        hsStatusMessage(pipe->GetErrorString());
        return true;
    }
    fPipeline = pipe;


    hsVector3 up;
    hsPoint3 from, at;
    //from.Set(0, 0, 10.f);
    from.Set(-260.4492f, -246.5235f, 224.1992f);
    //at.Set(0, 20.f, 10.f);
    at.Set(-239.9391f, 22.2739f, 104.351f);
    up.Set(0,0,-1.f);
    hsMatrix44 cam;
    cam.MakeCamera(&from,&at,&up);

    float yon = 500.0f;

    pipe->SetFOV( 60.f, int32_t( 60.f * pipe->Height() / pipe->Width() ) );
    pipe->SetDepth( 0.3f, yon );

    hsMatrix44 id;
    id.Reset();

    pipe->SetWorldToCamera( cam, id );
    pipe->RefreshMatrices();

    // Do this so we're still black before we show progress bars, but the correct color coming out of 'em
    fClearColor.Set( 0.f, 0.f, 0.5f, 1.f );
    pipe->SetClear(&fClearColor);
    pipe->ClearRenderTarget();

    return false;
}


bool plClient::StartInit()
{
    hsStatusMessage("Init client\n");

    RegisterAs(kClient_KEY);

    plGlobalVisMgr::Init();
    fPageMgr = new plPageTreeMgr();

    ILoadAge("GuildPub-Writers");

    return true;
}

bool plClient::MainLoop()
{
    if (!fDone)
    {
        plGlobalVisMgr::Instance()->Eval(fPipeline->GetViewPositionWorld());

        plRenderMsg* rendMsg = new plRenderMsg(fPipeline);
        plgDispatch::MsgSend(rendMsg);

        if (fPipeline->BeginRender())
        {
            return false;
        }

        fPipeline->ClearRenderTarget();

        fPageMgr->Render(fPipeline);

        fPipeline->EndRender();
    }

    return true;
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
}
