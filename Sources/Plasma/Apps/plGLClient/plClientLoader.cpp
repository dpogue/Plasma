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

#include "plClientLoader.h"
#include "plGLClient.h"
#include "plFileSystem.h"
#include "plPipeline.h"

#include "plClientResMgr/plClientResMgr.h"
#include "plNetClient/plNetClientMgr.h"
#include "plResMgr/plResManager.h"

enum
{
    kArgSkipLoginDialog,
    kArgServerIni,
    kArgLocalData,
    kArgSkipPreload,
    kArgPlayerId,
    kArgStartUpAgeName,
};

static const plCmdArgDef s_cmdLineArgs[] = {
    //{ kCmdArgFlagged  | kCmdTypeBool,       "SkipLoginDialog", kArgSkipLoginDialog },
    //{ kCmdArgFlagged  | kCmdTypeString,     "ServerIni",       kArgServerIni },
    { kCmdArgFlagged  | kCmdTypeBool,       "LocalData",       kArgLocalData   },
    //{ kCmdArgFlagged  | kCmdTypeBool,       "SkipPreload",     kArgSkipPreload },
    //{ kCmdArgFlagged  | kCmdTypeInt,        "PlayerId",        kArgPlayerId },
    { kCmdArgFlagged  | kCmdTypeString,     "Age",             kArgStartUpAgeName },
};

extern bool gDataServerLocal;


plClientLoader::plClientLoader() :
    fClient(nullptr),
    fWindow(nullptr),
    fDisplay(nullptr),
    fArguments(s_cmdLineArgs, std::size(s_cmdLineArgs))
{ }

void plClientLoader::Run()
{
    plResManager* resMgr = new plResManager();
    resMgr->SetDataPath("dat");
    hsgResMgr::Init(resMgr);

    if (!plFileInfo("resource.dat").Exists())
    {
        hsMessageBox("Required file 'resource.dat' not found.", "Error", hsMessageBoxNormal);
        return;
    }
    plClientResMgr::Instance().ILoadResources("resource.dat");

    fClient = new plClient();

    fClient->SetWindowHandle(fWindow);
#ifndef HS_BUILD_FOR_APPLE
    if (fClient->InitPipeline(fDisplay) || !fClient->StartInit()) {
        fClient->SetDone(true);
    }
#endif

    HandleArguments();
}

void plClientLoader::HandleArguments()
{
    if (fArguments.IsSpecified(kArgLocalData))
    {
        gDataServerLocal = true;
    }

    if (fArguments.IsSpecified(kArgStartUpAgeName))
    {
        gDataServerLocal = true;
        fClient->SetInitialAgeName(fArguments.GetString(kArgStartUpAgeName));
        fClient->SetQuitIntro(true);
    }
}

void plClientLoader::Init(int argc, const char** argv)
{
    hsAssert(fClient == nullptr, "trying to init the client more than once?");

    std::vector<ST::string> args;
    args.reserve(argc);
    for (size_t i = 0; i < argc; i++)
    {
        args.push_back(ST::string::from_utf8(argv[i]));
    }
    fArguments.Parse(args);

    hsThread::Start();
}

void plClientLoader::Start()
{
    //fClient->ResizeDisplayDevice(fClient->GetPipeline()->Width(), fClient->GetPipeline()->Height(), !fClient->GetPipeline()->IsFullScreen());

    /*
    // Show the client window
    ShowWindow(fWindow, SW_SHOW);
    BringWindowToTop(fWindow);
    */

    // Now, show the intro video, patch the global ages, etc...
    fClient->BeginGame();
}

// ===================================================
void plClientLoader::ShutdownStart()
{
    // Ensure that the client actually inited
    hsThread::Stop();

    // Now request the sane exit
    fClient->SetDone(true);

    if (plNetClientMgr* mgr = plNetClientMgr::GetInstance())
    {
        mgr->QueueDisableNet(false, nullptr);
    }
}

void plClientLoader::ShutdownEnd()
{
    if (fClient)
    {
        fClient->Shutdown();
    }

    hsAssert(hsgResMgr::ResMgr()->RefCnt() == 1, "resMgr has too many refs, expect mem leaks");
    hsgResMgr::Shutdown();
}

