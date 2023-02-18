struct UBO
{
    float4x4 g_WorldViewProj;
    float4x4 g_projection;
    float4x4 g_model;
    float4x4 g_view;
    float3 g_camPos;
    
};

cbuffer Constants : register(b0)
{
    UBO ubo;
};

cbuffer ObjConstants : register(b1)
{
    float3 objPos;
};

// Vertex shader takes two inputs: vertex position and uv coordinates.
// By convention, Diligent Engine expects vertex shader inputs to be 
// labeled 'ATTRIBn', where n is the attribute number.
struct VSInput
{
    float3 Pos : ATTRIB0;
    float2 UV : ATTRIB1;
    float3 Normal : ATTRIB2;
    float3 Tangent : ATTRIB3;
    float3 BiTangent : ATTRIB4;
};

struct PSInput
{
    float4 Pos : SV_POSITION;
    float3 WorldPos : WORLD_POS;
    float2 UV : TEX_COORD;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float3 BiTangent : BITANGENT;
    float3x3 tangentBasis : TBASIS;
};

// Note that if separate shader objects are not supported (this is only the case for old GLES3.0 devices), vertex
// shader output variable name must match exactly the name of the pixel shader input variable.
// If the variable has structure type (like in this example), the structure declarations must also be identical.
PSInput main(in VSInput input)
{
    PSInput output = (PSInput) 0;
    float3 localPos = mul(ubo.g_model, float4(input.Pos, 1.0)).xyz;
    output.WorldPos = localPos + objPos;
    output.Normal = mul((float3x3) ubo.g_model, input.Normal);
    output.Pos = mul(float4(input.Pos, 1.0), ubo.g_WorldViewProj);
    output.UV = input.UV;
    output.Tangent = input.Tangent;
    output.BiTangent = input.BiTangent;
    
    float3x3 TBN = float3x3(input.Tangent, input.BiTangent, input.Normal);
    output.tangentBasis = mul((float3x3) ubo.g_model, transpose(TBN));
    
    return output;
}