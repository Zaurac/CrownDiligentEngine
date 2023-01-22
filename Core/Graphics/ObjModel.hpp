#pragma once
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

#include "../../../DiligentTools/TextureLoader/interface/TextureLoader.h"
#include "../../../DiligentTools/TextureLoader/interface/TextureUtilities.h"

#include "../../DiligentCore/Graphics/GraphicsTools/interface/MapHelper.hpp"
#include "../../DiligentCore/Common/interface/AdvancedMath.hpp"
#include "../../DiligentCore/Common/interface/BasicMath.hpp"
#include "../../DiligentCore/Common/interface/RefCntAutoPtr.hpp"
#include "../../DiligentCore/Common/interface/BasicMath.hpp"

#include "../../DiligentCore/Graphics/GraphicsTools/interface/ShaderMacroHelper.hpp"

#include "Mesh.hpp"

#include "tiny_obj_loader.h"

#define TINYOBJLOADER_IMPLEMENTATION

using namespace Diligent;


class ObjModel
{
private:
	struct Vertex
	{
		float3 position;
		float3 normal;
		float2 uv;
	};

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	
	CameraAttribs m_CameraAttribs;

public:
	ObjModel(ISwapChain* pSwapChain, IRenderDevice* pRenderDevice, IEngineFactory* pEngineFactory, IDeviceContext* pDeviceContext);
	~ObjModel() = default;
	void loadObjFile(const std::string &path);
	void AssignLightAttribs(LightAttribs lightAttribs) { m_pLightAttribs = lightAttribs;};
	void CreatePipeline();
	void CreateShader();
	void update(float eplapsedTime, CameraAttribs cameraAttribs);
	void draw(CameraAttribs camAttrib, LightAttribs lightAttrib, const ViewFrustumExt& Frustum);
	void DrawShadowMap(CameraAttribs &camAttrib, ViewFrustumExt& frustrum);
	std::vector<Mesh> m_meshes;
private:

	//BASIC PIPELINE
	RefCntAutoPtr<IShader> pVS;
	RefCntAutoPtr<IShader> pPS;
	RefCntAutoPtr<IEngineFactory> m_pEngineFactory;
	RefCntAutoPtr<IRenderDevice> m_pDevice;
	RefCntAutoPtr<ISwapChain> m_pSwapChain;
	RefCntAutoPtr<IDeviceContext> m_pDeviceContext;
	RefCntAutoPtr<IPipelineState> m_pBasicPSO;
	RefCntAutoPtr<IBuffer> m_UBCamera;
	RefCntAutoPtr<IBuffer> m_UBLightAttribs;

	//SHADOW PIEPLINE
	RefCntAutoPtr<IPipelineState> m_ShadowPSO;
	RefCntAutoPtr<IShader> m_ShadowVS;

	ShadowSettings m_ShadowSettings;
	LightAttribs m_pLightAttribs;
};

