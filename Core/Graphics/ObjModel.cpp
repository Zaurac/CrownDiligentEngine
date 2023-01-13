#include "ObjModel.hpp"

#include <unordered_map>

#include <direct.h>
#define GetCurrentDir _getcwd

ObjModel::ObjModel(ISwapChain* pSwapChain, IRenderDevice* pRenderDevice, IEngineFactory* pEngineFactory, IDeviceContext* pDeviceContext)
	: m_pSwapChain(pSwapChain), m_pDevice(pRenderDevice), m_pEngineFactory(pEngineFactory), m_pDeviceContext(pDeviceContext)
{
	
}
void ObjModel::CreatePipeline()
{
	CreateShader();

	//UNIFORM BUFFER
	BufferDesc CBDesc;
	CBDesc.Name = "VS Constant CB";
	CBDesc.Size = sizeof(float4x4);
	CBDesc.Usage = USAGE_DYNAMIC;
	CBDesc.BindFlags = BIND_UNIFORM_BUFFER;
	CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
	m_pDevice->CreateBuffer(CBDesc, nullptr, &m_pUniformBuffer);

	GraphicsPipelineStateCreateInfo PSOCreateInfo;
	PSOCreateInfo.PSODesc.Name = "Model Pipeline";
	//LINK SHADER
	PSOCreateInfo.pPS = pPS;
	PSOCreateInfo.pVS = pVS;
	//TYPE PIPELINE
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
	// Disable depth testing
	PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = true;
	// clang-format on
	PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_BACK;

	LayoutElement LayoutElems[] =
	{
		//POSITION
		LayoutElement{0,0,3, VT_FLOAT32, false},
		//UV
		LayoutElement{1,0,2, VT_FLOAT32, false},
	};
	PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
	PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements = _countof(LayoutElems);

	PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

	ShaderResourceVariableDesc Vars[] =
	{
		{SHADER_TYPE_PIXEL, "g_Texture", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
	};

	PSOCreateInfo.PSODesc.ResourceLayout.Variables = Vars;
	PSOCreateInfo.PSODesc.ResourceLayout.NumVariables = _countof(Vars);

	SamplerDesc SamLinearDesc
	{
		 FILTER_TYPE_ANISOTROPIC, FILTER_TYPE_ANISOTROPIC, FILTER_TYPE_ANISOTROPIC,
		TEXTURE_ADDRESS_WRAP, TEXTURE_ADDRESS_WRAP, TEXTURE_ADDRESS_WRAP
	};

	ImmutableSamplerDesc ImtblSamplers[] =
	{
		{SHADER_TYPE_PIXEL, "g_Texture", SamLinearDesc}
	};

	PSOCreateInfo.PSODesc.ResourceLayout.ImmutableSamplers = ImtblSamplers;
	PSOCreateInfo.PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);
	

	m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pModelPipeline);
	m_pModelPipeline->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(m_pUniformBuffer);

	for (auto i = 0; i < m_meshes.size(); i++)
	{
		m_meshes[i].SetSystem(m_pDevice, m_pDeviceContext);
		m_meshes[i].CreatePipeline(m_pModelPipeline);
	}

	
}

void ObjModel::CreateShader()
{
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

	//VERTEX SHADER
	ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
	ShaderCI.EntryPoint = "main";
	//SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE;
	ShaderCI.Desc.Name = "Model vertex shader";
	ShaderCI.FilePath = "model.vsh";
	m_pDevice->CreateShader(ShaderCI, &pVS);


	//PIXEL SHADER
	ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
	ShaderCI.EntryPoint = "main";
	ShaderCI.Desc.Name = "Model pixel shader";
	ShaderCI.FilePath = "model.psh";
	m_pDevice->CreateShader(ShaderCI, &pPS);
}

void ObjModel::update(float eplapsedTime, float4x4 matrix)
{
	m_ModelMatrix = matrix;
	for (auto i = 0; i < m_meshes.size(); i++)
	{
		m_meshes[i].Update(m_ModelMatrix);
	}
}

void ObjModel::draw(bool bIsShadowPass, const ViewFrustumExt& Frustum)
{
	{
		MapHelper<float4x4> CBConstant(m_pDeviceContext, m_pUniformBuffer, MAP_WRITE, MAP_FLAG_DISCARD);
		*CBConstant = m_ModelMatrix.Transpose();
	}

	for (auto j =0; j < m_meshes.size(); ++j)
	{
		m_meshes[j].Draw(m_pDeviceContext, bIsShadowPass, Frustum);
	}
}



void ObjModel::loadObjFile(const std::string& path)
{
	tinyobj::attrib_t attributes;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warning;
	std::string error;

	

	 bool ret = tinyobj::LoadObj(&attributes, &shapes, &materials, &warning, &error, path.c_str(), "F:/CustomEngine/CrownDiligentEngine/assets/Model/Sponza/");
	
	if (!warning.empty()) {
		std::cout << "WARN: " << warning << std::endl;
	}
	if (!error.empty()) {
		std::cerr << error << std::endl;
	}

	if (!ret) {
		std::cerr << "Failed to load " << path.c_str() << std::endl;
	}

	/*printf("# of vertices  = %d\n", (int)(attributes.vertices.size()) / 3);
	printf("# of normals   = %d\n", (int)(attributes.normals.size()) / 3);
	printf("# of texcoords = %d\n", (int)(attributes.texcoords.size()) / 2);
	printf("# of materials = %d\n", (int)materials.size());
	printf("# of shapes    = %d\n", (int)shapes.size());*/

	materials.push_back(tinyobj::material_t());
	/*for (size_t i = 0; i < materials.size(); i++) {
		printf("material[%d].diffuse_texname = %s\n", int(i),
			materials[i].diffuse_texname.c_str());
	}*/

	
	std::unordered_map<const char*, RefCntAutoPtr<ITextureView>> m_textureArray;
	for (auto i = 0; i < shapes.size(); i++)
	{
		Mesh meshe;
		std::unordered_map<float3, uint32_t> uniqueVertices;
		for (const auto index : shapes[i].mesh.indices)
		{
			
			
			float3 position;
			position.x = attributes.vertices[3 * index.vertex_index];
			position.y = attributes.vertices[3 * index.vertex_index + 1];
			position.z = attributes.vertices[3 * index.vertex_index + 2];

			float2 uv;

			if (index.texcoord_index == -1)
			{
				uv = {0,0};
			}
			else
			{
				uv.x = attributes.texcoords[2 * index.texcoord_index];
				uv.y = 1.0f - attributes.texcoords[2 * index.texcoord_index + 1];
			}

			

			if (uniqueVertices.count(position) == 0)
			{
				uniqueVertices[position] = static_cast<uint32_t>(meshe.m_vertices.size());
				meshe.m_vertices.push_back(Mesh::Vertex{ position, uv });
			}

			meshe.m_indices.push_back(uniqueVertices[position]);

			
		}

		if (shapes[i].mesh.material_ids[0] >= 0)
		{
			tinyobj::material_t* mp = &materials[shapes[i].mesh.material_ids[0]];
			if (mp->diffuse_texname.length() >= 1)
			{
				auto t = m_textureArray.find("");
				if (m_textureArray[mp->diffuse_texname.c_str()])
				{
					meshe.m_diffuseTextureView = m_textureArray[mp->diffuse_texname.c_str()];
				}
				else
				{
					TextureLoadInfo loadInfo;
					loadInfo.IsSRGB = true;
					RefCntAutoPtr<ITexture> Tex;
					std::string test = "F:/CustomEngine/CrownDiligentEngine/assets/Model/Sponza/" + mp->diffuse_texname;
					CreateTextureFromFile(test.c_str(), loadInfo, m_pDevice, &Tex);
					m_textureArray[mp->diffuse_texname.c_str()] = Tex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
					meshe.m_diffuseTextureView = m_textureArray[mp->diffuse_texname.c_str()];
				}
				
			}
		}
		
		m_meshes.push_back(meshe);
		uniqueVertices.clear();
	}
	CreatePipeline();
}