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

#ifndef pyImageLibMod_h
#define pyImageLibMod_h

/////////////////////////////////////////////////////////////////////////////
//
// NAME: pyImageLibMod
//
// PURPOSE: Class wrapper for Python to a plImageLib Modifier
//

#include "pyGlueHelpers.h"

#include "plModifier/plImageLibMod.h"

class plKey;
class pyImage;

class pyImageLibMod
{
protected:
    plKey               fModifierKey;
    plImageLibMod*      fModifier;

    // For python glue only, do NOT call
    pyImageLibMod()
        : fModifierKey(), fModifier()
    { }

    // Constructor from C++
    pyImageLibMod(plKey ilmKey)
        : fModifierKey(std::move(ilmKey)), fModifier()
    { }

    // Constructor from C++ ... uses pointer instead of plKey
    pyImageLibMod(plImageLibMod* ilm)
        : fModifierKey(ilm->GetKey()), fModifier(ilm)
    {
        if (fModifierKey)
            fModifierKey->RefObject();
    }

public:
    ~pyImageLibMod()
    {
        if (fModifier && fModifierKey)
            fModifierKey->UnRefObject();
    }

    // required functions for PyObject interoperability
    PYTHON_CLASS_NEW_FRIEND(ptImageLibMod);
    static PyObject* New(plImageLibMod* ilm);
    static PyObject* New(plKey ilmKey);
    static PyObject* New(pyKey& ilmKey);
    PYTHON_CLASS_CHECK_DEFINITION; // returns true if the PyObject is a pyImageLibMod object
    PYTHON_CLASS_CONVERT_FROM_DEFINITION(pyImageLibMod); // converts a PyObject to a pyImageLibMod (throws error if not correct type)

    static void AddPlasmaClasses(PyObject* m);
    static void AddPlasmaMethods(PyObject* m);

    void setKey(pyKey& ilmKey);

    // override the equals to operator
    bool operator==(const pyImageLibMod& mod) const
    {
        // only thing that needs testing is the plKey, which is unique for all
        if (fModifierKey == ((pyImageLibMod&)mod).GetKey() )
            return true;
        else
            return false;
    }
    bool operator!=(const pyImageLibMod& mod) const { return !(mod == *this);   }

    // for C++ access
    plKey GetKey() const { return fModifier ? fModifier->GetKey() : fModifierKey; }

    // for python access
    PyObject* GetImage(const ST::string& name) const;
    std::vector<ST::string> GetImageNames() const;
    std::vector<PyObject*> GetImages() const;
};

#endif  // pyImageLibMod_h