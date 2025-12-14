#pragma once

/**
 * @file ComponentManager.h
 * @brief Component Manager for Shattered Moon ECS
 *
 * Manages all component types and their arrays, providing type-safe
 * access to component data through templates.
 */

#include "Component.h"

#include <memory>
#include <unordered_map>
#include <typeindex>
#include <string>

namespace SM
{
    // ============================================================================
    // ComponentTypeCounter
    // ============================================================================

    /**
     * @brief Compile-time counter for generating unique component type IDs
     *
     * Each component type gets a unique ID at runtime, starting from 0.
     */
    class ComponentTypeCounter
    {
    public:
        /**
         * @brief Get the unique ID for a component type
         * @tparam T The component type
         * @return The unique component type ID
         */
        template<typename T>
        static ComponentType GetID()
        {
            static ComponentType id = s_NextID++;
            return id;
        }

    private:
        static inline ComponentType s_NextID = 0;
    };

    // ============================================================================
    // ComponentManager Class
    // ============================================================================

    /**
     * @brief Manages registration and access to all component types
     *
     * Responsibilities:
     * - Registering new component types
     * - Adding/removing components to/from entities
     * - Providing type-safe access to component data
     * - Notifying component arrays when entities are destroyed
     */
    class ComponentManager
    {
    public:
        ComponentManager() = default;
        ~ComponentManager() = default;

        // Prevent copying
        ComponentManager(const ComponentManager&) = delete;
        ComponentManager& operator=(const ComponentManager&) = delete;

        // Allow moving
        ComponentManager(ComponentManager&&) = default;
        ComponentManager& operator=(ComponentManager&&) = default;

        /**
         * @brief Register a new component type
         * @tparam T The component type to register
         *
         * Must be called before using AddComponent, GetComponent, etc.
         * for a given component type.
         */
        template<typename T>
        void RegisterComponent();

        /**
         * @brief Get the unique type ID for a component
         * @tparam T The component type
         * @return The component type ID
         */
        template<typename T>
        ComponentType GetComponentType() const;

        /**
         * @brief Add a component to an entity
         * @tparam T The component type
         * @param entity The entity to add the component to
         * @param component The component data
         */
        template<typename T>
        void AddComponent(EntityID entity, T component);

        /**
         * @brief Remove a component from an entity
         * @tparam T The component type
         * @param entity The entity to remove the component from
         */
        template<typename T>
        void RemoveComponent(EntityID entity);

        /**
         * @brief Get a reference to an entity's component
         * @tparam T The component type
         * @param entity The entity to query
         * @return Reference to the component data
         */
        template<typename T>
        T& GetComponent(EntityID entity);

        /**
         * @brief Get a const reference to an entity's component
         * @tparam T The component type
         * @param entity The entity to query
         * @return Const reference to the component data
         */
        template<typename T>
        const T& GetComponent(EntityID entity) const;

        /**
         * @brief Try to get a component, returning nullptr if not found
         * @tparam T The component type
         * @param entity The entity to query
         * @return Pointer to component, or nullptr
         */
        template<typename T>
        T* TryGetComponent(EntityID entity);

        /**
         * @brief Try to get a const component, returning nullptr if not found
         * @tparam T The component type
         * @param entity The entity to query
         * @return Const pointer to component, or nullptr
         */
        template<typename T>
        const T* TryGetComponent(EntityID entity) const;

        /**
         * @brief Check if an entity has a specific component
         * @tparam T The component type
         * @param entity The entity to check
         * @return true if the entity has the component
         */
        template<typename T>
        bool HasComponent(EntityID entity) const;

        /**
         * @brief Notify all component arrays that an entity was destroyed
         * @param entity The destroyed entity
         */
        void EntityDestroyed(EntityID entity);

        /**
         * @brief Get the component array for a specific type
         * @tparam T The component type
         * @return Pointer to the component array
         */
        template<typename T>
        ComponentArray<T>* GetComponentArray();

        /**
         * @brief Get const access to the component array for a specific type
         * @tparam T The component type
         * @return Const pointer to the component array
         */
        template<typename T>
        const ComponentArray<T>* GetComponentArray() const;

        /**
         * @brief Check if a component type has been registered
         * @tparam T The component type
         * @return true if the component type is registered
         */
        template<typename T>
        bool IsComponentRegistered() const;

        /**
         * @brief Reset the component manager, clearing all data
         */
        void Reset();

    private:
        /** Map from type index to component arrays */
        std::unordered_map<std::type_index, std::shared_ptr<IComponentArray>> m_ComponentArrays;

        /** Map from type index to component type IDs */
        std::unordered_map<std::type_index, ComponentType> m_ComponentTypes;
    };

    // ============================================================================
    // ComponentManager Implementation
    // ============================================================================

    template<typename T>
    void ComponentManager::RegisterComponent()
    {
        std::type_index typeIndex = std::type_index(typeid(T));

        assert(m_ComponentArrays.find(typeIndex) == m_ComponentArrays.end() &&
               "Component type already registered");

        // Store the component type ID
        m_ComponentTypes[typeIndex] = ComponentTypeCounter::GetID<T>();

        // Create a new component array
        m_ComponentArrays[typeIndex] = std::make_shared<ComponentArray<T>>();
    }

    template<typename T>
    ComponentType ComponentManager::GetComponentType() const
    {
        std::type_index typeIndex = std::type_index(typeid(T));

        auto it = m_ComponentTypes.find(typeIndex);
        assert(it != m_ComponentTypes.end() && "Component not registered before use");

        return it->second;
    }

    template<typename T>
    void ComponentManager::AddComponent(EntityID entity, T component)
    {
        GetComponentArray<T>()->InsertData(entity, std::move(component));
    }

    template<typename T>
    void ComponentManager::RemoveComponent(EntityID entity)
    {
        GetComponentArray<T>()->RemoveData(entity);
    }

    template<typename T>
    T& ComponentManager::GetComponent(EntityID entity)
    {
        return GetComponentArray<T>()->GetData(entity);
    }

    template<typename T>
    const T& ComponentManager::GetComponent(EntityID entity) const
    {
        return GetComponentArray<T>()->GetData(entity);
    }

    template<typename T>
    T* ComponentManager::TryGetComponent(EntityID entity)
    {
        auto* array = GetComponentArray<T>();
        return array ? array->TryGetData(entity) : nullptr;
    }

    template<typename T>
    const T* ComponentManager::TryGetComponent(EntityID entity) const
    {
        const auto* array = GetComponentArray<T>();
        return array ? array->TryGetData(entity) : nullptr;
    }

    template<typename T>
    bool ComponentManager::HasComponent(EntityID entity) const
    {
        const auto* array = GetComponentArray<T>();
        return array && array->HasData(entity);
    }

    inline void ComponentManager::EntityDestroyed(EntityID entity)
    {
        // Notify each component array that an entity has been destroyed
        for (auto& [typeIndex, componentArray] : m_ComponentArrays)
        {
            componentArray->EntityDestroyed(entity);
        }
    }

    template<typename T>
    ComponentArray<T>* ComponentManager::GetComponentArray()
    {
        std::type_index typeIndex = std::type_index(typeid(T));

        auto it = m_ComponentArrays.find(typeIndex);
        assert(it != m_ComponentArrays.end() && "Component not registered before use");

        return static_cast<ComponentArray<T>*>(it->second.get());
    }

    template<typename T>
    const ComponentArray<T>* ComponentManager::GetComponentArray() const
    {
        std::type_index typeIndex = std::type_index(typeid(T));

        auto it = m_ComponentArrays.find(typeIndex);
        if (it == m_ComponentArrays.end())
        {
            return nullptr;
        }

        return static_cast<const ComponentArray<T>*>(it->second.get());
    }

    template<typename T>
    bool ComponentManager::IsComponentRegistered() const
    {
        std::type_index typeIndex = std::type_index(typeid(T));
        return m_ComponentArrays.find(typeIndex) != m_ComponentArrays.end();
    }

    inline void ComponentManager::Reset()
    {
        m_ComponentArrays.clear();
        m_ComponentTypes.clear();
    }

} // namespace SM
