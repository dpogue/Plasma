#include "plGLClient.h"

#include "hsTimer.h"
#include "plResMgr/plResManager.h"
#include "pnDispatch/plDispatch.h"

#include "plPipeline/GL/plGLPipeline.h"

#include "plMessage/plRenderMsg.h"

#include "plScene/plPageTreeMgr.h"
#include "plScene/plVisMgr.h"

#include <X11/Xlib.h>

plGLClient* plGLClient::fInstance = nullptr;

plGLClient::plGLClient()
:   fPipeline(nullptr),
    fPageMgr(nullptr),
    fWindowHndl(nullptr),
    fDone(false)
{
    hsStatusMessage("Constructing client\n");
    plGLClient::SetInstance(this);

    hsTimer::SetRealTime(true);
    hsTimer::SetTimeClamp(0.f);
}

plGLClient::~plGLClient()
{
    hsStatusMessage("Destructing client\n");

    plGLClient::SetInstance(nullptr);

    delete fPageMgr;
}


bool plGLClient::Shutdown()
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

    return false;
}


bool plGLClient::InitPipeline()
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
    from.Set(0, 0, 10.f);
    at.Set(0, 20.f, 10.f);
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
    fClearColor.Set( 0.f, 0.f, 0.f, 1.f );
    pipe->SetClear(&fClearColor);
    pipe->ClearRenderTarget();

    return false;
}


bool plGLClient::StartInit()
{
    hsStatusMessage("Init client\n");

    plGlobalVisMgr::Init();
    fPageMgr = new plPageTreeMgr();

    return true;
}

bool plGLClient::MainLoop()
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

        fPageMgr->Render(fPipeline);

        fPipeline->EndRender();
    }

    return true;
}
