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

#include <cstring>
#include <string_theory/stdio>

#include "plCmdParser.h"
#include "plContainer/plConfigInfo.h"

const char* defaultIniFile =
    //"[default]\n"
    "DispName=Myst Online: Uru Live again\n"
    "Port=14617";

int main(int argc, const char** argv)
{
    enum { kArgServerIni, kArgSection };
    const plCmdArgDef cmdLineArgs[] = {
        { kCmdArgFlagged | kCmdTypeString,  "ServerIni",    kArgServerIni },
        { kCmdArgFlagged | kCmdTypeString,  "Section",      kArgSection }
    };

    std::vector<ST::string> args;
    args.reserve(argc);
    for (size_t i = 0; i < argc; i++) {
        args.emplace_back(ST::string::from_utf8(argv[i]));
    }

    plCmdParser cmdParser(cmdLineArgs, std::size(cmdLineArgs));
    if (!cmdParser.Parse(args)) {
        ST::printf(stderr, "An error occurred while parsing the provided arguments.\n");
        return 1;
    }

    ST::string sectionName = plConfigInfo::GlobalSection();
    if (cmdParser.IsSpecified(kArgSection))
        sectionName = cmdParser.GetString(kArgSection);

    hsStream* stream = nullptr;

    if (cmdParser.IsSpecified(kArgServerIni)) {
        stream = new hsUNIXStream();
        stream->Open(cmdParser.GetString(kArgServerIni), "r");
    } else {
        stream = new hsRAMStream();
        stream->Write(strlen(defaultIniFile), defaultIniFile);
        stream->Rewind();
    }

    plIniStreamConfigSource iniFile(stream);
    plConfigInfo cfg;
    cfg.ReadFrom(&iniFile);

    delete stream;
    stream = nullptr;

    std::vector<ST::string> sections = cfg.GetSectionNames();
    ST::printf("We have {} ini file sections:\n", sections.size());
    for (auto s : sections) {
        ST::printf("\t- {}\n", s);
    }

    ST::printf("\nLooking for Server Port in {} section:\n", sectionName);
    bool found = false;
    int port = cfg.GetValue(sectionName, "Port", 1, &found);

    if (found)
        ST::printf("\tFound port {}\n", port);
    else
        ST::printf("\tNOT FOUND\n");

    return 0;
}
