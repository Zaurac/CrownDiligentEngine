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

	//BASIC PIPELINE
	//UNIFORM BUFFER
	BufferDesc CBDesc;
	CBDesc.Name = "VS Constant CB";
	CBDesc.Size = sizeof(CameraAttribs);
	CBDesc.Usage = USAGE_DYNAMIC;
	CBDesc.BindFlags = BIND_UNIFORM_BUFFER;
	CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
	m_pDevice->CreateBuffer(CBDesc, nullptr, &m_UBCamera);

	//LihhtAttribs UNIFORM BUFFER
	CBDesc.Name = "VS LightAttribs CB";
	CBDesc.Size = sizeof(LightAttribs);
	CBDesc.Usage = USAGE_DYNAMIC;
	CBDesc.BindFlags = BIND_UNIFORM_BUFFER;
	CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
	m_pDevice->CreateBuffer(CBDesc, nullptr, &m_UBLightAttribs);

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
		//NORMAL
		LayoutElement{1,0,3, VT_FLOAT32, false},
		//UV
		LayoutElement{2,0,2, VT_FLOAT32, false},
	};
	PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
	PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements = _countof(LayoutElems);

	PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

	ShaderResourceVariableDesc Vars[] =
	{
		{SHADER_TYPE_PIXEL, "g_tex2DDiffuse", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
		{SHADER_TYPE_PIXEL, m_ShadowSettings.iShadowMode == SHADOW_MODE_PCF ? "g_tex2DShadowMap" : "g_tex2DFilterableShadowMap", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
		{SHADER_TYPE_PIXEL, "g_tex2DAlpha", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
		{SHADER_TYPE_PIXEL, "g_Tex2DSpecular", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
	};

	PSOCreateInfo.PSODesc.ResourceLayout.Variables = Vars;
	PSOCreateInfo.PSODesc.ResourceLayout.NumVariables = _countof(Vars);

	SamplerDesc SamLinearDesc{
	FILTER_TYPE_ANISOTROPIC, FILTER_TYPE_ANISOTROPIC, FILTER_TYPE_ANISOTROPIC,
	TEXTURE_ADDRESS_WRAP, TEXTURE_ADDRESS_WRAP, TEXTURE_ADDRESS_WRAP };


	ImmutableSamplerDesc ImtblSamplers[] =
	{
		{SHADER_TYPE_PIXEL, "g_tex2DDiffuse", SamLinearDesc},
	};

	PSOCreateInfo.PSODesc.ResourceLayout.ImmutableSamplers = ImtblSamplers;
	PSOCreateInfo.PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);

	PSOCreateInfo.GraphicsPipeline.RasterizerDesc.DepthClipEnable = true;

	m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pBasicPSO);
	m_pBasicPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "cbCameraAttribs")->Set(m_UBCamera);
	m_pBasicPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "cbLightAttribs")->Set(m_UBLightAttribs);
	m_pBasicPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "cbLightAttribs")->Set(m_UBLightAttribs);

	//SHADOW MESH  PIPELINE
	GraphicsPipelineStateCreateInfo PSOShadowCreateInfo;
	PSOShadowCreateInfo.PSODesc.Name = "Mesh Shadow PSO";
	PSOShadowCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;
	PSOShadowCreateInfo.GraphicsPipeline.NumRenderTargets = 0;
	PSOShadowCreateInfo.GraphicsPipeline.RTVFormats[0]	  = TEX_FORMAT_UNKNOWN;
	PSOShadowCreateInfo.GraphicsPipeline.DSVFormat		  = m_ShadowSettings.Format;
	PSOShadowCreateInfo.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	PSOShadowCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_BACK;
	PSOShadowCreateInfo.GraphicsPipeline.RasterizerDesc.DepthClipEnable = false;
	PSOShadowCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthFunc = COMPARISON_FUNC_LESS_EQUAL;
	PSOShadowCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = true;
	
	PSOShadowCreateInfo.pVS = m_ShadowVS;
	PSOShadowCreateInfo.pPS = nullptr;
	PSOShadowCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
	PSOShadowCreateInfo.GraphicsPipeline.InputLayout.NumElements = _countof(LayoutElems);
	PSOShadowCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
	m_pDevice->CreatePipelineState(PSOShadowCreateInfo, &m_ShadowPSO);
	m_ShadowPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "cbCameraAttribs")->Set(m_UBCamera);

	for (auto i = 0; i < m_meshes.size(); i++)
	{
		m_meshes[i].SetSystem(m_pDevice, m_pDeviceContext, m_pEngineFactory);
		m_meshes[i].AssignPipeline(m_pBasicPSO, m_ShadowPSO);
	}
}

void ObjModel::CreateShader()
{
	ShaderMacroHelper Macros;
	Macros.AddShaderMacro("SHADOW_MODE", m_ShadowSettings.iShadowMode);
	Macros.AddShaderMacro("SHADOW_FILTER_SIZE", m_pLightAttribs.ShadowAttribs.iFixedFilterSize);
	Macros.AddShaderMacro("FILTER_ACROSS_CASCADES", m_ShadowSettings.FilterAcrossCascades);
	Macros.AddShaderMacro("BEST_CASCADE_SEARCH", m_ShadowSettings.SearchBestCascade);

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
	ShaderCI.Macros = Macros;
	ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
	ShaderCI.EntryPoint = "MeshVS";
	//SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE;
	ShaderCI.Desc.Name = "Model vertex shader";
	ShaderCI.FilePath = "MeshVS.vsh";
	m_pDevice->CreateShader(ShaderCI, &pVS);


	//PIXEL SHADER
	ShaderCI.Macros = Macros;
	ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
	ShaderCI.EntryPoint = "MeshPS";
	ShaderCI.Desc.Name = "Model pixel shader";
	ShaderCI.FilePath = "MeshPS.psh";
	m_pDevice->CreateShader(ShaderCI, &pPS);

	//SHADOW VERTEX SHADER
	Macros.AddShaderMacro("SHADOW_PASS", true);
	ShaderCI.Desc.Name = "Shadow Mesh VS";
	ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
	ShaderCI.Macros = Macros;
	ShaderCI.EntryPoint = "MeshVS";
	ShaderCI.FilePath = "MeshVS.vsh";
	m_pDevice->CreateShader(ShaderCI, &m_ShadowVS);
}

void ObjModel::update(float eplapsedTime, CameraAttribs cameraAttribs)
{
	m_CameraAttribs = cameraAttribs;

	for (auto i = 0; i < m_meshes.size(); i++)
	{
		m_meshes[i].Update();
	}
}


void ObjModel::draw(CameraAttribs camAttrib,LightAttribs lightAttrib ,const ViewFrustumExt& Frustum)
{
	{
		MapHelper<CameraAttribs> CBConstant(m_pDeviceContext, m_UBCamera, MAP_WRITE, MAP_FLAG_DISCARD);
		*CBConstant = camAttrib;
	}

	{
		MapHelper<LightAttribs> CBConstant(m_pDeviceContext, m_UBLightAttribs, MAP_WRITE, MAP_FLAG_DISCARD);
		*CBConstant = lightAttrib;
	}

	for (auto j =0; j < m_meshes.size(); ++j)
	{
		m_meshes[j].Draw(m_pDeviceContext, false, Frustum);
	}
}

void ObjModel::DrawShadowMap(CameraAttribs &camAttrib, ViewFrustumExt &frustrum)
{
	{
		MapHelper<CameraAttribs> CameraData(m_pDeviceContext, m_UBCamera, MAP_WRITE, MAP_FLAG_DISCARD);
		*CameraData = camAttrib;
	}

	for (auto j = 0; j < m_meshes.size(); ++j)
	{
		m_meshes[j].Draw(m_pDeviceContext, true, frustrum);
	}

}


void ObjModel::loadObjFile(const std::string& path)
{
	tinyobj::attrib_t attributes;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warning;
	std::string error;

	pathModel = "F:/CustomEngine/CrownDiligentEngine/assets/Model/Sponza/";


	 bool ret = tinyobj::LoadObj(&attributes, &shapes, &materials, &warning, &error, path.c_str(), pathModel.c_str());
	
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


	//DEFAULT DIFFUSE TEXTURE
	RefCntAutoPtr<ITextureView> m_DefaultDiffuseTexture;
	TextureLoadInfo loadInfo;
	loadInfo.IsSRGB = true;
	loadInfo.Format = TEXTURE_FORMAT::TEX_FORMAT_UNKNOWN;
	RefCntAutoPtr<ITexture> TexDiffuse;
	CreateTextureFromFile("F:/CustomEngine/CrownDiligentEngine/assets/default.jpg", loadInfo, m_pDevice, &TexDiffuse);
	m_DefaultDiffuseTexture = TexDiffuse->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);

	//DEFAULT ALPHA TEXTURE
	RefCntAutoPtr<ITextureView> m_DefaultAlphaTexture;
	loadInfo.Format = TEXTURE_FORMAT::TEX_FORMAT_RGBA8_UNORM;
	RefCntAutoPtr<ITexture> TexAlpha;
	CreateTextureFromFile("F:/CustomEngine/CrownDiligentEngine/assets/alpha.png", loadInfo, m_pDevice, &TexAlpha);
	m_DefaultAlphaTexture = TexAlpha->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
	
	
	std::unordered_map<const char*, RefCntAutoPtr<ITextureView>> m_textureArray;
	
	for (auto i = 0; i < shapes.size(); i++)
	{
		Mesh meshe;
		meshe.m_name = shapes[i].name;
		float3 boundMin, boundMax;
		std::unordered_map<float3, uint32_t> uniqueVertices;
		for (const auto index : shapes[i].mesh.indices)
		{
			
			
			float3 position;
			position.x = attributes.vertices[3 * index.vertex_index];
			position.y = attributes.vertices[3 * index.vertex_index + 1];
			position.z = attributes.vertices[3 * index.vertex_index + 2];

			

			float3 normal;
			normal.x = attributes.normals[3 * index.normal_index];
			normal.y = attributes.normals[3 * index.normal_index + 1];
			normal.z = attributes.normals[3 * index.normal_index + 2];


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

			boundMin = std::min(boundMin, position);
			boundMax = std::max(boundMax, position);

			if (uniqueVertices.count(position) == 0)
			{
				uniqueVertices[position] = static_cast<uint32_t>(meshe.m_vertices.size());
				meshe.m_vertices.push_back(Mesh::Vertex{ position, normal, uv  });
			}

			
			meshe.m_indices.push_back(uniqueVertices[position]);
		}

		//TEXTURING
		if (shapes[i].mesh.material_ids[0] >= 0)
		{
			tinyobj::material_t* mp = &materials[shapes[i].mesh.material_ids[0]];

			//ALPHA
			if (mp->alpha_texname.length() >= 1)
			{
				//std::cout << "Texture Alpha: " << mp->alpha_texname.c_str() << std::endl;
				if (m_textureArray[mp->alpha_texname.c_str()])
				{
					meshe.m_alphaTextureView = m_textureArray[mp->alpha_texname.c_str()];
				}
				else
				{
					TextureLoadInfo loadInfo;
					loadInfo.Format = TEXTURE_FORMAT::TEX_FORMAT_UNKNOWN;
					RefCntAutoPtr<ITexture> Tex;
					std::string test = pathModel + mp->alpha_texname;
					CreateTextureFromFile(test.c_str(), loadInfo, m_pDevice, &Tex);
					m_textureArray[mp->alpha_texname.c_str()] = Tex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
					meshe.m_alphaTextureView = m_textureArray[mp->alpha_texname.c_str()];
				}
			}
			else
			{
				meshe.m_alphaTextureView = m_DefaultAlphaTexture;
			}

			//DIFFUSE
			if (mp->diffuse_texname.length() >= 1)
			{
				//std::cout << "Texture Diffuse: " << mp->diffuse_texname.c_str() << std::endl;
				if (m_textureArray[mp->diffuse_texname.c_str()])
				{
					meshe.m_diffuseTextureView = m_textureArray[mp->diffuse_texname.c_str()];
				}
				else
				{
					TextureLoadInfo loadInfo;
					loadInfo.IsSRGB = true;
					loadInfo.Format = TEXTURE_FORMAT::TEX_FORMAT_UNKNOWN;
					RefCntAutoPtr<ITexture> Tex;
					std::string test = pathModel + mp->diffuse_texname;
					CreateTextureFromFile(test.c_str(), loadInfo, m_pDevice, &Tex);
					m_textureArray[mp->diffuse_texname.c_str()] = Tex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
					meshe.m_diffuseTextureView = m_textureArray[mp->diffuse_texname.c_str()];
				}
				
			}
			else
			{
				meshe.m_diffuseTextureView = m_DefaultDiffuseTexture;
			}
		}
		
		meshe.m_boundingBox.Max = boundMax;
		meshe.m_boundingBox.Min = boundMin;
		//DEBUG BOUNDING BOX CONSOLE
		//std::cout << "[Mesh Name] : "  << meshe.m_name << " BoundMin : " << boundMin << " BoundMax: " << boundMax << std::endl;
		m_meshes.push_back(meshe);
		uniqueVertices.clear();
	}
}