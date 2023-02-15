#pragma once

#include "Graphics/API/DX12/dx12_common.h"

// TODO: class Swapchain inside device
// TODO: Graphics context public interface, owns device and compute/upload contexts inside it

namespace vast::gfx
{
	class DX12Device;
	class DX12GraphicsCommandList;

	class DX12GraphicsContext final : public GraphicsContext
	{
	public:
		DX12GraphicsContext(const GraphicsParams& params);
		~DX12GraphicsContext();

		void BeginFrame() override;
		void EndFrame() override;
		void Submit() override;
		void Present() override;

		Texture& GetCurrentBackBuffer() const override;

		void AddBarrier(Resource& resource, const ResourceState& state) override;
		void FlushBarriers() override;

		void Reset() override;
		void ClearRenderTarget(const Texture& texture, float4 color) override;

	private:
		Ptr<DX12Device> m_Device;
		Ptr<DX12GraphicsCommandList> m_GraphicsCommandList;
	};
}