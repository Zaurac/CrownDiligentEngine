#include "GLFWSample.h"
#include "DiligentCore/Common/interface/AdvancedMath.hpp"
#include "DiligentCore/Common/interface/BasicMath.hpp"
#include "Core/Graphics/Primary/CubeInstance.hpp"
#include "DiligentCore/Graphics/GraphicsTools/interface/CommonlyUsedStates.h"
#include "DiligentCore/Graphics/GraphicsTools/interface/GraphicsUtilities.h"
#include "Core/Graphics/Primary/Sphere.hpp"
#include "Core/Graphics/ObjModel.hpp"
using namespace Diligent;

	class CrownDiligentEngine : public GLFWSample
	{

	public:
		CrownDiligentEngine();
		~CrownDiligentEngine() = default;

		void Initialize() override;


		void Update(double CurrTime, double ElapsedTime) override;
		//CUBEMAP BRDF
		void PrecomputeCubemaps();
		void PrecomputeBRDF();

		void Render() override;
		void CreateShadowMap();


		void ShutDown() override;

		void UpdateCamera(float ElapsedTime);
		void RenderShadowMap();

		float4x4 GetReferenceRotiation() const;
		

		void WindowResize(Uint32 Width, Uint32 Height) override;

		RefCntAutoPtr<ITextureView> m_TextureSphere;
		RefCntAutoPtr<ITextureView> m_TextureNormSphere;
		RefCntAutoPtr<ITextureView> m_TexturePhysicalSphere;
		RefCntAutoPtr<ITextureView> m_TextureMetalSphere;

		//CUBEMAP
		RefCntAutoPtr<IShaderResourceBinding> m_pPrecomputeIrradianceCubeSRB;
		RefCntAutoPtr<IShaderResourceBinding> m_pPrefilterEnvMapSRB;
		RefCntAutoPtr<ITextureView> pEnvironmentMap;
		RefCntAutoPtr<ITextureView> m_pIrradianceCubeSRV;
		RefCntAutoPtr<ITextureView> m_pPrefilteredEnvMapSRV;
		RefCntAutoPtr<IPipelineState> m_pPrefilterEnvMapPSO;
		RefCntAutoPtr<IPipelineState> m_pPrecomputeIrradianceCubePSO;
		RefCntAutoPtr<IBuffer> m_PrecomputeEnvMapAttribsCB;

		//BRDF
		RefCntAutoPtr<ITextureView> m_pBRDF_LUT_SRV;

	private:
		CubeInstanced * m_pCubeInstanced = NULL;
		//Sphere*m_pCube = NULL;

		std::vector<Sphere*> m_spheres;
		std::vector<std::string> materialNames;
		std::vector<float3> m_spheresPos;
		int32_t materialIndex = 8;
		int GRID_SIZE = 7;

		ObjModel *m_model = NULL;
		float4x4 cubeModelMatrix;

		//SHADOW
		CameraAttribs m_camAttribs;
		LightAttribs m_LigthAttribs;
		
		ShadowSettings m_shadowSettings;
		std::unique_ptr<ShadowMapManager> m_ShadowMapMgr;

		RefCntAutoPtr<ISampler> m_pComparisonSampler;
		RefCntAutoPtr<ISampler> m_pFilterableShadowMapSampler;

		struct Camera 
		{
			float4x4 viewMatrix;
			float4x4 projMatrix;
		}m_camera;

		//CAMERA + MOUSE
		float4x4 m_WorldViewProjMatrix;
		float4x4 m_worldMatrix;

		//Camera
		float3 m_ReferenceRightAxis = float3{ 1, 0, 0 };
		float3 m_ReferenceUpAxis = float3{ 0, 1, 0 };
		float3 m_ReferenceAheadAxis = float3{ 0, 0, 1 };

		double lastMouseX = 0.0, lastMouseY = 0.0;
		float yaw = 0.0f, pitch = 0.0f;
		
		float3 m_CameraPos;
		float    m_fMoveSpeed = 500.f;
		float    m_fRotateSpeed = 0.001f;
		float    m_fCurrentSpeed = 0.f;
	};