#pragma once

#include "Core/Core.h"

#include <chrono>

namespace vast
{
	class Timer
	{
	public:
		Timer()
		{
			m_StartTime = std::chrono::high_resolution_clock::now();
			m_DeltaTime = m_ElapsedTime = m_StartTime - m_StartTime;
		}
		void Update()
		{
			auto currentTime = std::chrono::high_resolution_clock::now() - m_StartTime;
			m_DeltaTime = currentTime - m_ElapsedTime;
			m_ElapsedTime = currentTime;
		}

		template<typename T>
		T GetDeltaSeconds() const
		{
			return std::chrono::duration_cast<std::chrono::duration<T>>(m_DeltaTime).count();
		}

		template<typename T>
		T GetElapsedSeconds() const
		{
			return std::chrono::duration_cast<std::chrono::duration<T>>(m_ElapsedTime).count();
		}

		template<typename T>
		T GetDeltaMilliseconds() const
		{
			return std::chrono::duration_cast<std::chrono::duration<T, std::milli>>(m_DeltaTime).count();
		}

		template<typename T>
		T GetElapsedMilliseconds() const
		{
			return std::chrono::duration_cast<std::chrono::duration<T, std::milli>>(m_ElapsedTime).count();
		}

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::nanoseconds> m_StartTime;
		std::chrono::nanoseconds m_DeltaTime;
		std::chrono::nanoseconds m_ElapsedTime;
	};
}