#pragma once

#include "Graphics/Resources.h"

namespace vast::gfx
{
	class ResourceManager
	{
	public:
		ResourceManager();
		~ResourceManager();

		void BeginFrame();

		BufferHandle CreateBuffer(const BufferDesc& desc, const void* initialData = nullptr, const size_t dataSize = 0);
		TextureHandle CreateTexture(const TextureDesc& desc, const void* initialData = nullptr);
		PipelineHandle CreatePipeline(const PipelineDesc& desc);
		PipelineHandle CreatePipeline(const ShaderDesc& csDesc);

		void DestroyBuffer(BufferHandle h);
		void DestroyTexture(TextureHandle h);
		void DestroyPipeline(PipelineHandle h);

		void UpdateBuffer(BufferHandle h, void* data, const size_t size);

		void ReloadShaders(PipelineHandle h);

		// TODO: This should be separate from the Context (ContentLoader?)
		TextureHandle LoadTextureFromFile(const std::string& filePath, bool sRGB = true);

		// Returns a BufferView containing CPU-write/GPU-read memory that is alive for the duration of
		// the frame and automatically invalidated after that.
		BufferView AllocTempBufferView(uint32 size, uint32 alignment = 0);

		// Waits for all active GPU work to finish as well as any queued resource destructions and 
		// shader reloads.
		void FlushGPU(); // TODO: Is this still a good name?

		void SetDebugName(BufferHandle h, const std::string& name);
		void SetDebugName(TextureHandle h, const std::string& name);

		const uint8* GetBufferData(BufferHandle h);

		// TODO: Review ready check for resources.
		bool GetIsReady(BufferHandle h);
		bool GetIsReady(TextureHandle h);

		TexFormat GetTextureFormat(TextureHandle h);

	private:
		void CreateFrameAllocators();
		void DestroyFrameAllocators();

		void ProcessDestructions(uint32 frameId);
		void ProcessShaderReloads();

	private:
		Ptr<HandlePool<Buffer, NUM_BUFFERS>> m_BufferHandles;
		Ptr<HandlePool<Texture, NUM_TEXTURES>> m_TextureHandles;
		Ptr<HandlePool<Pipeline, NUM_PIPELINES>> m_PipelineHandles;

		Array<Vector<BufferHandle>, NUM_FRAMES_IN_FLIGHT> m_BuffersMarkedForDestruction;
		Array<Vector<TextureHandle>, NUM_FRAMES_IN_FLIGHT> m_TexturesMarkedForDestruction;
		Array<Vector<PipelineHandle>, NUM_FRAMES_IN_FLIGHT> m_PipelinesMarkedForDestruction;

		Vector<PipelineHandle> m_PipelinesMarkedForShaderReload;

		struct TempAllocator
		{
			BufferHandle buffer;
			uint32 size = 0;
			uint32 offset = 0;

			void Reset() { offset = 0; }
		};
		// TODO: Could this be replaced by a single, big dynamic buffer?
		Array<TempAllocator, NUM_FRAMES_IN_FLIGHT> m_TempFrameAllocators;
	};

}