#pragma once

 #include "Core/Handles.h"

namespace vast::gfx
{
	enum class ResourceType
	{
		UNKNOWN,
		BUFFER,
		TEXTURE,
		PIPELINE,
		COUNT,
	};

	constexpr char* g_ResourceTypeNames[]
	{
		"Unknown",
		"Buffer",
		"Texture",
		"Pipeline",
	};

	static_assert(NELEM(g_ResourceTypeNames) == IDX(ResourceType::COUNT));	

	template<typename T>
	class Resource
	{
		template<typename T, typename H, const uint32 SIZE> friend class HandlePool;
	public:
		Resource(ResourceType type) : m_Type(type) {}

		const ResourceType GetResourceType() { return m_Type; }
		const char* GetResourceTypeName() { return g_ResourceTypeNames[IDX(m_Type)]; }

	protected:
		ResourceType m_Type;

	private:
		Handle<T> m_Handle;
	};

#define __VAST_RESOURCE_TYPE_COMMON_DECL(className, resourceType)											\
	className() : Resource<className>(resourceType) {}														\
	static constexpr ResourceType GetStaticResourceType() { return resourceType; }							\
	static constexpr char* GetStaticResourceTypeName() { return g_ResourceTypeNames[IDX(resourceType)]; }

	class Buffer : public Resource<Buffer>
	{
	public:
		__VAST_RESOURCE_TYPE_COMMON_DECL(Buffer, ResourceType::BUFFER);
	};

	class Texture : public Resource<Texture>
	{
	public:
		__VAST_RESOURCE_TYPE_COMMON_DECL(Texture, ResourceType::TEXTURE);
	};

	class Pipeline : public Resource<Pipeline>
	{
	public:
		__VAST_RESOURCE_TYPE_COMMON_DECL(Pipeline, ResourceType::PIPELINE);
	};

}