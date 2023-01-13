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

using namespace Diligent;

class Mesh
{
public:
	struct Vertex
	{
		float3 position;
		float2 uv;
	};

	float4x4 m_ModelMatrix;

	RefCntAutoPtr<IBuffer> m_UniformBuffer;
	RefCntAutoPtr<IBuffer> m_IndexBuffer;
	RefCntAutoPtr<IBuffer> m_VerticesBuffer;

	std::vector<Vertex> m_vertices;
	std::vector<uint32_t> m_indices;

	RefCntAutoPtr<ITextureView> m_diffuseTextureView;
	RefCntAutoPtr<IPipelineState> m_pipeline;
	RefCntAutoPtr<IShaderResourceBinding> m_pSRB;

	void SetSystem(IRenderDevice* pRenderDevice, IDeviceContext* immediateContext);
	void CreatePipeline(IPipelineState* pipeline);
	void Update(float4x4 matrix);
	void Draw(IDeviceContext* immediateContext, bool bIsShadowPass, const ViewFrustumExt& Frustum);

public:
	Mesh();
	~Mesh() = default;

private:
	void CreateBuffer();
	RefCntAutoPtr<IRenderDevice> m_pRenderDevice;
	RefCntAutoPtr<IDeviceContext> m_ImmediateContext;
};