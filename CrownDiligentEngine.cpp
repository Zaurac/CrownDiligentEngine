#include "CrownDiligentEngine.h"
#include "DiligentCore/Graphics/GraphicsTools/interface/MapHelper.hpp"


CrownDiligentEngine::CrownDiligentEngine()
{
}

void CrownDiligentEngine::Initialize()
{

	m_pCube = new Cube(m_pSwapChain, m_pDevice, m_pEngineFactory, m_pImmediateContext);
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
		m_model->m_meshes[i].AssignShadowManager(m_ShadowMapMgr);
	}
	

	m_model->CreatePipeline();



	m_CameraPos = float3(0, 0, 5);

	// Get projection matrix adjusted to the current screen orientation
	m_camera.projMatrix = GetAdjustedProjectionMatrix(PI_F / 4.0f, 0.1f, 25000.f);
}

void CrownDiligentEngine::Update(double CurrTime, double ElapsedTime)
{
	// Camera is at (0, 0, -5) looking along the Z axis
	m_worldMatrix = float4x4::Translation(m_CameraPos);

	//m_LigthAttribs.f4Direction = float4(m_LigthAttribs.f4Direction.x , m_LigthAttribs.f4Direction.y -= 10.f * ElapsedTime, m_LigthAttribs.f4Direction.z, m_LigthAttribs.f4Direction.w);
	UpdateCamera(ElapsedTime);

	// Get pretransform matrix that rotates the scene according the surface orientation
	auto SrfPreTransform = GetSurfacePretransformMatrix(float3{ 0, 0, 1 });
	
	// Compute world-view-projection matrix
	m_WorldViewProjMatrix = m_camera.viewMatrix * SrfPreTransform * m_camera.projMatrix;
	m_camAttribs.mViewProjT = m_WorldViewProjMatrix;
	m_pCube->Update(ElapsedTime, m_WorldViewProjMatrix);
	m_model->update(ElapsedTime, m_camAttribs);


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

	m_ShadowMapMgr.DistributeCascades(DistrInfo, m_LigthAttribs.ShadowAttribs);
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
	m_model->draw(m_camAttribs, m_LigthAttribs ,Frutstum);
	m_pCube->Draw();
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

	m_ShadowMapMgr.Initialize(m_pDevice, nullptr, SMMgrInitInfo);
}

void CrownDiligentEngine::ShutDown()
{

	m_pComparisonSampler->Release();
	m_pFilterableShadowMapSampler->Release();
	delete(m_pCube);
	delete(m_model);
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

	float3 PosDelta = MoveDirection * ElapsedTime;

	if (glfwGetMouseButton(m_Window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
	{
		yaw += (float)(mouseX - lastMouseX) * 0.1f * ElapsedTime;
		pitch += (float)(mouseY - lastMouseY) * 0.1f * ElapsedTime;
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

	float3 PosDeltaWorld = PosDelta * WorldRotation;
	m_CameraPos += PosDeltaWorld;

	m_camera.viewMatrix = float4x4::Translation(-m_CameraPos) * CameraRotation;
}

void CrownDiligentEngine::RenderShadowMap()
{
	auto iNumShadowCascades = m_LigthAttribs.ShadowAttribs.iNumCascades;
	for (int iCascade = 0; iCascade < iNumShadowCascades; ++iCascade)
	{
		const auto CascadeProjMatr = m_ShadowMapMgr.GetCascadeTranform(iCascade).Proj;

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

		auto* pCascadeDSV = m_ShadowMapMgr.GetCascadeDSV(iCascade);
		m_pImmediateContext->SetRenderTargets(0, nullptr, pCascadeDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		m_pImmediateContext->ClearDepthStencil(pCascadeDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		ViewFrustumExt Frutstumm;
		ExtractViewFrustumPlanesFromMatrix(WorldToLightProjSpaceMatr, Frutstumm, m_pDevice->GetDeviceInfo().IsGLDevice());
		m_model->DrawShadowMap(ShadowCameraAttribs, Frutstumm);

	}
		if (m_shadowSettings.iShadowMode > SHADOW_MODE_PCF)
			m_ShadowMapMgr.ConvertToFilterable(m_pImmediateContext, m_LigthAttribs.ShadowAttribs);
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


