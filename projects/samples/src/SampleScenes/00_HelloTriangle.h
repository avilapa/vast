#pragma once

#include "SampleBase.h"
#include "imgui/imgui.h"

class HelloTriangle final : public SampleBase
{
private:
	gfx::PipelineHandle m_TrianglePso;
	gfx::BufferHandle m_TriangleVtxBuf;
	uint32 m_TriangleVtxBufIdx;

	struct Vtx2fPos3fColor
	{
		s_float2 pos;
		s_float3 col;
	};

	Array<Vtx2fPos3fColor, 3> m_TriangleVertexData =
	{ {
		{{ -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }},
		{{  0.0f,  0.5f }, { 0.0f, 1.0f, 0.0f }},
		{{  0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }},
	} };

	bool m_UpdateTriangle = false;

public:
	HelloTriangle(gfx::GraphicsContext& ctx) : SampleBase(ctx)
	{
		gfx::RenderPassLayout trianglePass;
		trianglePass.renderTargets[0].format = m_GraphicsContext.GetBackBufferFormat();
		trianglePass.renderTargets[0].loadOp = gfx::LoadOp::CLEAR;
		trianglePass.renderTargets[0].storeOp = gfx::StoreOp::STORE;
		trianglePass.renderTargets[0].nextUsage = gfx::ResourceState::PRESENT;

		// TODO: Move triangle.hlsl to a project local shaders folder?
		m_TrianglePso = m_GraphicsContext.CreatePipeline(gfx::PipelineDesc::Builder()
			.SetVertexShader("triangle.hlsl", "VS_Main")
			.SetPixelShader("triangle.hlsl", "PS_Main")
			.SetDepthStencilState(gfx::DepthStencilState::Preset::kDisabled)
			.SetRenderPassLayout(trianglePass));

		auto vtxBufDesc = gfx::BufferDesc::Builder()
			.SetSize(sizeof(m_TriangleVertexData)).SetStride(sizeof(m_TriangleVertexData[0]))
			.SetViewFlags(gfx::BufferViewFlags::SRV)
			.SetCpuAccess(gfx::BufferCpuAccess::WRITE)
			.SetUsage(gfx::ResourceUsage::DYNAMIC)
			.SetIsRawAccess(true);

		m_TriangleVtxBuf = m_GraphicsContext.CreateBuffer(vtxBufDesc, &m_TriangleVertexData, sizeof(m_TriangleVertexData));
		m_TriangleVtxBufIdx = m_GraphicsContext.GetBindlessIndex(m_TriangleVtxBuf);
	}

	~HelloTriangle()
	{
		m_GraphicsContext.DestroyPipeline(m_TrianglePso);
		m_GraphicsContext.DestroyBuffer(m_TriangleVtxBuf);
	}

	void Update() override
	{
		if (m_UpdateTriangle)
		{
			m_UpdateTriangle = false;
			m_GraphicsContext.UpdateBuffer(m_TriangleVtxBuf, &m_TriangleVertexData, sizeof(m_TriangleVertexData));
		}
	}

	void Draw() override
	{
		m_GraphicsContext.BeginRenderPass(m_TrianglePso);
		{
			m_GraphicsContext.SetPushConstants(&m_TriangleVtxBufIdx, sizeof(uint32));
			m_GraphicsContext.Draw(3);
		}
		m_GraphicsContext.EndRenderPass();
	}

	void OnGUI() override
	{
		if (ImGui::Begin("vast UI", 0, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::PushItemWidth(300);
			ImGui::Text("Triangle Vertex Positions");
			if (ImGui::SliderFloat2("##1", (float*)&m_TriangleVertexData[0].pos, -1.0f, 1.0f)) m_UpdateTriangle = true;
			if (ImGui::SliderFloat2("##2", (float*)&m_TriangleVertexData[1].pos, -1.0f, 1.0f)) m_UpdateTriangle = true;
			if (ImGui::SliderFloat2("##3", (float*)&m_TriangleVertexData[2].pos, -1.0f, 1.0f)) m_UpdateTriangle = true;
			ImGui::Text("Triangle Vertex Colors");
			if (ImGui::ColorEdit3("##1", (float*)&m_TriangleVertexData[0].col)) m_UpdateTriangle = true;
			if (ImGui::ColorEdit3("##2", (float*)&m_TriangleVertexData[1].col)) m_UpdateTriangle = true;
			if (ImGui::ColorEdit3("##3", (float*)&m_TriangleVertexData[2].col)) m_UpdateTriangle = true;
			ImGui::PopItemWidth();
		}
		ImGui::End();
	}

};