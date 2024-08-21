#pragma once

#include <entt/entt.hpp>
#include <unordered_map>
#include <functional>
#include <typeindex>

class ComponentInspector
{
public:
	static void RegisterComponentInspectors();

	template <typename Component>
	static void RegisterInspector(std::function<void(entt::registry &, entt::entity)> inspector)
	{
		m_inspectors[typeid(Component).hash_code()] = inspector;
	}

	static void Render(entt::registry &registry, entt::entity entity);
private:
	static std::unordered_map<std::size_t, std::function<void(entt::registry &, entt::entity)>> m_inspectors;
};
