#include "Cube.hpp"


Cube::Cube(ISwapChain* pSwapChain, IRenderDevice* pRenderDevice, IEngineFactory *pEngineFactory, IDeviceContext* pDeviceContext)
	: m_pSwapChain(pSwapChain), m_pDevice(pRenderDevice), m_pEngineFactory(pEngineFactory), m_pDeviceContext(pDeviceContext)
{
	CreateCube();
	CreatePipeline();
	LoadTexture();
}

void Cube::Update(float eplapsedTime, float4x4 matrix)
{
	m_ModelMatrix = matrix;
}

void Cube::Draw()
{
	{
		MapHelper<float4x4> CBConstant(m_pDeviceContext, m_VSConstants, MAP_WRITE, MAP_FLAG_DISCARD);
		*CBConstant = m_ModelMatrix.Transpose();
	}

	Uint64 offset = 0;
	IBuffer* pBuffs[] = { m_CubeVertexBuffer };
	m_pDeviceContext->SetVertexBuffers(0, 1, pBuffs, &offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
	m_pDeviceContext->SetIndexBuffer(m_CubeIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	
	// Set the pipeline state in the immediate context
	m_pDeviceContext->SetPipelineState(m_pPSO);

	m_pDeviceContext->CommitShaderResources(m_pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	DrawIndexedAttribs DrawAttrs; // This is an indexed draw call
	DrawAttrs.IndexType = VT_UINT32; // Index type
	DrawAttrs.NumIndices = 36;
	// Verify the state of vertex and index buffers as well as consistence of 
	// render targets and correctness of draw command arguments
	DrawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
	m_pDeviceContext->DrawIndexed(DrawAttrs);
}

void Cube::ShutDown()
{
	/*m_pPSO->Release();
	m_pSRB->Release();
	m_VSConstants->Release();
	m_CubeVertexBuffer->Release();
	m_CubeIndexBuffer->Release();*/

	/*m_pEngineFactory->Release();
	m_pDevice->Release();
	m_pSwapChain->Release();
	m_pDeviceContext->Release();*/
}

Cube::~Cube()
{
}

/*     (-1,+1,+1)________________(+1,+1,+1)
			  /|              /|
			 / |             / |
			/  |            /  |
		   /   |           /   |
(-1,-1,+1) /____|__________/(+1,-1,+1)
		   |    |__________|____|
		   |   /(-1,+1,-1) |    /(+1,+1,-1)
		   |  /            |   /
		   | /             |  /
		   |/              | /
		   /_______________|/
		(-1,-1,-1)       (+1,-1,-1)*/

void Cube::CreateCube()
{
	m_vertices = {
		{float3(-1,-1,-1), float2(0,1)},
		{float3(-1,+1,-1), float2(0,0)},
		{float3(+1,+1,-1), float2(1,0)},
		{float3(+1,-1,-1), float2(1,1)},

		{float3(-1,-1,-1), float2(0,1)},
		{float3(-1,-1,+1), float2(0,0)},
		{float3(+1,-1,+1), float2(1,0)},
		{float3(+1,-1,-1), float2(1,1)},

		{float3(+1,-1,-1), float2(0,1)},
		{float3(+1,-1,+1), float2(1,1)},
		{float3(+1,+1,+1), float2(1,0)},
		{float3(+1,+1,-1), float2(0,0)},

		{float3(+1,+1,-1), float2(0,1)},
		{float3(+1,+1,+1), float2(0,0)},
		{float3(-1,+1,+1), float2(1,0)},
		{float3(-1,+1,-1), float2(1,1)},

		{float3(-1,+1,-1), float2(1,0)},
		{float3(-1,+1,+1), float2(0,0)},
		{float3(-1,-1,+1), float2(0,1)},
		{float3(-1,-1,-1), float2(1,1)},

		{float3(-1,-1,+1), float2(1,1)},
		{float3(+1,-1,+1), float2(0,1)},
		{float3(+1,+1,+1), float2(0,0)},
		{float3(-1,+1,+1), float2(1,0)}
	};

	
	m_indices = {
		2,0,1,    2,3,0,
		4,6,5,    4,7,6,
		8,10,9,   8,11,10,
		12,14,13, 12,15,14,
		16,18,17, 16,19,18,
		20,21,22, 20,22,23
	};



}

void Cube::CreatePipeline()
{
	//VERTEX BUFFER
	BufferDesc VertBufferDesc;
	VertBufferDesc.Name = "Cube Vertex Buffer";
	VertBufferDesc.Usage = USAGE_IMMUTABLE;
	VertBufferDesc.BindFlags = BIND_VERTEX_BUFFER;
	VertBufferDesc.Size = m_vertices.size() * sizeof(VertexCube);
	BufferData VBData;
	VBData.pData = m_vertices.data();
	VBData.DataSize = m_vertices.size() * sizeof(VertexCube);
	m_pDevice->CreateBuffer(VertBufferDesc, &VBData, &m_CubeVertexBuffer);

	// INDEX BUFFER
	BufferDesc IndBuffDesc;
	IndBuffDesc.Name = "Cube index buffer";
	IndBuffDesc.Usage = USAGE_IMMUTABLE;
	IndBuffDesc.BindFlags = BIND_INDEX_BUFFER;
	IndBuffDesc.Size = m_indices.size() * sizeof(float);
	BufferData IBData;
	IBData.pData = m_indices.data();
	IBData.DataSize = m_indices.size() * sizeof(float);
	m_pDevice->CreateBuffer(IndBuffDesc, &IBData, &m_CubeIndexBuffer);

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
		ShaderCI.FilePath = "cube.vsh";
		//ShaderCI.Source = VSSource;
		m_pDevice->CreateShader(ShaderCI, &pVS);
	}

	// Create a pixel shader
	RefCntAutoPtr<IShader> pPS;
	{
		ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
		ShaderCI.EntryPoint = "main";
		ShaderCI.Desc.Name = "Triangle pixel shader";
		//ShaderCI.Source = PSSource;
		ShaderCI.FilePath = "cube.psh";
		m_pDevice->CreateShader(ShaderCI, &pPS);

		//UNIFORM BUFFER
		BufferDesc CBDesc;
		CBDesc.Name = "VS Constant CB";
		CBDesc.Size = sizeof(float4x4);
		CBDesc.Usage = USAGE_DYNAMIC;
		CBDesc.BindFlags = BIND_UNIFORM_BUFFER;
		CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
		m_pDevice->CreateBuffer(CBDesc, nullptr, &m_VSConstants);
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
	PSOCreateInfo.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	// No back face culling for this tutorial
	PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_NONE;
	// Disable depth testing
	PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = true;
	// clang-format on
	PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_BACK;


	LayoutElement LayoutElems[] =
	{
		LayoutElement{0,0,3, VT_FLOAT32, false},
		LayoutElement{1,0,2,VT_FLOAT32, false}
	};
	PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
	PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements = _countof(LayoutElems);

	PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

	ShaderResourceVariableDesc Vars[] =
	{
		{SHADER_TYPE_PIXEL, "g_Texture",SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE}
	};

	PSOCreateInfo.PSODesc.ResourceLayout.Variables = Vars;
	PSOCreateInfo.PSODesc.ResourceLayout.NumVariables = _countof(Vars);


	SamplerDesc SamLinearDesc
	{
		 FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR,
		TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP
	};

	ImmutableSamplerDesc ImtblSamplers[] =
	{
		{SHADER_TYPE_PIXEL, "g_Texture", SamLinearDesc}
	};

	PSOCreateInfo.PSODesc.ResourceLayout.ImmutableSamplers = ImtblSamplers;
	PSOCreateInfo.PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);

	


	m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO);

	m_pPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(m_VSConstants);

	m_pPSO->CreateShaderResourceBinding(&m_pSRB, true);

}

void Cube::LoadTexture()
{
	TextureLoadInfo loadInfo;
	loadInfo.IsSRGB = true;
	RefCntAutoPtr<ITexture> Tex;
	CreateTextureFromFile("F:/CustomEngine/CrownDiligentEngine/assets/DGLogo.png", loadInfo, m_pDevice, &Tex);
	m_TextureSRV = Tex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);

	m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(m_TextureSRV);
}



