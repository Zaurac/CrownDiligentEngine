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

#include "../../../DiligentTools/TextureLoader/interface/TextureLoader.h"
#include "../../../DiligentTools/TextureLoader/interface/TextureUtilities.h"

#include <vector>

using namespace Diligent;

class Cube
{
private:
	void CreateCube();
	void CreatePipeline();
	void LoadTexture();

	struct VertexCube
	{
		float3 pos;
		float2 UV;
	};

	RefCntAutoPtr<IPipelineState> m_pPSO;
	RefCntAutoPtr<IShaderResourceBinding> m_pSRB;
	RefCntAutoPtr<ITextureView> m_TextureSRV;
	RefCntAutoPtr<IBuffer> m_VSConstants;
	RefCntAutoPtr<IBuffer> m_CubeVertexBuffer;
	RefCntAutoPtr<IBuffer>  m_CubeIndexBuffer;

	RefCntAutoPtr<IEngineFactory> m_pEngineFactory;
	RefCntAutoPtr<IRenderDevice> m_pDevice;
	RefCntAutoPtr<ISwapChain> m_pSwapChain;
	RefCntAutoPtr<IDeviceContext> m_pDeviceContext;

	std::vector<VertexCube> m_vertices;
	std::vector<Uint32> m_indices;

	float4x4 m_ModelMatrix;

	int nbrInstances = 10;
	int numTextures = 3;

public:
	Cube(ISwapChain* pSwapChain,IRenderDevice* pRenderDevice, IEngineFactory* pEngineFactory, IDeviceContext* pDeviceContext);
	void Update(float eplapsedTime, float4x4 matrix);
	void Draw();
	void ShutDown();
	~Cube();


};