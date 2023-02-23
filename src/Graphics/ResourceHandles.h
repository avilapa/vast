#pragma once

#include "Core/Core.h"
#include "Core/Types.h"

#include <utility> 

namespace vast
{

	static const uint16 kInvalidResourceHandle = UINT16_MAX;

	template<typename T>
	class ResourceHandle
	{
	public:
		ResourceHandle() : m_Index(kInvalidResourceHandle) {}

		inline const uint16 GetIdx() const { return m_Index; }
		inline bool IsValid() const { return m_Index != vast::kInvalidResourceHandle; }

	private:
		template<typename T, typename H, const uint32 numResources> friend class ResourceHandlePool;
		ResourceHandle(uint32 idx) : m_Index(static_cast<uint16>(idx)) {}
		uint16 m_Index;
	};

	template<typename T, typename H, const uint32 numResources>
	class ResourceHandlePool
	{
		typedef ResourceHandle<H> HANDLE;
	public:
		ResourceHandlePool() : m_UsedSlots(0) 
		{
			static_assert(sizeof(decltype(numResources)) >= sizeof(decltype(kInvalidResourceHandle)));
			static_assert(numResources > 0 && numResources < kInvalidResourceHandle, "Invalid pool size.");
			// Set up free queue with increasing indices.
			for (uint32 i = 0; i < numResources; ++i)
			{
				m_FreeSlotsQueue[i] = i;
			}
		}

		~ResourceHandlePool() {}

		std::pair<HANDLE, T*> AcquireResource()
		{
			HANDLE h = AcquireHandle();
			if (h.IsValid())
			{
				T* rsc = AccessResource(h);
				VAST_ASSERT(rsc);
				rsc->h = h;
				return { h, rsc };
			}
			return { h, nullptr };
		}

		void FreeResource(HANDLE h)
		{
			// TODO: Reset resource for reuse.
			VAST_ASSERTF(h.IsValid(), "Cannot free invalid index.");
			m_FreeSlotsQueue[--m_UsedSlots] = h.m_Index;
		}

		T* LookupResource(HANDLE h)
		{
			// Access resource with matching index check.
			if (h.IsValid())
			{
				VAST_ASSERT(h.GetIdx() < numResources);
				T* rsc = AccessResource(h);
				VAST_ASSERT(rsc);
				if (rsc->h.GetIdx() == h.GetIdx())
				{
					return rsc;
				}
			}
			return nullptr;
		}

	private:
		HANDLE AcquireHandle()
		{
			uint32 slotIdx = GetNextAvailableSlot();
			if (slotIdx != kInvalidResourceHandle)
			{
				return AllocHandleFromSlot(slotIdx);
			}
			else
			{
				VAST_ERROR("Handle pool capacity exhausted.");
				return HANDLE();
			}
		}

		HANDLE AllocHandleFromSlot(uint32 slotIdx)
		{
			// TODO: Add generation counter to avoid dangling handle / use-after-free.
			return HANDLE(slotIdx);
		}

		T* AccessResource(HANDLE h)
		{
			// Access resource without matching index check.
			return &m_Resources[h.GetIdx()];
		}

		uint32 GetNextAvailableSlot()
		{
			if (m_UsedSlots < numResources)
			{
				uint32 idx = m_FreeSlotsQueue[m_UsedSlots++];
				VAST_ASSERT(idx >= 0 && idx < numResources);
				return idx;
			}
			else
			{
				VAST_ASSERTF(0, "Handle pool capacity exhausted."); // TODO: Resize
				return kInvalidResourceHandle;
			}
		}

		uint32 m_UsedSlots;
		Array<uint32, numResources> m_FreeSlotsQueue;
		Array<T, numResources> m_Resources;
	};

}