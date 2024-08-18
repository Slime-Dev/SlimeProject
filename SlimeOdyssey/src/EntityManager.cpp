#include "EntityManager.h"

#include <entt/entt.hpp>
#include <functional>
#include <imgui.h>
#include <string>
#include <unordered_map>
#include <vector>

EntityManager::EntityManager()
      : m_registry()
{
	// Register built-in components with entt::meta system
	entt::meta<Hierarchy>().type("Hierarchy"_hs).data<&Hierarchy::parent>("parent"_hs).data<&Hierarchy::children>("children"_hs);

	entt::meta<Name>().type("Name"_hs).data<&Name::name>("name"_hs);
}

// Create a new entity
entt::entity EntityManager::createEntity(const std::string& name, entt::entity parent = entt::null)
{
	auto entity = m_registry.create();
	m_registry.emplace<Name>(entity, name);

	if (parent != entt::null)
	{
		setParent(entity, parent);
	}
	else
	{
		m_registry.emplace<Hierarchy>(entity);
	}

	return entity;
}

// Clone an existing entity and its children
entt::entity EntityManager::cloneEntity(entt::entity source, entt::entity newParent = entt::null)
{
	auto clone = m_registry.create();
	// m_registry.copy(clone, source); // TODO - implement this function

	// Update name to avoid duplicates
	auto& name = m_registry.get<Name>(clone);
	name.name += " (Clone)";

	// Handle hierarchy
	if (newParent != entt::null)
	{
		setParent(clone, newParent);
	}
	else if (m_registry.all_of<Hierarchy>(source))
	{
		auto& sourceHierarchy = m_registry.get<Hierarchy>(source);
		setParent(clone, sourceHierarchy.parent);
	}

	// Clone children recursively
	if (m_registry.all_of<Hierarchy>(source))
	{
		auto& sourceHierarchy = m_registry.get<Hierarchy>(source);
		for (auto child: sourceHierarchy.children)
		{
			cloneEntity(child, clone);
		}
	}

	return clone;
}

// Delete an entity and its children
void EntityManager::deleteEntity(entt::entity entity)
{
	if (m_registry.valid(entity))
	{
		// Remove from parent's children list
		if (auto* hierarchy = m_registry.try_get<Hierarchy>(entity))
		{
			if (hierarchy->parent != entt::null)
			{
				auto& parentHierarchy = m_registry.get<Hierarchy>(hierarchy->parent);
				auto it = std::find(parentHierarchy.children.begin(), parentHierarchy.children.end(), entity);
				if (it != parentHierarchy.children.end())
				{
					parentHierarchy.children.erase(it);
				}
			}
		}

		// Delete children recursively
		if (auto* hierarchy = m_registry.try_get<Hierarchy>(entity))
		{
			for (auto child: hierarchy->children)
			{
				deleteEntity(child);
			}
		}

		// Finally, delete the entity itself
		m_registry.destroy(entity);
	}
}

// Set parent of an entity
void EntityManager::setParent(entt::entity entity, entt::entity parent)
{
	auto& hierarchy = m_registry.get_or_emplace<Hierarchy>(entity);

	// Remove from old parent
	if (hierarchy.parent != entt::null)
	{
		auto& oldParentHierarchy = m_registry.get<Hierarchy>(hierarchy.parent);
		auto it = std::find(oldParentHierarchy.children.begin(), oldParentHierarchy.children.end(), entity);
		if (it != oldParentHierarchy.children.end())
		{
			oldParentHierarchy.children.erase(it);
		}
	}

	// Set new parent
	hierarchy.parent = parent;
	if (parent != entt::null)
	{
		auto& parentHierarchy = m_registry.get_or_emplace<Hierarchy>(parent);
		parentHierarchy.children.push_back(entity);
	}
}

// Render ImGui hierarchy and entity inspector
void EntityManager::renderImGui()
{
	renderImGuiHierarchy();
	renderImGuiInspector();
}

void EntityManager::renderImGuiHierarchy()
{
	if (ImGui::Begin("Entity Hierarchy"))
	{
		if (ImGui::Button("Create Root Entity"))
		{
			createEntity("New Root Entity");
		}

		auto view = m_registry.view<Hierarchy, Name>();
		for (auto entity: view)
		{
			auto& hierarchy = view.get<Hierarchy>(entity);
			if (hierarchy.parent == entt::null)
			{
				renderImGuiEntityNode(entity);
			}
		}
	}
	ImGui::End();
}

void EntityManager::renderImGuiEntityNode(entt::entity entity)
{
	auto& name = m_registry.get<Name>(entity);
	auto& hierarchy = m_registry.get<Hierarchy>(entity);

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
	if (hierarchy.children.empty())
	{
		flags |= ImGuiTreeNodeFlags_Leaf;
	}
	if (entity == m_selectedEntity)
	{
		flags |= ImGuiTreeNodeFlags_Selected;
	}

	bool opened = ImGui::TreeNodeEx((void*) (intptr_t) entt::to_integral(entity), flags, "%s", name.name.c_str());

	if (ImGui::IsItemClicked())
	{
		m_selectedEntity = entity;
	}

	if (ImGui::BeginPopupContextItem())
	{
		if (ImGui::MenuItem("Add Child"))
		{
			createEntity("New Entity", entity);
		}
		if (ImGui::MenuItem("Clone"))
		{
			cloneEntity(entity);
		}
		if (ImGui::MenuItem("Delete"))
		{
			if (entity == m_selectedEntity)
			{
				m_selectedEntity = entt::null;
			}
			deleteEntity(entity);
			ImGui::EndPopup();
			if (opened)
			{
				ImGui::TreePop();
			}
			return;
		}
		ImGui::EndPopup();
	}

	if (opened)
	{
		for (auto child: hierarchy.children)
		{
			renderImGuiEntityNode(child);
		}
		ImGui::TreePop();
	}
}

void EntityManager::renderImGuiInspector()
{
	if (ImGui::Begin("Entity Inspector"))
	{
		if (m_selectedEntity != entt::null && m_registry.valid(m_selectedEntity))
		{
			auto& name = m_registry.get<Name>(m_selectedEntity);
			char buffer[256];
			strcpy(buffer, name.name.c_str());
			if (ImGui::InputText("Name", buffer, sizeof(buffer)))
			{
				name.name = buffer;
			}

			ImGui::Separator();

			// Use entt::meta to iterate over components
			for (auto [id, storage]: m_registry.storage())
			{
				if (storage.contains(m_selectedEntity))
				{
					auto type = entt::resolve(id);
					if (type)
					{
						if (ImGui::CollapsingHeader(type.info().name().data()))
						{
							// Implement component inspector
							for (auto data: type.data())
							{
								const auto& name = data.prop("name"_hs).value().cast<const char*>();
								auto value = data.get({}, storage.get(m_selectedEntity));

								if (value.type() == entt::resolve<int>())
								{
									int val = value.cast<int>();
									if (ImGui::InputInt(name, &val))
									{
										data.set({}, storage.get(m_selectedEntity), val);
									}
								}
								else if (value.type() == entt::resolve<float>())
								{
									float val = value.cast<float>();
									if (ImGui::InputFloat(name, &val))
									{
										data.set({}, storage.get(m_selectedEntity), val);
									}
								}
								else if (value.type() == entt::resolve<std::string>())
								{
									std::string val = value.cast<std::string>();
									char buf[256];
									strcpy(buf, val.c_str());
									if (ImGui::InputText(name, buf, sizeof(buf)))
									{
										data.set({}, storage.get(m_selectedEntity), std::string(buf));
									}
								}
								// Add more type checks as needed
							}
						}
					}
				}
			}

			if (ImGui::Button("Add Component"))
			{
				ImGui::OpenPopup("AddComponentPopup");
			}

			if (ImGui::BeginPopup("AddComponentPopup"))
			{
				// Iterate through all registered component types
				entt::meta_range types = entt::resolve();
				for (auto type: types)
				{
					if (ImGui::MenuItem(type.info().name().data()))
					{
						// Check if the component doesn't already exist on the entity
						if (!m_registry.storage(type.id()).contains(m_selectedEntity))
						{
							// Create and add the component to the entity
							auto instance = type.construct();
							if (instance)
							{
								m_registry.emplace(m_selectedEntity, type.id(), std::move(instance));
							}
						}
						ImGui::CloseCurrentPopup();
					}
				}
				ImGui::EndPopup();
			}
		}
		else
		{
			ImGui::Text("No entity selected");
		}
	}
	ImGui::End();
}
