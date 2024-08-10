#include "Model.h"
#include "imgui.h"

void Model::ImGuiDebug()
{
    ImGui::Text("Pipeline: %s", modelResource->pipelineName.c_str());
    ImGui::Text("Vertex Count: %d", modelResource->vertices.size());
    ImGui::Text("Index Count: %d", modelResource->indices.size());
}

void Transform::ImGuiDebug()
{
    ImGui::DragFloat3("Position", &position.x, 0.1f);
    ImGui::DragFloat3("Rotation", &rotation.x, 0.1f);
    ImGui::DragFloat3("Scale", &scale.x, 0.1f);
}
