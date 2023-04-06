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
		ResourceHandle() : m_Bits({ 0, 0 }) {}
		inline bool IsValid() const { return m_Bits.gen != 0; }

	private:
		ResourceHandle(const uint32 idx, const uint32 gen) : m_Bits({ idx, gen }) {}

		inline const uint32 GetIndex() const { return m_Bits.idx; }
		inline const uint32 GetGeneration() const { return m_Bits.gen; }
		inline const uint32 GetPackedId() const { return m_PackedId; }

		static constexpr uint32 GetIndexBits() { return 20; }
		static constexpr uint32 GetGenerationBits() { return (sizeof(uint32) * 8) - GetIndexBits(); }

		union
		{
			struct Bitfield { uint32 idx : GetIndexBits(), gen : GetGenerationBits(); } m_Bits;
			uint32 m_PackedId;

			// This doesn't count single bits, but it rounds to bytes. But at least it prevents using +32 bits!
			static_assert(sizeof(Bitfield) <= sizeof(m_PackedId), "Mismatched index bits in resource handle.");
		};

		template<typename T, typename Timpl, const uint32 numResources> friend class ResourceHandlePool;
	};

	// TODO: Could separate knowledge from the object from the resource handle pool, and instead allocate a big buffer of plain memory.

	template<typename T, typename Timpl, const uint32 numResources>
	class ResourceHandlePool
	{
		typedef ResourceHandle<T> HANDLE;
		static_assert(numResources > 0 && numResources < (1 << HANDLE::GetIndexBits()), "Invalid pool size.");
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

		std::pair<HANDLE, Timpl*> AcquireResource()
		{
			HANDLE h = AcquireHandle();
			if (h.IsValid())
			{
				Timpl* rsc = AccessResource(h);
				VAST_ASSERT(rsc);
				rsc->m_Handle = h;
				return { h, rsc };
			}
			return { h, nullptr };
		}

		void FreeResource(const HANDLE& h)
		{
			VAST_ASSERTF(h.IsValid(), "Cannot free invalid index.");
			Timpl* rsc = LookupResource(h);
			if (rsc)
			{
				rsc->m_Handle = HANDLE();
				FreeHandle(h);
			}
			else
			{
				VAST_ERROR("Failed to free handle. Mismatched index with internal resource.");
			}
		}

		// Returned pointers should never be stored anywhere, and they should only be used within small code blocks.
		Timpl* LookupResource(const HANDLE& h)
		{
			// Access resource with matching id check.
			if (h.IsValid())
			{
				VAST_ASSERT(h.GetIndex() < numResources);
				Timpl* rsc = AccessResource(h);
				VAST_ASSERT(rsc);
				if (rsc->m_Handle.GetPackedId() == h.GetPackedId())
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
			if (slotIdx != kInvalidSlotIndex)
			{
				
				HANDLE h = AllocHandleFromSlot(slotIdx);
				VAST_INFO("[resource] Acquired new {} handle with index {}, gen {} ({}/{}).", T::GetStaticResourceTypeName(), h.GetIndex(), h.GetGeneration(), m_UsedSlots, numResources);
				return h;
			}
			else
			{
				VAST_ERROR("Handle pool capacity exhausted.");
				return HANDLE();
			}
		}

		HANDLE AllocHandleFromSlot(uint32 slotIdx)
		{
			// TODO: We don't handle overflow on the generation counters.
			return HANDLE({ slotIdx, ++m_GenerationCounters[slotIdx] });
		}

		uint32 GetSlotIndexFromHandle(const HANDLE& h) const
		{
			return static_cast<uint32>(h.GetIndex());
		}

		Timpl* AccessResource(const HANDLE& h)
		{
			// Access resource without matching id check.
			return &m_Resources[GetSlotIndexFromHandle(h)];
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
				VAST_ASSERTF(0, "Handle pool capacity exhausted.");
				return kInvalidSlotIndex;
			}
		}

		void FreeHandle(const HANDLE& h)
		{
			m_FreeSlotsQueue[--m_UsedSlots] = GetSlotIndexFromHandle(h);
			VAST_INFO("[resource] Freed {} handle with index {}, gen {} ({}/{}).", T::GetStaticResourceTypeName(), h.GetIndex(), h.GetGeneration(), m_UsedSlots, numResources);
		}

		static const uint32 kInvalidSlotIndex = UINT32_MAX;

		uint32 m_UsedSlots;
		Array<uint32, numResources> m_FreeSlotsQueue;
		Array<uint32, numResources> m_GenerationCounters;
		Array<Timpl,  numResources> m_Resources;
	};

}