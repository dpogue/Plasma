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
#include "plGLDevice.h"

#include <epoxy/gl.h>

#include "HeadSpin.h"
#include "hsBitVector.h"

#include "hsGMatState.inl"
#include "plPipeline.h"
#include "plPipeDebugFlags.h"

#include "plDrawable/plGBufferGroup.h"
#include "plGImage/plCubicEnvironmap.h"
#include "plGImage/plMipmap.h"
#include "plSurface/hsGMaterial.h"
#include "plSurface/plLayerInterface.h"

//#define USE_NEW_SHADERS 1

// From plGLDevice.cpp
extern GLfloat* hsMatrix2GL(const hsMatrix44& src, GLfloat* dst);

const char* VERTEX_SHADER_STRING = R"(#version 330
precision lowp int;

struct lightSource {
    vec4 position;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec3 direction;
    vec3 spotProps; // (falloff, theta, phi)
    float constAtten;
    float linAtten;
    float quadAtten;
    float scale;
};


// Input Attributes
layout(location =  0) in vec3 aVtxPosition;
layout(location =  1) in vec3 aVtxNormal;
layout(location =  2) in vec4 aVtxColor;
layout(location =  3) in vec3 aVtxUVWSrc[8];

uniform mat4 uMatrixL2W;
uniform mat4 uMatrixW2L;
uniform mat4 uMatrixW2C;
uniform mat4 uMatrixProj;

uniform vec4 uGlobalAmb;
uniform vec4 uAmbientCol;
uniform float uAmbientSrc;
uniform vec4 uDiffuseCol;
uniform float uDiffuseSrc;
uniform vec4 uEmissiveCol;
uniform float uEmissiveSrc;
uniform vec4 uSpecularCol;
uniform float uSpecularSrc;

uniform float uInvertVtxAlpha; // Effectively boolean, 0.0 or 1.0

uniform lightSource uLampSources[8];

// Varying outputs
out vec4 vCamPosition;
out vec4 vCamNormal;
out vec4 vVtxColor;
out vec3 vVtxUVWSrc[8];

void main() {
    vec4 MAmbient = mix(aVtxColor.bgra, uAmbientCol, uAmbientSrc);
    vec4 MDiffuse = mix(aVtxColor.bgra, uDiffuseCol, uDiffuseSrc);
    vec4 MEmissive = mix(aVtxColor.bgra, uEmissiveCol, uEmissiveSrc);
    vec4 MSpecular = mix(aVtxColor.bgra, uSpecularCol, uSpecularSrc);

    vec4 LAmbient = vec4(0.0, 0.0, 0.0, 0.0);
    vec4 LDiffuse = vec4(0.0, 0.0, 0.0, 0.0);

    vec3 Ndirection = normalize(mat3(uMatrixW2L) * aVtxNormal);

    for (int i = 0; i < 8; i++) {
        vVtxUVWSrc[i] = aVtxUVWSrc[i];

        float attenuation;
        vec3 direction;

        if (uLampSources[i].position.w == 0.0) {
            // Directional Light with no attenuation
            direction = normalize(uLampSources[i].direction);
            attenuation = 1.0;
        } else {
            // Omni Light in all directions
            vec3 v2l = uLampSources[i].position.xyz - vec3(uMatrixL2W * vec4(aVtxPosition, 1.0));
            float distance = length(v2l);
            direction = normalize(v2l);

            attenuation = 1.0 / (uLampSources[i].constAtten + uLampSources[i].linAtten * distance + uLampSources[i].quadAtten * pow(distance, 2.0));

            if (uLampSources[i].spotProps.x > 0.0) {
                // Spot Light with cone falloff
                float a = dot(direction, normalize(-uLampSources[i].direction));
                float theta = uLampSources[i].spotProps.y;
                float phi = uLampSources[i].spotProps.z;
                float i = pow((a - phi) / (theta - phi), uLampSources[i].spotProps.x);

                attenuation *= clamp(i, 0.0, 1.0);
            }
        }

        LAmbient.rgb = LAmbient.rgb + attenuation * (uLampSources[i].ambient.rgb * uLampSources[i].scale);
        LDiffuse.rgb = LDiffuse.rgb + MDiffuse.rgb * (uLampSources[i].diffuse.rgb * uLampSources[i].scale) * max(0.0, dot(Ndirection, direction)) * attenuation;
    }

    vec4 ambient = clamp(MAmbient * (uGlobalAmb + LAmbient), 0.0, 1.0);
    vec4 diffuse = clamp(LDiffuse, 0.0, 1.0);
    vec4 material = clamp(ambient + diffuse + MEmissive, 0.0, 1.0);

    vVtxColor = vec4(material.rgb, abs(uInvertVtxAlpha - MDiffuse.a));

    vCamPosition = uMatrixW2C * (uMatrixL2W * vec4(aVtxPosition, 1.0));
    vCamNormal = uMatrixW2C * (uMatrixL2W * vec4(aVtxNormal, 0.0));

    gl_Position = uMatrixProj * vCamPosition;
})";


const char* FRAGMENT_SHADER_STRING = R"(#version 330
precision mediump float;
precision lowp int;

uniform mat4 uLayerMat0;

uniform mat4 uLayerMat[8];
uniform int uLayerUVWSrc[8];
uniform int uStartingLayer;
uniform int uLayersAtOnce;

uniform sampler2D uTexture0;

uniform float uAlphaThreshold;
uniform int uFogExponential;
uniform highp vec2 uFogValues;
uniform vec3 uFogColor;

// Varying inputs
in highp vec4 vCamPosition;
in highp vec4 vCamNormal;
in vec4 vVtxColor;
in highp vec3 vVtxUVWSrc[8];

// Rendered outputs
out vec4 fragColor;

void main() {
    float baseAlpha;
    vec4 coords0;
    vec4 image0;
    float currAlpha;
    vec3 currColor;

    baseAlpha = vVtxColor.a;
    coords0 = uLayerMat0 * vec4(vVtxUVWSrc[0], 1.0);
    image0 = texture(uTexture0, coords0.xy);
    currColor = image0.rgb;
    currAlpha = image0.a * baseAlpha;
    currColor = vVtxColor.rgb * currColor;

    if (currAlpha < uAlphaThreshold) { discard; };

    highp float fogFactor = 1.0;
    if (uFogExponential > 0) {
        fogFactor = exp(-pow(uFogValues.y * length(vCamPosition.xyz), uFogValues.x));
    } else {
        if (uFogValues.y > 0.0) {
            highp float start = uFogValues.x;
            highp float end = uFogValues.y;
            fogFactor = (end - length(vCamPosition.xyz)) / (end - start);
        }
    }

    currColor = mix(currColor, uFogColor, 1.0 - clamp(fogFactor, 0.0, 1.0));

    fragColor = vec4(currColor, currAlpha);
})";

plGLMaterialShaderRef::plGLMaterialShaderRef(hsGMaterial* mat, plPipeline* pipe)
    : plGLDeviceRef(), fMaterial(mat), fPipeline(pipe), fVertShaderRef(0), fFragShaderRef(0)
{
    ISetupShaderContexts();
    ILoopOverLayers();
    ICompile();
    ISetShaderVariableLocs();
    ICleanupShaderContexts();
}

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
        if (!layer)
            continue;

        // Load the image
        plBitmap* img = plBitmap::ConvertNoRef(layer->GetTexture());

        if (!img)
            continue;

        plGLTextureRef* texRef = static_cast<plGLTextureRef*>(img->GetDeviceRef());

        if (!texRef->fRef)
            continue;

        fPipeline->CheckTextureRef(layer);

        LOG_GL_ERROR_CHECK("PRE-Active Texture failed")

        glActiveTexture(GL_TEXTURE0 + numTextures);
        LOG_GL_ERROR_CHECK("Active Texture failed")

        glBindTexture(texRef->fMapping, texRef->fRef);
        LOG_GL_ERROR_CHECK("Bind Texture failed");

        if (texRef->fMapping == GL_TEXTURE_2D) {
            // Ewww, but the same texture might be used by multiple layers with different clamping flags :(
            switch (layer->GetClampFlags()) {
            case hsGMatState::kClampTextureU:
                glTexParameteri(texRef->fMapping, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(texRef->fMapping, GL_TEXTURE_WRAP_T, GL_REPEAT);
                break;
            case hsGMatState::kClampTextureV:
                glTexParameteri(texRef->fMapping, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(texRef->fMapping, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                break;
            case hsGMatState::kClampTexture:
                glTexParameteri(texRef->fMapping, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(texRef->fMapping, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                break;
            default:
                glTexParameteri(texRef->fMapping, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(texRef->fMapping, GL_TEXTURE_WRAP_T, GL_REPEAT);
            }
        }

        if (this->uLayerMat[i] != -1) {
            GLfloat matrix[16];
            glUniformMatrix4fv(this->uLayerMat[i], 1, GL_TRUE, hsMatrix2GL(layer->GetTransform(), matrix));
        }

        if (this->uTexture[i] != -1)
            glUniform1i(this->uTexture[i], numTextures);
        LOG_GL_ERROR_CHECK("Uniform Texture failed")

        numTextures++;
    }
}


void plGLMaterialShaderRef::ICompile()
{
    LOG_GL_ERROR_CHECK("Begin Compile failed")

    int32_t numTextures = 0;
    hsBitVector usedUVWs;

    for (size_t i = 0; i < fMaterial->GetNumLayers(); i++) {
        plLayerInterface* layer = fMaterial->GetLayer(i);
        if (!layer)
            continue;

        fPipeline->CheckTextureRef(layer);
        LOG_GL_ERROR_CHECK(ST::format("Check Texture Ref on layer \"{}\" failed", layer->GetKeyName()));
    }

    const char* vs_code = VERTEX_SHADER_STRING;

#ifndef USE_NEW_SHADERS
    //ST::string vtx = fVertexShader->Render();
    ST::string frg = fFragmentShader->Render();

    //const char* vs_code = vtx.c_str();
    const char* fs_code = frg.c_str();
#else
    const char* fs_code = FRAGMENT_SHADER_STRING;
#endif

    fVertShaderRef = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(fVertShaderRef, 1, &vs_code, nullptr);
    glCompileShader(fVertShaderRef);
    LOG_GL_ERROR_CHECK("Vertex Shader compile failed");

#ifdef HS_DEBUGGING
    {
        GLint compiled = 0;
        glGetShaderiv(fVertShaderRef, GL_COMPILE_STATUS, &compiled);
        if (compiled == 0) {
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
    LOG_GL_ERROR_CHECK("Fragment Shader compile failed");

#ifdef HS_DEBUGGING
    {
        GLint compiled = 0;
        glGetShaderiv(fFragShaderRef, GL_COMPILE_STATUS, &compiled);
        if (compiled == 0) {
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
    LOG_GL_ERROR_CHECK("Create Program failed");

    if (epoxy_gl_version() >= 43) {
        const char* name = ST::format("hsGMaterial::{}", fMaterial->GetKeyName()).c_str();
        glObjectLabel(GL_PROGRAM, fRef, strlen(name), name);
    }

    glAttachShader(fRef, fVertShaderRef);
    LOG_GL_ERROR_CHECK("Attach Vertex Shader failed")

    glAttachShader(fRef, fFragShaderRef);
    LOG_GL_ERROR_CHECK("Attach Fragment Shader failed")
}


void plGLMaterialShaderRef::ISetupShaderContexts()
{
    fVertexShader   = std::make_shared<plShaderContext>(kVertex, kShaderVersion);
    fFragmentShader = std::make_shared<plShaderContext>(kFragment, kShaderVersion);

#ifndef USE_NEW_SHADERS
    // Helper function to invert colour
    auto invColor = std::make_shared<plShaderFunction>("invColor", "vec3");
    auto argColor = std::make_shared<plArgumentNode>("color", "vec3", 0);
    invColor->PushOp(RETURN(SUB(CONSTANT("1.0"), argColor)));

    // Helper function to invert alpha
    auto invAlpha = std::make_shared<plShaderFunction>("invAlpha", "float");
    auto argAlpha = std::make_shared<plArgumentNode>("alpha", "float", 0);
    invAlpha->PushOp(RETURN(SUB(CONSTANT("1.0"), argAlpha)));

    fVertexShader->PushFunction(invColor);
    fVertexShader->PushFunction(invAlpha);
    fFragmentShader->PushFunction(invColor);
    fFragmentShader->PushFunction(invAlpha);

    std::shared_ptr<plShaderStruct> lightSource = std::make_shared<plShaderStruct>("lightSource");
    lightSource->AddField(STRUCTVAR("vec4", "position"));
    lightSource->AddField(STRUCTVAR("vec4", "ambient"));
    lightSource->AddField(STRUCTVAR("vec4", "diffuse"));
    lightSource->AddField(STRUCTVAR("vec4", "specular"));
    lightSource->AddField(STRUCTVAR("vec3", "direction"));
    lightSource->AddField(STRUCTVAR("vec3", "spotProps"));
    lightSource->AddField(STRUCTVAR("float", "constAtten"));
    lightSource->AddField(STRUCTVAR("float", "linAtten"));
    lightSource->AddField(STRUCTVAR("float", "quadAtten"));
    lightSource->AddField(STRUCTVAR("float", "scale"));

    fVertexShader->PushStruct(lightSource);
#endif
}


void plGLMaterialShaderRef::ISetShaderVariableLocs()
{
#ifndef USE_NEW_SHADERS
    // Assign and bind the attribute locations for later
    glBindAttribLocation(fRef, kVtxPosition, "aVtxPosition");
    glBindAttribLocation(fRef, kVtxNormal, "aVtxNormal");
    glBindAttribLocation(fRef, kVtxColor, "aVtxColor");
    glBindAttribLocation(fRef, kVtxUVWSrc,  "aVtxUVWSrc");
#endif

    glLinkProgram(fRef);
    LOG_GL_ERROR_CHECK("Program Link failed");

    uPassNumber       = glGetUniformLocation(fRef, "uPassNumber");
    uAlphaThreshold   = glGetUniformLocation(fRef, "uAlphaThreshold");
    uInvertVtxAlpha   = glGetUniformLocation(fRef, "uInvertVtxAlpha");

    // Matrix inputs
    uMatrixProj     = glGetUniformLocation(fRef, "uMatrixProj");
    uMatrixC2W      = glGetUniformLocation(fRef, "uMatrixC2W");
    uMatrixW2C      = glGetUniformLocation(fRef, "uMatrixW2C");
    uMatrixL2W      = glGetUniformLocation(fRef, "uMatrixL2W");
    uMatrixW2L      = glGetUniformLocation(fRef, "uMatrixW2L");

    // Material inputs
    uGlobalAmbient  = glGetUniformLocation(fRef, "uGlobalAmb");
    uMatAmbientCol  = glGetUniformLocation(fRef, "uAmbientCol");
    uMatAmbientSrc  = glGetUniformLocation(fRef, "uAmbientSrc");
    uMatDiffuseCol  = glGetUniformLocation(fRef, "uDiffuseCol");
    uMatDiffuseSrc  = glGetUniformLocation(fRef, "uDiffuseSrc");
    uMatEmissiveCol = glGetUniformLocation(fRef, "uEmissiveCol");
    uMatEmissiveSrc = glGetUniformLocation(fRef, "uEmissiveSrc");
    uMatSpecularCol = glGetUniformLocation(fRef, "uSpecularCol");
    uMatSpecularSrc = glGetUniformLocation(fRef, "uSpecularSrc");

    // Fog inputs
    uFogExponential = glGetUniformLocation(fRef, "uFogExponential");
    uFogValues      = glGetUniformLocation(fRef, "uFogValues");
    uFogColor       = glGetUniformLocation(fRef, "uFogColor");

    size_t layerCount = fMaterial->GetNumLayers();

    uLayerMat.assign(layerCount, -1);
    for (size_t i = 0; i < layerCount; i++) {
        ST::string name = ST::format("uLayerMat{}", i);
        uLayerMat[i] = glGetUniformLocation(fRef, name.c_str());
    }

    uTexture.assign(layerCount, -1);
    for (size_t i = 0; i < layerCount; i++) {
        ST::string name = ST::format("uTexture{}", i);
        uTexture[i] = glGetUniformLocation(fRef, name.c_str());
    }
}


void plGLMaterialShaderRef::ICleanupShaderContexts()
{
    fVariables.clear();

    fVertexShader.reset();
    fFragmentShader.reset();
}


void plGLMaterialShaderRef::ILoopOverLayers()
{
    size_t j = 0;
    size_t pass = 0;

    // Build the vertex shader main function to assign to the varying vars
    std::shared_ptr<plShaderFunction> vertMain = std::make_shared<plShaderFunction>("main", "void");

    // Set the vertex transforms now
    std::shared_ptr<plTempVariableNode> pos = std::make_shared<plTempVariableNode>("pos", "vec4");
    std::shared_ptr<plAttributeNode> apos = IFindVariable<plAttributeNode>("aVtxPosition", "vec3");
    std::shared_ptr<plAttributeNode> anor = IFindVariable<plAttributeNode>("aVtxNormal", "vec3");
    std::shared_ptr<plUniformNode> mL2W = IFindVariable<plUniformNode>("uMatrixL2W", "mat4");
    std::shared_ptr<plUniformNode> mW2C = IFindVariable<plUniformNode>("uMatrixW2C", "mat4");
    std::shared_ptr<plUniformNode> mProj = IFindVariable<plUniformNode>("uMatrixProj", "mat4");

    std::shared_ptr<plVaryingNode> vCamPos = IFindVariable<plVaryingNode>("vCamPosition", "vec4");
    std::shared_ptr<plVaryingNode> vCamNor = IFindVariable<plVaryingNode>("vCamNormal", "vec4");

    vertMain->PushOp(ASSIGN(pos, MUL(mL2W, CALL("vec4", apos, CONSTANT("1.0")))));
    vertMain->PushOp(ASSIGN(pos, MUL(mW2C, pos)));

    vertMain->PushOp(ASSIGN(vCamPos, pos));
    vertMain->PushOp(ASSIGN(vCamNor, MUL(mW2C, MUL(mL2W, CALL("vec4", anor, CONSTANT("1.0"))))));

    vertMain->PushOp(ASSIGN(pos, MUL(mProj, pos)));
    vertMain->PushOp(ASSIGN(OUTPUT("gl_Position"), pos));

    vertMain->PushOp(ASSIGN(PROP(OUTPUT("gl_Position"), "z"),
                SUB(MUL(PROP(OUTPUT("gl_Position"), "z"), CONSTANT("2.0")), PROP(OUTPUT("gl_Position"), "w"))));

    // Build the fragment shader main function with the right passes
    std::shared_ptr<plShaderFunction> fragMain = std::make_shared<plShaderFunction>("main", "void");
    std::shared_ptr<plUniformNode> uPass = IFindVariable<plUniformNode>("uPassNumber", "int");

    for (j = 0; j < fMaterial->GetNumLayers(); )
    {
        size_t iCurrMat = j;
        std::shared_ptr<plShaderFunction> fragPass = std::make_shared<plShaderFunction>(ST::format("pass{}", pass), "vec4");
        std::shared_ptr<plShaderFunction> vertPass = std::make_shared<plShaderFunction>(ST::format("pass{}", pass), "void");

        j = IHandleMaterial(iCurrMat, vertPass, fragPass);

        if (j == -1)
            break;

        fVertexShader->PushFunction(vertPass);
        fFragmentShader->PushFunction(fragPass);

        std::shared_ptr<plConditionNode> passCond = COND(IS_EQ(uPass, CONSTANT(ST::format("{}", pass))));

        std::shared_ptr<plOutputVariableNode> output = IFindVariable<plOutputVariableNode>("fragColor", "vec4");
        passCond->PushOp(ASSIGN(output, CALL(fragPass->name)));

        // if (uPassNumber == curpass) { curpass(); }
        fragMain->PushOp(passCond);
        vertMain->PushOp(passCond);

        pass++;
        fPassIndices.push_back(iCurrMat);

#if 0
        ISetFogParameters(fMaterial->GetLayer(iCurrMat));
#endif
    }

    fFragmentShader->PushFunction(fragMain);

    for (const auto& pair : fVariables) {
        if (pair.second->klass == kVarying) {
            std::shared_ptr<plVaryingNode> vary = std::static_pointer_cast<plVaryingNode>(pair.second);

            if (vary->name == "vVtxColor" || vary->name == "vCamPosition" || vary->name == "vCamNormal") {
                continue;
            }

            ST::string name = ST::format("a{}", vary->name.substr(1));
            std::shared_ptr<plAttributeNode> attr = std::make_shared<plAttributeNode>(name, vary->type);

            vertMain->PushOp(ASSIGN(vary, attr));
        }
    }

    fVertexShader->PushFunction(vertMain);
}


const hsGMatState plGLMaterialShaderRef::ICompositeLayerState(plLayerInterface* layer)
{
    hsGMatState state;
    state.Composite(layer->GetState(), fPipeline->GetMaterialOverride(true), fPipeline->GetMaterialOverride(false));
    return state;
}


uint32_t plGLMaterialShaderRef::IHandleMaterial(uint32_t layer, std::shared_ptr<plShaderFunction> vfn, std::shared_ptr<plShaderFunction> ffn)
{
    if (!fMaterial || layer >= fMaterial->GetNumLayers() || !fMaterial->GetLayer(layer))
        return -1;

    if (false /*ISkipBumpMap(fMaterial, layer)*/)
        return -1;

    // Ignoring the bit about ATI Radeon and UVW limits

    if (fPipeline->IsDebugFlagSet(plPipeDbg::kFlagNoDecals) && (fMaterial->GetCompositeFlags() & hsGMaterial::kCompDecal))
        return -1;

    // Ignoring the bit about self-rendering cube maps

    plLayerInterface* currLay = /*IPushOverBaseLayer*/ fMaterial->GetLayer(layer);

    if (fPipeline->IsDebugFlagSet(plPipeDbg::kFlagBumpW) && (currLay->GetMiscFlags() & hsGMatState::kMiscBumpDu))
        currLay = fMaterial->GetLayer(++layer);

    //currLay = IPushOverAllLayer(currLay);

    hsGMatState state = ICompositeLayerState(currLay);

    // Stuff about ZInc

    if (fPipeline->IsDebugFlagSet(plPipeDbg::kFlagDisableSpecular))
        state.fShadeFlags &= ~hsGMatState::kShadeSpecular;

    if (fPipeline->IsDebugFlagSet(plPipeDbg::kFlagNoAlphaBlending))
        state.fBlendFlags &= ~hsGMatState::kBlendMask;

    if ((fPipeline->IsDebugFlagSet(plPipeDbg::kFlagBumpUV) || fPipeline->IsDebugFlagSet(plPipeDbg::kFlagBumpW)) && (state.fMiscFlags & hsGMatState::kMiscBumpChans) ) {
        switch (state.fMiscFlags & hsGMatState::kMiscBumpChans)
        {
            case hsGMatState::kMiscBumpDu:
                break;
            case hsGMatState::kMiscBumpDv:
                if (!(fMaterial->GetLayer(layer-2)->GetBlendFlags() & hsGMatState::kBlendAdd))
                {
                    state.fBlendFlags &= ~hsGMatState::kBlendMask;
                    state.fBlendFlags |= hsGMatState::kBlendMADD;
                }
                break;
            case hsGMatState::kMiscBumpDw:
                if (!(fMaterial->GetLayer(layer-1)->GetBlendFlags() & hsGMatState::kBlendAdd))
                {
                    state.fBlendFlags &= ~hsGMatState::kBlendMask;
                    state.fBlendFlags |= hsGMatState::kBlendMADD;
                }
                break;
            default:
                break;
        }
    }

    uint32_t currNumLayers = ILayersAtOnce(layer);

    if (state.fMiscFlags & (hsGMatState::kMiscBumpDu | hsGMatState::kMiscBumpDw)) {
        //ISetBumpMatrices(currLay);
    }

    std::shared_ptr<plVaryingNode> vVtxColor = IFindVariable<plVaryingNode>("vVtxColor", "vec4");

    std::shared_ptr<plShaderNode> lighting = ICalcLighting(vfn);
    std::shared_ptr<plShaderNode> baseAlpha = IBuildBaseAlpha(currLay, vfn);

    vfn->PushOp(ASSIGN(vVtxColor, CALL("vec4", PROP(lighting, "rgb"), baseAlpha)));

    std::shared_ptr<plTempVariableNode> fBaseAlpha = std::make_shared<plTempVariableNode>("baseAlpha", "float");
    ffn->PushOp(ASSIGN(fBaseAlpha, PROP(vVtxColor, "a")));

    ShaderBuilder sb;
    sb.fFunction = ffn;
    sb.fIteration = 0;
    sb.fPrevAlpha = fBaseAlpha;

#if 0
    if (state.fZFlags & hsGMatState::kZIncLayer) {
        // Set the Z-bias
        sb.fFunction->PushOp(ASSIGN(OUTPUT("gl_FragDepth"), ADD(CONSTANT("gl_FragCoord.z"), CONSTANT("-0.0001"))));
    } else {
        // Clear any Z-bias
        sb.fFunction->PushOp(ASSIGN(OUTPUT("gl_FragDepth"), CONSTANT("gl_FragCoord.z")));
    }
#endif

    for (int32_t i = 0; i < currNumLayers; i++) {
        sb.fIteration = i;
        sb.fCurrColor.reset();
        sb.fCurrAlpha.reset();
        sb.fCurrCoord.reset();
        sb.fCurrImage.reset();

        plLayerInterface* layPtr = fMaterial->GetLayer(layer + i);
        if (!layPtr)
            return -1;

        IBuildLayerTransform(layer + i, layPtr, &sb);
        IBuildLayerTexture(layer + i, layPtr, &sb);
        IBuildLayerBlend(layPtr, &sb);

        sb.fPrevColor = sb.fCurrColor;
        sb.fPrevAlpha = sb.fCurrAlpha;
    }

    //IHandleTextureMode(layer);
    //IHandleShadeMode(layer);
    //IHandleZMode();
    //IHandleMiscMode()
    //IHandleTextureStage(0, layer);

    // Handle High Alpha Threshold
    std::shared_ptr<plUniformNode> alphaThreshold = IFindVariable<plUniformNode>("uAlphaThreshold", "float");

    std::shared_ptr<plConditionNode> alphaTest = COND(IS_LESS(sb.fCurrAlpha, alphaThreshold));
    alphaTest->PushOp(CONSTANT("discard"));

    // if (final.a < alphaThreshold) { discard; }
    sb.fFunction->PushOp(alphaTest);

    // Multiply in the vertex color at the end (but alpha is premultiplied!)
    std::shared_ptr<plShaderNode> finalColor;

    if (sb.fCurrColor) {
        finalColor = MUL(PROP(vVtxColor, "rgb"), sb.fCurrColor, true);
    } else {
        finalColor = PROP(vVtxColor, "rgb");
    }

    sb.fFunction->PushOp(RETURN(CALL("vec4", finalColor, sb.fCurrAlpha)));

    return layer + currNumLayers;
}


uint32_t plGLMaterialShaderRef::ILayersAtOnce(uint32_t which)
{
    uint32_t currNumLayers = 1;

    plLayerInterface* lay = fMaterial->GetLayer(which);

    if (fPipeline->IsDebugFlagSet(plPipeDbg::kFlagNoMultitexture))
        return currNumLayers;

    if ((fPipeline->IsDebugFlagSet(plPipeDbg::kFlagBumpUV) || fPipeline->IsDebugFlagSet(plPipeDbg::kFlagBumpW)) && (lay->GetMiscFlags() & hsGMatState::kMiscBumpChans)) {
        currNumLayers = 2;
        return currNumLayers;
    }

    if ((lay->GetBlendFlags() & hsGMatState::kBlendNoColor) || (lay->GetMiscFlags() & hsGMatState::kMiscTroubledLoner))
        return currNumLayers;

    int i;
    int maxLayers = 8;
    if (which + maxLayers > fMaterial->GetNumLayers())
        maxLayers = fMaterial->GetNumLayers() - which;

    for (i = currNumLayers; i < maxLayers; i++) {
        plLayerInterface* lay = fMaterial->GetLayer(which + i);

        // Ignoring max UVW limit

        if ((lay->GetMiscFlags() & hsGMatState::kMiscBindNext) && (i+1 >= maxLayers))
            break;

        if (lay->GetMiscFlags() & hsGMatState::kMiscRestartPassHere)
            break;

        if (!(fMaterial->GetLayer(which + i - 1)->GetMiscFlags() & hsGMatState::kMiscBindNext) && !ICanEatLayer(lay))
            break;

        currNumLayers++;
    }

    return currNumLayers;
}


bool plGLMaterialShaderRef::ICanEatLayer(plLayerInterface* lay)
{
    if (!lay->GetTexture())
        return false;

    if ((lay->GetBlendFlags() & hsGMatState::kBlendNoColor) ||
        (lay->GetBlendFlags() & hsGMatState::kBlendAddColorTimesAlpha) ||
        (lay->GetMiscFlags() & hsGMatState::kMiscTroubledLoner))
        return false;

    if ((lay->GetBlendFlags() & hsGMatState::kBlendAlpha) && (lay->GetAmbientColor().a < 1.f))
        return false;

    if (!(lay->GetZFlags() & hsGMatState::kZNoZWrite))
        return false;

    return true;
}

std::shared_ptr<plTempVariableNode> plGLMaterialShaderRef::ICalcLighting(std::shared_ptr<plShaderFunction> fn)
{
    std::shared_ptr<plAttributeNode> acol = IFindVariable<plAttributeNode>("aVtxColor", "vec4");
    std::shared_ptr<plAttributeNode> apos = IFindVariable<plAttributeNode>("aVtxPosition", "vec3");
    std::shared_ptr<plAttributeNode> anor = IFindVariable<plAttributeNode>("aVtxNormal", "vec3");

    std::shared_ptr<plUniformNode> mL2W = IFindVariable<plUniformNode>("uMatrixL2W", "mat4");
    std::shared_ptr<plUniformNode> mW2L = IFindVariable<plUniformNode>("uMatrixW2L", "mat4");

    std::shared_ptr<plUniformNode> uGlobalAmb   = IFindVariable<plUniformNode>("uGlobalAmb", "vec4");
    std::shared_ptr<plUniformNode> uAmbientCol  = IFindVariable<plUniformNode>("uAmbientCol", "vec4");
    std::shared_ptr<plUniformNode> uAmbientSrc  = IFindVariable<plUniformNode>("uAmbientSrc", "float");

    std::shared_ptr<plUniformNode> uDiffuseCol  = IFindVariable<plUniformNode>("uDiffuseCol", "vec4");
    std::shared_ptr<plUniformNode> uDiffuseSrc  = IFindVariable<plUniformNode>("uDiffuseSrc", "float");

    std::shared_ptr<plUniformNode> uEmissiveCol = IFindVariable<plUniformNode>("uEmissiveCol", "vec4");
    std::shared_ptr<plUniformNode> uEmissiveSrc = IFindVariable<plUniformNode>("uEmissiveSrc", "float");

    std::shared_ptr<plUniformNode> uSpecularCol = IFindVariable<plUniformNode>("uSpecularCol", "vec4");
    std::shared_ptr<plUniformNode> uSpecularSrc = IFindVariable<plUniformNode>("uSpecularSrc", "float");

    std::shared_ptr<plUniformNode> uLampSources = IFindVariable<plUniformNode>("uLampSources", "lightSource", 8);

    // Local vars for the 4 material values
    // Material values
    std::shared_ptr<plTempVariableNode> MAmbient  = std::make_shared<plTempVariableNode>("MAmbient", "vec4");
    std::shared_ptr<plTempVariableNode> MDiffuse  = std::make_shared<plTempVariableNode>("MDiffuse", "vec4");
    std::shared_ptr<plTempVariableNode> MEmissive = std::make_shared<plTempVariableNode>("MEmissive", "vec4");
    std::shared_ptr<plTempVariableNode> MSpecular = std::make_shared<plTempVariableNode>("MSpecular", "vec4");

    // Aggregated Lamp values
    std::shared_ptr<plTempVariableNode> LAmbient  = std::make_shared<plTempVariableNode>("LAmbient", "vec4");
    std::shared_ptr<plTempVariableNode> LDiffuse  = std::make_shared<plTempVariableNode>("LDiffuse", "vec4");

    // Final output values
    std::shared_ptr<plTempVariableNode> ambient  = std::make_shared<plTempVariableNode>("ambient", "vec4");
    std::shared_ptr<plTempVariableNode> diffuse  = std::make_shared<plTempVariableNode>("diffuse", "vec4");
    std::shared_ptr<plTempVariableNode> specular = std::make_shared<plTempVariableNode>("specular", "vec4");

    fn->PushOp(ASSIGN(MAmbient,  CALL("mix", PROP(acol, "zyxw"), uAmbientCol, uAmbientSrc)));
    fn->PushOp(ASSIGN(MDiffuse,  CALL("mix", PROP(acol, "zyxw"), uDiffuseCol, uDiffuseSrc)));
    fn->PushOp(ASSIGN(MEmissive, CALL("mix", PROP(acol, "zyxw"), uEmissiveCol, uEmissiveSrc)));
    fn->PushOp(ASSIGN(MSpecular, CALL("mix", PROP(acol, "zyxw"), uSpecularCol, uSpecularSrc)));

    std::shared_ptr<plTempVariableNode> attenuation = std::make_shared<plTempVariableNode>("attenuation", "float");
    std::shared_ptr<plTempVariableNode> v2l = std::make_shared<plTempVariableNode>("v2l", "vec3");
    std::shared_ptr<plTempVariableNode> distance = std::make_shared<plTempVariableNode>("distance", "float");
    std::shared_ptr<plTempVariableNode> direction = std::make_shared<plTempVariableNode>("direction", "vec3");
    std::shared_ptr<plTempVariableNode> Ndirection = std::make_shared<plTempVariableNode>("Ndirection", "vec3");

    fn->PushOp(ASSIGN(LAmbient, CONSTANT("vec4(0.0, 0.0, 0.0, 0.0)")));
    fn->PushOp(ASSIGN(LDiffuse, CONSTANT("vec4(0.0, 0.0, 0.0, 0.0)")));

    //fn->PushOp(ASSIGN(Ndirection, CALL("normalize", CALL("vec3", MUL(mW2L, CALL("vec4", anor, CONSTANT("1.0")))))));
    fn->PushOp(ASSIGN(Ndirection, CALL("normalize", MUL(CALL("mat3", mW2L), anor))));

    for (size_t i = 0; i < 8; i++) {
        auto lamp = SUBVAL(uLampSources, ST::format("{}", i));

        fn->PushOp(ASSIGN(v2l, SUB(CALL("vec3", PROP(lamp, "position")), CALL("vec3", MUL(mL2W, CALL("vec4", apos, CONSTANT("1.0")))))));
        fn->PushOp(ASSIGN(distance, CALL("length", v2l)));
        fn->PushOp(ASSIGN(direction, CALL("normalize", v2l)));

        fn->PushOp(ASSIGN(attenuation, CALL("mix",
                        // attenuation = 1.0
                        CONSTANT("1.0"),
                        // attenuation = 1.0 / (constA + linA * dist + quadA * dist * dist)
                        DIV(CONSTANT("1.0"), ADD(PROP(lamp, "constAtten"), ADD(MUL(PROP(lamp, "linAtten"), distance), MUL(PROP(lamp, "quadAtten"), MUL(distance, distance))),  true)),
                        PROP(PROP(lamp, "position"), "w"))));

        // LAmbient = LAmbient + (Atten * Spot * La)
        fn->PushOp(ASSIGN(LAmbient, ADD(LAmbient, MUL(attenuation, MUL(PROP(lamp, "ambient"), PROP(lamp, "scale"), true)))));

        // LDiffuse = LDiffuse + (Cd * Ld * (N . Ldir) * Atten * Spot)
        fn->PushOp(ASSIGN(LDiffuse, ADD(LDiffuse, MUL(MDiffuse, MUL(MUL(PROP(lamp, "diffuse"), PROP(lamp, "scale"), true), MUL(CALL("max", CONSTANT("0.0"), CALL("dot", Ndirection, direction)), attenuation))))));
    }

    // ambient = ambient * (global amb + ambientL) [clamped 0.0 - 1.0]
    fn->PushOp(ASSIGN(ambient, CALL("clamp", MUL(MAmbient, ADD(uGlobalAmb, LAmbient, true)), CONSTANT("0.0"), CONSTANT("1.0"))));
    fn->PushOp(ASSIGN(diffuse, CALL("clamp", LDiffuse, CONSTANT("0.0"), CONSTANT("1.0"))));

    std::shared_ptr<plTempVariableNode> matValues = std::make_shared<plTempVariableNode>("material", "vec4");
    fn->PushOp(ASSIGN(matValues, CALL("clamp", ADD(ambient, ADD(diffuse, MEmissive)), CONSTANT("0.0"), CONSTANT("1.0"))));
    return matValues;
}


std::shared_ptr<plTempVariableNode> plGLMaterialShaderRef::IBuildBaseAlpha(plLayerInterface* layer, std::shared_ptr<plShaderFunction> fn)
{
    hsGMatState state = ICompositeLayerState(layer);

    // This will have been declared by ICalcLighting
    std::shared_ptr<plShaderNode> diffuse = CONSTANT("MDiffuse");

    // Local variable to store the starting alpha value
    std::shared_ptr<plTempVariableNode> base = std::make_shared<plTempVariableNode>("baseAlpha", "float");

    if (state.fBlendFlags & hsGMatState::kBlendInvertVtxAlpha) {
        // base = 1.0 - vVtxColor.a
        fn->PushOp(ASSIGN(base, CALL("invAlpha", PROP(diffuse, "a"))));
    } else {
        // base = vVtxColor.a
        fn->PushOp(ASSIGN(base, PROP(diffuse, "a")));
    }

    return base;
}


void plGLMaterialShaderRef::IBuildLayerTransform(uint32_t idx, plLayerInterface* layer, ShaderBuilder* sb)
{
    std::shared_ptr<plVariableNode> matrix;
    hsGMatState state = ICompositeLayerState(layer);

    if (state.fMiscFlags & (hsGMatState::kMiscUseReflectionXform | hsGMatState::kMiscUseRefractionXform)) {
        std::shared_ptr<plUniformNode> mC2W = IFindVariable<plUniformNode>("uMatrixC2W", "mat4");

        ST::string matName = ST::format("LayerMat{}", idx);
        matrix = std::make_shared<plTempVariableNode>(matName, "mat4");

        ST::string tempName = ST::format("t{}", idx);
        std::shared_ptr<plTempVariableNode> temp = std::make_shared<plTempVariableNode>(tempName, "float");

        sb->fFunction->PushOp(ASSIGN(matrix, mC2W));

        // mat[0][3] = mat[1][3] = mat[2][3] = 0
        sb->fFunction->PushOp(ASSIGN(SUBVAL(SUBVAL(matrix, "0"), "3"),
                              ASSIGN(SUBVAL(SUBVAL(matrix, "1"), "3"),
                              ASSIGN(SUBVAL(SUBVAL(matrix, "2"), "3"),
                              CONSTANT("0.0")))));

        // This is just a rotation about X of Pi/2 (y = z, z = -y),
        // followed by flipping Z to reflect back towards us (z = -z).

        // swap mat[1][0] and mat[2][0]
        sb->fFunction->PushOp(ASSIGN(temp, SUBVAL(SUBVAL(matrix, "1"), "0")));
        sb->fFunction->PushOp(ASSIGN(SUBVAL(SUBVAL(matrix, "1"), "0"), SUBVAL(SUBVAL(matrix, "2"), "0")));
        sb->fFunction->PushOp(ASSIGN(SUBVAL(SUBVAL(matrix, "2"), "0"), temp));

        // swap mat[1][1] and mat[2][1]
        sb->fFunction->PushOp(ASSIGN(temp, SUBVAL(SUBVAL(matrix, "1"), "1")));
        sb->fFunction->PushOp(ASSIGN(SUBVAL(SUBVAL(matrix, "1"), "1"), SUBVAL(SUBVAL(matrix, "2"), "1")));
        sb->fFunction->PushOp(ASSIGN(SUBVAL(SUBVAL(matrix, "2"), "1"), temp));

        // swap mat[1][2] and mat[2][2]
        sb->fFunction->PushOp(ASSIGN(temp, SUBVAL(SUBVAL(matrix, "1"), "2")));
        sb->fFunction->PushOp(ASSIGN(SUBVAL(SUBVAL(matrix, "1"), "2"), SUBVAL(SUBVAL(matrix, "2"), "2")));
        sb->fFunction->PushOp(ASSIGN(SUBVAL(SUBVAL(matrix, "2"), "2"), temp));

        if (state.fMiscFlags & hsGMatState::kMiscUseRefractionXform) {
            // Same as reflection, but then matrix = matrix * scaleMatNegateZ.

            // mat[0][2] = -mat[0][2];
            sb->fFunction->PushOp(ASSIGN(SUBVAL(SUBVAL(matrix, "0"), "2"), SUB(CONSTANT("0.0"), SUBVAL(SUBVAL(matrix, "0"), "2"))));

            // mat[1][2] = -mat[1][2];
            sb->fFunction->PushOp(ASSIGN(SUBVAL(SUBVAL(matrix, "1"), "2"), SUB(CONSTANT("0.0"), SUBVAL(SUBVAL(matrix, "1"), "2"))));

            // mat[2][2] = -mat[2][2];
            sb->fFunction->PushOp(ASSIGN(SUBVAL(SUBVAL(matrix, "2"), "2"), SUB(CONSTANT("0.0"), SUBVAL(SUBVAL(matrix, "2"), "2"))));
        }

#if 0
    } else if (state.fMiscFlags & hsGMatState::kMiscCam2Screen) {
    } else if (state.fMiscFlags & hsGMatState::kMiscProjection) {
        ST::string matName = ST::format("uLayerMat{}", idx);
        std::shared_ptr<plUniformNode> layMat = IFindVariable<plUniformNode>(matName, "mat4");
    } else if (state.fMiscFlags & hsGMatState::kMiscBumpChans) {
#endif
    } else {
        ST::string matName = ST::format("uLayerMat{}", idx);
        matrix = IFindVariable<plUniformNode>(matName, "mat4");
    }

    uint32_t uvwSrc = layer->GetUVWSrc();

    // Local variable to store the mesh uvw * layer matrix
    ST::string coordName = ST::format("coords{}", idx);
    std::shared_ptr<plTempVariableNode> coords = std::make_shared<plTempVariableNode>(coordName, "vec4");

    switch (uvwSrc) {
    case plLayerInterface::kUVWNormal:
        {
            std::shared_ptr<plVaryingNode> vCamNor = IFindVariable<plVaryingNode>("vCamNormal", "vec4");
            sb->fFunction->PushOp(ASSIGN(coords, MUL(matrix, vCamNor)));
        }
        break;
    case plLayerInterface::kUVWPosition:
        {
            std::shared_ptr<plVaryingNode> vCamPos = IFindVariable<plVaryingNode>("vCamPosition", "vec4");
            sb->fFunction->PushOp(ASSIGN(coords, MUL(matrix, vCamPos)));
        }
        break;
    case plLayerInterface::kUVWReflect:
        {
            std::shared_ptr<plVaryingNode> vCamPos = IFindVariable<plVaryingNode>("vCamPosition", "vec4");
            std::shared_ptr<plVaryingNode> vCamNor = IFindVariable<plVaryingNode>("vCamNormal", "vec4");
            std::shared_ptr<plUniformNode> mC2W = IFindVariable<plUniformNode>("uMatrixC2W", "mat4");

            sb->fFunction->PushOp(ASSIGN(coords, MUL(matrix, MUL(mC2W, CALL("reflect", CALL("normalize", vCamPos), CALL("normalize", vCamNor))))));
        }
        break;
    default:
        {
            uvwSrc &= plGBufferGroup::kUVCountMask;

            std::shared_ptr<plVaryingNode> layUVW = IFindVariable<plVaryingNode>("vVtxUVWSrc", "highp vec3", 8);
            sb->fFunction->PushOp(ASSIGN(coords, MUL(matrix, CALL("vec4", SUBVAL(layUVW, ST::format("{}", uvwSrc)), CONSTANT("1.0")))));
        }
        break;
    }

    sb->fCurrCoord = coords;
}


void plGLMaterialShaderRef::IBuildLayerTexture(uint32_t idx, plLayerInterface* layer, ShaderBuilder* sb)
{
    plBitmap* texture = layer->GetTexture();

    if (texture != nullptr && sb->fCurrCoord) {
        plMipmap* mip;
        plCubicEnvironmap* cube;

        // Local variable to store the mesh uvw * layer matrix
        ST::string imgName = ST::format("image{}", idx);
        std::shared_ptr<plTempVariableNode> img = std::make_shared<plTempVariableNode>(imgName, "vec4");

        sb->fCurrImage = img;

        if ((mip = plMipmap::ConvertNoRef(texture)) != nullptr) {
            ST::string samplerName = ST::format("uTexture{}", idx);
            std::shared_ptr<plUniformNode> sampler = IFindVariable<plUniformNode>(samplerName, "sampler2D");

            // image = texture(sampler, coords.xy)
            sb->fFunction->PushOp(ASSIGN(img, CALL("texture", sampler, PROP(sb->fCurrCoord, "xy"))));
        }

        if ((cube = plCubicEnvironmap::ConvertNoRef(texture)) != nullptr) {
            ST::string samplerName = ST::format("uTexture{}", idx);
            std::shared_ptr<plUniformNode> sampler = IFindVariable<plUniformNode>(samplerName, "samplerCube");

            // image = texture(sampler, coords.xyz)
            sb->fFunction->PushOp(ASSIGN(img, CALL("texture", sampler, PROP(sb->fCurrCoord, "xyz"))));
        }
    }
}


void plGLMaterialShaderRef::IBuildLayerBlend(plLayerInterface* layer, ShaderBuilder* sb)
{
    hsGMatState state = ICompositeLayerState(layer);

    if (!sb->fCurrImage) {
        hsStatusMessage("Got a layer with no image");
        sb->fCurrColor = sb->fPrevColor;
        sb->fCurrAlpha = sb->fPrevAlpha;
        return;
    }

    // Local variable to store the color value
    ST::string colName = ST::format("color{}", sb->fIteration);
    std::shared_ptr<plTempVariableNode> col = std::make_shared<plTempVariableNode>(colName, "vec3");

    // Local variable to store the alpha value
    ST::string alphaName = ST::format("alpha{}", sb->fIteration);
    std::shared_ptr<plTempVariableNode> alpha = std::make_shared<plTempVariableNode>(alphaName, "float");

    std::shared_ptr<plShaderNode> texCol;
    if (state.fBlendFlags & hsGMatState::kBlendInvertColor) {
        // color = 1.0 - texture.rgb
        texCol = CALL("invColor", PROP(sb->fCurrImage, "rgb"));
    } else {
        // color = texture.rgb
        texCol = PROP(sb->fCurrImage, "rgb");
    }


    if (sb->fIteration == 0) {
        // Leave fCurrColor null if we are blending without texture color
        if (state.fBlendFlags & hsGMatState::kBlendNoTexColor) {
            sb->fCurrColor = sb->fPrevColor;
        } else {
            sb->fFunction->PushOp(ASSIGN(col, texCol));
            sb->fCurrColor = col;
        }

        std::shared_ptr<plShaderNode> alphaVal;
        if (state.fBlendFlags & hsGMatState::kBlendInvertAlpha) {
            // 1.0 - texture.a
            alphaVal = CALL("invAlpha", PROP(sb->fCurrImage, "a"));
        } else {
            // texture.a
            alphaVal = PROP(sb->fCurrImage, "a");
        }


        if (state.fBlendFlags & hsGMatState::kBlendNoVtxAlpha || !sb->fPrevAlpha) {
            // Only use texture alpha
            sb->fFunction->PushOp(ASSIGN(alpha, alphaVal));

            sb->fCurrAlpha = alpha;
        } else if (state.fBlendFlags & hsGMatState::kBlendNoTexAlpha) {
            // Only use vertex alpha (prev alpha)
            sb->fCurrAlpha = sb->fPrevAlpha;
        } else {
            // Vertex alpha * base texture alpha
            sb->fFunction->PushOp(ASSIGN(alpha, MUL(alphaVal, sb->fPrevAlpha)));
            sb->fCurrAlpha = alpha;
        }
    } else {
        switch (state.fBlendFlags & hsGMatState::kBlendMask)
        {

            case hsGMatState::kBlendAddColorTimesAlpha:
                hsAssert(false, "Blend mode unsupported on upper layers");
                break;

            case hsGMatState::kBlendAlpha:
            {
                if (state.fBlendFlags & hsGMatState::kBlendNoTexColor) {
                    // color = prev
                    sb->fCurrColor = sb->fPrevColor;
                } else {
                    if (state.fBlendFlags & hsGMatState::kBlendInvertAlpha) {
                        // color = texture.rgb + (texture.a * prev)
                        sb->fFunction->PushOp(ASSIGN(col, ADD(texCol, MUL(PROP(sb->fCurrImage, "a"), sb->fPrevColor, true))));
                        sb->fCurrColor = col;
                    } else {
                        // color = mix(prev, texture.rgb, texture.a)
                        sb->fFunction->PushOp(ASSIGN(col, CALL("mix", sb->fPrevColor, texCol, PROP(sb->fCurrImage, "a"))));
                        sb->fCurrColor = col;
                    }
                }


                std::shared_ptr<plShaderNode> alphaVal;
                if (state.fBlendFlags & hsGMatState::kBlendInvertAlpha) {
                    // 1.0 - texture.a
                    alphaVal = CALL("invAlpha", PROP(sb->fCurrImage, "a"));
                } else {
                    // texture.a
                    alphaVal = PROP(sb->fCurrImage, "a");
                }

                if (state.fBlendFlags & hsGMatState::kBlendAlphaAdd) {
                    // alpha = alphaVal + prev
                    sb->fFunction->PushOp(ASSIGN(alpha, ADD(alphaVal, sb->fPrevAlpha)));
                    sb->fCurrAlpha = alpha;
                } else if (state.fBlendFlags & hsGMatState::kBlendAlphaMult) {
                    // alpha = alphaVal * prev
                    sb->fFunction->PushOp(ASSIGN(alpha, MUL(alphaVal, sb->fPrevAlpha)));
                    sb->fCurrAlpha = alpha;
                } else {
                    // alpha = prev
                    sb->fCurrAlpha = sb->fPrevAlpha;
                }
                break;
            }

            case hsGMatState::kBlendAdd:
            {
                // color = texture.rgb + prev
                sb->fFunction->PushOp(ASSIGN(col, ADD(texCol, sb->fPrevColor)));
                sb->fCurrColor = col;

                // alpha = prev
                sb->fCurrAlpha = sb->fPrevAlpha;
                break;
            }

            case hsGMatState::kBlendMult:
            {
                // color = color * prev
                sb->fFunction->PushOp(ASSIGN(col, MUL(texCol, sb->fPrevColor)));
                sb->fCurrColor = col;

                // alpha = prev
                sb->fCurrAlpha = sb->fPrevAlpha;
                break;
            }

            case hsGMatState::kBlendDot3:
            {
                // color = (color.r * prev.r + color.g * prev.g + color.b * prev.b)
                sb->fFunction->PushOp(ASSIGN(PROP(col, "r"), ASSIGN(PROP(col, "g"), ASSIGN(PROP(col, "b"), CALL("dot", PROP(col, "rgb"), PROP(sb->fPrevColor, "rgb"))))));
                sb->fCurrColor = col;

                // alpha = prev
                sb->fCurrAlpha = sb->fPrevAlpha;
                break;
            }

            case hsGMatState::kBlendAddSigned:
            {
                // color = color + prev - 0.5
                sb->fFunction->PushOp(ASSIGN(col, SUB(ADD(texCol, sb->fPrevColor), CONSTANT("0.5"))));
                sb->fCurrColor = col;

                // alpha = prev
                sb->fCurrAlpha = sb->fPrevAlpha;
                break;
            }

            case hsGMatState::kBlendAddSigned2X:
            {
                // color = (color + prev - 0.5) << 1
                // Note: using CALL here for multiplication to ensure parentheses
                sb->fFunction->PushOp(ASSIGN(col, CALL("2 *", SUB(ADD(texCol, sb->fPrevColor), CONSTANT("0.5")))));
                sb->fCurrColor = col;

                // alpha = prev
                sb->fCurrAlpha = sb->fPrevAlpha;
                break;
            }

            case 0:
            {
                // color = texture.rgb
                sb->fFunction->PushOp(ASSIGN(col, texCol));
                sb->fCurrColor = col;

                // alpha = texture.a
                sb->fFunction->PushOp(ASSIGN(alpha, PROP(sb->fCurrImage, "a")));
                sb->fCurrAlpha = alpha;
                break;
            }
        }
    }

}


#if 0
void plGLMaterialShaderRef::IHandleTextureMode(plLayerInterface* layer)
{
    plBitmap* bitmap = layer->GetTexture();

    if (bitmap) {
        // EnvBumpNext is unused in production -- ignoring for now


        if (layer->GetBlendFlags() & hsGMatState::kBlendNoTexColor) {
            // color = null
        } else {
            if (lay->GetBlendFlags() & hsGMatState::kBlendInvertColor) {
                // color = 1.0 - texture.rgb
            } else {
                // color = texture.rgb
            }
        }

        if (layer->GetBlendFlags() & hsGMatState::kBlendInvertAlpha) {
            // alpha1 = 1.0 - texture.a
        } else {
            // alpha1 = texture.a
        }

        if (layer->GetBlendFlags() & hsGMatState::kBlendInvertVtxAlpha) {
            // alpha2 = 1.0 - vVtxColor.a
        } else {
            // alpha2 = vVtxColor.a
        }

        if (layer->GetBlendFlags() & hsGMatState::kBlendNoVtxAlpha) {
            // alpha = alpha1
        } else if (layer->GetBlendFlags() & hsGMatState::kBlendNoTexAlpha) {
            // alpha = alpha2
        } else {
            // alpha = alpha1 * alpha2
        }
    } else if (piggybacks) {
        // Plasma says it selects the vertex colours, but actually selects an
        // undefined arg2. We're going to assume that's wrong and select the
        // vertex colours.

        // color = vVtxColor.rgb
        // alpha = vVtxColor.a
    } else {
        // color = vVtxColor.rgb
        if (layer->GetBlendFlags() & hsGMatState::kBlendInvertVtxAlpha) {
            // alpha = 1.0 - vVtxColor.a
        } else {
            // alpha = vVtxColor.a
        }
    }
}

void plGLMaterialShaderRef::IHandleStageBlend(const hsGMatState& state)
{
    const uint32_t blendFlags = state.fBlendFlags;

    if (blendFlags & hsGMatState::kBlendInvertColor) {
        // color = 1.0 - texture.rgb
    } else {
        // color = texture.rgb
    }

    // Ignoring the unused kBlendEnvBumpNext

    switch (blendFlags & hsGMatState::kBlendMask)
    {
        case hsGMatState::kBlendAlpha:
        {
            if (blendFlags & hsGMatState::kBlendNoTexColor) {
                // if (prev) { color = prev }
            } else {
                if (blendFlags & hsGMatState::kBlendInvertAlpha) {
                    // color = color.rgb + (texture.a * prev.rgb)
                } else {
                    // color = mix(prev.rgb, color.rgb, texture.a)
                }
            }

            if (blendFlags & hsGMatState::kBlendAlphaAdd) {
                if (blendFlags & hsGMatState::kBlendInvertAlpha) {
                    // alpha = (1.0 - texture.a) + prev
                } else {
                    // alpha = texture.a + prev
                }
            } else if (blendFlags & hsGMatState::kBlendAlphaMult) {
                if (blendFlags & hsGMatState::kBlendInvertAlpha) {
                    // alpha = (1.0 - texture.a) * prev
                } else {
                    // alpha = texture.a * prev
                }
            } else {
                // alpha = prev
            }
            break;
        }

        case hsGMatState::kBlendAdd:
        {
            // color = color + prev
            // alpha = prev
            break;
        }

        case hsGMatState::kBlendMult:
        {
            // color = color * prev
            // alpha = prev
            break;
        }

        case hsGMatState::kBlendDot3:
        {
            // color = (color.r * prev.r + color.g * prev.g + color.b * prev.b)
            // alpha = prev
            break;
        }

        case hsGMatState::kBlendAddSigned:
        {
            // color = color + prev - 0.5
            // alpha = prev
            break;
        }

        case hsGMatState::kBlendAddSigned2X:
        {
            // color = (color + prev - 0.5) << 1
            // alpha = prev
            break;
        }

        case hsGMatState::kBlendAddColorTimesAlpha:
            hsAssert(false, "Blend mode unsupported on upper layers");
            break;

        case 0:
        {
            // color = color
            // alpha = prev
            break;
        }
    }
}
#endif
