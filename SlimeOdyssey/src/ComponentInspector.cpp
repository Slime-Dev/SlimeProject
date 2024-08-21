#include "ComponentInspector.h"

#include <entt/entt.hpp>
#include <functional>
#include <imgui.h>
#include <string>
#include <unordered_map>
#include <vector>

#include <imgui.h>
#include <string>

#include "Model.h"
#include "Material.h"
#include "Camera.h"
#include "Light.h"

std::unordered_map<std::size_t, std::function<void(entt::registry &, entt::entity)>> ComponentInspector::m_inspectors;

void ComponentInspector::RegisterComponentInspectors()
{
	ComponentInspector::RegisterInspector<Model>(
	        [](entt::registry& registry, entt::entity entity)
	        {
		        if (ImGui::CollapsingHeader("Model"))
		        {
			        auto& model = registry.get<Model>(entity);
			        ImGui::Text("Pipeline: %s", model.modelResource->pipelineName.c_str());
			        ImGui::Text("Vertex Count: %d", model.modelResource->vertices.size());
			        ImGui::Text("Index Count: %d", model.modelResource->indices.size());

		        }
	        });

	ComponentInspector::RegisterInspector<PBRMaterialResource>(
	        [](entt::registry& registry, entt::entity entity)
	        {
		        if (ImGui::CollapsingHeader("Material"))
		        {
			        auto& material = registry.get<PBRMaterialResource>(entity);
			        ImGui::ColorEdit4("Color", &material.config.albedo.x);
			        ImGui::InputFloat("Metallic", &material.config.metallic);
			        ImGui::InputFloat("Roughness", &material.config.roughness);
			        ImGui::InputFloat("AO", &material.config.ao);
		        }
	        });

	ComponentInspector::RegisterInspector<Transform>(
	        [](entt::registry& registry, entt::entity entity)
	        {
		        if (ImGui::CollapsingHeader("Transform"))
		        {
			        auto& transform = registry.get<Transform>(entity);
			        ImGui::InputFloat3("Position", &transform.position.x);
			        ImGui::InputFloat3("Rotation", &transform.rotation.x);
			        ImGui::InputFloat3("Scale", &transform.scale.x);

					if (ImGui::Button("Reset"))
			        {
				        transform.position = glm::vec3(0.0f);
				        transform.rotation = glm::vec3(0.0f);
				        transform.scale = glm::vec3(1.0f);
			        }
					
					// Matrix debug
			        glm::mat4 model = transform.GetModelMatrix();
					ImGui::Text("Model Matrix");
					ImGui::Text("x: %f, y: %f, z: %f, w: %f", model[0][0], model[0][1], model[0][2], model[0][3]);
		        }
	        });

	ComponentInspector::RegisterInspector<Camera>(
	        [](entt::registry& registry, entt::entity entity)
	        {
		        if (ImGui::CollapsingHeader("Camera"))
		        {
			        auto& camera = registry.get<Camera>(entity);
			        camera.ImGuiDebug();
		        }
	        });

	ComponentInspector::RegisterInspector<PointLight>(
	        [](entt::registry& registry, entt::entity entity)
	        {
		        if (ImGui::CollapsingHeader("Point Light"))
		        {
			        auto& pointLight = registry.get<PointLight>(entity);
			        pointLight.ImGuiDebug();
		        }
	        });

	ComponentInspector::RegisterInspector<DirectionalLight>(
	        [](entt::registry& registry, entt::entity entity)
	        {
		        if (ImGui::CollapsingHeader("Directional Light"))
		        {
			        auto& directionalLight = registry.get<DirectionalLight>(entity);
			        directionalLight.ImGuiDebug();
		        }
	        });
}

void ComponentInspector::Render(entt::registry& registry, entt::entity entity)
{
		if (registry.try_get<Model>(entity))
			m_inspectors[typeid(Model).hash_code()](registry, entity);

		if (registry.try_get<PBRMaterialResource>(entity))
		    m_inspectors[typeid(PBRMaterialResource).hash_code()](registry, entity);

		if (registry.try_get<Transform>(entity))
		    m_inspectors[typeid(Transform).hash_code()](registry, entity);

		if (registry.try_get<Camera>(entity))
		    m_inspectors[typeid(Camera).hash_code()](registry, entity);

		if (registry.try_get<PointLight>(entity))
			m_inspectors[typeid(PointLight).hash_code()](registry, entity);

		if (registry.try_get<DirectionalLight>(entity))
			m_inspectors[typeid(DirectionalLight).hash_code()](registry, entity);
}