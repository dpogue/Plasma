#ifndef _plGLClient_h_
#define _plGLClient_h_

#define MINIMAL_GL_BUILD

#include "HeadSpin.h"

#include "plScene/plRenderRequest.h"

class plPageTreeMgr;
class plPipeline;

class plGLClient
{
protected:
    plPageTreeMgr*      fPageMgr;
    plPipeline*         fPipeline;
    hsColorRGBA         fClearColor;

    hsWindowHndl        fWindowHndl;
    bool                fDone;

    static plGLClient*  fInstance;

public:
    plGLClient();
    virtual ~plGLClient();

    static plGLClient*      GetInstance() { return fInstance; }
    static void             SetInstance(plGLClient* v) { fInstance = v; }


    bool InitPipeline();


    virtual bool StartInit();
    virtual bool Shutdown();
    virtual bool MainLoop();

    plGLClient& SetDone(bool done) { fDone = done; return *this; }
    bool GetDone() { return fDone; }

    virtual plGLClient& SetWindowHandle(hsWindowHndl hndl) { fWindowHndl = hndl; return *this; }
    hsWindowHndl GetWindowHandle() { return fWindowHndl; }
};

#endif
