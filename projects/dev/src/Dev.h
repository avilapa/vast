#pragma once

#include "vast.h"
#include "shaders_shared.h"

using namespace vast;

class Dev : public WindowedApp
{
public:
	Dev(int argc, char** argv);
	~Dev();

private:
	void Update() override;
	void Render() override;
	void OnGUI();

	void CreateTriangleResources();
	void CreateCubeResources();
	void CreateSphereResources();

	gfx::TextureHandle m_DepthRT;

	gfx::PipelineHandle m_TrianglePso;
	gfx::BufferHandle m_TriangleVtxBuf;
	uint32 m_TriangleVtxBufIdx;

	gfx::PipelineHandle m_MeshPso;
	gfx::ShaderResourceProxy m_MeshCbvBufProxy;

	gfx::BufferHandle m_CubeVtxBuf;
	gfx::BufferHandle m_CubeCbvBuf;	
	gfx::TextureHandle m_CubeColorTex;
	MeshCB m_CubeCB;

	gfx::BufferHandle m_SphereVtxBuf;
	gfx::BufferHandle m_SphereIdxBuf;
	gfx::BufferHandle m_SphereCbvBuf;
	gfx::TextureHandle m_SphereColorTex;
	MeshCB m_SphereCB;
};
