#pragma once

#include "Core/Core.h"
#include "Core/Types.h"

#include <utility> // std::pair

// ==================================== RESOURCE HANDLING =========================================
//
// In vast, the user manages GPU resources via 'handles' rather than raw pointers. A handle is, in
// essence, an opaque, trivially copyable, unique identifier. Resource ownership is delegated to
// the engine and the user only holds handles that refer to each resource internally. You can read
// more about the design pattern here: https://floooh.github.io/2018/06/17/handles-vs-pointers.html
//
// ResourceHandle is a type safe class to avoid type-checking IDs at runtime and to increase the
// amount of handles supported by each pool.
//
// ResourceHandlePool is a fixed size pool holding an array T of the resources, and is in charge of
// managing creation, validation and release of handles as well as ensuring the reusability of the
// resources these point to after their previous instances are no longer needed.
//
// A successful call to AcquireResource returns a valid handle and a pointer to the data it handles
// as an std::pair:
//
//		auto [h, buffer] = m_Buffers->AcquireResource();
//
// TODO: UNDER CONSTRUCTION
//
// ================================================================================================

namespace vast::gfx
{

	template<typename T>
	class ResourceHandle
	{
	public:
		ResourceHandle() : m_Index(0), m_Generation(0) {}
		bool IsValid() const { return m_Generation != 0; }
		bool operator==(const ResourceHandle<T>& o) const { return m_Index == o.m_Index && m_Generation == o.m_Generation; }
		bool operator!=(const ResourceHandle<T>& o) const { return m_Index != o.m_Index || m_Generation != o.m_Generation; }

		uint64 GetHashKey() const { return (((uint64)m_Index) << 16) + (uint64)m_Generation; }
		uint32 GetIndex() const { return m_Index; }
		uint32 GetGeneration() const { return m_Generation; }

	private:
		ResourceHandle(uint32 index, uint32 generation) : m_Index(index), m_Generation(generation) {}

		uint32 m_Index;
		uint32 m_Generation;

		template<typename T, typename Timpl, const uint32 numResources> friend class ResourceHandlePool;
	};

	// TODO: Could separate knowledge from the object from the resource handle pool, and instead allocate a big buffer of plain memory.

	template<typename T, typename Timpl, const uint32 numResources>
	class ResourceHandlePool
	{
		typedef ResourceHandle<T> H;
	public:
		ResourceHandlePool() : m_UsedSlots(0) 
		{
			// Set up free queue with increasing indices.
			for (uint32 i = 0; i < numResources; ++i)
			{
				m_FreeSlotsQueue[i] = i;
				m_GenerationCounters[i] = 0;
			}
		}

		~ResourceHandlePool() {}

		std::pair<H, Timpl*> AcquireResource()
		{
			H h = AcquireHandle();
			if (h.IsValid())
			{
				Timpl* rsc = AccessResource(h);
				VAST_ASSERT(rsc);
				rsc->m_Handle = h;
				return { h, rsc };
			}
			else
			{
				VAST_CRITICAL("[resource] Failed to acquire new {} resource.", T::GetStaticResourceTypeName());
				return { h, nullptr };
			}
		}

		void FreeResource(const H& h)
		{
			VAST_ASSERTF(h.IsValid(), "Cannot free invalid index.");
			Timpl* rsc = LookupResource(h);
			if (rsc)
			{
				rsc->m_Handle = H();
				FreeHandle(h);
			}
			else
			{
				VAST_ERROR("[resource] Failed to free handle. Mismatched index with internal resource.");
			}
		}

		// Returned pointers should never be stored anywhere, and they should only be used within small code blocks.
		Timpl* LookupResource(const H& h)
		{
			// Access resource with matching id check.
			if (h.IsValid())
			{
				VAST_ASSERT(h.GetIndex() < numResources);
				Timpl* rsc = AccessResource(h);
				VAST_ASSERT(rsc);
				if (rsc->m_Handle == h)
				{
					return rsc;
				}
			}
			return nullptr;
		}

	private:
		typedef uint32 SlotIdx;
		static const SlotIdx kInvalidSlotIndex = UINT32_MAX;

		SlotIdx GetNextAvailableSlotIndex()
		{
			if (m_UsedSlots < numResources)
			{
				SlotIdx slotIdx = m_FreeSlotsQueue[m_UsedSlots++];
				VAST_ASSERT(slotIdx >= 0 && slotIdx < numResources);
				return slotIdx;
			}
			else
			{
				VAST_ASSERTF(0, "Handle pool capacity exhausted.");
				return kInvalidSlotIndex;
			}
		}

		H AllocHandleFromSlotIndex(SlotIdx slotIdx)
		{
			// TODO: We don't handle overflow on the generation counters.
			return H(slotIdx, ++m_GenerationCounters[slotIdx]);
		}

		H AcquireHandle()
		{
			SlotIdx slotIdx = GetNextAvailableSlotIndex();

			if (slotIdx != kInvalidSlotIndex)
			{
				H h = AllocHandleFromSlotIndex(slotIdx);
				VAST_INFO("[resource] Acquired new {} handle with index {}, gen {} ({}/{})."
					, T::GetStaticResourceTypeName(), h.GetIndex(), h.GetGeneration(), m_UsedSlots, numResources);
				return h;
			}
			else
			{
				VAST_ERROR("[resource] {} handle pool capacity exhausted.", T::GetStaticResourceTypeName());
				return H();
			}
		}

		SlotIdx GetSlotIndexFromHandle(const H& h) const
		{
			return static_cast<SlotIdx>(h.GetIndex());
		}

		Timpl* AccessResource(const H& h)
		{
			// Access resource without matching id check.
			return &m_Resources[GetSlotIndexFromHandle(h)];
		}

		void FreeHandle(const H& h)
		{
			m_FreeSlotsQueue[--m_UsedSlots] = GetSlotIndexFromHandle(h);
			VAST_INFO("[resource] Freed {} handle with index {}, gen {} ({}/{})."
				, T::GetStaticResourceTypeName(), h.GetIndex(), h.GetGeneration(), m_UsedSlots, numResources);
		}

		uint32 m_UsedSlots;
		Array<SlotIdx, numResources> m_FreeSlotsQueue;
		Array<uint32, numResources> m_GenerationCounters;
		Array<Timpl,  numResources> m_Resources;
	};

}