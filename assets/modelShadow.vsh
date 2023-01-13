#include "BasicStructures.fxh"


#ifndef SHADOW_PASS
#   define SHADOW_PASS 0
#endif

cbuffer cbCameraAttribs
{
    CameraAttribs g_CameraAttribs;
};

#if !SHADOW_PASS
cbuffer cbLightAttribs
{
    LightAttribs g_LightAttribs;
};
#endif


struct VSInput
{
    float3 Pos : ATTRIB0;
    float2 UV  : ATTRIB1;
};

struct VSOutput
{ 
    float4 Pos : SV_POSITION; 
    float3 PosInLightViewSpace : LIGHT_SPACE_POS;
    float2 UV  : TEX_COORD; 
};


void main(in  VSInput VSIn,
          out VSOutput VSOut)
{
    
#if !SHADOW_PASS
    float4 LightSpacePos = mul(float4(VSIn.Pos, 1.0), g_LightAttribs.ShadowAttribs.mWorldToLightView);
    VSOut.PosInLightViewSpace = LightSpacePos.xyz / LightSpacePos.w;
#else
    VSOut.PosInLightViewSpace = float3(0.0, 0.0, 0.0);
#endif
    
    VSOut.Pos = mul(float4(VSIn.Pos, 1.0), g_CameraAttribs.mViewProj);
    VSOut.UV = VSIn.UV;
}