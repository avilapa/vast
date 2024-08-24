#pragma once

#include "Core/Core.h"

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
// HandlePool provides a fixed size pool of unique handles that can be used as coherent indexing 
// into an array of resources elsewhere.
//
// ================================================================================================

// TODO: We could allow pools to resize after their initial capacity is exhausted.
// TODO: We currently don't handle overflow of generation counters.

namespace vast
{

	template<typename T>
	class Handle
	{
		template<typename H, const uint32 SIZE> friend class HandlePool;
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

	template<typename H, const uint32 SIZE>
	class HandlePool
	{
		static const uint32 kInvalidHandleIdx = UINT32_MAX;
		FreeList<SIZE> m_FreeQueue;
		Array<uint32, SIZE> m_GenerationCounters;

	public:
		Handle<H> AllocHandle()
		{
			// Get next free handle index
			uint32 handleIdx = kInvalidHandleIdx;
			if (m_FreeQueue.GetUsedSlots() < SIZE)
			{
				handleIdx = m_FreeQueue.AllocIndex();
				VAST_ASSERT(handleIdx >= 0 && handleIdx < SIZE && handleIdx != kInvalidHandleIdx);

				// Increase generation counter every time a slot gets (re-)used.
				return Handle<H>(handleIdx, ++m_GenerationCounters[handleIdx]);
			}

			VAST_ASSERTF(0, "Pool capacity exhausted.");
			return Handle<H>();
		}

		void FreeHandle(Handle<H> h)
		{
			VAST_ASSERTF(h.IsValid(), "Cannot free invalid handle.");
			m_FreeQueue.FreeIndex(h.GetIndex());
		}
	};

	template<typename T, typename H, const uint32 SIZE>
	class ResourceHandler
	{
	public:
		T& AcquireResource(Handle<H> h)
		{
			T& r = AccessResource(h);
			r.h = h;
			return r;
		}

		// Get resource with matching id check
		T& LookupResource(Handle<H> h)
		{
			T& r = AccessResource(h);
			// Note: If it breaks here it is likely to be a 'use after free' situation.
			VAST_VERIFYF(h == r.h, "The handle provided doesn't match internal handle.");
			return r;
		}

		T& ReleaseResource(Handle<H> h)
		{
			T& r = LookupResource(h);
			r.h = Handle<H>();
			return r;
		}

	private:
		// Get resource without matching id check
		T& AccessResource(Handle<H> h)
		{
			VAST_ASSERT(h.IsValid());
			return m_Resources[h.GetIndex()];
		}

		Array<T, SIZE> m_Resources;
	};

}
