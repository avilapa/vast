#pragma once

#include "ISample.h"
#include "Rendering/Imgui.h"

using namespace vast;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Hello Triangle 
 * --------------
 * This sample implements the classic multi-color triangle as an introduction to the gfx API in
 * vast. As an extra, the triangle vertices can be edited from a simple graphical user interface.
 * 
 * All code for this sample is contained within this file plus a simple 'triangle.hlsl' shader file.
 * 
 * Topics: render to back buffer, push constants, bindless vertex buffer, user interface
 * 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
class HelloTriangle final : public ISample
{
private:
	PipelineHandle m_TrianglePso;
	BufferHandle m_TriangleVtxBuf;
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

	bool m_bUpdateTriangle = false;

public:
	HelloTriangle(GraphicsContext& ctx_) : ISample(ctx_)
	{
		// Create triangle pipeline state object and prepare it to render to the back buffer.
		PipelineDesc trianglePipelineDesc =
		{
			.vs = AllocVertexShaderDesc("00_Triangle.hlsl", m_SamplesShaderSourcePath.c_str()),
			.ps = AllocPixelShaderDesc("00_Triangle.hlsl", m_SamplesShaderSourcePath.c_str()),
			.depthStencilState = DepthStencilState::Preset::kDisabled,
			.renderPassLayout = {.rtFormats = { ctx.GetBackBufferFormat() }, },
		};
		m_TrianglePso = ctx.CreatePipeline(trianglePipelineDesc);

		// Create the triangle vertex buffer with an SRV to be able to access it bindlessly from the shader.
		BufferDesc vtxBufDesc =
		{
			.size	= sizeof(m_TriangleVertexData),
			.stride = sizeof(m_TriangleVertexData[0]),
			.viewFlags = BufViewFlags::SRV,
			.isRawAccess = true,
		};
		m_TriangleVtxBuf = ctx.CreateBuffer(vtxBufDesc, &m_TriangleVertexData, sizeof(m_TriangleVertexData));
		// Query the bindless descriptor index for the vertex buffer.
		m_TriangleVtxBufIdx = ctx.GetBindlessSRV(m_TriangleVtxBuf);
	}

	~HelloTriangle()
	{
		// Clean up GPU resources created for this sample.
		ctx.DestroyPipeline(m_TrianglePso);
		ctx.DestroyBuffer(m_TriangleVtxBuf);
	}

	void Render() override
	{
		// Copy the updated vertex data to the GPU if necessary
		if (m_bUpdateTriangle)
		{
			m_bUpdateTriangle = false;
			ctx.UpdateBuffer(m_TriangleVtxBuf, &m_TriangleVertexData, sizeof(m_TriangleVertexData));
		}

		// Transition necessary resource barriers to begin a render pass onto the back buffer and 
		// clear it.
		ctx.BeginRenderPassToBackBuffer(m_TrianglePso, LoadOp::CLEAR);
		{
			// Set the bindless vertex buffer index as a push constant for the shader to read and 
			// draw our vertices.
			ctx.SetPushConstants(&m_TriangleVtxBufIdx, sizeof(uint32));
			ctx.Draw(3);
		}
		ctx.EndRenderPass();
	}

	void OnGUI() override
	{
		if (ImGui::Begin("Hello Triangle UI", 0, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::PushItemWidth(300);
			ImGui::Text("Triangle Vertex Positions");
			if (ImGui::SliderFloat2("##1", (float*)&m_TriangleVertexData[0].pos, -1.0f, 1.0f)) m_bUpdateTriangle = true;
			if (ImGui::SliderFloat2("##2", (float*)&m_TriangleVertexData[1].pos, -1.0f, 1.0f)) m_bUpdateTriangle = true;
			if (ImGui::SliderFloat2("##3", (float*)&m_TriangleVertexData[2].pos, -1.0f, 1.0f)) m_bUpdateTriangle = true;
			ImGui::Text("Triangle Vertex Colors");
			if (ImGui::ColorEdit3("##1", (float*)&m_TriangleVertexData[0].col)) m_bUpdateTriangle = true;
			if (ImGui::ColorEdit3("##2", (float*)&m_TriangleVertexData[1].col)) m_bUpdateTriangle = true;
			if (ImGui::ColorEdit3("##3", (float*)&m_TriangleVertexData[2].col)) m_bUpdateTriangle = true;
			ImGui::PopItemWidth();
		}
		ImGui::End();
	}

};