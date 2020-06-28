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
//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//  plPlateProgressMgr Header                                               //
//                                                                          //
//// Description /////////////////////////////////////////////////////////////
//                                                                          //
//  Derived class of plProgressMgr to draw the progress bars via plates.    //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#ifndef _plPlateProgressMgr_h
#define _plPlateProgressMgr_h

#include "plProgressMgr/plProgressMgr.h"

class plPipeline;
class plDynamicTextMap;

//// Manager Class Definition ////////////////////////////////////////////////

class plPlateProgressMgr : public plProgressMgr
{
protected:
    static std::vector<ST::string> fImageRotation;
    static const ST::string fStaticTextIDs[];

    uint32_t            fCurrentImage;
    float               fLastDraw;
    plPlate*            fActivePlate;
    plPlate*            fStaticTextPlate;
    plPlate*            fProgressPlate;
    StaticText          fShowingStaticText;
    plDynamicTextMap*   fProgressMap;

    void Activate() override;
    void Deactivate() override;

    bool IDrawTheStupidThing(plPipeline* p, plOperationProgress* prog,
                             uint16_t x, uint16_t y,
                             uint16_t width, uint16_t height);

public:
    plPlateProgressMgr();
    ~plPlateProgressMgr();

    static void DeclareThyself();

    void Draw(plPipeline* pipe) override;

    const ST::string GetLoadingFrameID(uint32_t index);
    uint32_t NumLoadingFrames() const;
    const ST::string GetStaticTextID(StaticText staticTextType);
};

#endif //_plPlateProgressMgr_h
