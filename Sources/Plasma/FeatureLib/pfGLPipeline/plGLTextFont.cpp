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

#include "plGLTextFont.h"

#include "HeadSpin.h"

#include "plGLPipeline.h"

// Following number needs to be at least: 64 chars max in plTextFont drawn at any one time
//                                      * 4 primitives per char max (for bold text)
//                                      * 3 verts per primitive

//const uint32_t  kNumVertsInBuffer(32768);
const uint32_t    kNumVertsInBuffer(4608);

enum plGLTextFontShaderConstants : GLuint {
    kVtxPosition    = 0,
    kVtxColor       = 1,
    kVtxUVWSrc      = 2
};

plGLTextFont::plGLTextFont(plPipeline* pipe, plGLDevice* device)
    : plTextFont(pipe), fTexture(), fBuffer(), fState(), fShader(), fBufferCursor()
{
}

plGLTextFont::~plGLTextFont()
{
    DestroyObjects();
}

void plGLTextFont::ICreateTexture(uint16_t* data)
{
    if (fTexture)
        glDeleteTextures(1, &fTexture);

    glGenTextures(1, &fTexture);
    glBindTexture(GL_TEXTURE_2D, fTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fTextureWidth, fTextureHeight, 0, GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    if (plGLVersion() >= 43) {
        glObjectLabel(GL_TEXTURE, fTexture, -1, fFace.c_str());
    }
}

static const char* TEXTFONT_VERTEX_SHADER_STRING = R"(#version 120

attribute vec3 aVtxPosition;
attribute vec4 aVtxColor;
attribute vec3 aVtxUV;

uniform vec2 uPipeSize;

varying vec4 vVtxColor;
varying vec3 vVtxUV;

void main() {
    mat4 projMatrix = mat4(1.0);
    projMatrix[0][0] = 2.0 / uPipeSize.x;
    projMatrix[1][1] = -2.0 / uPipeSize.y;
    projMatrix[3][0] = -1.0;
    projMatrix[3][1] = 1.0;

    vVtxColor = aVtxColor.bgra;
    vVtxUV = aVtxUV;

    gl_Position = projMatrix * vec4(aVtxPosition, 1.0);
})";

static const char* TEXTFONT_FRAGMENT_SHADER_STRING = R"(#version 120

uniform sampler2D uTex;

varying vec4 vVtxColor;
varying vec3 vVtxUV;

void main() {
    gl_FragColor = texture2D(uTex, vec2(vVtxUV.x, vVtxUV.y)) * vVtxColor;
})";

void plGLTextFont::IInitStateBlocks()
{
    GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vshader, 1, &TEXTFONT_VERTEX_SHADER_STRING, nullptr);
    glCompileShader(vshader);
    LOG_GL_ERROR_CHECK("Vertex Shader compile failed");

    #ifdef HS_DEBUGGING
    {
        GLint compiled = 0;
        glGetShaderiv(vshader, GL_COMPILE_STATUS, &compiled);
        if (compiled == 0) {
            GLint length = 0;
            glGetShaderiv(vshader, GL_INFO_LOG_LENGTH, &length);
            if (length) {
                char* log = new char[length];
                glGetShaderInfoLog(vshader, length, &length, log);
                hsStatusMessage(log);
                delete[] log;
            }
        }
    }
    #endif

    GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader, 1, &TEXTFONT_FRAGMENT_SHADER_STRING, nullptr);
    glCompileShader(fshader);
    LOG_GL_ERROR_CHECK("Fragment Shader compile failed");

    #ifdef HS_DEBUGGING
    {
        GLint compiled = 0;
        glGetShaderiv(fshader, GL_COMPILE_STATUS, &compiled);
        if (compiled == 0) {
            GLint length = 0;
            glGetShaderiv(fshader, GL_INFO_LOG_LENGTH, &length);
            if (length) {
                char* log = new char[length];
                glGetShaderInfoLog(fshader, length, &length, log);
                hsStatusMessage(log);
                delete[] log;
            }
        }
    }
    #endif

    GLuint program = glCreateProgram();
    LOG_GL_ERROR_CHECK("Create Program failed");

    if (plGLVersion() >= 43) {
        const char* name = "TextFont";
        glObjectLabel(GL_PROGRAM, program, strlen(name), name);
    }

    glAttachShader(program, vshader);
    LOG_GL_ERROR_CHECK("Attach Vertex Shader failed");

    glAttachShader(program, fshader);
    LOG_GL_ERROR_CHECK("Attach Fragment Shader failed");

    glBindAttribLocation(program, 0, "aVtxPosition");
    LOG_GL_ERROR_CHECK("glBindAttribLocation 0 failed");
    glBindAttribLocation(program, 1, "aVtxColor");
    LOG_GL_ERROR_CHECK("glBindAttribLocation 1 failed");
    glBindAttribLocation(program, 2, "aVtxUV");
    LOG_GL_ERROR_CHECK("glBindAttribLocation 2 failed");

    glLinkProgram(program);
    LOG_GL_ERROR_CHECK("Program Link failed");

    GLint isLinked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE)
    {
        GLint maxLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

        // The maxLength includes the NULL character
        char* log = new char[maxLength];
        glGetProgramInfoLog(program, maxLength, &maxLength, log);

        LOG_GL_ERROR_CHECK(log);
        delete[] log;
    }

    fShader = program;

    if (plGLVersion() >= 30) {
        glGenVertexArrays(1, &fState);
        //glBindVertexArray(fState);
    }

    /*
    glGenBuffers(1, &fBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, fBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plFontVertex) * kNumVertsInBuffer, nullptr, GL_STATIC_DRAW);

    if (plGLVersion() >= 30) {
        glEnableVertexAttribArray(kVtxPosition);
        glVertexAttribPointer(kVtxPosition, 3, GL_FLOAT, GL_FALSE, sizeof(plFontVertex), (void*)(sizeof(float) * 0));

        glEnableVertexAttribArray(kVtxColor);
        glVertexAttribPointer(kVtxColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(plFontVertex), (void*)(sizeof(float) * 3));

        glEnableVertexAttribArray(kVtxUVWSrc);
        glVertexAttribPointer(kVtxUVWSrc, 3, GL_FLOAT, GL_FALSE, sizeof(plFontVertex), (void*)((sizeof(float) * 3) + sizeof(uint32_t)));

        glBindVertexArray(0);
    }
    */
}

void plGLTextFont::IDrawPrimitive(uint32_t count, plFontVertex* array)
{
    if (count == 0 || array == nullptr)
        return;

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plFontVertex) * 3 * count, array, GL_STATIC_DRAW);

    glEnableVertexAttribArray(kVtxPosition);
    glVertexAttribPointer(kVtxPosition, 3, GL_FLOAT, GL_FALSE, sizeof(plFontVertex), (void*)(sizeof(float) * 0));

    glEnableVertexAttribArray(kVtxColor);
    glVertexAttribPointer(kVtxColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(plFontVertex), (void*)(sizeof(float) * 3));

    glEnableVertexAttribArray(kVtxUVWSrc);
    glVertexAttribPointer(kVtxUVWSrc, 3, GL_FLOAT, GL_FALSE, sizeof(plFontVertex), (void*)(sizeof(float) * 3 + sizeof(uint32_t)));

    glDrawArrays(GL_TRIANGLES, 0, count * 3);

    LOG_GL_ERROR_CHECK("Render failed")

    glDeleteBuffers(1, &vbo);
}

void plGLTextFont::IDrawLines(uint32_t count, plFontVertex* array)
{
    if (count == 0 || array == nullptr)
        return;

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plFontVertex) * 3 * count, array, GL_STATIC_DRAW);

    glDrawArrays(GL_LINES, 0, count * 3);

    LOG_GL_ERROR_CHECK("Render failed")

    glDeleteBuffers(1, &vbo);
}

void plGLTextFont::FlushDraws()
{
}

void plGLTextFont::SaveStates()
{
    if (!fInitialized)
        IInitObjects();

    if (plGLVersion() >= 30 && fState)
        glBindVertexArray(fState);

    glActiveTexture(GL_TEXTURE0);
    LOG_GL_ERROR_CHECK("Active Texture failed")

    glBindTexture(GL_TEXTURE_2D, fTexture);
    LOG_GL_ERROR_CHECK("Bind Texture failed");

    glViewport(0, 0, fPipe->Width(), fPipe->Height());
    glDepthRange(0.0, 1.0);

    glUseProgram(fShader);
    LOG_GL_ERROR_CHECK("Use Program failed for plGLTextFont");

    glUniform1i(glGetUniformLocation(fShader, "uTex"), 0);
    glUniform2f(glGetUniformLocation(fShader, "uPipeSize"), float(fPipe->Width()), float(fPipe->Height()));
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_TRUE);
    glDisable(GL_CULL_FACE);
}

void plGLTextFont::RestoreStates()
{
    if (plGLVersion() >= 30)
        glBindVertexArray(0);

    glUseProgram(0);
}

void plGLTextFont::DestroyObjects()
{
    if (fTexture)
        glDeleteTextures(1, &fTexture);
    fTexture = 0;

    if (plGLVersion() >= 30 && fState) {
        if (plGLVersion() >= 45) {
            glDisableVertexArrayAttrib(fState, kVtxPosition);
            glDisableVertexArrayAttrib(fState, kVtxColor);
            glDisableVertexArrayAttrib(fState, kVtxUVWSrc);
        }

        glDeleteVertexArrays(1, &fState);
    }
    fState = 0;

    if (fBuffer)
        glDeleteBuffers(1, &fBuffer);
    fBuffer = 0;
}
