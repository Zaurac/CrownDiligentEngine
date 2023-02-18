#include "CrownDiligentEngine.h"
#include "DiligentCore/Graphics/GraphicsTools/interface/MapHelper.hpp"



CrownDiligentEngine::CrownDiligentEngine()
{
}

void CrownDiligentEngine::Initialize()
{

	//DIFFUSE TEXTURE
	RefCntAutoPtr<ITexture> pTexDiffuse;
	TextureLoadInfo loadInfo;
	loadInfo.IsSRGB = true;
	CreateTextureFromFile("F:/CustomEngine/CrownDiligentEngine/assets/pbr/rustediron2_basecolor.jpg", loadInfo, m_pDevice, &pTexDiffuse);
	m_TextureSphere = pTexDiffuse->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
	RefCntAutoPtr<ITexture> pTexNorm;
	CreateTextureFromFile("F:/CustomEngine/CrownDiligentEngine/assets/pbr/rustediron2_normal.jpg", loadInfo, m_pDevice, &pTexNorm);
	m_TextureNormSphere = pTexNorm->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
	RefCntAutoPtr<ITexture> pTexPhysical;
	CreateTextureFromFile("F:/CustomEngine/CrownDiligentEngine/assets/pbr/rustediron2_roughness.jpg", loadInfo, m_pDevice, &pTexPhysical);
	m_TexturePhysicalSphere = pTexPhysical->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
	RefCntAutoPtr<ITexture> pTexMetal;
	CreateTextureFromFile("F:/CustomEngine/CrownDiligentEngine/assets/pbr/rustediron2_metallic.jpg", loadInfo, m_pDevice, &pTexMetal);
	m_TextureMetalSphere = pTexMetal->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);

	RefCntAutoPtr<ITexture> EnvironmentMaps;
	CreateTextureFromFile("F:/CustomEngine/CrownDiligentEngine/assets/papermill.ktx", TextureLoadInfo{ "Environment map" }, m_pDevice, &EnvironmentMaps);
	pEnvironmentMap = EnvironmentMaps->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);


	//PRECOMPUTE BRDF/CUBEMAP FOR PBR
	PrecomputeBRDF();
	TextureDesc TexDesc;
	TexDesc.Name = "Irradiance cube map for GLTF renderer";
	TexDesc.Type = RESOURCE_DIM_TEX_CUBE;
	TexDesc.Usage = USAGE_DEFAULT;
	TexDesc.BindFlags = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
	TexDesc.Width = 64;
	TexDesc.Height = 64;
	TexDesc.Format = TEX_FORMAT_RGBA32_FLOAT;
	TexDesc.ArraySize = 6;
	TexDesc.MipLevels = 0;

	RefCntAutoPtr<ITexture> IrradainceCubeTex;
	m_pDevice->CreateTexture(TexDesc, nullptr, &IrradainceCubeTex);
	m_pIrradianceCubeSRV = IrradainceCubeTex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);

	TexDesc.Name = "Prefiltered environment map for GLTF renderer";
	TexDesc.Width = 256;
	TexDesc.Height = 256;
	TexDesc.Format = TEX_FORMAT_RGBA16_FLOAT;
	RefCntAutoPtr<ITexture> PrefilteredEnvMapTex;
	m_pDevice->CreateTexture(TexDesc, nullptr, &PrefilteredEnvMapTex);
	m_pPrefilteredEnvMapSRV = PrefilteredEnvMapTex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
	PrecomputeCubemaps();
	

	for (uint32_t y = 0; y < GRID_SIZE; y++) {
		for (uint32_t x = 0; x < GRID_SIZE; x++) {
			Sphere* pSphere = new Sphere(m_pSwapChain, m_pDevice, m_pEngineFactory, m_pImmediateContext);
			float3 test = float3(float(x - (GRID_SIZE / 2.0f)) * 2.5f, 0.0f, float(y - (GRID_SIZE / 2.0f)) * 2.5f);
			pSphere->m_SpherePos = test;
			pSphere->SetTexture("g_DiffTexture", m_TextureSphere);
			pSphere->SetTexture("g_NormTexture", m_TextureNormSphere);
			pSphere->SetTexture("g_RougTexture", m_TexturePhysicalSphere);
			pSphere->SetTexture("g_MetalTexture", m_TextureMetalSphere);

			//IBL
			pSphere->SetTexture("g_IrradianceMap", m_pIrradianceCubeSRV);
			pSphere->SetTexture("g_PrefilteredEnvMap", m_pPrefilteredEnvMapSRV);
			pSphere->SetTexture("g_BRDF_LUT", m_pBRDF_LUT_SRV);
			
			m_spheres.push_back(pSphere);
		}
	}

	m_pCubeInstanced = new CubeInstanced(m_pSwapChain, m_pDevice, m_pEngineFactory, m_pImmediateContext);
	//m_pCube = new Sphere(m_pSwapChain, m_pDevice, m_pEngineFactory, m_pImmediateContext);
	m_ShadowMapMgr = std::make_unique<ShadowMapManager>();
	m_model = new ObjModel(m_pSwapChain, m_pDevice, m_pEngineFactory, m_pImmediateContext);

	m_LigthAttribs.ShadowAttribs.iNumCascades = 4;
	m_LigthAttribs.ShadowAttribs.fFixedDepthBias = 0.0025f;
	m_LigthAttribs.ShadowAttribs.iFixedFilterSize = 5;
	m_LigthAttribs.ShadowAttribs.fFilterWorldSize = 0.1f;
	m_LigthAttribs.ShadowAttribs.bVisualizeCascades = false;

	m_LigthAttribs.f4Direction = float3(-0.522699475f, -10.481321275f, -1.703671455f);
	m_LigthAttribs.f4Intensity = float4(1, 0.8f, 0.5f, 1);
	m_LigthAttribs.f4AmbientLight = float4(0.125f, 0.125f, 0.125f, 1);

	CreateShadowMap();

	m_model->loadObjFile("F:/CustomEngine/CrownDiligentEngine/assets/Model/Sponza/sponza.obj");
	m_model->AssignLightAttribs(m_LigthAttribs);
	for (size_t i = 0; i < m_model->m_meshes.size(); i++)
	{
		m_model->m_meshes[i].AssignShadowManager(*m_ShadowMapMgr);
	}
	m_model->CreatePipeline();

	m_CameraPos = float3(0, 0, 0);

	m_worldMatrix = float4x4::Translation(0,0,0);
	cubeModelMatrix = float4x4::Translation(2, 1, 0) * float4x4::Scale(50);

	// Get projection matrix adjusted to the current screen orientation
	m_camera.projMatrix = GetAdjustedProjectionMatrix(PI_F / 4.0f, 0.1f, 25000.f);
}

void CrownDiligentEngine::Update(double CurrTime, double ElapsedTime)
{
	// Camera is at (0, 0, -5) looking along the Z axis
	

	//m_LigthAttribs.f4Direction = float4(m_LigthAttribs.f4Direction.x , m_LigthAttribs.f4Direction.y -= 10.f * ElapsedTime, m_LigthAttribs.f4Direction.z, m_LigthAttribs.f4Direction.w);
	UpdateCamera(ElapsedTime);

	// Get pretransform matrix that rotates the scene according the surface orientation
	auto SrfPreTransform = GetSurfacePretransformMatrix(float3{ 0, 0, 1 });
	
	// Compute world-view-projection matrix
	m_WorldViewProjMatrix = m_worldMatrix * m_camera.viewMatrix * SrfPreTransform * m_camera.projMatrix;
	m_camAttribs.mViewProjT = m_WorldViewProjMatrix;
	m_pCubeInstanced->Update(ElapsedTime, m_WorldViewProjMatrix);
	
	m_model->update(ElapsedTime, m_camAttribs);

	m_WorldViewProjMatrix = cubeModelMatrix * m_camera.viewMatrix * SrfPreTransform * m_camera.projMatrix;
	//m_pCube->Update(ElapsedTime, m_WorldViewProjMatrix);

	Sphere::UBOMatrices uboSphere;
	uboSphere.camPos = m_CameraPos;
	uboSphere.WorldViewProj = m_WorldViewProjMatrix.Transpose();
	uboSphere.projection = m_camera.projMatrix.Transpose();
	uboSphere.view = m_camera.viewMatrix.Transpose();
	uboSphere.model = cubeModelMatrix.Transpose();
	//m_pCube->setUboMatrice(uboSphere);

	Sphere::CustomLightAttribs test2;
	test2.f4AmbientLight = m_LigthAttribs.f4AmbientLight;
	test2.f4Direction = m_LigthAttribs.f4Direction;
	test2.f4Intensity = m_LigthAttribs.f4Intensity;

	for (size_t i = 0; i < m_spheres.size(); i++)
	{
		
		m_spheres[i]->SetLigthAttribs(test2);
		cubeModelMatrix = float4x4::Translation(m_spheres[i]->m_SpherePos) * float4x4::Scale(50);
		m_WorldViewProjMatrix = cubeModelMatrix * m_camera.viewMatrix * SrfPreTransform * m_camera.projMatrix;
		uboSphere.camPos = m_CameraPos;
		uboSphere.WorldViewProj = m_WorldViewProjMatrix.Transpose();
		uboSphere.projection = m_camera.projMatrix.Transpose();
		uboSphere.view = m_camera.viewMatrix.Transpose();
		uboSphere.model = cubeModelMatrix.Transpose();
		m_spheres[i]->setUboMatrice(uboSphere);
	}

	#pragma region Shadow Cascade

	//Distribute SHADOW CASCADE
	ShadowMapManager::DistributeCascadeInfo DistrInfo;
	DistrInfo.pCameraView = &m_camera.viewMatrix;
	DistrInfo.pCameraProj = &m_camera.projMatrix;
	float3 f3LightDirection = float3(m_LigthAttribs.f4Direction.x, m_LigthAttribs.f4Direction.y, m_LigthAttribs.f4Direction.z);
	DistrInfo.pLightDir = &f3LightDirection;

	DistrInfo.fPartitioningFactor = m_shadowSettings.PartitioningFactor;
	DistrInfo.SnapCascades = m_shadowSettings.SnapCascades;
	DistrInfo.EqualizeExtents = m_shadowSettings.EqualizeExtents;
	DistrInfo.StabilizeExtents = m_shadowSettings.StabilizeExtents;

	m_ShadowMapMgr->DistributeCascades(DistrInfo, m_LigthAttribs.ShadowAttribs);
#pragma endregion Shadow Cascade
}

void CrownDiligentEngine::Render()
{
	RenderShadowMap();
	
	// Reset default framebuffer
	auto* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();
	auto* pDSV = m_pSwapChain->GetDepthBufferDSV();
	m_pImmediateContext->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	// Clear the back buffer
	const float ClearColor[] = { 0.133f, 0.185f, 0.255f, 0.8f };
	m_pImmediateContext->ClearRenderTarget(pRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	m_pImmediateContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	
	// Get pretransform matrix that rotates the scene according the surface orientation
	auto SrfPreTransform = GetSurfacePretransformMatrix(float3{ 0, 0, 1 });
	
	const auto  CameraView = m_camera.viewMatrix * SrfPreTransform;
	const auto& CameraWorld = m_worldMatrix;
	float3      CameraWorldPos = float3::MakeVector(CameraWorld[3]);
	const auto& CamProj = m_camera.projMatrix;

	auto CameraViewProj = CameraView * CamProj;
	m_camAttribs.mProjT = CamProj.Transpose();
	m_camAttribs.mViewProjT = CameraViewProj.Transpose();
	m_camAttribs.mViewProjInvT = CameraViewProj.Inverse().Transpose();
	m_camAttribs.f4Position = float4(m_CameraPos, 1);

	ViewFrustumExt Frutstum;
	ExtractViewFrustumPlanesFromMatrix(CameraViewProj, Frutstum, m_pDevice->GetDeviceInfo().IsGLDevice());
	//SPONZA MODEL
	m_model->draw(m_camAttribs, m_LigthAttribs ,Frutstum);
	//m_pCubeInstanced->Draw();


	//m_pCube->Draw();

	for (size_t i = 0; i < m_spheres.size(); i++)
	{
		m_spheres[i]->Draw();
	}
	
}

void CrownDiligentEngine::ShutDown()
{
	m_pComparisonSampler->Release();
	m_pFilterableShadowMapSampler->Release();
	m_ShadowMapMgr.reset();
	m_ShadowMapMgr.release();
	delete(m_pCubeInstanced);
	delete(m_model);
	//delete(m_pCube);
}

void CrownDiligentEngine::UpdateCamera(float ElapsedTime)
{
	double mouseX,mouseY;
	glfwGetCursorPos(m_Window,&mouseX, &mouseY);

	float3 MoveDirection = float3(0,0,0);
	if (glfwGetKey(m_Window, GLFW_KEY_W) == GLFW_PRESS)
	{
		MoveDirection.z += 1.0f;
	}
	if (glfwGetKey(m_Window, GLFW_KEY_S) == GLFW_PRESS)
	{
		MoveDirection.z -= 1.0f;
	}

	if (glfwGetKey(m_Window, GLFW_KEY_A) == GLFW_PRESS)
	{
		MoveDirection.x -= 1.0f;
	}

	if (glfwGetKey(m_Window, GLFW_KEY_D) == GLFW_PRESS)
	{
		MoveDirection.x += 1.0f;
	}
	
	

	auto len = length(MoveDirection);
	if(len !=0)
		MoveDirection /= len;

	MoveDirection *= m_fMoveSpeed;

	m_fCurrentSpeed = length(MoveDirection);

	float3 PosDelta = MoveDirection;

	if (glfwGetMouseButton(m_Window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
	{
		yaw += (float)(mouseX - lastMouseX) * m_fRotateSpeed;
		pitch += (float)(mouseY - lastMouseY) * m_fRotateSpeed;
		glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
	else
	{
		glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}

	
	lastMouseX = mouseX;
	lastMouseY = mouseY;

	pitch = clamp(pitch, -90.0f, 90.0f);

	float4x4 ReferenceRotation = GetReferenceRotiation();
	float4x4 CameraRotation = float4x4::RotationArbitrary(m_ReferenceUpAxis, -yaw) *
		float4x4::RotationArbitrary(m_ReferenceRightAxis, -pitch) *
		ReferenceRotation;
	float4x4 WorldRotation = CameraRotation.Transpose();

	float3 PosDeltaWorld = PosDelta * WorldRotation * ElapsedTime;
	m_CameraPos += PosDeltaWorld;
	m_camera.viewMatrix = float4x4::Translation(-m_CameraPos) * CameraRotation;
}

void CrownDiligentEngine::CreateShadowMap()
{
	if (m_shadowSettings.Resolution >= 2048)
		m_LigthAttribs.ShadowAttribs.fFixedDepthBias = 0.0025f;
	else if (m_shadowSettings.Resolution >= 1024)
		m_LigthAttribs.ShadowAttribs.fFixedDepthBias = 0.005f;
	else
		m_LigthAttribs.ShadowAttribs.fFixedDepthBias = 0.0075f;

	ShadowMapManager::InitInfo SMMgrInitInfo;
	SMMgrInitInfo.Format = m_shadowSettings.Format;
	SMMgrInitInfo.Resolution = m_shadowSettings.Resolution;
	SMMgrInitInfo.NumCascades = static_cast<Uint32>(m_LigthAttribs.ShadowAttribs.iNumCascades);
	SMMgrInitInfo.ShadowMode = m_shadowSettings.iShadowMode;
	SMMgrInitInfo.Is32BitFilterableFmt = m_shadowSettings.Is32BitFilterableFmt;

	if (!m_pComparisonSampler)
	{
		SamplerDesc ComparsionSampler;
		ComparsionSampler.ComparisonFunc = COMPARISON_FUNC_LESS;
		// Note: anisotropic filtering requires SampleGrad to fix artifacts at
		// cascade boundaries
		ComparsionSampler.MinFilter = FILTER_TYPE_COMPARISON_LINEAR;
		ComparsionSampler.MagFilter = FILTER_TYPE_COMPARISON_LINEAR;
		ComparsionSampler.MipFilter = FILTER_TYPE_COMPARISON_LINEAR;
		m_pDevice->CreateSampler(ComparsionSampler, &m_pComparisonSampler);
	}
	SMMgrInitInfo.pComparisonSampler = m_pComparisonSampler;

	if (!m_pFilterableShadowMapSampler)
	{
		SamplerDesc SamplerDesc;
		SamplerDesc.MinFilter = FILTER_TYPE_ANISOTROPIC;
		SamplerDesc.MagFilter = FILTER_TYPE_ANISOTROPIC;
		SamplerDesc.MipFilter = FILTER_TYPE_ANISOTROPIC;
		SamplerDesc.MaxAnisotropy = m_LigthAttribs.ShadowAttribs.iMaxAnisotropy;
		m_pDevice->CreateSampler(SamplerDesc, &m_pFilterableShadowMapSampler);
	}
	SMMgrInitInfo.pFilterableShadowMapSampler = m_pFilterableShadowMapSampler;

	m_ShadowMapMgr->Initialize(m_pDevice, nullptr, SMMgrInitInfo);
}

void CrownDiligentEngine::RenderShadowMap()
{
	auto iNumShadowCascades = m_LigthAttribs.ShadowAttribs.iNumCascades;
	for (int iCascade = 0; iCascade < iNumShadowCascades; ++iCascade)
	{
		const auto CascadeProjMatr = m_ShadowMapMgr->GetCascadeTranform(iCascade).Proj;

		auto WorldToLightViewSpaceMatr = m_LigthAttribs.ShadowAttribs.mWorldToLightViewT.Transpose();
		auto WorldToLightProjSpaceMatr = WorldToLightViewSpaceMatr * CascadeProjMatr;

		CameraAttribs ShadowCameraAttribs = {};

		ShadowCameraAttribs.mViewT = m_LigthAttribs.ShadowAttribs.mWorldToLightViewT;
		ShadowCameraAttribs.mProjT = CascadeProjMatr.Transpose();
		ShadowCameraAttribs.mViewProjT = WorldToLightProjSpaceMatr.Transpose();

		ShadowCameraAttribs.f4ViewportSize.x = static_cast<float>(m_shadowSettings.Resolution);
		ShadowCameraAttribs.f4ViewportSize.y = static_cast<float>(m_shadowSettings.Resolution);
		ShadowCameraAttribs.f4ViewportSize.z = 1.f / ShadowCameraAttribs.f4ViewportSize.x;
		ShadowCameraAttribs.f4ViewportSize.w = 1.f / ShadowCameraAttribs.f4ViewportSize.y;

		auto* pCascadeDSV = m_ShadowMapMgr->GetCascadeDSV(iCascade);
		m_pImmediateContext->SetRenderTargets(0, nullptr, pCascadeDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		m_pImmediateContext->ClearDepthStencil(pCascadeDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		ViewFrustumExt Frutstumm;
		ExtractViewFrustumPlanesFromMatrix(WorldToLightProjSpaceMatr, Frutstumm, m_pDevice->GetDeviceInfo().IsGLDevice());
		m_model->DrawShadowMap(ShadowCameraAttribs, Frutstumm);

	}
	if (m_shadowSettings.iShadowMode > SHADOW_MODE_PCF)
		m_ShadowMapMgr->ConvertToFilterable(m_pImmediateContext, m_LigthAttribs.ShadowAttribs);
}

void CrownDiligentEngine::PrecomputeCubemaps()
{
	struct PrecomputeEnvMapAttribs
	{
		float4x4 Rotation;

		float Roughness;
		float EnvMapDim;
		uint  NumSamples;
		float Dummy;
	};

	// SHADER FACTORY
	RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
	m_pEngineFactory->CreateDefaultShaderSourceStreamFactory("F:/CustomEngine/CrownDiligentEngine/assets", &pShaderSourceFactory);

	if (!m_PrecomputeEnvMapAttribsCB)
	{
		CreateUniformBuffer(m_pDevice, sizeof(PrecomputeEnvMapAttribs), "Precompute env map attribs CB", &m_PrecomputeEnvMapAttribsCB);
	}

	if (!m_pPrecomputeIrradianceCubePSO)
	{
		ShaderCreateInfo ShaderCI;
		ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
		ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;

		ShaderMacroHelper Macros;
		Macros.AddShaderMacro("NUM_PHI_SAMPLES", 64);
		Macros.AddShaderMacro("NUM_THETA_SAMPLES", 32);
		ShaderCI.Macros = Macros;
		RefCntAutoPtr<IShader> pVS;
		{
			ShaderCI.Desc = { "Cubemap face VS", SHADER_TYPE_VERTEX, true };
			ShaderCI.EntryPoint = "main";
			ShaderCI.FilePath = "CubemapFace.vsh";

			m_pDevice->CreateShader(ShaderCI, &pVS);
		}

		// Create pixel shader
		RefCntAutoPtr<IShader> pPS;
		{
			ShaderCI.Desc = { "Precompute irradiance cube map PS", SHADER_TYPE_PIXEL, true };
			ShaderCI.EntryPoint = "main";
			ShaderCI.FilePath = "ComputeIrradianceMap.psh";

			m_pDevice->CreateShader(ShaderCI, &pPS);
		}

		GraphicsPipelineStateCreateInfo PSOCreateInfo;
		PipelineStateDesc& PSODesc = PSOCreateInfo.PSODesc;
		GraphicsPipelineDesc& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

		PSODesc.Name = "Precompute irradiance cube PSO";
		PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

		GraphicsPipeline.NumRenderTargets = 1;
		GraphicsPipeline.RTVFormats[0] = TEX_FORMAT_RGBA32_FLOAT;
		GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_NONE;
		GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

		PSOCreateInfo.pVS = pVS;
		PSOCreateInfo.pPS = pPS;

		PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
		// clang-format off
		ShaderResourceVariableDesc Vars[] =
		{
			{SHADER_TYPE_PIXEL, "g_EnvironmentMap", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
		};
		// clang-format on
		PSODesc.ResourceLayout.NumVariables = _countof(Vars);
		PSODesc.ResourceLayout.Variables = Vars;

		// clang-format off
		ImmutableSamplerDesc ImtblSamplers[] =
		{
			{SHADER_TYPE_PIXEL, "g_EnvironmentMap", Sam_LinearClamp}
		};
		// clang-format on
		PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);
		PSODesc.ResourceLayout.ImmutableSamplers = ImtblSamplers;

		m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPrecomputeIrradianceCubePSO);
		m_pPrecomputeIrradianceCubePSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "cbTransform")->Set(m_PrecomputeEnvMapAttribsCB);
		m_pPrecomputeIrradianceCubePSO->CreateShaderResourceBinding(&m_pPrecomputeIrradianceCubeSRB, true);
	}

	if (!m_pPrefilterEnvMapPSO)
	{
		ShaderCreateInfo ShaderCI;
		ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
		ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;

		ShaderMacroHelper Macros;
		Macros.AddShaderMacro("OPTIMIZE_SAMPLES", 1);
		ShaderCI.Macros = Macros;

		RefCntAutoPtr<IShader> pVS;
		{
			ShaderCI.Desc = { "Cubemap face VS", SHADER_TYPE_VERTEX, true };
			ShaderCI.EntryPoint = "main";
			ShaderCI.FilePath = "CubemapFace.vsh";

			m_pDevice->CreateShader(ShaderCI, &pVS);
		}

		// Create pixel shader
		RefCntAutoPtr<IShader> pPS;
		{
			ShaderCI.Desc = { "Prefilter environment map PS", SHADER_TYPE_PIXEL, true };
			ShaderCI.EntryPoint = "main";
			ShaderCI.FilePath = "PrefilterEnvMap.psh";

			m_pDevice->CreateShader(ShaderCI, &pPS);
		}

		GraphicsPipelineStateCreateInfo PSOCreateInfo;
		PipelineStateDesc& PSODesc = PSOCreateInfo.PSODesc;
		GraphicsPipelineDesc& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

		PSODesc.Name = "Prefilter environment map PSO";
		PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

		GraphicsPipeline.NumRenderTargets = 1;
		GraphicsPipeline.RTVFormats[0] = TEX_FORMAT_RGBA32_FLOAT;
		GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_NONE;
		GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

		PSOCreateInfo.pVS = pVS;
		PSOCreateInfo.pPS = pPS;

		PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
		// clang-format off
		ShaderResourceVariableDesc Vars[] =
		{
			{SHADER_TYPE_PIXEL, "g_EnvironmentMap", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
		};
		// clang-format on
		PSODesc.ResourceLayout.NumVariables = _countof(Vars);
		PSODesc.ResourceLayout.Variables = Vars;

		// clang-format off
		ImmutableSamplerDesc ImtblSamplers[] =
		{
			{SHADER_TYPE_PIXEL, "g_EnvironmentMap", Sam_LinearClamp}
		};
		// clang-format on
		PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);
		PSODesc.ResourceLayout.ImmutableSamplers = ImtblSamplers;

		m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPrefilterEnvMapPSO);
		m_pPrefilterEnvMapPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "cbTransform")->Set(m_PrecomputeEnvMapAttribsCB);
		m_pPrefilterEnvMapPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "FilterAttribs")->Set(m_PrecomputeEnvMapAttribsCB);
		m_pPrefilterEnvMapPSO->CreateShaderResourceBinding(&m_pPrefilterEnvMapSRB, true);
	}


	// clang-format off
	const std::array<float4x4, 6> Matrices =
	{
		/* +X */ float4x4::RotationY(+PI_F / 2.f),
		/* -X */ float4x4::RotationY(-PI_F / 2.f),
		/* +Y */ float4x4::RotationX(-PI_F / 2.f),
		/* -Y */ float4x4::RotationX(+PI_F / 2.f),
		/* +Z */ float4x4::Identity(),
		/* -Z */ float4x4::RotationY(PI_F)
	};
	// clang-format on

	m_pImmediateContext->SetPipelineState(m_pPrecomputeIrradianceCubePSO);
	m_pPrecomputeIrradianceCubeSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_EnvironmentMap")->Set(pEnvironmentMap);
	m_pImmediateContext->CommitShaderResources(m_pPrecomputeIrradianceCubeSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	auto* pIrradianceCube = m_pIrradianceCubeSRV->GetTexture();
	const auto& IrradianceCubeDesc = pIrradianceCube->GetDesc();
	for (Uint32 mip = 0; mip < IrradianceCubeDesc.MipLevels; ++mip)
	{
		for (Uint32 face = 0; face < 6; ++face)
		{
			TextureViewDesc RTVDesc{ "RTV for irradiance cube texture", TEXTURE_VIEW_RENDER_TARGET, RESOURCE_DIM_TEX_2D_ARRAY };
			RTVDesc.MostDetailedMip = mip;
			RTVDesc.FirstArraySlice = face;
			RTVDesc.NumArraySlices = 1;
			RefCntAutoPtr<ITextureView> pRTV;
			pIrradianceCube->CreateView(RTVDesc, &pRTV);
			ITextureView* ppRTVs[] = { pRTV };
			m_pImmediateContext->SetRenderTargets(_countof(ppRTVs), ppRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
			{
				MapHelper<PrecomputeEnvMapAttribs> Attribs(m_pImmediateContext, m_PrecomputeEnvMapAttribsCB, MAP_WRITE, MAP_FLAG_DISCARD);
				Attribs->Rotation = Matrices[face];
			}
			DrawAttribs drawAttrs(4, DRAW_FLAG_VERIFY_ALL);
			m_pImmediateContext->Draw(drawAttrs);
		}
	}

	m_pImmediateContext->SetPipelineState(m_pPrefilterEnvMapPSO);
	m_pPrefilterEnvMapSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_EnvironmentMap")->Set(pEnvironmentMap);
	m_pImmediateContext->CommitShaderResources(m_pPrefilterEnvMapSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	auto* pPrefilteredEnvMap = m_pPrefilteredEnvMapSRV->GetTexture();
	const auto& PrefilteredEnvMapDesc = pPrefilteredEnvMap->GetDesc();
	for (Uint32 mip = 0; mip < PrefilteredEnvMapDesc.MipLevels; ++mip)
	{
		for (Uint32 face = 0; face < 6; ++face)
		{
			TextureViewDesc RTVDesc{ "RTV for prefiltered env map cube texture", TEXTURE_VIEW_RENDER_TARGET, RESOURCE_DIM_TEX_2D_ARRAY };
			RTVDesc.MostDetailedMip = mip;
			RTVDesc.FirstArraySlice = face;
			RTVDesc.NumArraySlices = 1;
			RefCntAutoPtr<ITextureView> pRTV;
			pPrefilteredEnvMap->CreateView(RTVDesc, &pRTV);
			ITextureView* ppRTVs[] = { pRTV };
			m_pImmediateContext->SetRenderTargets(_countof(ppRTVs), ppRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

			{
				MapHelper<PrecomputeEnvMapAttribs> Attribs(m_pImmediateContext, m_PrecomputeEnvMapAttribsCB, MAP_WRITE, MAP_FLAG_DISCARD);
				Attribs->Rotation = Matrices[face];
				Attribs->Roughness = static_cast<float>(mip) / static_cast<float>(PrefilteredEnvMapDesc.MipLevels);
				Attribs->EnvMapDim = static_cast<float>(PrefilteredEnvMapDesc.Width);
				Attribs->NumSamples = 256;
			}

			DrawAttribs drawAttrs(4, DRAW_FLAG_VERIFY_ALL);
			m_pImmediateContext->Draw(drawAttrs);
		}
	}

	// clang-format off
	StateTransitionDesc Barriers[] =
	{
		{m_pPrefilteredEnvMapSRV->GetTexture(), RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_SHADER_RESOURCE, STATE_TRANSITION_FLAG_UPDATE_STATE},
		{m_pIrradianceCubeSRV->GetTexture(),    RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_SHADER_RESOURCE, STATE_TRANSITION_FLAG_UPDATE_STATE}
	};
	// clang-format on
	m_pImmediateContext->TransitionResourceStates(_countof(Barriers), Barriers);

	// To avoid crashes on some low-end Android devices
	m_pImmediateContext->Flush();
}

void CrownDiligentEngine::PrecomputeBRDF()
{
	TextureDesc TexDesc;
	TexDesc.Name = "GLTF BRDF Look-up texture";
	TexDesc.Type = RESOURCE_DIM_TEX_2D;
	TexDesc.Usage = USAGE_DEFAULT;
	TexDesc.BindFlags = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
	TexDesc.Width = 512;
	TexDesc.Height = 512;
	TexDesc.Format = TEX_FORMAT_RG16_FLOAT;
	TexDesc.MipLevels = 1;
	RefCntAutoPtr<ITexture> pBRDF_LUT;
	m_pDevice->CreateTexture(TexDesc,nullptr, &pBRDF_LUT);
	m_pBRDF_LUT_SRV = pBRDF_LUT->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);

	RefCntAutoPtr<IPipelineState> PrecomputeBRDF_PSO;
	{
		GraphicsPipelineStateCreateInfo PSOCreateInfo;
		PipelineStateDesc& PSODesc = PSOCreateInfo.PSODesc;
		GraphicsPipelineDesc& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

		PSODesc.Name = "Precompute GLTF BRDF LUT PSO";
		PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

		GraphicsPipeline.NumRenderTargets = 1;
		GraphicsPipeline.RTVFormats[0] = TexDesc.Format;
		GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_NONE;
		GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

		// SHADER FACTORY
		RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
		m_pEngineFactory->CreateDefaultShaderSourceStreamFactory("F:/CustomEngine/CrownDiligentEngine/assets", &pShaderSourceFactory);

		ShaderCreateInfo ShaderCI;
		ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
		ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
		RefCntAutoPtr<IShader> pVS;
		{
			ShaderCI.Desc = { "Full screen triangle VS", SHADER_TYPE_VERTEX, true };
			ShaderCI.EntryPoint = "FullScreenTriangleVS";
			ShaderCI.FilePath = "FullScreenTriangleVS.fx";

			m_pDevice->CreateShader(ShaderCI, &pVS);
		}

		// Create pixel shader
		RefCntAutoPtr<IShader> pPS;
		{
			ShaderCI.Desc = { "Precompute GLTF BRDF PS", SHADER_TYPE_PIXEL, true };
			ShaderCI.EntryPoint = "PrecomputeBRDF_PS";
			ShaderCI.FilePath = "PrecomputeGLTF_BRDF.psh";

			m_pDevice->CreateShader(ShaderCI, &pPS);
		}

		// Finally, create the pipeline state
		PSOCreateInfo.pVS = pVS;
		PSOCreateInfo.pPS = pPS;
		m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &PrecomputeBRDF_PSO);
	}
	m_pImmediateContext->SetPipelineState(PrecomputeBRDF_PSO);

	ITextureView* pRTVs[] = { pBRDF_LUT->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET) };
	m_pImmediateContext->SetRenderTargets(1, pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	DrawAttribs attrs(3, DRAW_FLAG_VERIFY_ALL);
	m_pImmediateContext->Draw(attrs);

	// clang-format off
	StateTransitionDesc Barriers[] =
	{
		{pBRDF_LUT, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_SHADER_RESOURCE, STATE_TRANSITION_FLAG_UPDATE_STATE}
	};
	// clang-format on
	m_pImmediateContext->TransitionResourceStates(_countof(Barriers), Barriers);
}

#include <algorithm>

float4x4 CrownDiligentEngine::GetReferenceRotiation() const
{
	// clang-format off
	return float4x4
	{
		m_ReferenceRightAxis.x, m_ReferenceUpAxis.x, m_ReferenceAheadAxis.x, 0,
		m_ReferenceRightAxis.y, m_ReferenceUpAxis.y, m_ReferenceAheadAxis.y, 0,
		m_ReferenceRightAxis.z, m_ReferenceUpAxis.z, m_ReferenceAheadAxis.z, 0,
							 0,                   0,                      0, 1
	};
	// clang-format on
}

void CrownDiligentEngine::WindowResize(Uint32 Width, Uint32 Height)
{
	float NearPlane = 0.1f;
	float FarPlane = 25000.f;
	float AspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
	m_camera.projMatrix = GetAdjustedProjectionMatrix(PI_F / 4.f, NearPlane, FarPlane);
}

DEFINE_APLLICATION_MAIN(CrownDiligentEngine)


