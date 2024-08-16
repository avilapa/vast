#include "vastpch.h"
#include "Graphics/GPUProfiler.h"
#include "Graphics/GraphicsBackend.h"
#include "Graphics/ResourceManager.h"

namespace vast
{

	GPUProfiler::GPUProfiler(ResourceManager& resourceManager)
		: m_ResourceManager(resourceManager)
		, m_TimestampCount(0)
		, m_TimestampFrequency(0.0)
		, m_TimestampsReadbackBuf({})
		, m_TimestampData({ nullptr })
	{
		m_TimestampFrequency = gfx::GetTimestampFrequency();

		BufferDesc readbackBufferDesc
		{
			.size = NUM_TIMESTAMP_QUERIES * sizeof(uint64),
			.usage = ResourceUsage::READBACK
		};

		for (uint32 i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
		{
			m_TimestampsReadbackBuf[i] = m_ResourceManager.CreateBuffer(readbackBufferDesc);
			m_TimestampData[i] = reinterpret_cast<const uint64*>(m_ResourceManager.GetBufferData(m_TimestampsReadbackBuf[i]));
		}
	}

	GPUProfiler::~GPUProfiler()
	{
		for (uint32 i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
		{
			m_ResourceManager.DestroyBuffer(m_TimestampsReadbackBuf[i]);
		}
	}

	uint32 GPUProfiler::BeginTimestamp()
	{
		gfx::BeginTimestamp(m_TimestampCount * 2);
		return m_TimestampCount++;
	}

	void GPUProfiler::EndTimestamp(uint32 timestampIdx)
	{
		gfx::EndTimestamp(timestampIdx * 2 + 1);
	}

	void GPUProfiler::CollectTimestamps()
	{
		if (m_TimestampCount == 0)
			return;

		gfx::CollectTimestamps(m_TimestampsReadbackBuf[gfx::GetFrameId()], m_TimestampCount * 2);
		m_TimestampCount = 0;
	}

	double GPUProfiler::GetTimestampDuration(uint32 timestampIdx)
	{
		const uint64* data = m_TimestampData[gfx::GetFrameId()];
		VAST_ASSERT(data && m_TimestampFrequency);

		uint64 tStart = data[timestampIdx * 2];
		uint64 tEnd = data[timestampIdx * 2 + 1];

		if (tEnd > tStart)
		{
			return double(tEnd - tStart) / m_TimestampFrequency;
		}

		return 0.0;
	}

}
