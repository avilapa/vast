#pragma once

#include "Core/Core.h"
#include "Graphics/Handles.h"
#include "Graphics/Resources.h"
#include "Graphics/ShaderResourceProxy.h"

namespace vast
{

	struct GraphicsParams
	{
		GraphicsParams()
			: swapChainSize(1600, 900)
			, swapChainFormat(TexFormat::RGBA8_UNORM)
			, backBufferFormat(TexFormat::RGBA8_UNORM_SRGB)
		{}

		uint2 swapChainSize;
		TexFormat swapChainFormat;
		TexFormat backBufferFormat;
	};

	class GPUResourceManager;
	class GPUProfiler;
	
	class GraphicsContext
	{
	public:
		GraphicsContext(WindowHandle windowHandle, const GraphicsParams& params = GraphicsParams());
		~GraphicsContext();

		void BeginFrame();
		void EndFrame();

		bool IsInFrame() const;
		double GetLastFrameDuration();

		void BeginRenderPass(PipelineHandle h, const RenderPassDesc desc);
		// Renders to the back buffer as the only target
		void BeginRenderPassToBackBuffer(PipelineHandle h, LoadOp loadOp = LoadOp::LOAD, StoreOp storeOp = StoreOp::STORE);
		void EndRenderPass();

		bool IsInRenderPass() const;

		// TODO: We may want to do Begin/End functions for compute to be analogous with graphics 
		// pipelines, despite it being perhaps redundant.
		void BindPipelineForCompute(PipelineHandle h);

		// Waits for the current batch of GPU work to finish.
		void WaitForIdle();
		// Waits for all active GPU work to finish as well as any queued resource destructions and 
		// shader reloads.
		void FlushGPU();

		// - Resource Bindings ----------------------------------------------------------------- //

		// Resource Transitions
		void AddBarrier(BufferHandle h, ResourceState newState);
		void AddBarrier(TextureHandle h, ResourceState newState);
		void FlushBarriers();

		// Resource View Binding
		void BindVertexBuffer(BufferHandle h, uint32 offset = 0, uint32 stride = 0);
		void BindIndexBuffer(BufferHandle h, uint32 offset = 0, IndexBufFormat format = IndexBufFormat::R16_UINT);
		void BindConstantBuffer(ShaderResourceProxy proxy, BufferHandle h, uint32 offset = 0);

		// Passes some data to be used in the GPU under a constant buffer declared in a shader using slot
		// 'PushConstantRegister'. Push Constants are best suited for small amounts of data that change
		// frequently (e.g. PerObjectSpace). Size parameter is expected in bytes.
		void SetPushConstants(const void* data, const uint32 size);

		void BindSRV(ShaderResourceProxy proxy, BufferHandle h);
		void BindSRV(ShaderResourceProxy proxy, TextureHandle h);
		void BindUAV(ShaderResourceProxy proxy, TextureHandle h, uint32 mipLevel = 0);

		// - Pipeline State -------------------------------------------------------------------- //

		void SetScissorRect(int4 rect);
		void SetBlendFactor(float4 blend);

		// - Draw/Dispatch Calls --------------------------------------------------------------- //

		void Draw(uint32 vtxCount, uint32 vtxStartLocation = 0);
		void DrawIndexed(uint32 idxCount, uint32 startIdxLocation = 0, uint32 baseVtxLocation = 0);
		void DrawInstanced(uint32 vtxCountPerInst, uint32 instCount, uint32 vtxStartLocation = 0, uint32 instStartLocation = 0);
		void DrawIndexedInstanced(uint32 idxCountPerInstance, uint32 instanceCount, uint32 startIdxLocation, uint32 baseVtxLocation, uint32 startInstLocation);
		void DrawFullscreenTriangle();

		void Dispatch(uint3 threadGroupCount);

		// - Swap Chain/Back Buffers ----------------------------------------------------------- //

		uint2 GetBackBufferSize() const;
		float GetBackBufferAspectRatio() const;
		TexFormat GetBackBufferFormat() const;

		// - Submodules ------------------------------------------------------------------------ //

		GPUResourceManager& GetGPUResourceManager();
		GPUProfiler& GetGPUProfiler();

	private:
		Ptr<GPUResourceManager> m_GPUResourceManager;
		Ptr<GPUProfiler> m_GpuProfiler;

		bool m_bHasFrameBegun = false;
		bool m_bHasRenderPassBegun = false;

		uint32 m_GpuFrameTimestampIdx;
	};
}