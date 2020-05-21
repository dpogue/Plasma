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

#include "HeadSpin.h"
#include "plPlateProgressMgr.h"
#include "plPipeline.h"
#include "plPlates.h"
#include "hsTimer.h"

#include "plClientResMgr/plClientResMgr.h"

#include <regex>

// Draw Colors
enum
{
    kTitleColor = 0xccb0b0b0,
    kProgressBarColor = 0xff302b3a,
    kInfoColor = 0xff635e6d,
};

std::vector<ST::string> plPlateProgressMgr::fImageRotation;

const ST::string plPlateProgressMgr::fStaticTextIDs[] = {
    ST_LITERAL("xLoading_Linking_Text.png"),
    ST_LITERAL("xLoading_Updating_Text.png")
};

//// Constructor & Destructor ////////////////////////////////////////////////

plPlateProgressMgr::plPlateProgressMgr() :
    plProgressMgr(),
    fCurrentImage(0),
    fLastDraw(0.0f),
    fActivePlate(nullptr),
    fStaticTextPlate(nullptr),
    fProgressPlate(nullptr),
    fProgressMap(nullptr)
{
    // Find linking-animation frame IDs and store the sorted list
    std::regex re("xLoading_Linking\\.[0-9]+?\\.png");
    for (const auto& name : plClientResMgr::Instance().getResourceNames()) {
        if (std::regex_match(name.begin(), name.end(), re))
            fImageRotation.push_back(name);
    }
    std::sort(fImageRotation.begin(), fImageRotation.end());
}

plPlateProgressMgr::~plPlateProgressMgr()
{
}

void plPlateProgressMgr::DeclareThyself()
{
    static plPlateProgressMgr thyself;
}

void plPlateProgressMgr::Activate()
{
    if (fStaticTextPlate == nullptr && fCurrentStaticText != plProgressMgr::kNone)
    {
        plPlateManager::Instance().CreatePlate(&fStaticTextPlate);

        fStaticTextPlate->CreateFromResource(GetStaticTextID(fCurrentStaticText));
        fStaticTextPlate->SetVisible(true);
        fStaticTextPlate->SetOpacity(1.0f);
        fStaticTextPlate->SetSize(2 * 0.192f, 2 * 0.041f, true);
        fStaticTextPlate->SetPosition(0, 0.5f, 0);
    }

    if (fActivePlate == nullptr)
    {
        plPlateManager::Instance().CreatePlate(&fActivePlate);

        fActivePlate->CreateFromResource(GetLoadingFrameID(fCurrentImage));
        fActivePlate->SetVisible(true);
        fActivePlate->SetOpacity(1.0f);
        fActivePlate->SetSize(0.6, 0.6, true);
        fActivePlate->SetPosition(0, 0, 0);
    }
}

void plPlateProgressMgr::Deactivate()
{
    if (fStaticTextPlate)
    {
        fStaticTextPlate->SetVisible(false);
        plPlateManager::Instance().DestroyPlate(fStaticTextPlate);
        fStaticTextPlate = nullptr;
    }

    if (fActivePlate)
    {
        fActivePlate->SetVisible(false);
        plPlateManager::Instance().DestroyPlate(fActivePlate);
        fActivePlate = nullptr;
    }
}

//// Draw ////////////////////////////////////////////////////////////////////

void plPlateProgressMgr::Draw(plPipeline* pipe)
{
    uint16_t fontSize, scrnWidth, scrnHeight, width, height, x, y;

    plOperationProgress *prog;

    if (fOperations == nullptr)
    {
        return;
    }

    scrnWidth = (uint16_t)pipe->Width();
    scrnHeight = (uint16_t)pipe->Height();

    fontSize = 16;
    width = scrnWidth - 64;
    height = 16;
    x = ( scrnWidth - width ) >> 1;
    y = scrnHeight - 44 - (2 * height) - fontSize;


    if (fActivePlate)
    {
        float currentMs = hsTimer::GetMilliSeconds<float>();
        if ((currentMs - fLastDraw) > 30)
        {
            fCurrentImage++;
            if (fCurrentImage >= NumLoadingFrames())
                fCurrentImage = 0;

            fLastDraw = currentMs;

            fActivePlate->ReloadFromResource(GetLoadingFrameID(fCurrentImage));
            fActivePlate->SetVisible(true);
            fActivePlate->SetOpacity(1.0f);
            fActivePlate->SetSize(0.6, 0.6, true);
            fActivePlate->SetPosition(0, 0, 0);
        }
    }

    for (prog = fOperations; prog != nullptr; prog = prog->GetNext())
    {
        if (IDrawTheStupidThing(pipe, prog, x, y, width, height))
        {
            y -= fontSize + 8 + height + 4;
        }
    }
}

//// IDrawTheStupidThing /////////////////////////////////////////////////////

bool plPlateProgressMgr::IDrawTheStupidThing(plPipeline *p, plOperationProgress *prog, uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    return false;
}


const ST::string plPlateProgressMgr::GetLoadingFrameID(uint32_t index)
{
    if (index < fImageRotation.size())
        return fImageRotation[index];
    else
        return fImageRotation[0];
}

uint32_t plPlateProgressMgr::NumLoadingFrames() const
{
    return fImageRotation.size();
}

const ST::string plPlateProgressMgr::GetStaticTextID(StaticText staticTextType)
{
    return fStaticTextIDs[staticTextType];
}
