#pragma once

#include "Core/Core.h"
#include "Core/Types.h"

#include <utility> 

namespace vast
{

	static const uint16 kInvalidHandle = UINT16_MAX;

	template<typename T>
	class Handle
	{
		template<typename T, const uint16 maxHandles> friend class HandlePool;
	public:
		Handle() : m_Index(kInvalidHandle) {}

		inline const uint16 GetIdx() const { return m_Index; }
		inline bool IsValid() const { return m_Index != vast::kInvalidHandle; }

	private:
		Handle(uint16 idx) : m_Index(idx) {}

		uint16 m_Index;
	};

	template<typename T, const uint16 maxHandles>
	class HandlePool
	{
	public:
		HandlePool() : m_UsedHandles(0) 
		{
			static_assert(maxHandles > 0 && maxHandles < UINT16_MAX, "Invalid pool size.");

			for (uint16 i = 0; i < maxHandles; ++i)
			{
				m_FreeSlots[i] = i;
			}
		}

		~HandlePool() {}

		Handle<T> Acquire()
		{
			if (m_UsedHandles < maxHandles)
			{
				return Handle<T>(m_FreeSlots[m_UsedHandles++]);
			}
			else
			{
				// TODO: Resize
				VAST_ASSERTF(0, "Handle pool capacity exhausted.");
				return Handle<T>();
			}
		}

		void Free(Handle<T> h)
		{
			VAST_ASSERTF(h.IsValid(), "Cannot free invalid index.");
			m_FreeSlots[--m_UsedHandles] = h.m_Index;
		}

	private:
		uint16 m_UsedHandles;
		Array<uint16, maxHandles> m_FreeSlots;
	};

	template<typename T, typename H, const uint16 numResources>
	class ResourceManager
	{
	public:
		std::pair<Handle<H>, T*> AcquireResource()
		{
			Handle<H> h = m_Handles.Acquire();
			T* rsc = LookupResource(h);
			return { h, rsc };
		}

		T* FreeResource(const Handle<H>& h)
		{
			T* rsc = LookupResource(h);
			m_Handles.Free(h);
			return rsc;
		}
		
		T* LookupResource(const Handle<H>& h)
		{
			VAST_ASSERT(h.IsValid());
			T* rsc = &m_Resources[h.GetIdx()];
			VAST_ASSERT(rsc);
			return rsc;
		}

	private:
		HandlePool<H, numResources> m_Handles;
		Array<T, numResources> m_Resources;
	};
}