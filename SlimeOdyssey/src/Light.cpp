#include "Light.h"
#include "imgui.h"

void PointLightObject::ImGuiDebug()
{
    ImGui::Text("Point Light");
    ImGui::DragFloat3("Position", &light.pos.x, 0.1f);
    ImGui::ColorEdit3("Colour", &light.colour.x);
    ImGui::DragFloat3("View", &light.view.x, 0.1f);
    ImGui::DragFloat("Ambient Strength", &light.ambientStrength, 0.1f);
    ImGui::DragFloat("Specular Strength", &light.specularStrength, 0.1f);
    ImGui::DragFloat("Shininess", &light.shininess, 0.1f);

}

void DirectionalLightObject::ImGuiDebug()
{
    ImGui::Text("Directional Light");
    ImGui::DragFloat3("Direction", &light.direction.x, 0.1f);
    ImGui::ColorEdit3("Colour", &light.lightColor.x);
}
