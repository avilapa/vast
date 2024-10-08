#pragma once

#include "Graphics/Resources.h"
#include "Graphics/ShaderResourceProxy.h"

namespace vast
{
	struct TempAllocator
	{
		BufferHandle buffer;
		uint32 bufferSize = 0;
		uint32 usedMemory = 0;

		uint32 Alloc(uint32 size, uint32 alignment = 0);
		void Reset();
	};

	class GPUResourceManager
	{
		friend class GraphicsContext;
	public:
		GPUResourceManager();
		~GPUResourceManager();

		BufferHandle CreateBuffer(const BufferDesc& desc, const void* initialData = nullptr, const size_t dataSize = 0, const std::string& name = "Unnamed Buffer");
		TextureHandle CreateTexture(const TextureDesc& desc, const void* initialData = nullptr, const std::string name = "Unnamed Texture");
		PipelineHandle CreatePipeline(const PipelineDesc& desc);
		PipelineHandle CreatePipeline(const ShaderDesc& desc);

		void DestroyBuffer(BufferHandle h);
		void DestroyTexture(TextureHandle h);
		void DestroyPipeline(PipelineHandle h);

		void UpdateBuffer(BufferHandle h, void* data, const size_t size);

		ShaderResourceProxy LookupShaderResource(PipelineHandle h, const std::string& shaderResourceName);

		void ReloadShaders(PipelineHandle h);

		// TODO: This should be separate from the Resource Manager (ContentLoader?)
		TextureHandle LoadTextureFromFile(const std::string& filePath, bool sRGB = true);

		// Returns a BufferView containing CPU-write/GPU-read memory that is alive for the duration of
		// the frame and automatically invalidated after that.
		BufferView AllocTempBufferView(uint32 size, uint32 alignment = 0);

		const uint8* GetBufferData(BufferHandle h);

		// TODO: Review ready check for resources.
		bool GetIsReady(BufferHandle h);
		bool GetIsReady(TextureHandle h);

		TexFormat GetTextureFormat(TextureHandle h);

		// Query Bindless View Indices
		uint32 GetBindlessSRV(BufferHandle h);
		uint32 GetBindlessSRV(TextureHandle h);
		uint32 GetBindlessUAV(TextureHandle h, uint32 mipLevel = 0);

	private:
		void BeginFrame();
		void ProcessDestructions(uint32 frameId);
		void ProcessShaderReloads();

	private:
		HandlePool<Buffer, NUM_BUFFERS> m_BufferHandles;
		HandlePool<Texture, NUM_TEXTURES> m_TextureHandles;
		HandlePool<Pipeline, NUM_PIPELINES> m_PipelineHandles;

		Array<Vector<BufferHandle>, NUM_FRAMES_IN_FLIGHT> m_BuffersMarkedForDestruction;
		Array<Vector<TextureHandle>, NUM_FRAMES_IN_FLIGHT> m_TexturesMarkedForDestruction;
		Array<Vector<PipelineHandle>, NUM_FRAMES_IN_FLIGHT> m_PipelinesMarkedForDestruction;

		Vector<PipelineHandle> m_PipelinesMarkedForShaderReload;

		Array<TempAllocator, NUM_FRAMES_IN_FLIGHT> m_TempFrameAllocators;
	};

}