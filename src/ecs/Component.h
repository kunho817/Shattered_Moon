#pragma once

/**
 * @file Component.h
 * @brief Component array template for Shattered Moon ECS
 *
 * ComponentArray<T> provides cache-friendly, contiguous storage for components
 * of a specific type. Uses sparse-dense array pattern for O(1) operations.
 */

#include "Entity.h"

#include <array>
#include <unordered_map>
#include <cassert>
#include <optional>

namespace SM
{
    // ============================================================================
    // IComponentArray Interface
    // ============================================================================

    /**
     * @brief Abstract interface for type-erased component array operations
     *
     * Used by ComponentManager to store different component arrays in a single container.
     */
    class IComponentArray
    {
    public:
        virtual ~IComponentArray() = default;

        /**
         * @brief Notify that an entity has been destroyed
         * @param entity The destroyed entity
         *
         * Each component array must remove any data associated with the destroyed entity.
         */
        virtual void EntityDestroyed(EntityID entity) = 0;

        /**
         * @brief Check if the array contains data for an entity
         * @param entity The entity to check
         * @return true if the entity has this component type
         */
        virtual bool HasData(EntityID entity) const = 0;

        /**
         * @brief Get the number of components stored
         * @return The component count
         */
        virtual std::size_t Size() const = 0;
    };

    // ============================================================================
    // ComponentArray<T> Template
    // ============================================================================

    /**
     * @brief Contiguous storage for components of type T
     *
     * Uses a packed array with entity-to-index and index-to-entity mappings
     * for O(1) insertion, removal, and lookup while maintaining cache-friendly
     * data layout.
     *
     * Memory Layout:
     * - Components are stored contiguously with no gaps
     * - When a component is removed, the last component is moved to fill the gap
     * - This maintains data locality but does not preserve insertion order
     *
     * @tparam T The component type to store
     */
    template<typename T>
    class ComponentArray : public IComponentArray
    {
    public:
        ComponentArray() = default;
        ~ComponentArray() override = default;

        // Prevent copying (arrays may be large)
        ComponentArray(const ComponentArray&) = delete;
        ComponentArray& operator=(const ComponentArray&) = delete;

        // Allow moving
        ComponentArray(ComponentArray&&) = default;
        ComponentArray& operator=(ComponentArray&&) = default;

        /**
         * @brief Add a component to an entity
         * @param entity The entity to add the component to
         * @param component The component data
         */
        void InsertData(EntityID entity, T component);

        /**
         * @brief Remove a component from an entity
         * @param entity The entity to remove the component from
         */
        void RemoveData(EntityID entity);

        /**
         * @brief Get a reference to an entity's component
         * @param entity The entity to query
         * @return Reference to the component data
         */
        T& GetData(EntityID entity);

        /**
         * @brief Get a const reference to an entity's component
         * @param entity The entity to query
         * @return Const reference to the component data
         */
        const T& GetData(EntityID entity) const;

        /**
         * @brief Try to get a component, returning nullptr if not found
         * @param entity The entity to query
         * @return Pointer to component data, or nullptr
         */
        T* TryGetData(EntityID entity);

        /**
         * @brief Try to get a const component, returning nullptr if not found
         * @param entity The entity to query
         * @return Const pointer to component data, or nullptr
         */
        const T* TryGetData(EntityID entity) const;

        /**
         * @brief Called when an entity is destroyed
         * @param entity The destroyed entity
         */
        void EntityDestroyed(EntityID entity) override;

        /**
         * @brief Check if an entity has this component
         * @param entity The entity to check
         * @return true if the entity has this component
         */
        bool HasData(EntityID entity) const override;

        /**
         * @brief Get the number of components stored
         * @return The component count
         */
        std::size_t Size() const override { return m_Size; }

        /**
         * @brief Get raw access to the component array
         * @return Pointer to the first component
         *
         * Useful for iterating over all components for cache-friendly processing.
         */
        T* Data() { return m_ComponentArray.data(); }
        const T* Data() const { return m_ComponentArray.data(); }

        /**
         * @brief Get the entity at a given index in the packed array
         * @param index The index to query
         * @return The entity ID at that index
         */
        EntityID GetEntityAtIndex(std::size_t index) const
        {
            assert(index < m_Size && "Index out of range");
            return m_IndexToEntity[index];
        }

    private:
        /** Packed array of components (contiguous memory) */
        std::array<T, MAX_ENTITIES> m_ComponentArray;

        /** Map from entity ID to array index (sparse array) */
        std::unordered_map<EntityID, std::size_t> m_EntityToIndex;

        /** Map from array index to entity ID (for swapping on removal) */
        std::array<EntityID, MAX_ENTITIES> m_IndexToEntity;

        /** Current number of valid entries */
        std::size_t m_Size = 0;
    };

    // ============================================================================
    // ComponentArray Implementation
    // ============================================================================

    template<typename T>
    void ComponentArray<T>::InsertData(EntityID entity, T component)
    {
        assert(m_EntityToIndex.find(entity) == m_EntityToIndex.end() &&
               "Component added to same entity more than once");
        assert(m_Size < MAX_ENTITIES && "Component array full");

        // Put new entry at end of array
        std::size_t newIndex = m_Size;
        m_EntityToIndex[entity] = newIndex;
        m_IndexToEntity[newIndex] = entity;
        m_ComponentArray[newIndex] = std::move(component);
        ++m_Size;
    }

    template<typename T>
    void ComponentArray<T>::RemoveData(EntityID entity)
    {
        auto it = m_EntityToIndex.find(entity);
        if (it == m_EntityToIndex.end())
        {
            return; // Entity doesn't have this component
        }

        // Copy element from end into deleted element's place to maintain density
        std::size_t indexOfRemoved = it->second;
        std::size_t indexOfLast = m_Size - 1;

        if (indexOfRemoved != indexOfLast)
        {
            // Move last element to removed position
            m_ComponentArray[indexOfRemoved] = std::move(m_ComponentArray[indexOfLast]);

            // Update the mapping for the moved element
            EntityID lastEntity = m_IndexToEntity[indexOfLast];
            m_EntityToIndex[lastEntity] = indexOfRemoved;
            m_IndexToEntity[indexOfRemoved] = lastEntity;
        }

        // Remove the entity from the map and shrink
        m_EntityToIndex.erase(entity);
        --m_Size;
    }

    template<typename T>
    T& ComponentArray<T>::GetData(EntityID entity)
    {
        auto it = m_EntityToIndex.find(entity);
        assert(it != m_EntityToIndex.end() && "Retrieving non-existent component");
        return m_ComponentArray[it->second];
    }

    template<typename T>
    const T& ComponentArray<T>::GetData(EntityID entity) const
    {
        auto it = m_EntityToIndex.find(entity);
        assert(it != m_EntityToIndex.end() && "Retrieving non-existent component");
        return m_ComponentArray[it->second];
    }

    template<typename T>
    T* ComponentArray<T>::TryGetData(EntityID entity)
    {
        auto it = m_EntityToIndex.find(entity);
        if (it == m_EntityToIndex.end())
        {
            return nullptr;
        }
        return &m_ComponentArray[it->second];
    }

    template<typename T>
    const T* ComponentArray<T>::TryGetData(EntityID entity) const
    {
        auto it = m_EntityToIndex.find(entity);
        if (it == m_EntityToIndex.end())
        {
            return nullptr;
        }
        return &m_ComponentArray[it->second];
    }

    template<typename T>
    void ComponentArray<T>::EntityDestroyed(EntityID entity)
    {
        // Simply remove the component data if it exists
        RemoveData(entity);
    }

    template<typename T>
    bool ComponentArray<T>::HasData(EntityID entity) const
    {
        return m_EntityToIndex.find(entity) != m_EntityToIndex.end();
    }

} // namespace SM
