#pragma once

#include <entt/entt.hpp>
#include <functional>
#include <imgui.h>
#include <string>
#include <unordered_map>
#include <vector>

using namespace entt::literals;

class EntityManager
{
public:
	EntityManager();

	// Component to represent hierarchy
	struct Hierarchy
	{
		entt::entity parent = entt::null;
		std::vector<entt::entity> children;
	};

	// Component for names
	struct Name
	{
		std::string name;
	};

	// Create a new entity
	entt::entity createEntity(const std::string& name, entt::entity parent = entt::null);

	// Clone an existing entity and its children
	entt::entity cloneEntity(entt::entity source, entt::entity newParent = entt::null);

	// Delete an entity and its children
	void deleteEntity(entt::entity entity);

	// Set parent of an entity
	void setParent(entt::entity entity, entt::entity parent);

	// Query entities with specific components
	template<typename... Components>
	auto getEntitiesWith()
	{
		return m_registry.view<Components...>();
	}

	// Add a component to an entity
	template<typename T, typename... Args>
	T& addComponent(entt::entity entity, Args&&... args)
	{
		return m_registry.emplace<T>(entity, std::forward<Args>(args)...);
	}

	// Remove a component from an entity
	template<typename T>
	void removeComponent(entt::entity entity)
	{
		m_registry.remove<T>(entity);
	}

	// Check if an entity has a component
	template<typename T>
	bool hasComponent(entt::entity entity)
	{
		return m_registry.all_of<T>(entity);
	}

	// Get a component from an entity
	template<typename T>
	T& getComponent(entt::entity entity)
	{
		return m_registry.get<T>(entity);
	}

	// Render ImGui hierarchy and entity inspector
	void renderImGui();

	// Register a custom ImGui render function for a component
	template<typename T>
	void registerComponentEditor(std::function<void(T&)> editorFunc)
	{
		auto type = entt::type_hash<T>::value();
		m_componentEditors[type] = [editorFunc](void* component) { editorFunc(*static_cast<T*>(component)); };
	}

private:
	entt::registry m_registry;
	entt::entity m_selectedEntity = entt::null;
	std::unordered_map<entt::id_type, std::function<void(void*)>> m_componentEditors;

	// Render ImGui hierarchy and entity inspector
	void renderImGuiHierarchy();
	void renderImGuiEntityNode(entt::entity entity);
	void renderImGuiInspector();
};