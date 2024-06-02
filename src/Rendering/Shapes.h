#pragma once

#include "Core/Types.h"
#include "Shaders/VertexInputs_shared.h"

namespace vast::gfx
{

	struct Cube
	{
		static inline Array<Vtx3fPos, 8> s_VerticesIndexed_Pos =
		{ {
			{{-1.0f,  1.0f,  1.0f }},
			{{ 1.0f,  1.0f,  1.0f }},
			{{-1.0f, -1.0f,  1.0f }},
			{{ 1.0f, -1.0f,  1.0f }},
			{{-1.0f,  1.0f, -1.0f }},
			{{ 1.0f,  1.0f, -1.0f }},
			{{-1.0f, -1.0f, -1.0f }},
			{{ 1.0f, -1.0f, -1.0f }},
		} };

		static inline Array<uint16, 36> s_Indices =
		{ {
			0, 1, 2,
			1, 3, 2,
			4, 6, 5,
			5, 6, 7,
			0, 2, 4,
			4, 2, 6,
			1, 5, 3,
			5, 7, 3,
			0, 4, 1,
			4, 5, 1,
			2, 3, 6,
			6, 3, 7,
		} };

		static inline Array<Vtx3fPos3fNormal2fUv, 36> s_Vertices_PosNormalUv =
		{ {
			{{ 1.0f,-1.0f, 1.0f }, { 0.0f,-1.0f, 0.0f }, { 1.0f, 1.0f }},
			{{ 1.0f,-1.0f,-1.0f }, { 0.0f,-1.0f, 0.0f }, { 1.0f, 0.0f }},
			{{-1.0f,-1.0f, 1.0f }, { 0.0f,-1.0f, 0.0f }, { 0.0f, 1.0f }},
			{{-1.0f,-1.0f, 1.0f }, { 0.0f,-1.0f, 0.0f }, { 0.0f, 1.0f }},
			{{ 1.0f,-1.0f,-1.0f }, { 0.0f,-1.0f, 0.0f }, { 1.0f, 0.0f }},
			{{-1.0f,-1.0f,-1.0f }, { 0.0f,-1.0f, 0.0f }, { 0.0f, 0.0f }},
			{{ 1.0f, 1.0f,-1.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f }},
			{{ 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }},
			{{-1.0f, 1.0f,-1.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f }},
			{{-1.0f, 1.0f,-1.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f }},
			{{ 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }},
			{{-1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }},
			{{-1.0f,-1.0f,-1.0f }, {-1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }},
			{{-1.0f, 1.0f,-1.0f }, {-1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }},
			{{-1.0f,-1.0f, 1.0f }, {-1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f }},
			{{-1.0f,-1.0f, 1.0f }, {-1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f }},
			{{-1.0f, 1.0f,-1.0f }, {-1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }},
			{{-1.0f, 1.0f, 1.0f }, {-1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }},
			{{-1.0f,-1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }},
			{{-1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }},
			{{ 1.0f,-1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }},
			{{ 1.0f,-1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }},
			{{-1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }},
			{{ 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }},
			{{ 1.0f,-1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }},
			{{ 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }},
			{{ 1.0f,-1.0f,-1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f }},
			{{ 1.0f,-1.0f,-1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f }},
			{{ 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }},
			{{ 1.0f, 1.0f,-1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }},
			{{ 1.0f,-1.0f,-1.0f }, { 0.0f, 0.0f,-1.0f }, { 1.0f, 1.0f }},
			{{ 1.0f, 1.0f,-1.0f }, { 0.0f, 0.0f,-1.0f }, { 1.0f, 0.0f }},
			{{-1.0f,-1.0f,-1.0f }, { 0.0f, 0.0f,-1.0f }, { 0.0f, 1.0f }},
			{{-1.0f,-1.0f,-1.0f }, { 0.0f, 0.0f,-1.0f }, { 0.0f, 1.0f }},
			{{ 1.0f, 1.0f,-1.0f }, { 0.0f, 0.0f,-1.0f }, { 1.0f, 0.0f }},
			{{-1.0f, 1.0f,-1.0f }, { 0.0f, 0.0f,-1.0f }, { 0.0f, 0.0f }},
		} };

	};

	struct Sphere
	{
		static void ConstructUVSphere(const float radius, const uint32 vCount, const uint32 hCount, Vector<Vtx3fPos3fNormal2fUv>& vtx, Vector<uint16>& idx)
		{
			// Generate vertices
			float invR = 1.0f / radius;
			float vStep = float(PI) / vCount;
			float hStep = 2.0f * float(PI) / hCount;

			for (uint32 v = 0; v < vCount + 1; ++v)
			{
				float vAngle = float(PI) * 0.5f - float(v) * vStep;
				float xz = radius * cos(vAngle);
				float y = radius * sin(vAngle);
				for (uint32 h = 0; h < hCount + 1; ++h)
				{
					float hAngle = h * hStep;
					float x = xz * cos(hAngle);
					float z = xz * sin(hAngle);

					Vtx3fPos3fNormal2fUv i;
					i.pos = s_float3(x, y, z);
					i.normal = s_float3(x * invR, y * invR, z * invR);
					i.uv = s_float2(float(h) / hCount, float(v) / vCount);
					vtx.push_back(i);
				}
			}
			Vtx3fPos3fNormal2fUv i;
			i.pos = s_float3(0.0f, -1.0f, 0.0f);
			vtx.push_back(i);

			// Generate indices
			uint32 k1, k2;
			for (uint32 v = 0; v < vCount; ++v)
			{
				k1 = v * (hCount + 1);
				k2 = k1 + hCount + 1;
				for (uint32 h = 0; h < hCount + 1; ++h, ++k1, ++k2)
				{
					if (v != 0)
					{
						idx.push_back(static_cast<uint16>(k1));
						idx.push_back(static_cast<uint16>(k2));
						idx.push_back(static_cast<uint16>(k1 + 1));
					}

					if (v != (vCount - 1))
					{
						idx.push_back(static_cast<uint16>(k1 + 1));
						idx.push_back(static_cast<uint16>(k2));
						idx.push_back(static_cast<uint16>(k2 + 1));
					}
				}
			}
		}
	};

}
