#include "BasicStructures.fxh"

#ifndef SHADOW_PASS
#   define SHADOW_PASS 0
#endif

cbuffer cbCameraAttribs
{
    CameraAttribs g_CameraAttribs;
};


cbuffer cbLightAttribs
{
    LightAttribs g_LightAttribs;
};

// Vertex shader takes two inputs: vertex position and uv coordinates.
// By convention, Diligent Engine expects vertex shader inputs to be 
// labeled 'ATTRIBn', where n is the attribute number.
struct VSInput
{
    float3 Pos : ATTRIB0;
    float3 Normal : ATTRIB1;
    float2 UV  : ATTRIB2;
};

struct PSInput 
{ 
    float4 Pos : SV_POSITION; 
    float2 UV  : TEX_COORD; 
    float3 PosInLightViewSpace : LIGHT_SPACE_POS;
};

// Note that if separate shader objects are not supported (this is only the case for old GLES3.0 devices), vertex
// shader output variable name must match exactly the name of the pixel shader input variable.
// If the variable has structure type (like in this example), the structure declarations must also be identical.
void main(in  VSInput VSIn,
          out PSInput PSIn) 
{
    float4 LightSpacePos = mul(float4(VSIn.Pos, 1.0), g_LightAttribs.ShadowAttribs.mWorldToLightView);
    PSIn.PosInLightViewSpace = LightSpacePos.xyz / LightSpacePos.w;
    
    PSIn.Pos = mul(float4(VSIn.Pos, 1.0), g_CameraAttribs.mViewProj);
    PSIn.UV  = VSIn.UV;
}