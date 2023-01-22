#include "Mesh.hpp"


Mesh::Mesh()
{
	

}

void Mesh::CreateBuffer()
{
	//Vertex Buffer
	BufferDesc vertexBufferDesc;
	vertexBufferDesc.Name = "Model Vertex Buffer";
	vertexBufferDesc.Usage = USAGE_IMMUTABLE;
	vertexBufferDesc.BindFlags = BIND_VERTEX_BUFFER;
	vertexBufferDesc.Size = m_vertices.size() * sizeof(Vertex);
	BufferData VBData;
	VBData.DataSize = m_vertices.size() * sizeof(Vertex);
	VBData.pData = m_vertices.data();
	m_pRenderDevice->CreateBuffer(vertexBufferDesc, &VBData, &m_VerticesBuffer);

	//INDEX BUFFER
	BufferDesc indexBufferDesc;
	indexBufferDesc.Name = "Model Index Buffer";
	indexBufferDesc.Usage = USAGE_IMMUTABLE;
	indexBufferDesc.BindFlags = BIND_INDEX_BUFFER;
	indexBufferDesc.Size = m_indices.size() * sizeof(uint32_t);
	BufferData indexData;
	indexData.DataSize = m_indices.size() * sizeof(uint32_t);
	indexData.pData = m_indices.data();
	m_pRenderDevice->CreateBuffer(indexBufferDesc, &indexData, &m_IndexBuffer);
}

void Mesh::SetSystem(IRenderDevice* pRenderDevice, IDeviceContext* immediateContext,IEngineFactory* eningeFactory)
{
	m_pRenderDevice = pRenderDevice;
	m_ImmediateContext = immediateContext;
	m_EngineFactory = eningeFactory;
}

void Mesh::AssignPipeline(IPipelineState * basic_pipeline, IPipelineState* shadow_pipeline)
{
	CreateBuffer();

	m_pipeline = basic_pipeline;
	m_ShadowPSO = shadow_pipeline;

	m_pipeline->CreateShaderResourceBinding(&m_pSRB, true);

	//TODO: Optimize load Texture
	#pragma region TextureRegionBasicPipeline

	if (!m_diffuseTextureView)
	{
		TextureLoadInfo loadInfo;
		loadInfo.IsSRGB = true;
		loadInfo.Format = TEXTURE_FORMAT::TEX_FORMAT_RGBA8_UNORM;
		RefCntAutoPtr<ITexture> Tex;
		CreateTextureFromFile("F:/CustomEngine/CrownDiligentEngine/assets/default.jpg", loadInfo, m_pRenderDevice, &Tex);
		m_diffuseTextureView = Tex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
		m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2DDiffuse")->Set(m_diffuseTextureView);

	}
	else
	{
		VERIFY(m_diffuseTextureView != nullptr, "Material must have diffuse color texture");
		m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2DDiffuse")->Set(m_diffuseTextureView);
	}

	if (m_alphaTextureView)
	{
		VERIFY(m_diffuseTextureView != nullptr, "Material must have Alpha color texture");
		m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2DAlpha")->Set(m_alphaTextureView);
	}

	if (m_shadowSettings.iShadowMode == SHADOW_MODE_PCF)
	{
		m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2DShadowMap")->Set(m_shadowMapManager.GetSRV());
	}
	else
	{
		m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2DFilterableShadowMap")->Set(m_shadowMapManager.GetFilterableSRV());
	}

	
	#pragma endregion TextureRegionBasicPipeline

	m_ShadowPSO->CreateShaderResourceBinding(&m_ShadowSRB, true);
}

void Mesh::Update()
{
	
}

void Mesh::Draw(IDeviceContext* immediateContext,bool bIsShadowPass, const ViewFrustumExt& Frustum)
{
	// Note that Vulkan requires shadow map to be transitioned to DEPTH_READ state, not SHADER_RESOURCE
	immediateContext->TransitionShaderResources((bIsShadowPass ? m_ShadowPSO : m_pipeline), (bIsShadowPass ? m_ShadowSRB : m_pSRB));


	Uint64 offset[] = { 0 };
	IBuffer* pBuffs[] = { m_VerticesBuffer };
	immediateContext->SetVertexBuffers(0, 1, pBuffs, offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

	if (m_indices.size() > 0)
	{
		immediateContext->SetIndexBuffer(m_IndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		auto& pPSO = (bIsShadowPass ? m_ShadowPSO : m_pipeline);


		// Set the pipeline state in the immediate context
		m_ImmediateContext->SetPipelineState(pPSO);

		//immediateContext->CommitShaderResources(m_pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		m_ImmediateContext->CommitShaderResources((bIsShadowPass ? m_ShadowSRB : m_pSRB), RESOURCE_STATE_TRANSITION_MODE_VERIFY);


		DrawIndexedAttribs DrawAttrs; // This is an indexed draw call
		DrawAttrs.IndexType = VT_UINT32; // Index type
		DrawAttrs.NumIndices = m_indices.size();
		// Verify the state of vertex and index buffers as well as consistence of 
		// render targets and correctness of draw command arguments
		DrawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
		immediateContext->DrawIndexed(DrawAttrs);
	}
	else
	{
		auto& pPSO = (bIsShadowPass ? m_ShadowPSO : m_pipeline);
		// Set the pipeline state in the immediate context
		m_ImmediateContext->SetPipelineState(pPSO);

		m_ImmediateContext->CommitShaderResources((bIsShadowPass ? m_ShadowSRB : m_pSRB), RESOURCE_STATE_TRANSITION_MODE_VERIFY);

		DrawAttribs DrawAttrs; // This is an indexed draw call
		DrawAttrs.NumVertices = m_vertices.size();
		DrawAttrs.FirstInstanceLocation = 0;
		// Verify the state of vertex and index buffers as well as consistence of 
		// render targets and correctness of draw command arguments
		DrawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
		immediateContext->Draw(DrawAttrs);
	}
	
}


