#pragma once

/**
 * @file Entity.h
 * @brief Entity system for Shattered Moon ECS architecture
 *
 * Entities are simple identifiers (IDs) that group components together.
 * The EntityManager handles creation, destruction, and lifecycle of entities.
 */

#include <cstdint>
#include <queue>
#include <array>
#include <bitset>
#include <cassert>

namespace SM
{
    // ============================================================================
    // Type Definitions
    // ============================================================================

    /**
     * @brief Entity identifier type
     *
     * Using uint32_t allows for over 4 billion entities, which is sufficient
     * for most games while keeping memory usage reasonable.
     */
    using EntityID = std::uint32_t;

    /**
     * @brief Component type identifier
     *
     * Each component type gets a unique ID for signature matching.
     */
    using ComponentType = std::uint8_t;

    /**
     * @brief Maximum number of entities that can exist simultaneously
     */
    constexpr EntityID MAX_ENTITIES = 10000;

    /**
     * @brief Maximum number of unique component types
     */
    constexpr ComponentType MAX_COMPONENTS = 64;

    /**
     * @brief Invalid entity identifier constant
     */
    constexpr EntityID INVALID_ENTITY = std::numeric_limits<EntityID>::max();

    /**
     * @brief Component signature bitset
     *
     * Each bit represents whether an entity has a particular component type.
     * Used for system filtering - systems only process entities with matching signatures.
     */
    using Signature = std::bitset<MAX_COMPONENTS>;

    // ============================================================================
    // EntityManager Class
    // ============================================================================

    /**
     * @brief Manages entity lifecycle and component signatures
     *
     * Responsibilities:
     * - Creating and destroying entities
     * - Tracking which entities are alive
     * - Managing entity signatures (which components each entity has)
     * - Recycling destroyed entity IDs
     */
    class EntityManager
    {
    public:
        EntityManager();
        ~EntityManager() = default;

        // Prevent copying
        EntityManager(const EntityManager&) = delete;
        EntityManager& operator=(const EntityManager&) = delete;

        // Allow moving
        EntityManager(EntityManager&&) = default;
        EntityManager& operator=(EntityManager&&) = default;

        /**
         * @brief Create a new entity
         * @return The ID of the newly created entity, or INVALID_ENTITY if failed
         */
        EntityID CreateEntity();

        /**
         * @brief Destroy an entity and mark its ID for reuse
         * @param entity The entity to destroy
         */
        void DestroyEntity(EntityID entity);

        /**
         * @brief Check if an entity is currently alive
         * @param entity The entity to check
         * @return true if the entity exists and is alive
         */
        bool IsAlive(EntityID entity) const;

        /**
         * @brief Set the component signature for an entity
         * @param entity The entity to modify
         * @param signature The new signature
         */
        void SetSignature(EntityID entity, Signature signature);

        /**
         * @brief Get the component signature for an entity
         * @param entity The entity to query
         * @return The entity's signature
         */
        Signature GetSignature(EntityID entity) const;

        /**
         * @brief Get the number of currently active entities
         * @return The count of living entities
         */
        std::uint32_t GetLivingEntityCount() const { return m_LivingEntityCount; }

        /**
         * @brief Reset the EntityManager to initial state
         *
         * Destroys all entities and resets the ID pool.
         */
        void Reset();

    private:
        /** Queue of available entity IDs for reuse */
        std::queue<EntityID> m_AvailableEntities;

        /** Array tracking whether each entity ID is currently in use */
        std::array<bool, MAX_ENTITIES> m_EntityAlive;

        /** Array of component signatures indexed by entity ID */
        std::array<Signature, MAX_ENTITIES> m_Signatures;

        /** Number of currently active entities */
        std::uint32_t m_LivingEntityCount = 0;
    };

    // ============================================================================
    // EntityManager Implementation (Inline)
    // ============================================================================

    inline EntityManager::EntityManager()
    {
        // Initialize the queue with all possible entity IDs
        for (EntityID entity = 0; entity < MAX_ENTITIES; ++entity)
        {
            m_AvailableEntities.push(entity);
            m_EntityAlive[entity] = false;
        }
    }

    inline EntityID EntityManager::CreateEntity()
    {
        if (m_LivingEntityCount >= MAX_ENTITIES)
        {
            // No more entities available
            return INVALID_ENTITY;
        }

        // Get the next available ID from the queue
        EntityID id = m_AvailableEntities.front();
        m_AvailableEntities.pop();

        // Mark as alive and clear signature
        m_EntityAlive[id] = true;
        m_Signatures[id].reset();
        ++m_LivingEntityCount;

        return id;
    }

    inline void EntityManager::DestroyEntity(EntityID entity)
    {
        if (entity >= MAX_ENTITIES || !m_EntityAlive[entity])
        {
            return;
        }

        // Reset signature and mark as dead
        m_Signatures[entity].reset();
        m_EntityAlive[entity] = false;

        // Put the ID back in the available queue for reuse
        m_AvailableEntities.push(entity);
        --m_LivingEntityCount;
    }

    inline bool EntityManager::IsAlive(EntityID entity) const
    {
        if (entity >= MAX_ENTITIES)
        {
            return false;
        }
        return m_EntityAlive[entity];
    }

    inline void EntityManager::SetSignature(EntityID entity, Signature signature)
    {
        assert(entity < MAX_ENTITIES && "Entity out of range");
        assert(m_EntityAlive[entity] && "Cannot set signature of dead entity");
        m_Signatures[entity] = signature;
    }

    inline Signature EntityManager::GetSignature(EntityID entity) const
    {
        assert(entity < MAX_ENTITIES && "Entity out of range");
        return m_Signatures[entity];
    }

    inline void EntityManager::Reset()
    {
        // Clear the queue and refill it
        while (!m_AvailableEntities.empty())
        {
            m_AvailableEntities.pop();
        }

        for (EntityID entity = 0; entity < MAX_ENTITIES; ++entity)
        {
            m_AvailableEntities.push(entity);
            m_EntityAlive[entity] = false;
            m_Signatures[entity].reset();
        }

        m_LivingEntityCount = 0;
    }

} // namespace SM
