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
#include "hsResMgr.h"
#include "hsTimer.h"

#include "plClientResMgr/plClientResMgr.h"
#include "plGImage/plDynamicTextMap.h"
#include "pnKeyedObject/plUoid.h"
#include "plSurface/hsGMaterial.h"
#include "plSurface/plLayer.h"

#include <regex>

// Draw Colors
enum
{
    kTitleColor = 0xccb0b0b0,
    kProgressBarColor = 0xff302b3a,
    kInfoColor = 0xff635e6d,
    kGlobalColor = 0xff353a2b,
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
    if (fProgressPlate == nullptr)
    {
        uint32_t width = plPlateManager::Instance().GetPipeWidth();
        uint32_t height = plPlateManager::Instance().GetPipeHeight();

        if (fProgressMap == nullptr)
        {
            plDynamicTextMap* map = new plDynamicTextMap(width, uint32_t(height/2.0), false);
            hsgResMgr::ResMgr()->NewKey("PlateProgressMap#0", map, plLocation::kGlobalFixedLoc);

#ifdef PLASMA_EXTERNAL_RELEASE
            map->SetFont("Tahoma", 8);
#else
            map->SetFont("Courier", 8);
#endif

            hsRefCnt_SafeAssign(fProgressMap, map);
        }

        plPlateManager::Instance().CreatePlate(&fProgressPlate);

        fProgressPlate->CreateMaterial(width, uint32_t(height/2.0), false, fProgressMap);
        fProgressPlate->SetVisible(true);
        fProgressPlate->SetOpacity(1.0f);
        fProgressPlate->SetSize(2.0f, 1.0f, false);
        fProgressPlate->SetPosition(0, 0.5, 0);

        // Set the layer transform to match the intended size of the DTM
        if (plLayer* lay = plLayer::ConvertNoRef(fProgressPlate->GetMaterial()->GetLayer(0)))
        {
            lay->SetTransform(fProgressMap->GetLayerTransform());
        }
    }

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

    if (fProgressPlate)
    {
        fProgressPlate->SetVisible(false);
        plPlateManager::Instance().DestroyPlate(fProgressPlate);
        fProgressPlate = nullptr;
    }

    if (fProgressMap)
    {
        hsRefCnt_SafeUnRef(fProgressMap);
        fProgressMap = nullptr;
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

    fontSize = fProgressMap->GetFontSize();
    width = scrnWidth - 64;
    height = 16;
    x = ( scrnWidth - width ) >> 1;
    y = uint32_t(scrnHeight/2.0) - 44 - (2 * height) - fontSize;


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
    if (!fProgressMap)
    {
        return false;
    }

    hsColorRGBA clearColor;
    hsColorRGBA barColor;

    clearColor.Set(0.0f, 0.0f, 0.0f, 0.0f);
    barColor.FromARGB32(kProgressBarColor);

    fProgressMap->ClearToColor(clearColor);

    bool drew_something = false;
    uint16_t downsz = (fProgressMap->GetFontSize() << 1) + 4;

    // draw the title
    if (!prog->GetTitle().empty()) {
        hsColorRGBA textColor;
        textColor.FromARGB32(kTitleColor);
        fProgressMap->SetTextColor(textColor);
        y -= downsz;
        fProgressMap->DrawString(x, y, prog->GetTitle());
        y += downsz;
        drew_something = true;
    }

    // draw a progress bar
    if (prog->GetMax() > 0.f) {
        fProgressMap->FrameRect(x, y, width, height, barColor);

        x += 2;
        y += 2;
        width -= 4;
        height -= 4;

        uint16_t drawWidth = width;

        if (prog->GetProgress() <= prog->GetMax())
        {
            drawWidth = (uint16_t)((float)width * prog->GetProgress() / prog->GetMax());
        }

        if (drawWidth > 0)
        {
            fProgressMap->FillRect(x, y, drawWidth, height, barColor);
        }

        y += height + 2;

        drew_something = true;
    }

    hsColorRGBA infoColor;
    infoColor.FromARGB32(kInfoColor);
    fProgressMap->SetTextColor(infoColor);

    //  draw the left justified status text
    if (!prog->GetStatusText().empty()) {
        fProgressMap->DrawString(x, y, prog->GetStatusText());
        drew_something = true;
    }

    // draw the right justified info text
    if (!prog->GetInfoText().empty()) {
        uint16_t right_x = 2 + x + width - fProgressMap->CalcStringWidth(prog->GetInfoText());
        fProgressMap->DrawString(right_x, y, prog->GetInfoText());
        drew_something = true;
    }

    return drew_something;
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
