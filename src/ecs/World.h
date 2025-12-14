#pragma once

/**
 * @file World.h
 * @brief ECS World/Registry - Central coordinator for the ECS architecture
 *
 * The World class integrates EntityManager, ComponentManager, and SystemManager
 * providing a unified interface for all ECS operations.
 */

#include "Entity.h"
#include "Component.h"
#include "ComponentManager.h"
#include "System.h"
#include "SystemManager.h"

#include <memory>
#include <functional>

namespace SM
{
    // ============================================================================
    // World Class
    // ============================================================================

    /**
     * @brief Central ECS coordinator
     *
     * The World class is the main entry point for all ECS operations.
     * It manages entities, components, and systems, ensuring they work together
     * correctly.
     *
     * Usage example:
     * @code
     * World world;
     *
     * // Register components
     * world.RegisterComponent<TransformComponent>();
     * world.RegisterComponent<MeshComponent>();
     *
     * // Create entity with components
     * EntityID entity = world.CreateEntity();
     * world.AddComponent(entity, TransformComponent{});
     * world.AddComponent(entity, MeshComponent{ PrimitiveMesh::Cube });
     *
     * // Register and run systems
     * world.RegisterSystem<RenderSystem>();
     * world.Update(deltaTime);
     * @endcode
     */
    class World
    {
    public:
        World();
        ~World();

        // Prevent copying
        World(const World&) = delete;
        World& operator=(const World&) = delete;

        // Allow moving
        World(World&&) = default;
        World& operator=(World&&) = default;

        // ====================================================================
        // Entity Operations
        // ====================================================================

        /**
         * @brief Create a new entity
         * @return The ID of the new entity, or INVALID_ENTITY if failed
         */
        EntityID CreateEntity();

        /**
         * @brief Create a new entity with a name
         * @param name The name for the entity (via TagComponent)
         * @return The ID of the new entity
         */
        EntityID CreateEntity(const std::string& name);

        /**
         * @brief Destroy an entity and all its components
         * @param entity The entity to destroy
         */
        void DestroyEntity(EntityID entity);

        /**
         * @brief Check if an entity is currently alive
         * @param entity The entity to check
         * @return true if the entity exists
         */
        bool IsAlive(EntityID entity) const;

        /**
         * @brief Get the number of living entities
         * @return The entity count
         */
        std::uint32_t GetEntityCount() const;

        // ====================================================================
        // Component Operations
        // ====================================================================

        /**
         * @brief Register a component type
         * @tparam T The component type to register
         *
         * Must be called before using AddComponent, GetComponent, etc.
         */
        template<typename T>
        void RegisterComponent();

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
         * @return Reference to the component
         */
        template<typename T>
        T& GetComponent(EntityID entity);

        /**
         * @brief Get a const reference to an entity's component
         * @tparam T The component type
         * @param entity The entity to query
         * @return Const reference to the component
         */
        template<typename T>
        const T& GetComponent(EntityID entity) const;

        /**
         * @brief Try to get a component (returns nullptr if not found)
         * @tparam T The component type
         * @param entity The entity to query
         * @return Pointer to component or nullptr
         */
        template<typename T>
        T* TryGetComponent(EntityID entity);

        /**
         * @brief Try to get a const component (returns nullptr if not found)
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
         * @brief Get the component type ID
         * @tparam T The component type
         * @return The unique component type ID
         */
        template<typename T>
        ComponentType GetComponentType() const;

        // ====================================================================
        // System Operations
        // ====================================================================

        /**
         * @brief Register a new system
         * @tparam T The system type (must derive from ISystem)
         * @tparam Args Constructor argument types
         * @param args Constructor arguments
         * @return Pointer to the registered system
         */
        template<typename T, typename... Args>
        T* RegisterSystem(Args&&... args);

        /**
         * @brief Get a registered system
         * @tparam T The system type
         * @return Pointer to the system, or nullptr
         */
        template<typename T>
        T* GetSystem();

        /**
         * @brief Set the required signature for a system
         * @tparam T The system type
         * @param signature The required component signature
         */
        template<typename T>
        void SetSystemSignature(Signature signature);

        // ====================================================================
        // Lifecycle Operations
        // ====================================================================

        /**
         * @brief Initialize all registered systems
         *
         * Call this after registering all systems, before the first Update.
         */
        void InitializeSystems();

        /**
         * @brief Update all systems
         * @param deltaTime Time since last frame
         */
        void Update(float deltaTime);

        /**
         * @brief Shutdown all systems
         *
         * Call this before destroying the world or shutting down the engine.
         */
        void ShutdownSystems();

        /**
         * @brief Reset the world, destroying all entities and clearing systems
         */
        void Reset();

        // ====================================================================
        // Query Operations
        // ====================================================================

        /**
         * @brief Iterate over all entities with specific components
         * @tparam Components The component types required
         * @param callback Function called for each matching entity
         */
        template<typename... Components>
        void ForEach(std::function<void(EntityID, Components&...)> callback);

        /**
         * @brief Get a view of entities with specific components
         * @tparam Components The component types required
         * @return Vector of matching entity IDs
         */
        template<typename... Components>
        std::vector<EntityID> GetEntitiesWith() const;

        /**
         * @brief Find the first entity with a matching name
         * @param name The name to search for
         * @return The entity ID, or INVALID_ENTITY if not found
         */
        EntityID FindEntityByName(const std::string& name) const;

        /**
         * @brief Find all entities with a specific tag
         * @param tag The tag to search for
         * @return Vector of matching entity IDs
         */
        std::vector<EntityID> FindEntitiesByTag(const std::string& tag) const;

        // ====================================================================
        // Manager Access
        // ====================================================================

        /**
         * @brief Get the entity manager
         * @return Reference to the entity manager
         */
        EntityManager& GetEntityManager() { return m_EntityManager; }
        const EntityManager& GetEntityManager() const { return m_EntityManager; }

        /**
         * @brief Get the component manager
         * @return Reference to the component manager
         */
        ComponentManager& GetComponentManager() { return m_ComponentManager; }
        const ComponentManager& GetComponentManager() const { return m_ComponentManager; }

        /**
         * @brief Get the system manager
         * @return Reference to the system manager
         */
        SystemManager& GetSystemManager() { return m_SystemManager; }
        const SystemManager& GetSystemManager() const { return m_SystemManager; }

    private:
        /**
         * @brief Update entity signature after component changes
         * @param entity The entity whose signature changed
         */
        void UpdateEntitySignature(EntityID entity);

        EntityManager m_EntityManager;
        ComponentManager m_ComponentManager;
        SystemManager m_SystemManager;
        bool m_SystemsInitialized = false;
    };

    // ============================================================================
    // World Implementation
    // ============================================================================

    inline World::World() = default;

    inline World::~World()
    {
        if (m_SystemsInitialized)
        {
            ShutdownSystems();
        }
    }

    inline EntityID World::CreateEntity()
    {
        return m_EntityManager.CreateEntity();
    }

    inline void World::DestroyEntity(EntityID entity)
    {
        if (!m_EntityManager.IsAlive(entity))
        {
            return;
        }

        // Notify systems first
        m_SystemManager.EntityDestroyed(entity);

        // Then notify component manager to clean up component data
        m_ComponentManager.EntityDestroyed(entity);

        // Finally destroy the entity
        m_EntityManager.DestroyEntity(entity);
    }

    inline bool World::IsAlive(EntityID entity) const
    {
        return m_EntityManager.IsAlive(entity);
    }

    inline std::uint32_t World::GetEntityCount() const
    {
        return m_EntityManager.GetLivingEntityCount();
    }

    inline void World::InitializeSystems()
    {
        if (!m_SystemsInitialized)
        {
            m_SystemManager.InitializeSystems(*this);
            m_SystemsInitialized = true;
        }
    }

    inline void World::Update(float deltaTime)
    {
        m_SystemManager.UpdateSystems(*this, deltaTime);
    }

    inline void World::ShutdownSystems()
    {
        if (m_SystemsInitialized)
        {
            m_SystemManager.ShutdownSystems(*this);
            m_SystemsInitialized = false;
        }
    }

    inline void World::Reset()
    {
        ShutdownSystems();
        m_SystemManager.Reset();
        m_ComponentManager.Reset();
        m_EntityManager.Reset();
    }

    inline void World::UpdateEntitySignature(EntityID entity)
    {
        Signature signature = m_EntityManager.GetSignature(entity);
        m_SystemManager.EntitySignatureChanged(entity, signature);
    }

    // Template implementations

    template<typename T>
    void World::RegisterComponent()
    {
        m_ComponentManager.RegisterComponent<T>();
    }

    template<typename T>
    void World::AddComponent(EntityID entity, T component)
    {
        m_ComponentManager.AddComponent<T>(entity, std::move(component));

        // Update signature
        Signature signature = m_EntityManager.GetSignature(entity);
        signature.set(m_ComponentManager.GetComponentType<T>(), true);
        m_EntityManager.SetSignature(entity, signature);

        // Notify systems
        m_SystemManager.EntitySignatureChanged(entity, signature);
    }

    template<typename T>
    void World::RemoveComponent(EntityID entity)
    {
        m_ComponentManager.RemoveComponent<T>(entity);

        // Update signature
        Signature signature = m_EntityManager.GetSignature(entity);
        signature.set(m_ComponentManager.GetComponentType<T>(), false);
        m_EntityManager.SetSignature(entity, signature);

        // Notify systems
        m_SystemManager.EntitySignatureChanged(entity, signature);
    }

    template<typename T>
    T& World::GetComponent(EntityID entity)
    {
        return m_ComponentManager.GetComponent<T>(entity);
    }

    template<typename T>
    const T& World::GetComponent(EntityID entity) const
    {
        return m_ComponentManager.GetComponent<T>(entity);
    }

    template<typename T>
    T* World::TryGetComponent(EntityID entity)
    {
        return m_ComponentManager.TryGetComponent<T>(entity);
    }

    template<typename T>
    const T* World::TryGetComponent(EntityID entity) const
    {
        return m_ComponentManager.TryGetComponent<T>(entity);
    }

    template<typename T>
    bool World::HasComponent(EntityID entity) const
    {
        return m_ComponentManager.HasComponent<T>(entity);
    }

    template<typename T>
    ComponentType World::GetComponentType() const
    {
        return m_ComponentManager.GetComponentType<T>();
    }

    template<typename T, typename... Args>
    T* World::RegisterSystem(Args&&... args)
    {
        return m_SystemManager.RegisterSystem<T>(std::forward<Args>(args)...);
    }

    template<typename T>
    T* World::GetSystem()
    {
        return m_SystemManager.GetSystem<T>();
    }

    template<typename T>
    void World::SetSystemSignature(Signature signature)
    {
        m_SystemManager.SetSignature<T>(signature);
    }

    template<typename... Components>
    void World::ForEach(std::function<void(EntityID, Components&...)> callback)
    {
        // Get all entities and check if they have all required components
        for (EntityID entity = 0; entity < MAX_ENTITIES; ++entity)
        {
            if (!m_EntityManager.IsAlive(entity))
            {
                continue;
            }

            // Check if entity has all required components
            bool hasAll = (m_ComponentManager.HasComponent<Components>(entity) && ...);

            if (hasAll)
            {
                callback(entity, m_ComponentManager.GetComponent<Components>(entity)...);
            }
        }
    }

    template<typename... Components>
    std::vector<EntityID> World::GetEntitiesWith() const
    {
        std::vector<EntityID> result;

        for (EntityID entity = 0; entity < MAX_ENTITIES; ++entity)
        {
            if (!m_EntityManager.IsAlive(entity))
            {
                continue;
            }

            // Check if entity has all required components
            bool hasAll = (m_ComponentManager.HasComponent<Components>(entity) && ...);

            if (hasAll)
            {
                result.push_back(entity);
            }
        }

        return result;
    }

} // namespace SM
