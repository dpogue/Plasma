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

#ifndef _plGLClient_h_
#define _plGLClient_h_

#include "HeadSpin.h"
#include <list>
#include <vector>

#include "pnKeyedObject/hsKeyedObject.h"
#include "pnKeyedObject/plUoid.h"
#include "plScene/plRenderRequest.h"

class pfConsoleEngine;
class pfConsole;
class plFontCache;
class plMovieMsg;
class plMoviePlayer;
class plOperationProgress;
class plPageTreeMgr;
class plPipeline;
class plResPatcherMsg;
class plSceneNode;
class plTransitionMgr;

typedef void (*plMessagePumpProc)();

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


    hsBitVector                 fFlags;

    plPageTreeMgr*              fPageMgr;
    hsTArray<plRoomRec>         fRooms;
    plSceneNode*                fCurrentNode;

    plPipeline*                 fPipeline;
    hsColorRGBA                 fClearColor;
    plTransitionMgr*            fTransitionMgr;

    plFontCache*                fFontCache;

    pfConsoleEngine*            fConsoleEngine;
    pfConsole*                  fConsole;

    hsWindowHndl                fWindowHndl;
    bool                        fDone;
    double                      fLastProgressUpdate;
    plOperationProgress*        fProgressBar;

    bool                        fHoldLoadRequests;
    std::list<LoadRequest*>     fLoadRooms;
    int                         fNumLoadingRooms;
    std::vector<plLocation>     fRoomsLoading;
    int                         fNumPostLoadMsgs;
    float                       fPostLoadMsgInc;

    static plClient*            fInstance;
    static bool                 fDelayMS;

    bool                        fQuitIntro;
    std::vector<plMoviePlayer*> fMovies;

    plMessagePumpProc           fMessagePumpProc;

    hsTArray<plRenderRequest*>  fPreRenderRequests;
    hsTArray<plRenderRequest*>  fPostRenderRequests;

public:
    enum
    {
        kFlagIniting,
        kFlagDBGDisableRender,
        kFlagDBGDisableRRequests,
        kFlagAsyncInitComplete,
        kFlagGlobalDataLoaded,
    };


    plClient();
    virtual ~plClient();

    CLASSNAME_REGISTER(plClient);
    GETINTERFACE_ANY(plClient, hsKeyedObject);

    static plClient* GetInstance() { return fInstance; }
    static void SetInstance(plClient* v) { fInstance = v; }

    bool MsgReceive(plMessage* msg) HS_OVERRIDE;

    bool InitPipeline(hsWindowHndl display);


    virtual bool StartInit();
    virtual bool Shutdown();
    virtual bool MainLoop();

    bool HasFlag(int f) const { return fFlags.IsBitSet(f); }
    void SetFlag(int f, bool on=true) { fFlags.SetBit(f, on); }

    plClient& SetDone(bool done) { fDone = done; return *this; }
    bool GetDone() { return fDone; }

    bool GetQuitIntro() const { return fQuitIntro; }
    void SetQuitIntro(bool on) { fQuitIntro = on; }

    // Set this to true to queue any room load requests that come in.  Set it to false to process them.
    void SetHoldLoadRequests(bool hold);

    virtual plClient& SetWindowHandle(hsWindowHndl hndl) { fWindowHndl = hndl; return *this; }
    hsWindowHndl GetWindowHandle() { return fWindowHndl; }

    plPipeline* GetPipeline() { return fPipeline; }

    plSceneNode* GetCurrentScene() { return fCurrentNode; }

    pfConsoleEngine* GetConsoleEngine() { return fConsoleEngine; }

    bool BeginGame();

    void SetMessagePumpProc(plMessagePumpProc proc) { fMessagePumpProc = proc; }

protected:
    /**
     * Detect audio/video settings and save them to their respective ini file,
     * if ini files don't exist.
     */
    void IDetectAudioVideoSettings();
    void IWriteDefaultGraphicsSettings(const plFileName& destFile);
    void IWriteDefaultAudioSettings(const plFileName& destFile);

    // Hackery to avoid all of plAgeLoader and the netclient stuff
    bool ILoadAge(const ST::string& ageName);
    bool IUpdate();
    bool IDraw();
    bool IDrawProgress();

    bool IFlushRenderRequests();
    void IProcessPreRenderRequests();
    void IProcessPostRenderRequests();
    void IProcessRenderRequests(hsTArray<plRenderRequest*>& reqs);
    void IAddRenderRequest(plRenderRequest* req);

    void IStartProgress(const char *title, float len);
    void IIncProgress(float byHowMuch, const char *text);
    void IStopProgress();

    static void IDispatchMsgReceiveCallback();
    static void IReadKeyedObjCallback(plKey key);
    static void IProgressMgrCallbackProc(plOperationProgress* progress);

    bool IPlayIntroMovie(const char* movieName, float endDelay, float posX, float posY, float scaleX, float scaleY, float volume = 1.0);
    bool IHandleMovieMsg(plMovieMsg* mov);
    void IKillMovies();
    void IServiceMovies();

    int IFindRoomByLoc(const plLocation& loc);
    bool IIsRoomLoading(const plLocation& loc);
    void IQueueRoomLoad(const std::vector<plLocation>& locs, bool hold);
    void ILoadNextRoom();
    void IUnloadRooms(const std::vector<plLocation>& locs);
    void IRoomLoaded(plSceneNode* node, bool hold);
    void IRoomUnloaded(plSceneNode* node);

    void ICompleteInit();
    void IOnAsyncInitComplete();
    void IHandlePatcherMsg(plResPatcherMsg* msg);
};

#endif
