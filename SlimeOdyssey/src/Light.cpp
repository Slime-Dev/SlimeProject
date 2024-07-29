#include "Light.h"
#include "imgui.h"

void PointLightObject::ImGuiDebug()
{
    ImGui::Text("Point Light");
    ImGui::DragFloat3("Position", &light.pos.x, 0.1f);
    ImGui::ColorEdit3("Colour", &light.colour.x);
    ImGui::DragFloat3("View", &light.direction.x, 0.1f);
    ImGui::DragFloat("Ambient Strength", &light.ambientStrength, 0.1f);
    ImGui::DragFloat("Specular Strength", &light.specularStrength, 0.1f);
    ImGui::DragFloat("Shininess", &light.shininess, 0.1f);

    ImGui::Spacing();
    ImGui::Text("Light Space Matrix");
    for (int i = 0; i < 4; i++)
    {
        ImGui::Text("%f %f %f %f", light.lightSpaceMatrix[i][0], light.lightSpaceMatrix[i][1], light.lightSpaceMatrix[i][2], light.lightSpaceMatrix[i][3]);
    }
}

void DirectionalLightObject::ImGuiDebug()
{
    ImGui::Text("Directional Light");

    ImGui::DragFloat3("Direction", &light.direction.x, 0.1f);
    ImGui::ColorEdit3("Colour", &light.color.x);
    ImGui::DragFloat("Ambient Strength", &light.ambientStrength, 0.1f);

	ImGui::Spacing();
    ImGui::Text("Light Space Matrix");
    for (int i = 0; i < 4; i++)
    {
        ImGui::Text("%f %f %f %f", light.lightSpaceMatrix[i][0], light.lightSpaceMatrix[i][1], light.lightSpaceMatrix[i][2], light.lightSpaceMatrix[i][3]);
    }
}
