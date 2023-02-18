#include "Sphere.hpp"
#include <random>
#include <string>

#define GRID_DIM 7

Sphere::Sphere(ISwapChain* pSwapChain, IRenderDevice* pRenderDevice, IEngineFactory *pEngineFactory, IDeviceContext* pDeviceContext)
	: m_pSwapChain(pSwapChain), m_pDevice(pRenderDevice), m_pEngineFactory(pEngineFactory), m_pDeviceContext(pDeviceContext)
{
	CreateSphere();
	CreatePipeline();
	//LoadTexture();

	
}

void Sphere::Update(float eplapsedTime, float4x4 matrix)
{
	
}

void Sphere::Draw()
{
	{
		MapHelper<UBOMatrices> CBConstant(m_pDeviceContext, m_VSConstants, MAP_WRITE, MAP_FLAG_DISCARD);
		*CBConstant = m_uboMatrices;
	}

	{
		MapHelper<float3> CBConstant(m_pDeviceContext, m_ObjConstants, MAP_WRITE, MAP_FLAG_DISCARD);
		*CBConstant = m_SpherePos;
	}

	{
		MapHelper<CustomLightAttribs> LightConstant(m_pDeviceContext, m_UBOLightAttribs, MAP_WRITE, MAP_FLAG_DISCARD);
		*LightConstant = m_lightAttrib;
	}

	Uint64 offset[] = { 0,0 };
	IBuffer* pBuffs[] = { m_SphereVertexBuffer };
	m_pDeviceContext->SetVertexBuffers(0, _countof(pBuffs), pBuffs, offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
	m_pDeviceContext->SetIndexBuffer(m_SphereIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	// Set the pipeline state in the immediate context
	m_pDeviceContext->SetPipelineState(m_pPSO);

	m_pDeviceContext->CommitShaderResources(m_pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	DrawIndexedAttribs DrawAttrs; // This is an indexed draw call
	DrawAttrs.IndexType = VT_UINT32; // Index type
	DrawAttrs.NumIndices = indexCount;
	// Verify the state of vertex and index buffers as well as consistence of 
	// render targets and correctness of draw command arguments
	DrawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
	m_pDeviceContext->DrawIndexed(DrawAttrs);
}

void Sphere::ShutDown()
{
}

Sphere::~Sphere()
{
}

void Sphere::CreateSphere()
{
	const unsigned int X_SEGMENTS = 64;
	const unsigned int Y_SEGMENTS = 64;

	const float PI = 3.14159265359f;
	for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
	{
		for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
		{
			float xSegment = (float)x / (float)X_SEGMENTS;
			float ySegment = (float)y / (float)Y_SEGMENTS;
			float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
			float yPos = std::cos(ySegment * PI);
			float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

			float3 tangent;
			float3 bitangent;
			float3 edge1 = float3(xPos, yPos, zPos) - float3(xPos-1, yPos, zPos);
			float3 edge2 = float3(xPos, yPos, zPos) - float3(xPos, yPos-1, zPos);
			tangent = normalize(cross(edge1, edge2));
			bitangent = normalize(cross(tangent, float3(xPos, yPos, zPos)));


			m_vertices.push_back(Vertex{float3(xPos, yPos, zPos), float2(xSegment, ySegment), float3(xPos, yPos, zPos), tangent, bitangent});
		}
	}

	bool oddRow = false;
	for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
	{
		if (!oddRow) // even rows: y == 0, y == 2; and so on
		{
			for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
			{
				m_indices.push_back(y * (X_SEGMENTS + 1) + x);
				m_indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
			}
		}
		else
		{
			for (int x = X_SEGMENTS; x >= 0; --x)
			{
				m_indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				m_indices.push_back(y * (X_SEGMENTS + 1) + x);
			}
		}
		oddRow = !oddRow;
	}
	indexCount = static_cast<unsigned int>(m_indices.size());

}

void Sphere::CreatePipeline()
{
	//VERTEX BUFFER
	BufferDesc VertBufferDesc;
	VertBufferDesc.Name = "Cube Vertex Buffer";
	VertBufferDesc.Usage = USAGE_IMMUTABLE;
	VertBufferDesc.BindFlags = BIND_VERTEX_BUFFER;
	VertBufferDesc.Size = m_vertices.size() * sizeof(Vertex);
	BufferData VBData;
	VBData.pData = m_vertices.data();
	VBData.DataSize = m_vertices.size() * sizeof(Vertex);
	m_pDevice->CreateBuffer(VertBufferDesc, &VBData, &m_SphereVertexBuffer);

	// INDEX BUFFER
	BufferDesc IndBuffDesc;
	IndBuffDesc.Name = "Cube index buffer";
	IndBuffDesc.Usage = USAGE_IMMUTABLE;
	IndBuffDesc.BindFlags = BIND_INDEX_BUFFER;
	IndBuffDesc.Size = m_indices.size() * sizeof(float);
	BufferData IBData;
	IBData.pData = m_indices.data();
	IBData.DataSize = m_indices.size() * sizeof(float);
	m_pDevice->CreateBuffer(IndBuffDesc, &IBData, &m_SphereIndexBuffer);
	
	// UBOLIGHT BUFFER
	BufferDesc LightBuffDesc;
	LightBuffDesc.Name = "UBO Light Buffer";
	LightBuffDesc.Usage = USAGE_DYNAMIC;
	LightBuffDesc.BindFlags = BIND_UNIFORM_BUFFER;
	LightBuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
	LightBuffDesc.Size = sizeof(CustomLightAttribs);
	m_pDevice->CreateBuffer(LightBuffDesc, nullptr, &m_UBOLightAttribs);

	// UBOLIGHT BUFFER
	BufferDesc ObjBuffDesc;
	ObjBuffDesc.Name = "OBJ POS Buffer";
	ObjBuffDesc.Usage = USAGE_DYNAMIC;
	ObjBuffDesc.BindFlags = BIND_UNIFORM_BUFFER;
	ObjBuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
	ObjBuffDesc.Size = sizeof(float3);
	m_pDevice->CreateBuffer(ObjBuffDesc, nullptr, &m_ObjConstants);


	//UNIFORM BUFFER
	BufferDesc CBDesc;
	CBDesc.Name = "VS Constant CB";
	CBDesc.Size = sizeof(UBOMatrices);
	CBDesc.Usage = USAGE_DYNAMIC;
	CBDesc.BindFlags = BIND_UNIFORM_BUFFER;
	CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
	m_pDevice->CreateBuffer(CBDesc, nullptr, &m_VSConstants);


	GraphicsPipelineStateCreateInfo PSOCreateInfo;

	ShaderCreateInfo ShaderCI;
	// Tell the system that the shader source code is in HLSL.
	// For OpenGL, the engine will convert this into GLSL under the hood.
	ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
	// OpenGL backend requires emulated combined HLSL texture samplers (g_Texture + g_Texture_sampler combination)
	ShaderCI.Desc.UseCombinedTextureSamplers = true;

	// SHADER FACTORY
	RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
	m_pEngineFactory->CreateDefaultShaderSourceStreamFactory("F:/CustomEngine/CrownDiligentEngine/assets/", &pShaderSourceFactory);
	ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;

	// Create a vertex shader
	RefCntAutoPtr<IShader> pVS;
	{
		ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
		ShaderCI.EntryPoint = "main";
		ShaderCI.Desc.Name = "Triangle vertex shader";
		ShaderCI.FilePath = "RenderTest_PBR.vsh";
		m_pDevice->CreateShader(ShaderCI, &pVS);
	}
	// Create a pixel shader
	RefCntAutoPtr<IShader> pPS;
	{
		ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
		ShaderCI.EntryPoint = "main";
		ShaderCI.Desc.Name = "Triangle pixel shader";
		ShaderCI.FilePath = "RenderTest_PBR.psh";
		m_pDevice->CreateShader(ShaderCI, &pPS);
	}

	

	// Finally, create the pipeline state
	PSOCreateInfo.pVS = pVS;
	PSOCreateInfo.pPS = pPS;
	// Pipeline state name is used by the engine to report issues.
	// It is always a good idea to give objects descriptive names.
	PSOCreateInfo.PSODesc.Name = "Cube PSO";

	// This is a graphics pipeline
	PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

	// clang-format off
	// This tutorial will render to a single render target
	PSOCreateInfo.GraphicsPipeline.NumRenderTargets = 1;
	// Set render target format which is the format of the swap chain's color buffer
	PSOCreateInfo.GraphicsPipeline.RTVFormats[0] = m_pSwapChain->GetDesc().ColorBufferFormat;
	// Use the depth buffer format from the swap chain
	PSOCreateInfo.GraphicsPipeline.DSVFormat = m_pSwapChain->GetDesc().DepthBufferFormat;
	// Primitive topology defines what kind of primitives will be rendered by this pipeline state
	PSOCreateInfo.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	// Disable depth testing
	PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = true;
	// clang-format on
	PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_NONE;


	LayoutElement LayoutElems[] =
	{
		//POSITION
		LayoutElement{0,0,3, VT_FLOAT32, false},
		//UV
		LayoutElement{1,0,2, VT_FLOAT32, false},
		//Normal
		LayoutElement{2,0,3, VT_FLOAT32, false},
		//Tangent
		LayoutElement{3,0,3, VT_FLOAT32, false},
		//BiTangent
		LayoutElement{4,0,3, VT_FLOAT32, false},
	};
	PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
	PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements = _countof(LayoutElems);

	PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

	ShaderResourceVariableDesc Vars[] =
	{
		{SHADER_TYPE_PIXEL, "g_DiffTexture",SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
		{SHADER_TYPE_PIXEL, "g_NormTexture",SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
		{SHADER_TYPE_PIXEL, "g_IrradianceMap",SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
		{SHADER_TYPE_PIXEL, "g_PrefilteredEnvMap",SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
		{SHADER_TYPE_PIXEL, "g_BRDF_LUT",SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
		{SHADER_TYPE_PIXEL, "g_RougTexture",SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
		{SHADER_TYPE_PIXEL, "g_MetalTexture",SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
		
	};

	PSOCreateInfo.PSODesc.ResourceLayout.Variables = Vars;
	PSOCreateInfo.PSODesc.ResourceLayout.NumVariables = _countof(Vars);
	/*PSOCreateInfo.PSODesc.ResourceLayout.Variables = 0;
	PSOCreateInfo.PSODesc.ResourceLayout.NumVariables = 0;*/


	SamplerDesc SamLinearDesc
	{
		 FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR,
		TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP
	};

	ImmutableSamplerDesc ImtblSamplers[] =
	{
		{SHADER_TYPE_PIXEL, "g_DiffTexture", SamLinearDesc}
	};

	PSOCreateInfo.PSODesc.ResourceLayout.ImmutableSamplers = ImtblSamplers;
	PSOCreateInfo.PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);
	//PSOCreateInfo.PSODesc.ResourceLayout.ImmutableSamplers = nullptr;
	//PSOCreateInfo.PSODesc.ResourceLayout.NumImmutableSamplers = 0;
	m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO);

	//VERTEX
	m_pPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(m_VSConstants);
	m_pPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "ObjConstants")->Set(m_ObjConstants);
	//PIXEL
	m_pPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "ubo")->Set(m_VSConstants);
	m_pPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "cbLightAttribs")->Set(m_UBOLightAttribs);

	//BINDING
	m_pPSO->CreateShaderResourceBinding(&m_pSRB, true);

}

void Sphere::LoadTexture()
{
	//DIFFUSE TEXTURE
	RefCntAutoPtr<ITexture> pTexDiffuse;
	TextureLoadInfo loadInfo;
	loadInfo.IsSRGB = true;
	CreateTextureFromFile("F:/CustomEngine/CrownDiligentEngine/assets/pbr/damaged_diffuse.jpg", loadInfo, m_pDevice, &pTexDiffuse);
	m_TextureSRV = pTexDiffuse->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
	m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_DiffTexture")->Set(m_TextureSRV);

	//NORMAL TEXTURE
	RefCntAutoPtr<ITexture> pTexNormal;
	//CreateTextureFromFile("F:/CustomEngine/CrownDiligentEngine/assets/pbr/damaged_normal.jpg", loadInfo, m_pDevice, &pTexNormal);
	//m_TextureNormal = pTexNormal->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
	//m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_NormalMap")->Set(m_TextureNormal);

	//METAL/SPECULAR TEXTURE
	RefCntAutoPtr<ITexture> pTexMetal;
	//CreateTextureFromFile("F:/CustomEngine/CrownDiligentEngine/assets/pbr/damaged_metalness.jpg", loadInfo, m_pDevice, &pTexMetal);
	//m_TextureMetal = pTexMetal->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
	//m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_MetalTexture")->Set(m_TextureMetal);

	//ROUGNESS TEXTURE
	RefCntAutoPtr<ITexture> pTexRoug;
	//CreateTextureFromFile("F:/CustomEngine/CrownDiligentEngine/assets/pbr/damaged_roughness.jpg", loadInfo, m_pDevice, &pTexRoug);
	//m_TextureRoug = pTexRoug->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
	//m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_PhysicalDescriptorMap")->Set(m_TextureRoug);

	//AO TEXTURE
	RefCntAutoPtr<ITexture> pTexAO;
	//CreateTextureFromFile("F:/CustomEngine/CrownDiligentEngine/assets/pbr/damaged_ao.jpg", loadInfo, m_pDevice, &pTexAO);
	//m_TextureAO = pTexAO->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
	//m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_AOTexture")->Set(m_TextureAO);
}

void Sphere::SetTexture(const char* name, ITextureView *texture)
{
	m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, name)->Set(texture);
}



