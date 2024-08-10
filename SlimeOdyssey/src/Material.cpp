#include "Material.h"

#include "imgui.h"

void PBRMaterial::ImGuiDebug()
{
	if (!materialResource)
	{
		return;
	}

	if (ImGui::ColorEdit3("Albedo", &materialResource->config.albedo.x))
	{
		materialResource->dirty = true;
	}
	
	if (ImGui::DragFloat("Metallic", &materialResource->config.metallic, 0.01f, 0.0f, 1.0f))
	{
		materialResource->dirty = true;
	}
	
	if (ImGui::DragFloat("Roughness", &materialResource->config.roughness, 0.01f, 0.0f, 1.0f))
	{
		materialResource->dirty = true;
	}

	if (ImGui::DragFloat("AO", &materialResource->config.ao, 0.01f, 0.0f, 1.0f))
	{
		materialResource->dirty = true;
	}
}

void BasicMaterial::ImGuiDebug()
{
	if (!materialResource)
	{
		return;
	}

	if (ImGui::ColorEdit3("Albedo", &materialResource->config.albedo.x))
	{
		materialResource->dirty = true;
	}
}
