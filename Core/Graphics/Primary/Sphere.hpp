#include "../../../DiligentCore/Graphics/GraphicsTools/interface/MapHelper.hpp"
#include "../../../DiligentCore/Common/interface/BasicMath.hpp"
#include "../../../DiligentCore/Common/interface/RefCntAutoPtr.hpp"

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

#include "../../../DiligentCore/Graphics/GraphicsTools/interface/ShaderMacroHelper.hpp"

#include "../../../DiligentTools/TextureLoader/interface/TextureLoader.h"
#include "../../../DiligentTools/TextureLoader/interface/TextureUtilities.h"

#include <vector>

using namespace Diligent;

class Sphere
{
public:
	float3 m_SpherePos;

	struct UBOMatrices {
		float4x4 WorldViewProj;
		float4x4 projection;
		float4x4 model;
		float4x4 view;
		float3 camPos;
	} m_uboMatrices;

	void setUboMatrice(UBOMatrices ubo) { m_uboMatrices = ubo;};

	struct CustomLightAttribs
	{
		float4 f4Direction;
		float4 f4AmbientLight;
		float4 f4Intensity;
	};

	void SetTexture(const char* name, ITextureView* texture);

private:
	void CreateSphere();
	void CreatePipeline();
	void LoadTexture();

	struct Vertex
	{
		float3 pos;
		float2 UV;
		float3 Normal;
		float3 Tangent;
		float3 BiTangent;
	};

	

	RefCntAutoPtr<IPipelineState> m_pPSO;
	RefCntAutoPtr<IShaderResourceBinding> m_pSRB;


	RefCntAutoPtr<ITextureView> m_TextureSRV;
	//RefCntAutoPtr<ITextureView> m_TextureNormal;
	//RefCntAutoPtr<ITextureView> m_TextureMetal;
	//RefCntAutoPtr<ITextureView> m_TextureRoug;
	//RefCntAutoPtr<ITextureView> m_TextureAO;


	RefCntAutoPtr<IBuffer> m_VSConstants;
	RefCntAutoPtr<IBuffer> m_UBOMaterialConstants;
	
	RefCntAutoPtr<IBuffer> m_ObjConstants;

	RefCntAutoPtr<IBuffer> m_UBOLightAttribs;

	
	CustomLightAttribs m_lightAttrib;

	RefCntAutoPtr<IBuffer> m_SphereVertexBuffer;
	RefCntAutoPtr<IBuffer>  m_SphereIndexBuffer;

	RefCntAutoPtr<IEngineFactory> m_pEngineFactory;
	RefCntAutoPtr<IRenderDevice> m_pDevice;
	RefCntAutoPtr<ISwapChain> m_pSwapChain;
	RefCntAutoPtr<IDeviceContext> m_pDeviceContext;

	std::vector<Vertex> m_vertices;
	std::vector<Uint32> m_indices;
	unsigned int indexCount;

	int nbrInstances = 10;
	int numTextures = 3;

public:
	Sphere(ISwapChain* pSwapChain,IRenderDevice* pRenderDevice, IEngineFactory* pEngineFactory, IDeviceContext* pDeviceContext);
	void Update(float eplapsedTime, float4x4 matrix);
	void Draw();
	void ShutDown();
	void SetLigthAttribs(CustomLightAttribs &lightAttribs) { m_lightAttrib = lightAttribs;};
	~Sphere();

	
};