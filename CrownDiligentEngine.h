#include "GLFWSample.h"
#include "DiligentCore/Common/interface/AdvancedMath.hpp"
#include "DiligentCore/Common/interface/BasicMath.hpp"
#include "Core/Graphics/Primary/Cube.hpp"
#include "Core/Graphics/ObjModel.hpp"
using namespace Diligent;

	class CrownDiligentEngine : public GLFWSample
	{

	public:
		CrownDiligentEngine();
		~CrownDiligentEngine() = default;

		void Initialize() override;


		void Update(double CurrTime, double ElapsedTime) override;


		void Render() override;
		void CreateShadowMap();


		void ShutDown() override;

		void UpdateCamera(float ElapsedTime);
		void RenderShadowMap();

		float4x4 GetReferenceRotiation() const;
		

		void WindowResize(Uint32 Width, Uint32 Height) override;

	private:
		Cube * m_pCube = NULL;
		ObjModel *m_model = NULL;

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
		float    m_fCurrentSpeed = 0.f;
	};