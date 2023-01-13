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


		void ShutDown() override;

		void UpdateCamera(float ElapsedTime);


		float4x4 GetReferenceRotiation() const;

		
	private:
		//std::unique_ptr<Cube> m_pCube;
		Cube * m_pCube = NULL;
		ObjModel *m_model = NULL;

		float4x4 m_WorldViewProjMatrix;
		//float4x4 View;

		//Camera
		float3 m_ReferenceRightAxis = float3{ 1, 0, 0 };
		float3 m_ReferenceUpAxis = float3{ 0, 1, 0 };
		float3 m_ReferenceAheadAxis = float3{ 0, 0, 1 };

		double lastMouseX = 0.0, lastMouseY = 0.0;
		float yaw = 0.0f, pitch = 0.0f;
		
		float3 m_CameraPos;
		float4x4 m_ViewMatrix;
		float    m_fMoveSpeed = 100.f;
		float    m_fCurrentSpeed = 0.f;

	};