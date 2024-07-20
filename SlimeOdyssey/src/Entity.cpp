#include "Entity.h"
#include "EntityManager.h"

static unsigned int currentId = 0;

Entity::Entity(const std::string& name)
      : m_name(name), m_active(true)
{
	m_id = currentId++;
}

unsigned int Entity::GetId() const
{
	return m_id;
}

const std::string& Entity::GetName() const
{
	return m_name;
}

void Entity::SetName(const std::string& newName)
{
	m_name = newName;
}

bool Entity::IsActive() const
{
	return m_active;
}

void Entity::SetActive(bool isActive)
{
	m_active = isActive;
}

void Entity::RemoveAllComponents()
{
	m_components.clear();
}

size_t Entity::GetComponentCount() const
{
	return m_components.size();
}

std::vector<std::type_index> Entity::GetComponentTypes() const
{
	std::vector<std::type_index> types;
	for (const auto& pair: m_components)
	{
		types.push_back(pair.first);
	}
	return types;
}

void Entity::ImGuiDebug()
{
	// Loop through all components and call ImGuiDebug on each
	for (const auto& pair: m_components)
	{
		pair.second->ImGuiDebug();
	}
}

void Entity::SetEntityManager(EntityManager* manager)
{
	m_entityManager = manager;
}

// Add tag functionality
void Entity::AddTag(const std::string& tag)
{
	m_tags.insert(tag);
}

void Entity::RemoveTag(const std::string& tag)
{
	m_tags.erase(tag);
}

bool Entity::HasTag(const std::string& tag) const
{
	return m_tags.find(tag) != m_tags.end();
}

const std::unordered_map<std::type_index, std::shared_ptr<Component>>& Entity::GetComponents() const
{
	return m_components;
}
