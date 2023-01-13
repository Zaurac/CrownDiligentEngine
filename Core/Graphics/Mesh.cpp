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

	//UNIFORM BUFFER
	BufferDesc CBDesc;
	CBDesc.Name = "VS Constant CB";
	CBDesc.Size = sizeof(float4x4);
	CBDesc.Usage = USAGE_DYNAMIC;
	CBDesc.BindFlags = BIND_UNIFORM_BUFFER;
	CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
	m_pRenderDevice->CreateBuffer(CBDesc, nullptr, &m_UniformBuffer);
}

void Mesh::SetSystem(IRenderDevice* pRenderDevice, IDeviceContext* immediateContext)
{
	m_pRenderDevice = pRenderDevice;
	m_ImmediateContext = immediateContext;
}

void Mesh::CreatePipeline(IPipelineState * pipeline)
{
	CreateBuffer();

	m_pipeline = pipeline;

	m_pipeline->CreateShaderResourceBinding(&m_pSRB, true);

	if (!m_diffuseTextureView)
	{
		TextureLoadInfo loadInfo;
		loadInfo.IsSRGB = true;
		loadInfo.Format = TEXTURE_FORMAT::TEX_FORMAT_RGBA8_UNORM;
		RefCntAutoPtr<ITexture> Tex;
		CreateTextureFromFile("F:/CustomEngine/CrownDiligentEngine/assets/default.jpg", loadInfo, m_pRenderDevice, &Tex);
		m_diffuseTextureView = Tex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
		m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(m_diffuseTextureView);

	}
	else
	{
		m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(m_diffuseTextureView);
	}
	if (!m_alphaTextureView)
	{
		TextureLoadInfo loadInfo;
		loadInfo.IsSRGB = true;
		loadInfo.Format = TEXTURE_FORMAT::TEX_FORMAT_RGBA8_UNORM;
		RefCntAutoPtr<ITexture> Tex;
		CreateTextureFromFile("F:/CustomEngine/CrownDiligentEngine/assets/alpha.png", loadInfo, m_pRenderDevice, &Tex);
		m_alphaTextureView = Tex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
		m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_AlphaTexture")->Set(m_alphaTextureView);
	}
	else
	{
		m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_AlphaTexture")->Set(m_alphaTextureView);
	}
	
}

void Mesh::Update(float4x4 matrix)
{
	m_ModelMatrix = matrix;
}

void Mesh::Draw(IDeviceContext* immediateContext,bool bIsShadowPass, const ViewFrustumExt& Frustum)
{
	

	Uint64 offset[] = { 0 };
	IBuffer* pBuffs[] = { m_VerticesBuffer };
	immediateContext->SetVertexBuffers(0, 1, pBuffs, offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

	if (m_indices.size() > 0)
	{
		immediateContext->SetIndexBuffer(m_IndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		// Set the pipeline state in the immediate context
		m_ImmediateContext->SetPipelineState(m_pipeline);

		immediateContext->CommitShaderResources(m_pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

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
		// Set the pipeline state in the immediate context
		m_ImmediateContext->SetPipelineState(m_pipeline);

		immediateContext->CommitShaderResources(m_pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		DrawAttribs DrawAttrs; // This is an indexed draw call
		DrawAttrs.NumVertices = m_vertices.size();
		DrawAttrs.FirstInstanceLocation = 0;
		// Verify the state of vertex and index buffers as well as consistence of 
		// render targets and correctness of draw command arguments
		DrawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
		immediateContext->Draw(DrawAttrs);
	}
	
}


