"// InterpolateIrradiance.fx\n"
"// Interpolates irradiance from ray marching samples to all epipolar samples\n"
"\n"
"#include \"AtmosphereShadersCommon.fxh\"\n"
"\n"
"Texture2D<uint2>  g_tex2DInterpolationSource;\n"
"Texture2D<float3> g_tex2DInitialInsctrIrradiance;\n"
"\n"
"void InterpolateIrradiancePS(FullScreenTriangleVSOutput VSOut,\n"
"                             // IMPORTANT: non-system generated pixel shader input\n"
"                             // arguments must have the exact same name as vertex shader\n"
"                             // outputs and must go in the same order.\n"
"                             // Moreover, even if the shader is not using the argument,\n"
"                             // it still must be declared.\n"
"\n"
"                             out float4 f4InterpolatedIrradiance : SV_Target)\n"
"{\n"
"    int iSampleInd = int(VSOut.f4PixelPos.x);\n"
"    int iSliceInd = int(VSOut.f4PixelPos.y);\n"
"    // Get interpolation sources\n"
"    uint2 ui2InterpolationSources = g_tex2DInterpolationSource.Load( int3(iSampleInd, iSliceInd, 0) );\n"
"    float fInterpolationPos = float(iSampleInd - int(ui2InterpolationSources.x)) / float( max(ui2InterpolationSources.y - ui2InterpolationSources.x, 1u) );\n"
"\n"
"    float3 f3SrcInsctr0 = g_tex2DInitialInsctrIrradiance.Load( int3(ui2InterpolationSources.x, iSliceInd, 0) );\n"
"    float3 f3SrcInsctr1 = g_tex2DInitialInsctrIrradiance.Load( int3(ui2InterpolationSources.y, iSliceInd, 0));\n"
"\n"
"    // Ray marching samples are interpolated from themselves\n"
"    f4InterpolatedIrradiance.rgb = lerp(f3SrcInsctr0, f3SrcInsctr1, fInterpolationPos);\n"
"    f4InterpolatedIrradiance.a = 1.0;\n"
"}\n"
