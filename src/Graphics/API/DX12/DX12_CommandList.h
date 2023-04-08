#pragma once

#include "Graphics/API/DX12/DX12_Common.h"

namespace vast::gfx
{
	class DX12Device;
	class DX12RenderPassDescriptorHeap;

	class DX12CommandList
	{
	public:
		DX12CommandList(DX12Device& device, D3D12_COMMAND_LIST_TYPE commandListType);
		virtual ~DX12CommandList();

		D3D12_COMMAND_LIST_TYPE GetCommandType() const;
		ID3D12GraphicsCommandList* GetCommandList() const;

		void Reset(uint32 frameId);
		void AddBarrier(DX12Resource& resource, D3D12_RESOURCE_STATES newState);
		void FlushBarriers();

		void CopyResource(const DX12Resource& dst, const DX12Resource& src);
		void CopyBufferRegion(DX12Resource& dst, uint64 dstOffset, DX12Resource& src, uint64 srcOffset, uint64 numBytes);
		void CopyTextureRegion(DX12Resource& dst, DX12Resource& src, size_t srcOffset, Array<D3D12_PLACED_SUBRESOURCE_FOOTPRINT, MAX_TEXTURE_SUBRESOURCE_COUNT>& subresourceLayouts, uint32 numSubresources);

	protected:
		void BindDescriptorHeaps(uint32 frameId);

		DX12Device& m_Device;
		D3D12_COMMAND_LIST_TYPE m_CommandType;

		Array<ID3D12CommandAllocator*, NUM_FRAMES_IN_FLIGHT> m_CommandAllocators;
		ID3D12GraphicsCommandList4* m_CommandList;
		Array<D3D12_RESOURCE_BARRIER, MAX_QUEUED_BARRIERS> m_ResourceBarrierQueue;
		uint32 m_NumQueuedBarriers;

		DX12RenderPassDescriptorHeap* m_CurrentSRVDescriptorHeap;
	};

	class DX12GraphicsCommandList final : public DX12CommandList
	{
	public:
		DX12GraphicsCommandList(DX12Device& device);

#if VAST_GFX_DX12_USE_RENDER_PASSES
		void BeginRenderPass(DX12Texture** rt, uint32 rtCount, DX12Texture* ds, ClearFlags clear);
		void EndRenderPass();
#else
		void SetRenderTargets(DX12Texture** rt, uint32 rtCount, DX12Texture* ds);
		void ClearRenderTarget(const DX12Texture& rt);
		void ClearDepthStencilTarget(const DX12Texture& dst, float depth, uint8 stencil);
#endif // VAST_GFX_DX12_USE_RENDER_PASSES

		void SetPipeline(DX12Pipeline* pipeline);
		void SetVertexBuffer(const DX12Buffer& buf, uint32 offset, uint32 stride);
		void SetIndexBuffer(const DX12Buffer& buf, uint32 offset, DXGI_FORMAT format);
		void SetConstantBuffer(const DX12Buffer& buf, uint32 offset, uint32 slotIndex);
		void SetDescriptorTable(const D3D12_GPU_DESCRIPTOR_HANDLE& gpuHandle);
		void SetPushConstants(const void* data, const uint32 size);

		void SetDefaultViewportAndScissor(uint2 windowSize);
		void SetScissorRect(const D3D12_RECT& rect);
	private:
		DX12Pipeline* m_CurrentPipeline;
	};

	struct BufferUpload
	{
		DX12Buffer* buf = nullptr;
		Ptr<uint8[]> data;
		size_t size = 0;
	};

	struct TextureUpload
	{
		DX12Texture* tex = nullptr;
		Ptr<uint8[]> data;
		size_t size = 0;
		uint32 numSubresources = 0;
		Array<D3D12_PLACED_SUBRESOURCE_FOOTPRINT, MAX_TEXTURE_SUBRESOURCE_COUNT> subresourceLayouts = { 0 };
	};

	class DX12UploadCommandList final : public DX12CommandList
	{
	public:
		DX12UploadCommandList(DX12Device& device);
		~DX12UploadCommandList();

		void UploadBuffer(Ptr<BufferUpload> upload);
		void UploadTexture(Ptr<TextureUpload> upload);
		void ProcessUploads();
		void ResolveProcessedUploads();

	private:
		Ptr<DX12Buffer> m_BufferUploadHeap;
		Ptr<DX12Buffer> m_TextureUploadHeap;
		Vector<Ptr<BufferUpload>> m_BufferUploads;
		Vector<DX12Buffer*> m_BufferUploadsInProgress;
		Vector<Ptr<TextureUpload>> m_TextureUploads;
		Vector<DX12Texture*> m_TextureUploadsInProgress;
	};

}