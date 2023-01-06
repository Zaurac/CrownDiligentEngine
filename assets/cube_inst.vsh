cbuffer Constants
{
    float4x4 g_WorldViewProj;
};

// Vertex shader takes two inputs: vertex position and uv coordinates.
// By convention, Diligent Engine expects vertex shader inputs to be 
// labeled 'ATTRIBn', where n is the attribute number.
struct VSInput
{
    float3 Pos : ATTRIB0;
    float2 UV : ATTRIB1;
    
    float4 MtrxRow0 : ATTRIB2;
    float4 MtrxRow1 : ATTRIB3;
    float4 MtrxRow2 : ATTRIB4;
    float4 MtrxRow3 : ATTRIB5;
    float TexArrInd : ATTRIB6;
};

struct PSInput
{
    float4 Pos : SV_POSITION;
    float2 UV : TEX_COORD;
    float TexIndex : TEX_ARRAY_INDEX;
};

// Note that if separate shader objects are not supported (this is only the case for old GLES3.0 devices), vertex
// shader output variable name must match exactly the name of the pixel shader input variable.
// If the variable has structure type (like in this example), the structure declarations must also be identical.
void main(in VSInput VSIn,
          out PSInput PSIn)
{
    float4x4 InstanceMatr = MatrixFromRows(VSIn.MtrxRow0, VSIn.MtrxRow1, VSIn.MtrxRow2, VSIn.MtrxRow3);
    
    float4 TransformedPos = float4(VSIn.Pos, 1.0);
    
    TransformedPos = mul(TransformedPos,InstanceMatr);
    
    PSIn.TexIndex = VSIn.TexArrInd;
    
    PSIn.Pos = mul(TransformedPos, g_WorldViewProj);
    PSIn.UV = VSIn.UV;
    
    //PSIn.Pos = mul(float4(VSIn.Pos, 1.0), g_WorldViewProj);
    //PSIn.UV = VSIn.UV;
}
