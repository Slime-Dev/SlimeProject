#pragma once

#include <bitset>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

class Entity;

// Maximum number of component types
const size_t MAX_COMPONENTS = 32;

// ComponentMask to efficiently store and compare component combinations
using ComponentMask = std::bitset<MAX_COMPONENTS>;

class EntityManager
{
public:
	// Add an entity to the manager
	void AddEntity(std::shared_ptr<Entity> entity);

	// Moves the entity to the manager and turns it into a shared pointer
	std::shared_ptr<Entity> AddEntity(Entity& entity);

	// Remove an entity from the manager
	void RemoveEntity(const Entity& entity);
	void RemoveEntity(std::shared_ptr<Entity> entity);

	// Get all entities
	const std::vector<std::shared_ptr<Entity>>& GetEntities() const;

	// Filter entities by custom predicate
	std::vector<std::shared_ptr<Entity>> GetEntitiesWhere(const std::function<bool(const Entity&)>& predicate) const;

	// Get entities by tag
	std::vector<std::shared_ptr<Entity>> GetEntitiesByTag(const std::string& tag) const;

	// Get entity by id
	std::shared_ptr<Entity> GetEntityById(unsigned int id) const;

	// Get entity by name
	std::shared_ptr<Entity> GetEntityByName(const std::string& name) const;

	// Get entities with specific components
	template<typename... Ts>
	std::vector<std::shared_ptr<Entity>> GetEntitiesWithComponents()
	{
		ComponentMask mask = GetComponentMask<Ts...>();
		std::vector<std::shared_ptr<Entity>> result;
		for (size_t i = 0; i < m_entities.size(); ++i)
		{
			if ((m_entityMasks[i] & mask) == mask)
			{
				result.push_back(m_entities[i]);
			}
		}
		return result;
	}

	// Get entity count with specific components
	template<typename... Ts>
	size_t GetEntityCountWithComponents() const
	{
		ComponentMask mask = GetComponentMask<Ts...>();
		size_t count = 0;
		for (const auto& entityMask: m_entityMasks)
		{
			if ((entityMask & mask) == mask)
			{
				++count;
			}
		}
		return count;
	}

	// Execute a function for each entity with specific components
	template<typename... Ts, typename Func>
	void ForEachEntityWith(Func func)
	{
		ComponentMask mask = GetComponentMask<Ts...>();
		for (size_t i = 0; i < m_entities.size(); ++i)
		{
			if ((m_entityMasks[i] & mask) == mask)
			{
				func(*m_entities[i]);
			}
		}
	}

	// Update component masks when an entity's components change
	void OnEntityComponentChanged(const Entity& entity);

	// Shows a window of all entities and their components
	void ImGuiDebug();

private:
	// Helper function to get or create component type index
	std::uint64_t GetComponentTypeIndex(const std::type_index& type) const
	{
		auto it = m_componentTypeIndices.find(type);
		if (it == m_componentTypeIndices.end())
		{
			m_componentTypeIndices[type] = m_nextComponentTypeIndex;
			return m_nextComponentTypeIndex++;
		}
		return it->second;
	}

	template<typename... Ts>
	ComponentMask GetComponentMask() const
	{
		ComponentMask mask;
		(mask.set(GetComponentTypeIndex(typeid(Ts))), ...);
		return mask;
	}

	// Update entity's component mask
	void UpdateEntityMask(const Entity& entity);

	// Get entity index
	size_t GetEntityIndex(const Entity& entity) const;

	// Imgui debug
	Entity* m_selectedEntity = nullptr;
	float m_splitRatio = 0.5f;
	void RenderComponentDetails();
	void RenderEntityTree(const std::string& searchStr);

	std::vector<std::shared_ptr<Entity>> m_entities;
	std::vector<ComponentMask> m_entityMasks;

	mutable std::unordered_map<std::type_index, std::uint64_t> m_componentTypeIndices;
	mutable std::uint64_t m_nextComponentTypeIndex = 0;

	;
};
