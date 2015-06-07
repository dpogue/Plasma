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

#include "plGLMaterialShaderRef.h"

#include "HeadSpin.h"
#include "hsBitVector.h"

#include "plDrawable/plGBufferGroup.h"
#include "plGImage/plMipmap.h"
#include "plSurface/hsGMaterial.h"
#include "plSurface/plLayerInterface.h"

#include "plGLDevice.h"

#include <GL/gl.h>
#include <GL/glext.h>


plGLMaterialShaderRef::~plGLMaterialShaderRef()
{
    Release();
}

void plGLMaterialShaderRef::Release()
{
    if (fVertShaderRef) {
        glDeleteShader(fVertShaderRef);
        fVertShaderRef = 0;
    }

    if (fFragShaderRef) {
        glDeleteShader(fFragShaderRef);
        fFragShaderRef = 0;
    }

    if (fRef) {
        glDeleteProgram(fRef);
        fRef = 0;
    }

    SetDirty(true);
}


void plGLMaterialShaderRef::SetupTextureRefs()
{
    int32_t numTextures = 0;

    for (size_t i = 0; i < fMaterial->GetNumLayers(); i++) {
        plLayerInterface* layer = fMaterial->GetLayer(i);
        if (!layer) {
            continue;
        }

        if (layer->GetUVWSrc() & (plLayerInterface::kUVWNormal | plLayerInterface::kUVWPosition | plLayerInterface::kUVWReflect)) {
            // Now we have a different problem...
            continue;
        }

        // Load the image
        plMipmap* img = plMipmap::ConvertNoRef(layer->GetTexture());
        if (!img) {
            continue;
        }

        GLenum e;
        plGLTextureRef* texRef = (plGLTextureRef*)img->GetDeviceRef();

        if (!texRef->fRef) {
            continue;
        }

#ifdef HS_DEBUGGING
        if ((e = glGetError()) != GL_NO_ERROR) {
            hsStatusMessage(plFormat("PRE-Active Texture failed {}", uint32_t(e)).c_str());
        }
#endif

        glActiveTexture(GL_TEXTURE0 + numTextures);

#ifdef HS_DEBUGGING
        if ((e = glGetError()) != GL_NO_ERROR) {
            hsStatusMessage(plFormat("Active Texture failed {}", uint32_t(e)).c_str());
        }
#endif

        glBindTexture(GL_TEXTURE_2D, texRef->fRef);

#ifdef HS_DEBUGGING
        if ((e = glGetError()) != GL_NO_ERROR) {
            hsStatusMessage(plFormat("Bind Texture failed {}", uint32_t(e)).c_str());
        }
#endif

        plString name = plFormat("uTexture{}", numTextures);

        GLint texture = glGetUniformLocation(fRef, name.c_str());
        glUniform1i(texture, numTextures);

#ifdef HS_DEBUGGING
        if ((e = glGetError()) != GL_NO_ERROR) {
            hsStatusMessage(plFormat("Uniform Texture failed {}", uint32_t(e)).c_str());
        }
#endif

        numTextures++;
    }
}


void plGLMaterialShaderRef::ICompile()
{
    hsBitVector uvLayers;
    fNumUVs = 0;
    int32_t numTextures = 0;

    for (size_t i = 0; i < fMaterial->GetNumLayers(); i++) {
        plLayerInterface* layer = fMaterial->GetLayer(i);
        if (!layer) {
            continue;
        }

        if (layer->GetUVWSrc() & (plLayerInterface::kUVWNormal | plLayerInterface::kUVWPosition | plLayerInterface::kUVWReflect)) {
            // Now we have a different problem...
            continue;
        }

        uint32_t uv = layer->GetUVWSrc() & plGBufferGroup::kUVCountMask;

        if (!uvLayers.IsBitSet(uv)) {
            fNumUVs++;
            uvLayers.SetBit(uv);
        }


        // Load the image
        plMipmap* img = plMipmap::ConvertNoRef(layer->GetTexture());
        if (!img) {
            continue;
        }

        numTextures++;

        plGLTextureRef* texRef = new plGLTextureRef();
        texRef->fOwner = img;
        img->SetDeviceRef(texRef);

        GLenum e;

        glGenTextures(1, &texRef->fRef);

#ifdef HS_DEBUGGING
        if ((e = glGetError()) != GL_NO_ERROR) {
            hsStatusMessage(plFormat("Gen Textures failed {}", uint32_t(e)).c_str());
        }
#endif

        glBindTexture(GL_TEXTURE_2D, texRef->fRef);

#ifdef HS_DEBUGGING
        if ((e = glGetError()) != GL_NO_ERROR) {
            hsStatusMessage(plFormat("Bind Texture failed {}", uint32_t(e)).c_str());
        }
#endif

        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        if (img->IsCompressed()) {
            GLuint dxCompression = 0;
            uint8_t compType = img->fDirectXInfo.fCompressionType;

            if (compType == plBitmap::DirectXInfo::kDXT1)
                dxCompression = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            else if (compType == plBitmap::DirectXInfo::kDXT5)
                dxCompression = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;

            for (uint8_t i = 0; i < img->GetNumLevels(); i++) {
                img->SetCurrLevel(i);

                if (img->GetCurrWidth() < 4 || img->GetCurrHeight() < 4) {
                    continue;
                }

                glCompressedTexImage2D(GL_TEXTURE_2D, i, dxCompression,
                                        img->GetCurrWidth(), img->GetCurrHeight(),
                                        0, img->GetCurrLevelSize(), img->GetCurrLevelPtr());

#ifdef HS_DEBUGGING
                if ((e = glGetError()) != GL_NO_ERROR) {
                    hsStatusMessage(plFormat("Texture Image failed {} at level {}", uint32_t(e), i).c_str());
                }
#endif
            }
        }
    }

    plStringStream vs_src;

    vs_src << "#version 100" << "\n";
    vs_src << "\n";
    vs_src << "uniform mat4 uMatrixL2W;" << "\n";
    vs_src << "uniform mat4 uMatrixW2C;" << "\n";
    vs_src << "uniform mat4 uMatrixProj;" << "\n";
    vs_src << "\n";
    vs_src << "attribute vec3 aVtxPosition;" << "\n";
    vs_src << "attribute vec3 aVtxNormal;" << "\n";
    vs_src << "attribute vec4 aVtxColor;" << "\n";
    for (int32_t i = 0; i < fNumUVs; i++) {
        vs_src << "attribute vec3 aVtxUVWSrc" << i << ";" << "\n";
    }
    vs_src << "\n";
    vs_src << "varying vec3 vVtxNormal;" << "\n";
    vs_src << "varying vec4 vVtxColor;" << "\n";
    for (int32_t i = 0; i < fNumUVs; i++) {
        vs_src << "varying vec3 vVtxUVWSrc" << i << ";" << "\n";
    }
    vs_src << "\n";
    vs_src << "void main() {" << "\n";
    vs_src << "    vVtxNormal = aVtxNormal;" << "\n";
    vs_src << "    vVtxColor = aVtxColor.zyxw;" << "\n";
    for (int32_t i = 0; i < fNumUVs; i++) {
        vs_src << "    vVtxUVWSrc" << i << " = aVtxUVWSrc" << i << ";" << "\n";
    }
    vs_src << "\n";
    vs_src << "    vec4 pos = uMatrixL2W * vec4(aVtxPosition, 1.0);" << "\n";
    vs_src << "         pos = uMatrixW2C * pos;" << "\n";
    vs_src << "         pos = uMatrixProj * pos;" << "\n";
    vs_src << "\n";
    vs_src << "    gl_Position = pos;" << "\n";
    vs_src << "}";



    plStringStream fs_src;
    fs_src << "#version 100" << "\n";
    fs_src << "\n";
    for (int32_t i = 0; i < numTextures; i++) {
        fs_src << "uniform sampler2D uTexture" << i << ";" << "\n";
    }
    fs_src << "\n";
    fs_src << "varying lowp vec3 vVtxNormal;" << "\n";
    fs_src << "varying mediump vec4 vVtxColor;" << "\n";
    for (int32_t i = 0; i < fNumUVs; i++) {
        fs_src << "varying mediump vec3 vVtxUVWSrc" << i << ";" << "\n";
    }
    fs_src << "\n";
    fs_src << "void main() {" << "\n";
    fs_src << "    mediump vec4 color = vVtxColor;" << "\n";
    /*if (numTextures > 0) {
        fs_src << "    color *= texture2D(uTexture0, vec2(vVtxUVWSrc0.x, vVtxUVWSrc0.y));" << "\n";
    }*/

    for (int32_t i = 0, tex = 0; i < fMaterial->GetNumLayers(); i++) {
        plLayerInterface* layer = fMaterial->GetLayer(i);
        if (!layer) {
            continue;
        }

        if (layer->GetUVWSrc() & (plLayerInterface::kUVWNormal | plLayerInterface::kUVWPosition | plLayerInterface::kUVWReflect)) {
            // Now we have a different problem...
            continue;
        }

        plMipmap* img = plMipmap::ConvertNoRef(layer->GetTexture());
        if (!img) {
            continue;
        }

        uint32_t uv = layer->GetUVWSrc() & plGBufferGroup::kUVCountMask;

        fs_src << "    color *= texture2D(uTexture" << tex << ", vec2(vVtxUVWSrc" << uv << ".x, vVtxUVWSrc" << uv << ".y));" << "\n";
        tex++;
    }

    fs_src << "    gl_FragColor = color;" << "\n";
    fs_src << "}";

    plString vtx = vs_src.GetString();
    plString frg = fs_src.GetString();

    const char* vs_code = vtx.c_str();
    const char* fs_code = frg.c_str();


    fVertShaderRef = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(fVertShaderRef, 1, &vs_code, nullptr);
    glCompileShader(fVertShaderRef);

#ifdef HS_DEBUGGING
    {
        GLint compiled = 0;
        glGetShaderiv(fVertShaderRef, GL_COMPILE_STATUS, &compiled);
        if (compiled == 0) {
            hsStatusMessage("Not compiled");
            GLint length = 0;
            glGetShaderiv(fVertShaderRef, GL_INFO_LOG_LENGTH, &length);
            if (length) {
                char* log = new char[length];
                glGetShaderInfoLog(fVertShaderRef, length, &length, log);
                hsStatusMessage(log);
            }
        }
    }
#endif

    fFragShaderRef = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fFragShaderRef, 1, &fs_code, nullptr);
    glCompileShader(fFragShaderRef);

#ifdef HS_DEBUGGING
    {
        GLint compiled = 0;
        glGetShaderiv(fFragShaderRef, GL_COMPILE_STATUS, &compiled);
        if (compiled == 0) {
            hsStatusMessage("Not compiled");
            GLint length = 0;
            glGetShaderiv(fFragShaderRef, GL_INFO_LOG_LENGTH, &length);
            if (length) {
                char* log = new char[length];
                glGetShaderInfoLog(fFragShaderRef, length, &length, log);
                hsStatusMessage(log);
            }
        }
    }
#endif

    fRef = glCreateProgram();

#ifdef HS_DEBUGGING
    GLenum e_cp;
    if ((e_cp = glGetError()) != GL_NO_ERROR) {
        hsStatusMessage(plFormat("Create Program failed {}", uint32_t(e_cp)).c_str());
    }
#endif

    glAttachShader(fRef, fVertShaderRef);

#ifdef HS_DEBUGGING
    GLenum e_vs;
    if ((e_vs = glGetError()) != GL_NO_ERROR) {
        hsStatusMessage(plFormat("Vertex Attach failed {}", uint32_t(e_vs)).c_str());
    }
#endif

    glAttachShader(fRef, fFragShaderRef);

#ifdef HS_DEBUGGING
    GLenum e_fs;
    if ((e_fs = glGetError()) != GL_NO_ERROR) {
        hsStatusMessage(plFormat("Fragment Attach failed {}", uint32_t(e_fs)).c_str());
    }
#endif

    glLinkProgram(fRef);

#ifdef HS_DEBUGGING
    GLenum e;
    if ((e = glGetError()) != GL_NO_ERROR) {
        hsStatusMessage(plFormat("Prg Link failed {}", uint32_t(e)).c_str());
    }
#endif
}
