#include "vastpch.h"
#include "Graphics/GraphicsContext.h"
#include "Graphics/GraphicsBackend.h"
#include "Graphics/GPUResourceManager.h"
#include "Graphics/GPUProfiler.h"

#include "Core/EventTypes.h"

namespace vast
{

	static void OnWindowResizeEvent(const WindowResizeEvent& event)
	{
		const uint2 scSize = gfx::GetBackBufferSize();

		if (event.m_WindowSize.x != scSize.x || event.m_WindowSize.y != scSize.y)
		{
			gfx::WaitForIdle();
			gfx::ResizeSwapChainAndBackBuffers(event.m_WindowSize);
		}
	}

	GraphicsContext::GraphicsContext(const GraphicsParams& params /* = GraphicsParams() */)
		: m_GPUResourceManager(nullptr)
		, m_GpuProfiler(nullptr)
		, m_bHasFrameBegun(false)
		, m_bHasRenderPassBegun(false)
		, m_GpuFrameTimestampIdx(0)
	{
		VAST_PROFILE_TRACE_FUNCTION;
		VAST_LOG_INFO("[gfx] Initializing GraphicsContext...");

		gfx::Init(params);

		m_GPUResourceManager = MakePtr<GPUResourceManager>();
		m_GpuProfiler = MakePtr<GPUProfiler>(*m_GPUResourceManager);

		Event::Subscribe<WindowResizeEvent>("GraphicsContext", VAST_EVENT_HANDLER(OnWindowResizeEvent, WindowResizeEvent));
	}

	GraphicsContext::~GraphicsContext()
	{
		VAST_PROFILE_TRACE_FUNCTION;

		m_GpuProfiler = nullptr;
		m_GPUResourceManager = nullptr;

		gfx::Shutdown();
	}

	void GraphicsContext::BeginFrame()
	{
		VAST_PROFILE_TRACE_FUNCTION;
		VAST_ASSERTF(!m_bHasFrameBegun, "A frame is already running");

		m_bHasFrameBegun = true;

		m_GPUResourceManager->BeginFrame();
		gfx::BeginFrame();
		m_GpuFrameTimestampIdx = m_GpuProfiler->BeginTimestamp();
	}

	void GraphicsContext::EndFrame()
	{
		VAST_PROFILE_TRACE_FUNCTION;
		VAST_ASSERTF(m_bHasFrameBegun, "No frame is currently running.");

		m_GpuProfiler->EndTimestamp(m_GpuFrameTimestampIdx);
		m_GpuProfiler->CollectTimestamps();

		gfx::EndFrame();

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
		VAST_PROFILE_TRACE_BEGIN("Render Pass");
		VAST_PROFILE_TRACE_FUNCTION;
		VAST_ASSERTF(!m_bHasRenderPassBegun, "A render pass is already running.");
		VAST_ASSERT(h.IsValid());

		m_bHasRenderPassBegun = true;
		gfx::BeginRenderPass(h, targets);
	}

	void GraphicsContext::BeginRenderPassToBackBuffer(PipelineHandle h, LoadOp loadOp /* = LoadOp::LOAD */, StoreOp storeOp /* = StoreOp::STORE */)
	{
		VAST_PROFILE_TRACE_BEGIN("Render Pass");
		VAST_PROFILE_TRACE_FUNCTION;
		VAST_ASSERTF(!m_bHasRenderPassBegun, "A render pass is already running.");
		VAST_ASSERT(h.IsValid());

		m_bHasRenderPassBegun = true;
		gfx::BeginRenderPassToBackBuffer(h, loadOp, storeOp);
	}

	void GraphicsContext::EndRenderPass()
	{
		VAST_PROFILE_TRACE_FUNCTION;
		VAST_ASSERTF(m_bHasRenderPassBegun, "No render pass is currently running.");

		m_bHasRenderPassBegun = false;
		gfx::EndRenderPass();

		VAST_PROFILE_TRACE_END("Render Pass");
	}

	bool GraphicsContext::IsInRenderPass() const
	{
		return m_bHasRenderPassBegun;
	}

	void GraphicsContext::BindPipelineForCompute(PipelineHandle h)
	{
		VAST_PROFILE_TRACE_FUNCTION;
		VAST_ASSERTF(!m_bHasRenderPassBegun, "Cannot bind another pipeline in the middle of a render pass.");
		VAST_ASSERT(h.IsValid());

		gfx::BindPipelineForCompute(h);
	}

	void GraphicsContext::WaitForIdle()
	{
		VAST_PROFILE_TRACE_FUNCTION;

		gfx::WaitForIdle();
	}

	void GraphicsContext::FlushGPU()
	{
		VAST_PROFILE_TRACE_FUNCTION;

		gfx::WaitForIdle();
		m_GPUResourceManager->ProcessShaderReloads();
		for (uint32 i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
		{
			m_GPUResourceManager->ProcessDestructions(i);
		}
	}

	//

	void GraphicsContext::AddBarrier(BufferHandle h, ResourceState newState)
	{
		VAST_ASSERT(h.IsValid());
		gfx::AddBarrier(h, newState);
	}

	void GraphicsContext::AddBarrier(TextureHandle h, ResourceState newState)
	{
		VAST_ASSERT(h.IsValid());
		gfx::AddBarrier(h, newState);
	}

	void GraphicsContext::FlushBarriers()
	{
		gfx::FlushBarriers();
	}

	//

	void GraphicsContext::BindVertexBuffer(BufferHandle h, uint32 offset /* = 0 */, uint32 stride /* = 0 */)
	{
		VAST_ASSERT(h.IsValid());
		gfx::BindVertexBuffer(h, offset, stride);
	}

	void GraphicsContext::BindIndexBuffer(BufferHandle h, uint32 offset /* = 0 */, IndexBufFormat format /* = IndexBufFormat::R16_UINT */)
	{
		VAST_ASSERT(h.IsValid());
		gfx::BindIndexBuffer(h, offset, format);
	}

	void GraphicsContext::BindConstantBuffer(ShaderResourceProxy proxy, BufferHandle h, uint32 offset /* = 0 */)
	{
		VAST_ASSERT(h.IsValid() && proxy.IsValid());
		gfx::BindConstantBuffer(proxy, h, offset);
	}

	void GraphicsContext::SetPushConstants(const void* data, const uint32 size)
	{
		VAST_ASSERT(data && size);
		gfx::SetPushConstants(data, size);
	}

	void GraphicsContext::BindSRV(ShaderResourceProxy proxy, BufferHandle h)
	{
		VAST_ASSERT(h.IsValid() && proxy.IsValid());
		(void)proxy; // TODO: This is currently not being used as an index, only to check that the name exists in the shader.
		gfx::BindSRV(h);
	}

	void GraphicsContext::BindSRV(ShaderResourceProxy proxy, TextureHandle h)
	{
		VAST_ASSERT(h.IsValid() && proxy.IsValid());
		(void)proxy; // TODO: This is currently not being used as an index, only to check that the name exists in the shader.
		gfx::BindSRV(h);
	}

	void GraphicsContext::BindUAV(ShaderResourceProxy proxy, TextureHandle h, uint32 mipLevel /* = 0 */)
	{
		VAST_ASSERT(h.IsValid() && proxy.IsValid());
		(void)proxy; // TODO: This is currently not being used as an index, only to check that the name exists in the shader.
		gfx::BindUAV(h, mipLevel);
	}

	//

	void GraphicsContext::SetScissorRect(int4 rect)
	{
		gfx::SetScissorRect(rect);
	}

	void GraphicsContext::SetBlendFactor(float4 blend)
	{
		gfx::SetBlendFactor(blend);
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
		gfx::DrawInstanced(vtxCountPerInstance, instCount, vtxStartLocation, instStartLocation);
	}

	void GraphicsContext::DrawIndexedInstanced(uint32 idxCountPerInst, uint32 instCount, uint32 startIdxLocation, uint32 baseVtxLocation, uint32 startInstLocation)
	{
		gfx::DrawIndexedInstanced(idxCountPerInst, instCount, startIdxLocation, baseVtxLocation, startInstLocation);
	}

	void GraphicsContext::DrawFullscreenTriangle()
	{
		gfx::DrawFullscreenTriangle();
	}

	void GraphicsContext::Dispatch(uint3 threadGroupCount)
	{
		gfx::Dispatch(threadGroupCount);
	}

	//

	uint2 GraphicsContext::GetBackBufferSize() const
	{
		return gfx::GetBackBufferSize();
	}

	float GraphicsContext::GetBackBufferAspectRatio() const
	{
		auto size = GetBackBufferSize();
		return float(size.x) / float(size.y);
	}

	TexFormat GraphicsContext::GetBackBufferFormat() const
	{
		return gfx::GetBackBufferFormat();
	}

	//

	GPUResourceManager& GraphicsContext::GetGPUResourceManager()
	{
		VAST_ASSERT(m_GPUResourceManager);
		return *m_GPUResourceManager;
	}
	
	GPUProfiler& GraphicsContext::GetGPUProfiler()
	{
		VAST_ASSERT(m_GpuProfiler);
		return *m_GpuProfiler;
	}

}
