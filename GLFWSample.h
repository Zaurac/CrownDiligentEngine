#pragma region regionName

#ifndef ENGINE_DLL
#    define ENGINE_DLL 1
#endif

#ifndef D3D11_SUPPORTED
#    define D3D11_SUPPORTED 0
#endif

#ifndef D3D12_SUPPORTED
#    define D3D12_SUPPORTED 0
#endif

#ifndef GL_SUPPORTED
#    define GL_SUPPORTED 0
#endif

#ifndef VULKAN_SUPPORTED
#    define VULKAN_SUPPORTED 0
#endif

#ifndef METAL_SUPPORTED
#    define METAL_SUPPORTED 0
#endif

#if PLATFORM_WIN32
#    define GLFW_EXPOSE_NATIVE_WIN32 1
#endif

#if PLATFORM_LINUX
#    define GLFW_EXPOSE_NATIVE_X11 1
#endif

#if PLATFORM_MACOS
#    define GLFW_EXPOSE_NATIVE_COCOA 1
#endif

#pragma endregion regionName

#if D3D11_SUPPORTED
#    include "../../../Graphics/GraphicsEngineD3D11/interface/EngineFactoryD3D11.h"
#endif
#if D3D12_SUPPORTED
#    include "../../../Graphics/GraphicsEngineD3D12/interface/EngineFactoryD3D12.h"
#endif
#if GL_SUPPORTED
#    include "../../../Graphics/GraphicsEngineOpenGL/interface/EngineFactoryOpenGL.h"
#endif
#if VULKAN_SUPPORTED
#    include "../../../Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h"
#endif
#if METAL_SUPPORTED
#    include "../../../Graphics/GraphicsEngineMetal/interface/EngineFactoryMtl.h"
#endif

//STD
#include <chrono>
#include <vector>
#include <iostream>

//DILIGENT
#include "DiligentCore/Common/interface/RefCntAutoPtr.hpp"
#include "RenderDevice.h"
#include "DeviceContext.h"
#include "SwapChain.h"
#include "DiligentTools/RenderStateNotation/interface/RenderStateNotationLoader.h"
#include "DiligentCore/Common/interface/BasicMath.hpp"
#include "DiligentCore/Common/interface/Timer.hpp"

//GLFW
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

//IMGUI
#include "DiligentTools/ThirdParty/imgui/imgui.h"
#include "DiligentTools/Imgui/interface/ImGuiImplWin32.hpp"

//CUSTOM
#include "Core/Profiler.hpp"

using namespace Diligent;

RefCntAutoPtr<IEngineFactory> m_pEngineFactory;
RefCntAutoPtr<IRenderDevice>  m_pDevice;
RefCntAutoPtr<IDeviceContext> m_pImmediateContext;
RefCntAutoPtr<ISwapChain>     m_pSwapChain;

GLFWwindow* m_Window = nullptr;

IEngineFactory* GetEngineFactory() { return m_pDevice->GetEngineFactory(); }
IRenderDevice* GetRenderDevice() { return m_pDevice; }
IDeviceContext* GetContext() { return m_pImmediateContext; }
ISwapChain* GetSwapChain() { return m_pSwapChain; }

std::unique_ptr<ImGuiImplWin32>  m_pImGui;
Profiler m_Profiler;

class GLFWSample
{
public:
	GLFWSample() = default;
	~GLFWSample() = default;

	virtual void Initialize() = 0;
	virtual void Update(double CurrTime, double ElapsedTime) = 0;
	virtual void Render() = 0;
	virtual void ShutDown() = 0;
	virtual void WindowResize(Uint32 Width, Uint32 Height) = 0;

	// Returns projection matrix adjusted to the current screen orientation
	float4x4 GetAdjustedProjectionMatrix(float FOV, float NearPlane, float FarPlane) const
	{
		const auto& SCDesc = m_pSwapChain->GetDesc();

		float AspectRatio = static_cast<float>(SCDesc.Width) / static_cast<float>(SCDesc.Height);
		float XScale, YScale;
		if (SCDesc.PreTransform == SURFACE_TRANSFORM_ROTATE_90 ||
			SCDesc.PreTransform == SURFACE_TRANSFORM_ROTATE_270 ||
			SCDesc.PreTransform == SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90 ||
			SCDesc.PreTransform == SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270)
		{
			// When the screen is rotated, vertical FOV becomes horizontal FOV
			XScale = 1.f / std::tan(FOV / 2.f);
			// Aspect ratio is inversed
			YScale = XScale * AspectRatio;
		}
		else
		{
			YScale = 1.f / std::tan(FOV / 2.f);
			XScale = YScale / AspectRatio;
		}

		float4x4 Proj;
		Proj._11 = XScale;
		Proj._22 = YScale;
		Proj.SetNearFarClipPlanes(NearPlane, FarPlane, m_pDevice->GetDeviceInfo().IsGLDevice());

		Proj = float4x4::Projection(FOV, AspectRatio, NearPlane, FarPlane, m_pDevice->GetDeviceInfo().IsGLDevice());

		return Proj;
	}

	// Returns pretransform matrix that matches the current screen rotation
	float4x4 GetSurfacePretransformMatrix(const float3& f3CameraViewAxis) const
	{
		const auto& SCDesc = m_pSwapChain->GetDesc();
		switch (SCDesc.PreTransform)
		{
		case SURFACE_TRANSFORM_ROTATE_90:
			// The image content is rotated 90 degrees clockwise.
			return float4x4::RotationArbitrary(f3CameraViewAxis, -PI_F / 2.f);

		case SURFACE_TRANSFORM_ROTATE_180:
			// The image content is rotated 180 degrees clockwise.
			return float4x4::RotationArbitrary(f3CameraViewAxis, -PI_F);

		case SURFACE_TRANSFORM_ROTATE_270:
			// The image content is rotated 270 degrees clockwise.
			return float4x4::RotationArbitrary(f3CameraViewAxis, -PI_F * 3.f / 2.f);

		case SURFACE_TRANSFORM_OPTIMAL:
			UNEXPECTED("SURFACE_TRANSFORM_OPTIMAL is only valid as parameter during swap chain initialization.");
			return float4x4::Identity();

		case SURFACE_TRANSFORM_HORIZONTAL_MIRROR:
		case SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90:
		case SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180:
		case SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270:
			UNEXPECTED("Mirror transforms are not supported");
			return float4x4::Identity();

		default:
			return float4x4::Identity();
		}
	}

private:

	
};

int m_key;
int m_state;
bool windowResize;

static void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);

	m_key = key;
	m_state = action;
}

static void resize_callback(GLFWwindow* window, int width, int height)
{
	m_pSwapChain->Resize(static_cast<Uint32>(width), static_cast<Uint32>(height));
	windowResize = true;
}

int WindowGLFWMain(int argc, char** argv, GLFWSample* app)
{
	if (glfwInit() != GLFW_TRUE)
		return false;

	RENDER_DEVICE_TYPE DevType = RENDER_DEVICE_TYPE_D3D11;
	//if (!Samp->ProcessCommandLine(argc, argv, DevType))
		//return -1;

	int APIHint = GLFW_NO_API;
#if !PLATFORM_WIN32
	if (DevType == RENDER_DEVICE_TYPE_GL)
	{
		// On platforms other than Windows Diligent Engine
		// attaches to existing OpenGL context
		APIHint = GLFW_OPENGL_API;
	}
#endif
	

	glfwWindowHint(GLFW_CLIENT_API, APIHint);
	if (APIHint == GLFW_OPENGL_API)
	{
		// We need compute shaders, so request OpenGL 4.2 at least
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	}

	m_Window = glfwCreateWindow(800, 600, "crown", nullptr, nullptr);
	if (m_Window == nullptr)
	{
		LOG_ERROR_MESSAGE("Failed to create GLFW window");
		return false;
	}

	

	glfwSetWindowSizeLimits(m_Window, 320, 240, GLFW_DONT_CARE, GLFW_DONT_CARE);

	glfwMakeContextCurrent(m_Window);
	glfwSwapInterval(0);

#if PLATFORM_WIN32
	Win32NativeWindow Window{ glfwGetWin32Window(m_Window) };
#endif
#if PLATFORM_LINUX
	LinuxNativeWindow Window;
	Window.WindowId = glfwGetX11Window(m_Window);
	Window.pDisplay = glfwGetX11Display();
	if (DevType == RENDER_DEVICE_TYPE_GL)
		glfwMakeContextCurrent(m_Window);
#endif
#if PLATFORM_MACOS
	MacOSNativeWindow Window;
	if (DevType == RENDER_DEVICE_TYPE_GL)
		glfwMakeContextCurrent(m_Window);
	else
		Window.pNSView = GetNSWindowView(m_Window);
#endif

	glfwSetErrorCallback(error_callback);
	glfwSetFramebufferSizeCallback(m_Window, resize_callback);
	glfwSetKeyCallback(m_Window, key_callback);

	SwapChainDesc SCDesc;
	//Change buffer count for DOULE OR TRIPLE BUFFERING
	SCDesc.BufferCount = 2;
	switch (DevType)
	{
#if D3D11_SUPPORTED
	case RENDER_DEVICE_TYPE_D3D11:
	{
#    if ENGINE_DLL
		// Load the dll and import GetEngineFactoryD3D11() function
		auto* GetEngineFactoryD3D11 = LoadGraphicsEngineD3D11();
#    endif
		auto* pFactoryD3D11 = GetEngineFactoryD3D11();
		m_pEngineFactory = pFactoryD3D11;
		

		EngineD3D11CreateInfo EngineCI;
		pFactoryD3D11->CreateDeviceAndContextsD3D11(EngineCI, &m_pDevice, &m_pImmediateContext);
		pFactoryD3D11->CreateSwapChainD3D11(m_pDevice, m_pImmediateContext, SCDesc, FullScreenModeDesc{}, Window, &m_pSwapChain);
		m_pImGui.reset(new ImGuiImplWin32((HWND)Window.hWnd, m_pDevice, SCDesc.ColorBufferFormat, SCDesc.DepthBufferFormat));

	}
	break;
#endif // D3D11_SUPPORTED


#if D3D12_SUPPORTED
	case RENDER_DEVICE_TYPE_D3D12:
	{
#    if ENGINE_DLL
		// Load the dll and import GetEngineFactoryD3D12() function
		auto* GetEngineFactoryD3D12 = LoadGraphicsEngineD3D12();
#    endif
		auto* pFactoryD3D12 = GetEngineFactoryD3D12();
		m_pEngineFactory = pFactoryD3D12;

		EngineD3D12CreateInfo EngineCI;
		pFactoryD3D12->CreateDeviceAndContextsD3D12(EngineCI, &m_pDevice, &m_pImmediateContext);
		pFactoryD3D12->CreateSwapChainD3D12(m_pDevice, m_pImmediateContext, SCDesc, FullScreenModeDesc{}, Window, &m_pSwapChain);

		m_pImGui.reset(new ImGuiImplWin32((HWND)Window.hWnd, m_pDevice, SCDesc.ColorBufferFormat, SCDesc.DepthBufferFormat));
	}
	break;
#endif // D3D12_SUPPORTED


#if GL_SUPPORTED
	case RENDER_DEVICE_TYPE_GL:
	{
#    if EXPLICITLY_LOAD_ENGINE_GL_DLL
		// Load the dll and import GetEngineFactoryOpenGL() function
		auto GetEngineFactoryOpenGL = LoadGraphicsEngineOpenGL();
#    endif
		auto* pFactoryOpenGL = GetEngineFactoryOpenGL();
		m_pEngineFactory = pFactoryOpenGL;

		EngineGLCreateInfo EngineCI;
		EngineCI.Window = Window;
		pFactoryOpenGL->CreateDeviceAndSwapChainGL(EngineCI, &m_pDevice, &m_pImmediateContext, SCDesc, &m_pSwapChain);
	
		m_pImGui.reset( new ImGuiImplWin32((HWND)Window.hWnd, m_pDevice, SCDesc.ColorBufferFormat, SCDesc.DepthBufferFormat));
	}
	break;
#endif // GL_SUPPORTED


#if VULKAN_SUPPORTED
	case RENDER_DEVICE_TYPE_VULKAN:
	{
#    if EXPLICITLY_LOAD_ENGINE_VK_DLL
		// Load the dll and import GetEngineFactoryVk() function
		auto* GetEngineFactoryVk = LoadGraphicsEngineVk();
#    endif
		EngineVkCreateInfo EngineCI;

		const char* const ppIgnoreDebugMessages[] = //
		{
			// Validation Performance Warning: [ UNASSIGNED-CoreValidation-Shader-OutputNotConsumed ]
			// vertex shader writes to output location 1.0 which is not consumed by fragment shader
			"UNASSIGNED-CoreValidation-Shader-OutputNotConsumed" //
		};
		EngineCI.ppIgnoreDebugMessageNames = ppIgnoreDebugMessages;
		EngineCI.IgnoreDebugMessageCount = _countof(ppIgnoreDebugMessages);

		auto* pFactoryVk = GetEngineFactoryVk();
		m_pEngineFactory = pFactoryVk;

		
		pFactoryVk->CreateDeviceAndContextsVk(EngineCI, &m_pDevice, &m_pImmediateContext);
		pFactoryVk->CreateSwapChainVk(m_pDevice, m_pImmediateContext, SCDesc, Window, &m_pSwapChain);

		m_pImGui.reset(new ImGuiImplWin32((HWND)Window.hWnd, m_pDevice, SCDesc.ColorBufferFormat, SCDesc.DepthBufferFormat));
	}
	break;
#endif // VULKAN_SUPPORTED

#if METAL_SUPPORTED
	case RENDER_DEVICE_TYPE_METAL:
	{
		auto* pFactoryMtl = GetEngineFactoryMtl();

		EngineMtlCreateInfo EngineCI;
		pFactoryMtl->CreateDeviceAndContextsMtl(EngineCI, &m_pDevice, &m_pImmediateContext);
		pFactoryMtl->CreateSwapChainMtl(m_pDevice, m_pImmediateContext, SCDesc, Window, &m_pSwapChain);
	}
	break;
#endif // METAL_SUPPORTED

	default:
		std::cerr << "Unknown/unsupported device type";
		return false;
		break;
	}

	if (m_pDevice == nullptr || m_pImmediateContext == nullptr || m_pSwapChain == nullptr)
		return false;

	

	if (m_pDevice->GetDeviceInfo().Features.TimestampQueries)
	{
		m_Profiler.Initialize(m_pDevice);
	}

	app->Initialize();

	Diligent::Timer Timer;
	auto   PrevTime = Timer.GetElapsedTime();
	double filteredFrameTime = 0.0;

	

	while (!glfwWindowShouldClose(m_Window))
	{

		glfwPollEvents();

		auto CurrTime = Timer.GetElapsedTime();
		auto ElapsedTime = CurrTime - PrevTime;
		PrevTime = CurrTime;

		

		const auto& SCDesc = m_pSwapChain->GetDesc();
		m_pImGui->NewFrame(SCDesc.Width, SCDesc.Height, SCDesc.PreTransform);

		m_Profiler.Update(ElapsedTime);
		m_Profiler.UpdateUI();

		app->Update(CurrTime, ElapsedTime);

		if (windowResize)
		{
			app->WindowResize(SCDesc.Width, SCDesc.Height);
			windowResize = false;
		}

		auto* pContext = GetContext();
		auto* pSwapchain = GetSwapChain();
		

		// Clear the back buffer
		//const float ClearColor[] = { 0.350f, 0.350f, 0.350f, 1.0f };
		// Let the engine perform required state transitions
		//auto* pRTV = pSwapchain->GetCurrentBackBufferRTV();
		//auto* pDSV = pSwapchain->GetDepthBufferDSV();

		//pContext->SetRenderTargets(1, &pRTV, pDSV, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		const float DebugColor[] = { 1.f, 0.f, 0.f, 1.f };
		m_pImmediateContext->BeginDebugGroup("Graphics", DebugColor);

		m_Profiler.Begin(pContext, Profiler::GRAPHICS_1);
		app->Render();
		m_Profiler.End(pContext, Profiler::GRAPHICS_1);

		m_pImmediateContext->EndDebugGroup();

		//pContext->SetRenderTargets(1, &pRTV, pDSV, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		if (m_pImGui)
		{
			//No need to call EndFrame as ImGui::Render calls it automatically
			m_pImGui->Render(m_pImmediateContext);
		}
		else
		{
			LOG_INFO_MESSAGE(DEBUG_MESSAGE_SEVERITY_INFO, "m_pImgui == nullptr");
		}

		double filterScale = 0.2;
		filteredFrameTime = filteredFrameTime * (1.0 - filterScale) + filterScale * ElapsedTime;
		std::stringstream fpsCounterSS;
		fpsCounterSS << "" << " - " << std::fixed << std::setprecision(1) << filteredFrameTime * 1000;
		fpsCounterSS << " ms (" << 1.0 / filteredFrameTime << " fps)";

		//std::cout << fpsCounterSS.str() << std::endl;

		pSwapchain->Present(0);
	}

	//m_pImGui->EndFrame();

	
	app->ShutDown();

	

	if (m_pImmediateContext)
		m_pImmediateContext->Flush();

	m_pSwapChain = nullptr;
	m_pImmediateContext = nullptr;
	m_pDevice = nullptr;

	if (m_Window)
	{
		glfwDestroyWindow(m_Window);
		glfwTerminate();
		
	}
	exit(EXIT_SUCCESS);
}



#define DEFINE_APLLICATION_MAIN(appclass)							\
extern int WindowGLFWMain(int argc, char** argv, GLFWSample* app);  \
																	\
int main(int argc, char** argv)										\
{																	\
  /* code for main function goes here */							\
  appclass app;														\
  return WindowGLFWMain(argc, argv, &app);							\
}