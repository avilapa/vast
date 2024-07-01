#include "vastpch.h"
#include "Rendering/ShaderDebug.h"

#include "Shaders/ShaderDebug_shared.h"
#include "Rendering/Imgui.h"

namespace vast::gfx
{

	void ShaderDebug::Reset(ShaderDebug_PerFrame& data)
	{
		data.var0 = float4(0.0f, 0.0f, 0.0f, 0.0f);
		data.var1 = float4(0.0f, 0.0f, 0.0f, 0.0f);
		data.flags = 0;
	}

	// Note: avoid runtime string compositing
	static const char* s_ShaderDebugToggleTags[] =
	{
		"##shaderdebugtoggle0",
		"##shaderdebugtoggle1",
		"##shaderdebugtoggle2",
		"##shaderdebugtoggle3",
		"##shaderdebugtoggle4",
		"##shaderdebugtoggle5",
		"##shaderdebugtoggle6",
		"##shaderdebugtoggle7",
		"##shaderdebugtoggle8",
		"##shaderdebugtoggle9",
		"##shaderdebugtoggle10",
		"##shaderdebugtoggle11",
		"##shaderdebugtoggle12",
		"##shaderdebugtoggle13",
		"##shaderdebugtoggle14",
		"##shaderdebugtoggle15",
		"##shaderdebugtoggle16",
		"##shaderdebugtoggle17",
		"##shaderdebugtoggle18",
		"##shaderdebugtoggle19",
		"##shaderdebugtoggle20",
		"##shaderdebugtoggle21",
		"##shaderdebugtoggle22",
		"##shaderdebugtoggle23",
		"##shaderdebugtoggle24",
		"##shaderdebugtoggle25",
		"##shaderdebugtoggle26",
		"##shaderdebugtoggle27",
		"##shaderdebugtoggle28",
		"##shaderdebugtoggle29",
		"##shaderdebugtoggle30",
		"##shaderdebugtoggle31",
	};

	static const char* s_ShaderDebugToggleDefaultText[] =
	{
		"Shader Debug Toggle 0",
		"Shader Debug Toggle 1",
		"Shader Debug Toggle 2",
		"Shader Debug Toggle 3",
		"Shader Debug Toggle 4",
		"Shader Debug Toggle 5",
		"Shader Debug Toggle 6",
		"Shader Debug Toggle 7",
		"Shader Debug Toggle 8",
		"Shader Debug Toggle 9",
		"Shader Debug Toggle 10",
		"Shader Debug Toggle 11",
		"Shader Debug Toggle 12",
		"Shader Debug Toggle 13",
		"Shader Debug Toggle 14",
		"Shader Debug Toggle 15",
		"Shader Debug Toggle 16",
		"Shader Debug Toggle 17",
		"Shader Debug Toggle 18",
		"Shader Debug Toggle 19",
		"Shader Debug Toggle 20",
		"Shader Debug Toggle 21",
		"Shader Debug Toggle 22",
		"Shader Debug Toggle 23",
		"Shader Debug Toggle 24",
		"Shader Debug Toggle 25",
		"Shader Debug Toggle 26",
		"Shader Debug Toggle 27",
		"Shader Debug Toggle 28",
		"Shader Debug Toggle 29",
		"Shader Debug Toggle 30",
		"Shader Debug Toggle 31",
	};

	static void DrawDebugToggle(uint32& flags, const uint32 i)
	{
		ImGui::CheckboxFlags(s_ShaderDebugToggleTags[i], &flags, (1 << i));
		ImGui::SameLine();
		// TODO: Allow setting persistent names to each var, with a button to clear
		ImGui::Text(s_ShaderDebugToggleDefaultText[i]);
	}

	static const char* s_ShaderDebugVarTags[] =
	{
		"##shaderdebugvar0x",
		"##shaderdebugvar0y",
		"##shaderdebugvar0z",
		"##shaderdebugvar0w",
		"##shaderdebugvar1x",
		"##shaderdebugvar1y",
		"##shaderdebugvar1z",
		"##shaderdebugvar1w",
	};
	
	static const char* s_ShaderDebugVarText[] =
	{
		"Shader Debug Var 0 X",
		"Shader Debug Var 0 Y",
		"Shader Debug Var 0 Z",
		"Shader Debug Var 0 W",
		"Shader Debug Var 1 X",
		"Shader Debug Var 1 Y",
		"Shader Debug Var 1 Z",
		"Shader Debug Var 1 W",
	};

	static void DrawDebugVar(float4& data, const uint32 i)
	{
		uint32 idx = i * 4;
		ImGui::SliderFloat(s_ShaderDebugVarTags[idx], (float*)&data[0], 0.0f, 1.0f);
		ImGui::SameLine();
		ImGui::Text(s_ShaderDebugVarText[idx]);
		++idx;
		ImGui::SliderFloat(s_ShaderDebugVarTags[idx], (float*)&data.y, 0.0f, 1.0f);
		ImGui::SameLine();
		ImGui::Text(s_ShaderDebugVarText[idx]);
		++idx;
		ImGui::SliderFloat(s_ShaderDebugVarTags[idx], (float*)&data[2], 0.0f, 1.0f);
		ImGui::SameLine();
		ImGui::Text(s_ShaderDebugVarText[idx]);
		++idx;
		ImGui::SliderFloat(s_ShaderDebugVarTags[idx], (float*)&data[3], 0.0f, 1.0f);
		ImGui::SameLine();
		ImGui::Text(s_ShaderDebugVarText[idx]);
	}

	void ShaderDebug::OnGUI(ShaderDebug_PerFrame& data)
	{
		const uint32 maxToggles = 32;
		static uint32 numToggles = 4;
		ImGui::BeginDisabled(numToggles >= maxToggles);
		if (ImGui::Button("+"))
		{
			++numToggles;
		}
		ImGui::EndDisabled();

		for (uint32 i = 0; i < numToggles; ++i)
		{
			DrawDebugToggle(data.flags, i);
		}

		ImGui::Separator();
		DrawDebugVar(data.var0, 0);
		ImGui::Separator();
		DrawDebugVar(data.var1, 1);
	}

}
