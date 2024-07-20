#pragma once
#include <memory>
#include <ostream>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include "Component.h"

#include "EntityManager.h"
#include <spdlog/spdlog.h>

class Entity
{
public:
	Entity(const std::string& name = "");

	unsigned int GetId() const;

	const std::string& GetName() const;

	void SetName(const std::string& newName);

	bool IsActive() const;

	void SetActive(bool isActive);

    template<typename T, typename... Args>
	T& AddComponent(Args&&... args)
	{
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");
		auto component = std::make_shared<T>(std::forward<Args>(args)...);
		m_components[std::type_index(typeid(T))] = std::static_pointer_cast<Component>(component);
		if (m_entityManager)
		{
			m_entityManager->OnEntityComponentChanged(*this);
		}
		return *component;
	}

	template<typename T>
	void RemoveComponent()
	{
		m_components.erase(std::type_index(typeid(T)));
		if (m_entityManager)
		{
			m_entityManager->OnEntityComponentChanged(*this);
		}
	}

	template<typename T>
	bool HasComponent() const
	{
		return m_components.find(std::type_index(typeid(T))) != m_components.end();
	}

	template<typename T>
	T& GetComponent() const
	{
		auto it = m_components.find(std::type_index(typeid(T)));
		if (it != m_components.end())
		{
			return *std::static_pointer_cast<T>(it->second);
		}
		spdlog::error("Component of type '{}' not found for entity '{}'", typeid(T).name(), m_name);
	}

	template<typename T>
	T* GetComponentPtr() const
	{
		auto it = m_components.find(std::type_index(typeid(T)));
		if (it != m_components.end())
		{
			return std::static_pointer_cast<T>(it->second).get();
		}
		return nullptr;
	}

	template<typename... Ts>
	bool HasComponents() const
	{
		return (HasComponent<Ts>() && ...);
	}

	template<typename... Ts>
	std::tuple<Ts&...> GetComponents() const
	{
		return std::tuple<Ts&...>(GetComponent<Ts>()...);
	}

	void RemoveAllComponents();

	size_t GetComponentCount() const;

	std::vector<std::type_index> GetComponentTypes() const;

	void SetEntityManager(EntityManager* manager);

	void AddTag(const std::string& tag);
	void RemoveTag(const std::string& tag);
	bool HasTag(const std::string& tag) const;

	const std::unordered_map<std::type_index, std::shared_ptr<Component>>& GetComponents() const;

	// Allow for debug print in spdlog
	friend std::ostream& operator<<(std::ostream& os, const Entity& entity)
	{
		os << "Entity: " << entity.m_name << " (ID: " << entity.m_id << ")";

		if (entity.m_components.size() > 0)
		{
			os << "\nComponents:";
			for (const auto& pair: entity.m_components)
			{
				os << "\n- " << pair.first.name();
			}
		}

		return os;
	}

	// Imgui debug
	void ImGuiDebug();

private:
	std::unordered_map<std::type_index, std::shared_ptr<Component>> m_components;
	unsigned int m_id;
	std::string m_name;
	bool m_active;
	EntityManager* m_entityManager = nullptr;
	std::unordered_set<std::string> m_tags;
};
