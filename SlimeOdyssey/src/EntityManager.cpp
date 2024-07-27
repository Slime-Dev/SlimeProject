#include "EntityManager.h"

#include <functional>
#include <memory>
#include <vector>

#include "Entity.h"
#include <cstdint>
#include <iterator>
#include <string>
#include <typeindex>
#include <type_traits>
#include <algorithm>

#include "imgui.h"
#include <glm/common.hpp>

// Update entity's component mask
void EntityManager::UpdateEntityMask(const Entity& entity)
{
	size_t entityIndex = GetEntityIndex(entity);
	if (entityIndex >= m_entityMasks.size())
	{
		m_entityMasks.resize(entityIndex + 1);
	}
	ComponentMask& mask = m_entityMasks[entityIndex];
	mask.reset();
	for (const auto& [type, component]: entity.GetComponents())
	{
		mask.set(GetComponentTypeIndex(type));
	}
}

// Get entity index
size_t EntityManager::GetEntityIndex(const Entity& entity) const
{
	auto it = std::find_if(m_entities.begin(), m_entities.end(), [&entity](const auto& e) { return e->GetId() == entity.GetId(); });
	return std::distance(m_entities.begin(), it);
}

// Add an entity to the manager
void EntityManager::AddEntity(std::shared_ptr<Entity> entity)
{
	m_entities.push_back(std::move(entity));
	UpdateEntityMask(*m_entities.back());
}

// Moves the entity to the manager and turns it into a shared pointer
std::shared_ptr<Entity> EntityManager::AddEntity(Entity& entity)
{
	m_entities.push_back(std::make_shared<Entity>(std::move(entity)));
	UpdateEntityMask(*m_entities.back());
	return m_entities.back();
}

// Remove an entity from the manager
void EntityManager::RemoveEntity(const Entity& entity)
{
	auto it = std::remove_if(m_entities.begin(), m_entities.end(), [&entity](const auto& e) { return e->GetId() == entity.GetId(); });
	if (it != m_entities.end())
	{
		m_entities.erase(it, m_entities.end());
		size_t entityIndex = GetEntityIndex(entity);
		if (entityIndex < m_entityMasks.size())
		{
			m_entityMasks.erase(m_entityMasks.begin() + entityIndex);
		}
	}
}

// Get all entities
const std::vector<std::shared_ptr<Entity>>& EntityManager::GetEntities() const
{
	return m_entities;
}

// Filter entities by custom predicate
std::vector<std::shared_ptr<Entity>> EntityManager::GetEntitiesWhere(const std::function<bool(const Entity&)>& predicate) const
{
	std::vector<std::shared_ptr<Entity>> result;
	std::copy_if(m_entities.begin(), m_entities.end(), std::back_inserter(result), [&predicate](const auto& entity) { return predicate(*entity); });
	return result;
}

// Get entities by tag
std::vector<std::shared_ptr<Entity>> EntityManager::GetEntitiesByTag(const std::string& tag) const
{
	return GetEntitiesWhere([&tag](const Entity& e) { return e.HasTag(tag); });
}

// Get entity by id
std::shared_ptr<Entity> EntityManager::GetEntityById(unsigned int id) const
{
	auto it = std::find_if(m_entities.begin(), m_entities.end(), [&id](const auto& e) { return e->GetId() == id; });
	return it != m_entities.end() ? *it : nullptr;
}

// Get entity by name
std::shared_ptr<Entity> EntityManager::GetEntityByName(const std::string& name) const
{
	auto it = std::find_if(m_entities.begin(), m_entities.end(), [&name](const auto& e) { return e->GetName() == name; });
	return it != m_entities.end() ? *it : nullptr;
}

// Update component masks when an entity's components change
void EntityManager::OnEntityComponentChanged(const Entity& entity)
{
	UpdateEntityMask(entity);
}

// Shows a window of all entities and their components
void EntityManager::ImGuiDebug()
{
    ImGui::SetNextWindowSizeConstraints(ImVec2(300, 300), ImVec2(FLT_MAX, FLT_MAX));
    ImGui::Begin("Entity Manager", nullptr, ImGuiWindowFlags_NoScrollbar);

    // Search bar and controls
    static char searchBuffer[256] = "";
    ImGui::InputText("Search Entities", searchBuffer, IM_ARRAYSIZE(searchBuffer));
    std::string searchStr = searchBuffer;
    ImGui::Text("Total Entities: %zu", m_entities.size());
    ImGui::Separator();

    // Calculate available height for split view
    float availableHeight = ImGui::GetContentRegionAvail().y;

    // Splitter
    static float splitHeight = availableHeight * 0.5f; // Initial split at 50%
    
    ImGui::BeginChild("TopRegion", ImVec2(0, splitHeight), true);
    RenderEntityTree(searchStr);
    ImGui::EndChild();

    // Splitter bar
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));

    ImGui::Button("##splitter", ImVec2(-1, 5));
    if (ImGui::IsItemActive())
    {
        splitHeight += ImGui::GetIO().MouseDelta.y;
        splitHeight = glm::clamp(splitHeight, availableHeight * 0.1f, availableHeight * 0.9f);
    }

    ImGui::PopStyleColor(3);

    ImGui::BeginChild("BottomRegion", ImVec2(0, 0), true);
    RenderComponentDetails();
    ImGui::EndChild();

    ImGui::End();
}

void EntityManager::RenderEntityTree(const std::string& searchStr)
{
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 6)); // Reduce vertical spacing between items

	for (const auto& entity: m_entities)
	{
		if (!searchStr.empty() && entity->GetName().find(searchStr) == std::string::npos)
		{
			continue;
		}

		bool isSelected = (m_selectedEntity == entity.get());

		ImGui::PushID(entity.get());

		// Use Selectable for the full-width background and selection
		if (ImGui::Selectable(entity->GetName().c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap))
		{
			m_selectedEntity = entity.get();
		}

		// Context menu
		if (ImGui::BeginPopupContextItem("EntityContextMenu"))
		{
			if (ImGui::MenuItem("Delete Entity"))
			{
				// TODO: Implement delete functionality
			}
			if (ImGui::MenuItem("Clone Entity"))
			{
				// TODO: Implement clone functionality
			}
			ImGui::EndPopup();
		}

		ImGui::PopID();
	}

	ImGui::PopStyleVar();
}

void EntityManager::RenderComponentDetails()
{
    if (m_selectedEntity)
    {
        // Component Count
        ImGui::Text("Component Count: %zu", m_selectedEntity->GetComponentCount());
        ImGui::Separator();
        for (const auto& [type, component] : m_selectedEntity->GetComponents())
        {
            ImGui::PushID(type.name());
            if (ImGui::CollapsingHeader(type.name(), ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Indent();
                component->ImGuiDebug();
                ImGui::Unindent();
            }
            ImGui::PopID();
        }
    }
    else
    {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Select an entity to view its components");
    }
}