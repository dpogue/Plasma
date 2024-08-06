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

#include "pmConsoleCommander.h"

#include <string_theory/string>
#include <string_theory/string_stream>

#include "plClassIndexMacros.h"
#include "plCreatableIndex.h"
#include "pnFactory/plFactory.h"
#include "plMessage/plConsoleMsg.h"

#define FACTORY_NEW(cls) static_cast<cls*>(plFactory::Create(CLASS_INDEX_SCOPED(cls)));

struct CommandEntry {
    const ST::string label;
    const ST::string cmd;
};

static const CommandEntry commands[] = {
    { ST_LITERAL("Respawn Avatar"),           ST_LITERAL("Avatar.Spawn.Respawn") },

    { ST_LITERAL("Go to Reischu Apartment"),  ST_LITERAL("Net.LinkWithOriginalBook ReischuApartment, Default:LinkInPointDefault:") },
    { ST_LITERAL("Reischu Day 0%"),           ST_LITERAL("Net.ForceSetAgeTimeOfDay 0.0") },
    { ST_LITERAL("Reischu Day 25%"),           ST_LITERAL("Net.ForceSetAgeTimeOfDay 0.25") },
    { ST_LITERAL("Reischu Day 50%"),           ST_LITERAL("Net.ForceSetAgeTimeOfDay 0.50") },
    { ST_LITERAL("Reischu Day 75%"),           ST_LITERAL("Net.ForceSetAgeTimeOfDay 0.75") },
    { ST_LITERAL("Reischu Day 100%"),           ST_LITERAL("Net.ForceSetAgeTimeOfDay 0.99") },

    { ST_LITERAL("Go to Explorers Emporium"), ST_LITERAL("Net.LinkWithOriginalBook ExplorersEmporium, Default:LinkInPointDefault:") },

    { ST_LITERAL("Go to Venalem"),            ST_LITERAL("Net.LinkWithOriginalBook Venalem, Default:LinkInPointDefault:") },

    { ST_LITERAL("Go to Descent"),            ST_LITERAL("Net.LinkWithOriginalBook Descent, Default:LinkInPointDefault:") },
    { ST_LITERAL("Descent Elevator"),         ST_LITERAL("Avatar.Spawn.Go 18") },
    { ST_LITERAL("Descent Floor"),            ST_LITERAL("Avatar.Spawn.Go 4") },

    { ST_LITERAL("Go to Chiso"),              ST_LITERAL("Net.LinkWithOriginalBook ChisoPreniv, Default:LinkInPointDefault:") },
    { ST_LITERAL("Go downstairs Chiso"),      ST_LITERAL("Avatar.Spawn.Go 2") },

    //{ ST_LITERAL("Go to Sakura Vale"),        ST_LITERAL("Net.LinkWithOriginalBook SakuraVale, Default:LinkInPointDefault:") },

    { ST_LITERAL("H'uru Presentation"),       ST_LITERAL("Game.ShowDialog M24Huru") },
    { ST_LITERAL("'One More Thing...'"),      ST_LITERAL("Game.ShowDialog M24OMT") },
    { ST_LITERAL("Hide 'One More Thing...'"), ST_LITERAL("Game.HideDialog M24OMT") },
    { ST_LITERAL("Go to Relto"),              ST_LITERAL("Net.LinkToMyPersonalAge") }
};

static ST::string RenderWebPage() {
    const char endl = '\n';
    ST::string_stream ss;
    ss << ST_LITERAL(R"(<!doctype html>)") << endl;
    ss << ST_LITERAL(R"(<html lang="en">)") << endl;
    ss << ST_LITERAL(R"(  <head>)") << endl;
    ss << ST_LITERAL(R"(    <meta charset="utf-8">)") << endl;
    ss << ST_LITERAL(R"(    <meta name="theme-color" content="#3e364e">)") << endl;
    ss << ST_LITERAL(R"(    <meta name="viewport" content="width=device-width,initial-scale=1.0,viewport-fit=cover">)") << endl;
    ss << ST_LITERAL(R"(    <title>Plasma Console Commander</title>)") << endl;
    ss << ST_LITERAL(R"(    <style>
body {
  margin: 0;
  font-family: system-ui, sans-serif;
  background: black;
}

header {
  background: #3e364e;
  position: sticky;
  top: 0;
  padding: env(safe-area-inset-top, 0px) 0 0 0;
  color: white;
  text-align: center;
}

header h1 {
  margin: 0;
  line-height: 64px;
  vertical-align: middle;
  font-weight: 500;
  text-shadow: 0 -1px 1px black;
}

main {
  max-width: 820px;
  margin: 0 auto;
  padding: 1.5em;
}

@media screen and (min-width: 600px) {
  main {
    display: grid;
    grid-template-columns: 1fr 1fr;
    grid-gap: 0 1.5em;
  }
}

button {
  width: 100%;
  padding: 1em;
  font-weight: 500;
  font-family: system-ui;
  font-size: 1.25rem;
  background: #231f2b;
  color: palegreen;
  border-radius: 10px;
  margin-bottom: 1.5rem;
}
)");
    ss << ST_LITERAL(R"(    </style>)") << endl;
    ss << ST_LITERAL(R"(  </head>)") << endl;
    ss << ST_LITERAL(R"(  <body>)") << endl;

    ss << ST_LITERAL(R"(    <header>)") << endl;
    ss << ST_LITERAL(R"(      <h1>Console Commander</h1>)") << endl;
    ss << ST_LITERAL(R"(    </header>)") << endl;

    ss << ST_LITERAL(R"(    <main>)") << endl;

    /*
    ss << ST_LITERAL(R"(      <form method="POST" action="/console">)") << endl;
    ss << ST_LITERAL(R"(        <input name="cmd" type="text">)") << endl;
    ss << ST_LITERAL(R"(        <input type="submit">)") << endl;
    ss << ST_LITERAL(R"(      </form>)") << endl;
     */

    for (const CommandEntry& ce : commands) {
        ss << ST_LITERAL(R"(      <form method="POST" action="/console">)") << endl;
        ss << ST_LITERAL(R"(        <input type="hidden" name="cmd" value=")") << ce.cmd << ST_LITERAL(R"(">)") << endl;
        ss << ST_LITERAL(R"(        <button type="submit">)") << ce.label << ST_LITERAL(R"(</button>)") << endl;
        ss << ST_LITERAL(R"(      </form>)") << endl;
    }

    ss << ST_LITERAL(R"(    </main>)") << endl;
    ss << ST_LITERAL(R"(  </body>)") << endl;
    ss << ST_LITERAL(R"(</html>)") << endl;

    return ss.to_string();
}

void pmConsoleCommander::Init(plFactory* factory)
{
    fServer = new crow::SimpleApp();

    CROW_ROUTE((*fServer), "/console").methods("GET"_method)([](){
        ST::string page = RenderWebPage();
        return page.to_std_string();
    });


    CROW_ROUTE((*fServer), "/console").methods("POST"_method)([](const crow::request& req){
        auto params = req.get_body_params();
        auto cmd = params.get("cmd");

        hsStatusMessageF("Requesting to run console command: %s\n", cmd);
        plConsoleMsg* msg = FACTORY_NEW(plConsoleMsg);
        msg->SetCmd(plConsoleMsg::kExecuteLine);
        msg->SetString(cmd);
        msg->Send(nullptr, true);

        ST::string page = RenderWebPage();
        return page.to_std_string();
    });

    hsThread::Start();
}

void pmConsoleCommander::Run()
{
    hsThread::SetThisThreadName(ST_LITERAL("modConsoleCommander"));

    (*fServer).port(1812).run();
}

void pmConsoleCommander::OnQuit()
{
    (*fServer).stop();
    delete fServer;
    fServer = nullptr;
}
