#pragma once

#include "Core/Core.h"
#include "Graphics/Resources.h"

namespace vast
{
	class GPUResourceManager;

	class GPUProfiler
	{
		friend class GraphicsContext;
	public:
		GPUProfiler(GPUResourceManager& resourceManager);
		~GPUProfiler();
		
		uint32 BeginTimestamp();
		void EndTimestamp(uint32 timestampIdx);

		// Call after EndFrame() and before the next BeginFrame() to collect data on the frame that just ended.
		double GetTimestampDuration(uint32 timestampIdx);

	private:
		void CollectTimestamps();

	private:
		GPUResourceManager& m_ResourceManager;

		uint32 m_TimestampCount;
		double m_TimestampFrequency;
		Array<BufferHandle, NUM_FRAMES_IN_FLIGHT> m_TimestampsReadbackBuf;
		Array<const uint64*, NUM_FRAMES_IN_FLIGHT> m_TimestampData;
	};


}