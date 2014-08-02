#ifndef _plGLClient_h_
#define _plGLClient_h_

#define MINIMAL_GL_BUILD

#include "HeadSpin.h"
#include <list>
#include <vector>

#include "pnKeyedObject/hsKeyedObject.h"
#include "pnKeyedObject/plUoid.h"
#include "plScene/plRenderRequest.h"

class plPageTreeMgr;
class plPipeline;
class plSceneNode;

/**
 * Our OpenGL-based plClient.
 * Not to be confused with the other plClient.
 *
 * I'd wanted to name this plGLClient, but needed the fixed key stuff and
 * didn't want to have to deal with the issues of registering a new creatable
 * type for this... so we just pretend that we're plClient.
 */
class plClient : public hsKeyedObject
{
protected:
    class plRoomRec
    {
    public:
        plSceneNode* fNode;
        uint32_t fFlags;

        plRoomRec() { fNode = nullptr; fFlags = 0; }
        plRoomRec(plSceneNode* n, uint32_t f) : fNode(n), fFlags(f) {}

        enum Flags
        {
            kHeld = 0x00000001
        };
    };

    class LoadRequest
    {
    public:
        LoadRequest(const plLocation& loc, bool hold) { this->loc = loc; this->hold = hold; }
        plLocation loc;
        bool hold;
    };


    plPageTreeMgr*          fPageMgr;
    std::list<plRoomRec>    fRooms;
    plSceneNode*            fCurrentNode;

    plPipeline*             fPipeline;
    hsColorRGBA             fClearColor;

    hsWindowHndl            fWindowHndl;
    bool                    fDone;
    double                  fLastProgressUpdate;

    bool                    fHoldLoadRequests;
    std::list<LoadRequest*> fLoadRooms;
    int                     fNumLoadingRooms;
    std::vector<plLocation> fRoomsLoading;

    static plClient*        fInstance;

public:
    plClient();
    virtual ~plClient();

    CLASSNAME_REGISTER(plClient);
    GETINTERFACE_ANY(plClient, hsKeyedObject);

    static plClient*      GetInstance() { return fInstance; }
    static void           SetInstance(plClient* v) { fInstance = v; }

    virtual bool MsgReceive(plMessage* msg);

    bool InitPipeline();


    virtual bool StartInit();
    virtual bool Shutdown();
    virtual bool MainLoop();

    plClient& SetDone(bool done) { fDone = done; return *this; }
    bool GetDone() { return fDone; }

    virtual plClient& SetWindowHandle(hsWindowHndl hndl) { fWindowHndl = hndl; return *this; }
    hsWindowHndl GetWindowHandle() { return fWindowHndl; }

protected:
    // Hackery to avoid all of plAgeLoader and the netclient stuff
    bool ILoadAge(const plString& ageName);
    bool IUpdate();
    bool IDraw();

    int IFindRoomByLoc(const plLocation& loc);
    bool IIsRoomLoading(const plLocation& loc);
    void IQueueRoomLoad(const std::vector<plLocation>& locs, bool hold);
    void ILoadNextRoom();
    void IRoomLoaded(plSceneNode* node, bool hold);
};

#endif
