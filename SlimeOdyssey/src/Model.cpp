#include "Model.h"
#include "imgui.h"

void PBRMaterial::ImGuiDebug()
{
    if (!materialResource)
    {
		return;
    }

    ImGui::ColorEdit3("Albedo", &materialResource->config.albedo.x);
    ImGui::DragFloat("Metallic", &materialResource->config.metallic, 0.1f);
    ImGui::DragFloat("Roughness", &materialResource->config.roughness, 0.1f);
    ImGui::DragFloat("AO", &materialResource->config.ao, 0.1f);
}

void BasicMaterial::ImGuiDebug()
{
	if (!materialResource)
	{
		return;
	}

	ImGui::ColorEdit3("Albedo", &materialResource->config.albedo.x);
}

void Model::ImGuiDebug()
{
    ImGui::Text("Pipeline: %s", modelResource->pipeLineName.c_str());
    ImGui::Text("Vertex Count: %d", modelResource->vertices.size());
    ImGui::Text("Index Count: %d", modelResource->indices.size());
}

void Transform::ImGuiDebug()
{
    ImGui::DragFloat3("Position", &position.x, 0.1f);
    ImGui::DragFloat3("Rotation", &rotation.x, 0.1f);
    ImGui::DragFloat3("Scale", &scale.x, 0.1f);
}
