#include "vastpch.h"
#include "Graphics/GraphicsContext.h"
#include "Graphics/GraphicsBackend.h"
#include "Graphics/ResourceManager.h"
#include "Graphics/GPUProfiler.h"

#include "Core/EventTypes.h"

namespace vast::gfx
{

	static void OnWindowResizeEvent(const WindowResizeEvent& event)
	{
		const uint2 scSize = GraphicsBackend::GetBackBufferSize();

		if (event.m_WindowSize.x != scSize.x || event.m_WindowSize.y != scSize.y)
		{
			GraphicsBackend::WaitForIdle();
			GraphicsBackend::ResizeSwapChainAndBackBuffers(event.m_WindowSize);
		}
	}

	GraphicsContext::GraphicsContext(const GraphicsParams& params /* = GraphicsParams() */)
		: m_ResourceManager(nullptr)
		, m_GpuProfiler(nullptr)
		, m_bHasFrameBegun(false)
		, m_bHasRenderPassBegun(false)
		, m_GpuFrameTimestampIdx(0)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Create Graphics Context");
		VAST_LOG_INFO("[gfx] Initializing GraphicsContext...");

		GraphicsBackend::Init(params);

		m_ResourceManager = MakePtr<ResourceManager>();
		m_GpuProfiler = MakePtr<GPUProfiler>(*m_ResourceManager);

		VAST_SUBSCRIBE_TO_EVENT("gfxctx", WindowResizeEvent, VAST_EVENT_HANDLER_CB(OnWindowResizeEvent, WindowResizeEvent));
	}

	GraphicsContext::~GraphicsContext()
	{
		m_GpuProfiler = nullptr;
		m_ResourceManager = nullptr;

		GraphicsBackend::Shutdown();
	}

	void GraphicsContext::BeginFrame()
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Begin Frame");
		VAST_ASSERTF(!m_bHasFrameBegun, "A frame is already running");
		m_bHasFrameBegun = true;

		m_ResourceManager->BeginFrame();

		GraphicsBackend::BeginFrame();

		m_GpuFrameTimestampIdx = m_GpuProfiler->BeginTimestamp();
	}

	void GraphicsContext::EndFrame()
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "End Frame");
		VAST_ASSERTF(m_bHasFrameBegun, "No frame is currently running.");

		m_GpuProfiler->EndTimestamp(m_GpuFrameTimestampIdx);
		m_GpuProfiler->CollectTimestamps();

		GraphicsBackend::EndFrame();

		m_bHasFrameBegun = false;
	}

	bool GraphicsContext::IsInFrame() const
	{
		return m_bHasFrameBegun;
	}

	double GraphicsContext::GetLastFrameDuration()
	{
		VAST_ASSERT(m_GpuProfiler);
		return m_GpuProfiler->GetTimestampDuration(m_GpuFrameTimestampIdx);
	}

	void GraphicsContext::BeginRenderPass(PipelineHandle h, const RenderPassTargets targets)
	{
		VAST_PROFILE_TRACE_BEGIN("gfx", "Render Pass");
		VAST_ASSERTF(!m_bHasRenderPassBegun, "A render pass is already running.");
		m_bHasRenderPassBegun = true;
		VAST_ASSERT(h.IsValid());
		GraphicsBackend::BeginRenderPass(h, targets);
	}

	void GraphicsContext::BeginRenderPassToBackBuffer(PipelineHandle h, LoadOp loadOp /* = LoadOp::LOAD */, StoreOp storeOp /* = StoreOp::STORE */)
	{
		VAST_PROFILE_TRACE_BEGIN("gfx", "Render Pass");
		VAST_ASSERTF(!m_bHasRenderPassBegun, "A render pass is already running.");
		m_bHasRenderPassBegun = true;
		VAST_ASSERT(h.IsValid());
		GraphicsBackend::BeginRenderPassToBackBuffer(h, loadOp, storeOp);
	}

	void GraphicsContext::EndRenderPass()
	{
		VAST_ASSERTF(m_bHasRenderPassBegun, "No render pass is currently running.");
		m_bHasRenderPassBegun = false;
		GraphicsBackend::EndRenderPass();
	}

	bool GraphicsContext::IsInRenderPass() const
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Compute Dispatch");
		return m_bHasRenderPassBegun;
	}

	void GraphicsContext::BindPipelineForCompute(PipelineHandle h)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Bind Pipeline For Compute");
		VAST_ASSERTF(!m_bHasRenderPassBegun, "Cannot bind another pipeline in the middle of a render pass.");
		VAST_ASSERT(h.IsValid());
		GraphicsBackend::BindPipelineForCompute(h);
	}

	void GraphicsContext::WaitForIdle()
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Wait CPU on GPU");
		GraphicsBackend::WaitForIdle();
	}

	void GraphicsContext::FlushGPU()
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Flush GPU Work");
		GraphicsBackend::WaitForIdle();

		m_ResourceManager->ProcessShaderReloads();

		for (uint32 i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
		{
			m_ResourceManager->ProcessDestructions(i);
		}
	}

	//

	// TODO: This is ugly!
	BufferHandle GraphicsContext::CreateBuffer(const BufferDesc& desc, const void* initialData, const size_t dataSize) { return m_ResourceManager->CreateBuffer(desc, initialData, dataSize); }
	TextureHandle GraphicsContext::CreateTexture(const TextureDesc& desc, const void* initialData) { return m_ResourceManager->CreateTexture(desc, initialData); }
	PipelineHandle GraphicsContext::CreatePipeline(const PipelineDesc& desc) { return m_ResourceManager->CreatePipeline(desc); }
	PipelineHandle GraphicsContext::CreatePipeline(const ShaderDesc& csDesc) { return m_ResourceManager->CreatePipeline(csDesc); }
	void GraphicsContext::DestroyBuffer(BufferHandle h) { m_ResourceManager->DestroyBuffer(h); }
	void GraphicsContext::DestroyTexture(TextureHandle h) { m_ResourceManager->DestroyTexture(h); }
	void GraphicsContext::DestroyPipeline(PipelineHandle h) { m_ResourceManager->DestroyPipeline(h); }
	void GraphicsContext::UpdateBuffer(BufferHandle h, void* data, const size_t size) { m_ResourceManager->UpdateBuffer(h, data, size); }
	void GraphicsContext::ReloadShaders(PipelineHandle h) { m_ResourceManager->ReloadShaders(h); }
	TextureHandle GraphicsContext::LoadTextureFromFile(const std::string& filePath, bool sRGB) { return m_ResourceManager->LoadTextureFromFile(filePath, sRGB); }
	void GraphicsContext::SetDebugName(BufferHandle h, const std::string& name) { m_ResourceManager->SetDebugName(h, name); }
	void GraphicsContext::SetDebugName(TextureHandle h, const std::string& name) { m_ResourceManager->SetDebugName(h, name); }
	const uint8* GraphicsContext::GetBufferData(BufferHandle h) { return m_ResourceManager->GetBufferData(h); }
	bool GraphicsContext::GetIsReady(BufferHandle h) { return m_ResourceManager->GetIsReady(h); }
	bool GraphicsContext::GetIsReady(TextureHandle h) { return m_ResourceManager->GetIsReady(h); }
	TexFormat GraphicsContext::GetTextureFormat(TextureHandle h) { return m_ResourceManager->GetTextureFormat(h); }

	//

	ShaderResourceProxy GraphicsContext::LookupShaderResource(PipelineHandle h, const std::string& shaderResourceName)
	{
		return GraphicsBackend::LookupShaderResource(h, shaderResourceName);
	}

	//

	void GraphicsContext::AddBarrier(BufferHandle h, ResourceState newState)
	{
		VAST_ASSERT(h.IsValid());
		GraphicsBackend::AddBarrier(h, newState);
	}

	void GraphicsContext::AddBarrier(TextureHandle h, ResourceState newState)
	{
		VAST_ASSERT(h.IsValid());
		GraphicsBackend::AddBarrier(h, newState);
	}

	void GraphicsContext::FlushBarriers()
	{
		GraphicsBackend::FlushBarriers();
	}

	//

	void GraphicsContext::BindVertexBuffer(BufferHandle h, uint32 offset /* = 0 */, uint32 stride /* = 0 */)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Bind Vertex Buffer");
		VAST_ASSERT(h.IsValid());
		GraphicsBackend::BindVertexBuffer(h, offset, stride);
	}

	void GraphicsContext::BindIndexBuffer(BufferHandle h, uint32 offset /* = 0 */, IndexBufFormat format /* = IndexBufFormat::R16_UINT */)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Bind Index Buffer");
		VAST_ASSERT(h.IsValid());
		GraphicsBackend::BindIndexBuffer(h, offset, format);
	}

	void GraphicsContext::BindConstantBuffer(ShaderResourceProxy proxy, BufferHandle h, uint32 offset /* = 0 */)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Bind Constant Buffer");
		VAST_ASSERT(h.IsValid() && proxy.IsValid());
		GraphicsBackend::BindConstantBuffer(proxy, h, offset);
	}

	void GraphicsContext::SetPushConstants(const void* data, const uint32 size)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Set Push Constants");
		VAST_ASSERT(data && size);
		GraphicsBackend::SetPushConstants(data, size);
	}

	void GraphicsContext::BindSRV(ShaderResourceProxy proxy, BufferHandle h)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Bind SRV");
		VAST_ASSERT(h.IsValid() && proxy.IsValid());
		(void)proxy; // TODO: This is currently not being used as an index, only to check that the name exists in the shader.
		GraphicsBackend::BindSRV(h);
	}

	void GraphicsContext::BindSRV(ShaderResourceProxy proxy, TextureHandle h)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Bind SRV");
		VAST_ASSERT(h.IsValid() && proxy.IsValid());
		(void)proxy; // TODO: This is currently not being used as an index, only to check that the name exists in the shader.
		GraphicsBackend::BindSRV(h);
	}

	void GraphicsContext::BindUAV(ShaderResourceProxy proxy, TextureHandle h, uint32 mipLevel /* = 0 */)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Bind UAV");
		VAST_ASSERT(h.IsValid() && proxy.IsValid());
		(void)proxy; // TODO: This is currently not being used as an index, only to check that the name exists in the shader.
		GraphicsBackend::BindUAV(h, mipLevel);
	}

	uint32 GraphicsContext::GetBindlessSRV(BufferHandle h)
	{
		VAST_ASSERT(h.IsValid());
		return GraphicsBackend::GetBindlessSRV(h);
	}

	uint32 GraphicsContext::GetBindlessSRV(TextureHandle h)
	{
		VAST_ASSERT(h.IsValid());
		return GraphicsBackend::GetBindlessSRV(h);
	}

	uint32 GraphicsContext::GetBindlessUAV(TextureHandle h, uint32 mipLevel /* = 0 */)
	{
		VAST_ASSERT(h.IsValid());
		return GraphicsBackend::GetBindlessUAV(h, mipLevel);
	}

	//

	void GraphicsContext::SetScissorRect(int4 rect)
	{
		GraphicsBackend::SetScissorRect(rect);
	}

	void GraphicsContext::SetBlendFactor(float4 blend)
	{
		GraphicsBackend::SetBlendFactor(blend);
	}

	//

	void GraphicsContext::Draw(uint32 vtxCount, uint32 vtxStartLocation /* = 0 */)
	{
		DrawInstanced(vtxCount, 1, vtxStartLocation, 0);
	}

	void GraphicsContext::DrawIndexed(uint32 idxCount, uint32 startIdxLocation /* = 0 */, uint32 baseVtxLocation /* = 0 */)
	{
		DrawIndexedInstanced(idxCount, 1, startIdxLocation, baseVtxLocation, 0);
	}

	void GraphicsContext::DrawInstanced(uint32 vtxCountPerInstance, uint32 instCount, uint32 vtxStartLocation /* = 0 */, uint32 instStartLocation /* = 0 */)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Draw Instanced");
		GraphicsBackend::DrawInstanced(vtxCountPerInstance, instCount, vtxStartLocation, instStartLocation);
	}

	void GraphicsContext::DrawIndexedInstanced(uint32 idxCountPerInst, uint32 instCount, uint32 startIdxLocation, uint32 baseVtxLocation, uint32 startInstLocation)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Draw Indexed Instanced");
		GraphicsBackend::DrawIndexedInstanced(idxCountPerInst, instCount, startIdxLocation, baseVtxLocation, startInstLocation);
	}

	void GraphicsContext::DrawFullscreenTriangle()
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Draw Fullscreen Triangle");
		GraphicsBackend::DrawFullscreenTriangle();
	}

	void GraphicsContext::Dispatch(uint3 threadGroupCount)
	{
		GraphicsBackend::Dispatch(threadGroupCount);
	}

	//

	uint2 GraphicsContext::GetBackBufferSize() const
	{
		return GraphicsBackend::GetBackBufferSize();
	}

	float GraphicsContext::GetBackBufferAspectRatio() const
	{
		auto size = GetBackBufferSize();
		return float(size.x) / float(size.y);
	}

	TexFormat GraphicsContext::GetBackBufferFormat() const
	{
		return GraphicsBackend::GetBackBufferFormat();
	}

	//

	ResourceManager& GraphicsContext::GetResourceManager()
	{
		VAST_ASSERT(m_ResourceManager);
		return *m_ResourceManager;
	}
	
	GPUProfiler& GraphicsContext::GetGPUProfiler()
	{
		VAST_ASSERT(m_GpuProfiler);
		return *m_GpuProfiler;
	}

}
