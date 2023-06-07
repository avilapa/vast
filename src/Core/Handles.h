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
// The Handle class takes a template parameter to make handles type-safe (avoids type-checking IDs
// at runtime). This means that each handle type must have it's own pool, which increases the total
// amount of possible unique objects we can have, since indices are duplicated across each handle
// type. 
//
// HandlePool is a fixed size pool owning an array T of the resources. It manages allocation,
// validation and releasing of handles, as well as ensuring the re-usability of the resources these 
// point to after their previous instances are no longer needed. However it has no knowledge of the
// data it owns, which should still be initialized and reset by the engine.
//
// ================================================================================================

// TODO: We could allow pools to resize after their initial capacity is exhausted.
// TODO: We currently don't handle overflow of generation counters.

namespace vast::gfx
{

	template<typename T>
	class Handle
	{
		template<typename T, typename H, const uint32 SIZE> friend class HandlePool;
	public:
		Handle() : m_Index(0), m_Generation(0) {}
		bool IsValid() const { return m_Generation != 0; }
		bool operator==(const Handle<T>& o) const { return m_Index == o.m_Index && m_Generation == o.m_Generation; }
		bool operator!=(const Handle<T>& o) const { return m_Index != o.m_Index || m_Generation != o.m_Generation; }

		uint32 GetIndex() const { return m_Index; }
		uint32 GetGeneration() const { return m_Generation; }
		uint64 GetHashKey() const { return (((uint64)m_Index) << 16) + (uint64)m_Generation; }

	private:
		Handle(uint32 index, uint32 generation) : m_Index(index), m_Generation(generation) {}

		uint32 m_Index;
		uint32 m_Generation;
	};

	template<typename T, typename H, const uint32 SIZE>
	class HandlePool
	{
	public:
		HandlePool() : m_UsedSlots(0), m_GenerationCounters() {	InitializeFreeQueue(); }
		~HandlePool() {}

		// Allocates a new resource handle and returns a pointer to the resource for initialization.
		std::pair<Handle<H>, T*> AcquireResource()
		{
			Handle<H> h = AllocHandle();

			if (h.IsValid())
			{
				T* rsc = AccessResource(h);
				VAST_ASSERT(rsc);
				rsc->m_Handle = h;
				VAST_TRACE("[handles] Acquired new {} handle with index {}, gen {} ({}/{}).", H::GetStaticResourceTypeName(), h.GetIndex(), h.GetGeneration(), m_UsedSlots, SIZE);
				return { h, rsc };
			}
			else
			{
				VAST_CRITICAL("[handles] Failed to acquire new {} resource.", H::GetStaticResourceTypeName());
				return { h, nullptr };
			}
		}

		// Resets internal resource handle and returns handle index to the free queue for reuse.
		void FreeResource(Handle<H> h)
		{
			VAST_ASSERTF(h.IsValid(), "Cannot free invalid index.");

			T* rsc = LookupResource(h);
			if (rsc)
			{
				rsc->m_Handle = Handle<H>(); // Reset internal resource handle to invalid handle.
				FreeHandle(h);
				VAST_TRACE("[handles] Freed {} handle with index {}, gen {} ({}/{}).", H::GetStaticResourceTypeName(), h.GetIndex(), h.GetGeneration(), m_UsedSlots, SIZE);
			}
			else
			{
				VAST_ASSERTF(0, "Failed to free handle. Check for double free.");
			}
		}

		// Provides resource access with matching handle index check.
		// Note: Returned pointers should never be stored anywhere, and they should only be used 
		// within small code blocks.
		T* LookupResource(Handle<H> h)
		{
			VAST_ASSERTF(h.IsValid() && h.GetIndex() < SIZE, "Cannot lookup invalid index.");
			T* rsc = AccessResource(h);
			VAST_ASSERT(rsc);
			if (rsc->m_Handle == h)
			{
				return rsc;
			}
			else
			{
				VAST_ERROR("[handles] Failed to lookup resource. Mismatched handle index with internal resource.");
				return nullptr;
			}
		}

	private:
		static const uint32 kInvalidHandleIdx = UINT32_MAX;

		uint32 m_UsedSlots;
		Array<uint32, SIZE> m_FreeQueue;
		Array<uint32, SIZE> m_GenerationCounters;
		Array<T, SIZE> m_Resources;

	private:
		// Sets up free queue with increasing indices.
		void InitializeFreeQueue()
		{
			for (uint32 i = 0; i < SIZE; ++i)
			{
				m_FreeQueue[i] = i;
			}
		}

		uint32 GetNextFreeHandleIndex()
		{
			if (m_UsedSlots < SIZE)
			{
				uint32 idx = m_FreeQueue[m_UsedSlots++];
				VAST_ASSERT(idx >= 0 && idx < SIZE);
				return idx;
			}
			else
			{
				VAST_ERROR("[resource] [handles] {} handle pool capacity exhausted.", H::GetStaticResourceTypeName());
				VAST_ASSERTF(0, "Pool capacity exhausted.");
				return kInvalidHandleIdx;
			}
		}

		Handle<H> AllocHandle()
		{
			uint32 handleIdx = GetNextFreeHandleIndex();

			if (handleIdx != kInvalidHandleIdx)
			{
				return Handle<H>(handleIdx, ++m_GenerationCounters[handleIdx]);
			}
			else
			{
				return Handle<H>();
			}
		}

		void FreeHandle(Handle<H> h)
		{
			m_FreeQueue[--m_UsedSlots] = h.GetIndex();
		}

		// Access resource without matching index handle check.
		T* AccessResource(Handle<H> h)
		{
			return &m_Resources[h.GetIndex()];
		}
	};

}