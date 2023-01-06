#include "CrownDiligentEngine.h"
#include "DiligentCore/Graphics/GraphicsTools/interface/MapHelper.hpp"

CrownDiligentEngine::CrownDiligentEngine()
{
}

void CrownDiligentEngine::Initialize()
{
	
	
	//manager.SetDisplaySize(m_pSwapChain->GetDesc().Width, m_pSwapChain->GetDesc().Height);

	m_pCube = new Cube(m_pSwapChain, m_pDevice, m_pEngineFactory, m_pImmediateContext);
	//m_pCube = std::make_unique<Cube>(m_pSwapChain, m_pDevice, m_pEngineFactory, m_pImmediateContext);

	//pos = float3(0,0,5);
	m_CameraPos = float3(0, 0, 5);
}

void CrownDiligentEngine::Update(double CurrTime, double ElapsedTime)
{
	// Camera is at (0, 0, -5) looking along the Z axis
	

	UpdateCamera(ElapsedTime);

	//View = float4x4::Translation(pos);

	// Apply rotation
	float4x4 CubeModelTransform = float4x4::RotationY(static_cast<float>(CurrTime) * 0.0f) * float4x4::RotationX(-PI_F * 0.0f);

	

	// Get pretransform matrix that rotates the scene according the surface orientation
	auto SrfPreTransform = GetSurfacePretransformMatrix(float3{ 0, 0, 1 });

	// Get projection matrix adjusted to the current screen orientation
	auto Proj = GetAdjustedProjectionMatrix(PI_F / 4.0f, 0.1f, 1000.f);

	// Compute world-view-projection matrix
	m_WorldViewProjMatrix = CubeModelTransform * m_ViewMatrix * SrfPreTransform * Proj;
	m_pCube->Update(ElapsedTime, m_WorldViewProjMatrix);

	
}

void CrownDiligentEngine::Render()
{
	
	// Clear the back buffer
	const float ClearColor[] = { 0.350f, 0.350f, 0.350f, 1.0f };
	// Let the engine perform required state transitions
	auto* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();
	auto* pDSV = m_pSwapChain->GetDepthBufferDSV();
	m_pImmediateContext->ClearRenderTarget(pRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	m_pImmediateContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	m_pCube->Draw();

	
}

void CrownDiligentEngine::ShutDown()
{
	m_pCube->ShutDown();
	//m_pCube.release();
	delete(m_pCube);
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

	m_ViewMatrix = float4x4::Translation(-m_CameraPos) * CameraRotation;
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

DEFINE_APLLICATION_MAIN(CrownDiligentEngine)