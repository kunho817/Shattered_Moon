#pragma once

/**
 * @file SystemManager.h
 * @brief System Manager for Shattered Moon ECS
 *
 * Manages registration, updating, and lifecycle of all ECS systems.
 */

#include "System.h"
#include "Entity.h"

#include <memory>
#include <unordered_map>
#include <vector>
#include <typeindex>
#include <algorithm>
#include <cassert>

namespace SM
{
    // Forward declaration
    class World;

    // ============================================================================
    // SystemManager Class
    // ============================================================================

    /**
     * @brief Manages all ECS systems
     *
     * Responsibilities:
     * - Registering and storing systems
     * - Updating systems each frame in priority order
     * - Notifying systems when entity signatures change
     * - Managing system lifecycle (init, update, shutdown)
     */
    class SystemManager
    {
    public:
        SystemManager() = default;
        ~SystemManager() = default;

        // Prevent copying
        SystemManager(const SystemManager&) = delete;
        SystemManager& operator=(const SystemManager&) = delete;

        // Allow moving
        SystemManager(SystemManager&&) = default;
        SystemManager& operator=(SystemManager&&) = default;

        /**
         * @brief Register a new system
         * @tparam T The system type (must derive from ISystem)
         * @tparam Args Constructor argument types
         * @param args Constructor arguments for the system
         * @return Pointer to the created system
         */
        template<typename T, typename... Args>
        T* RegisterSystem(Args&&... args);

        /**
         * @brief Get a registered system by type
         * @tparam T The system type
         * @return Pointer to the system, or nullptr if not registered
         */
        template<typename T>
        T* GetSystem();

        /**
         * @brief Get a registered system by type (const version)
         * @tparam T The system type
         * @return Const pointer to the system, or nullptr if not registered
         */
        template<typename T>
        const T* GetSystem() const;

        /**
         * @brief Check if a system is registered
         * @tparam T The system type
         * @return true if the system is registered
         */
        template<typename T>
        bool HasSystem() const;

        /**
         * @brief Remove a system
         * @tparam T The system type to remove
         */
        template<typename T>
        void RemoveSystem();

        /**
         * @brief Set the component signature for a system
         * @tparam T The system type
         * @param signature The required component signature
         */
        template<typename T>
        void SetSignature(Signature signature);

        /**
         * @brief Notify systems when an entity's signature changes
         * @param entity The entity whose signature changed
         * @param entitySignature The entity's new signature
         *
         * This determines whether each system should process the entity.
         */
        void EntitySignatureChanged(EntityID entity, Signature entitySignature);

        /**
         * @brief Notify systems when an entity is destroyed
         * @param entity The destroyed entity
         */
        void EntityDestroyed(EntityID entity);

        /**
         * @brief Initialize all systems
         * @param world Reference to the ECS world
         */
        void InitializeSystems(World& world);

        /**
         * @brief Update all enabled systems in priority order
         * @param world Reference to the ECS world
         * @param deltaTime Time since last frame
         */
        void UpdateSystems(World& world, float deltaTime);

        /**
         * @brief Shutdown all systems
         * @param world Reference to the ECS world
         */
        void ShutdownSystems(World& world);

        /**
         * @brief Sort systems by priority
         *
         * Call this after changing system priorities to ensure correct
         * execution order.
         */
        void SortSystemsByPriority();

        /**
         * @brief Get all registered systems
         * @return Vector of system pointers in priority order
         */
        const std::vector<ISystem*>& GetSystems() const { return m_SystemOrder; }

        /**
         * @brief Reset the system manager, removing all systems
         */
        void Reset();

    private:
        /** Map from type index to system instances */
        std::unordered_map<std::type_index, std::shared_ptr<ISystem>> m_Systems;

        /** Map from type index to required signatures */
        std::unordered_map<std::type_index, Signature> m_Signatures;

        /** Systems sorted by priority for update order */
        std::vector<ISystem*> m_SystemOrder;

        /** Flag indicating if systems are initialized */
        bool m_Initialized = false;
    };

    // ============================================================================
    // SystemManager Implementation
    // ============================================================================

    template<typename T, typename... Args>
    T* SystemManager::RegisterSystem(Args&&... args)
    {
        static_assert(std::is_base_of_v<ISystem, T>, "T must derive from ISystem");

        std::type_index typeIndex = std::type_index(typeid(T));

        assert(m_Systems.find(typeIndex) == m_Systems.end() &&
               "System already registered");

        // Create the system
        auto system = std::make_shared<T>(std::forward<Args>(args)...);
        T* rawPtr = system.get();

        // Store in map
        m_Systems[typeIndex] = system;

        // Add to ordered list
        m_SystemOrder.push_back(rawPtr);

        // Initialize default signature from the system
        m_Signatures[typeIndex] = rawPtr->GetRequiredSignature();

        // Sort systems by priority
        SortSystemsByPriority();

        return rawPtr;
    }

    template<typename T>
    T* SystemManager::GetSystem()
    {
        std::type_index typeIndex = std::type_index(typeid(T));

        auto it = m_Systems.find(typeIndex);
        if (it == m_Systems.end())
        {
            return nullptr;
        }

        return static_cast<T*>(it->second.get());
    }

    template<typename T>
    const T* SystemManager::GetSystem() const
    {
        std::type_index typeIndex = std::type_index(typeid(T));

        auto it = m_Systems.find(typeIndex);
        if (it == m_Systems.end())
        {
            return nullptr;
        }

        return static_cast<const T*>(it->second.get());
    }

    template<typename T>
    bool SystemManager::HasSystem() const
    {
        std::type_index typeIndex = std::type_index(typeid(T));
        return m_Systems.find(typeIndex) != m_Systems.end();
    }

    template<typename T>
    void SystemManager::RemoveSystem()
    {
        std::type_index typeIndex = std::type_index(typeid(T));

        auto it = m_Systems.find(typeIndex);
        if (it != m_Systems.end())
        {
            // Remove from ordered list
            ISystem* sysPtr = it->second.get();
            m_SystemOrder.erase(
                std::remove(m_SystemOrder.begin(), m_SystemOrder.end(), sysPtr),
                m_SystemOrder.end()
            );

            // Remove from maps
            m_Systems.erase(it);
            m_Signatures.erase(typeIndex);
        }
    }

    template<typename T>
    void SystemManager::SetSignature(Signature signature)
    {
        std::type_index typeIndex = std::type_index(typeid(T));

        assert(m_Systems.find(typeIndex) != m_Systems.end() &&
               "System must be registered before setting signature");

        m_Signatures[typeIndex] = signature;
    }

    inline void SystemManager::EntitySignatureChanged(EntityID entity, Signature entitySignature)
    {
        // Notify each system that an entity's signature changed
        for (auto& [typeIndex, system] : m_Systems)
        {
            auto signatureIt = m_Signatures.find(typeIndex);
            if (signatureIt == m_Signatures.end())
            {
                continue;
            }

            const Signature& systemSignature = signatureIt->second;

            // Check if entity signature matches system requirements
            // Entity must have ALL components the system requires
            if ((entitySignature & systemSignature) == systemSignature)
            {
                system->OnEntityAdded(entity);
            }
            else
            {
                system->OnEntityRemoved(entity);
            }
        }
    }

    inline void SystemManager::EntityDestroyed(EntityID entity)
    {
        // Remove entity from all systems
        for (auto& [typeIndex, system] : m_Systems)
        {
            system->OnEntityRemoved(entity);
        }
    }

    inline void SystemManager::InitializeSystems(World& world)
    {
        if (m_Initialized)
        {
            return;
        }

        for (auto* system : m_SystemOrder)
        {
            system->Initialize(world);
        }

        m_Initialized = true;
    }

    inline void SystemManager::UpdateSystems(World& world, float deltaTime)
    {
        for (auto* system : m_SystemOrder)
        {
            if (system->IsEnabled())
            {
                system->Update(world, deltaTime);
            }
        }
    }

    inline void SystemManager::ShutdownSystems(World& world)
    {
        if (!m_Initialized)
        {
            return;
        }

        // Shutdown in reverse order
        for (auto it = m_SystemOrder.rbegin(); it != m_SystemOrder.rend(); ++it)
        {
            (*it)->Shutdown(world);
        }

        m_Initialized = false;
    }

    inline void SystemManager::SortSystemsByPriority()
    {
        std::sort(m_SystemOrder.begin(), m_SystemOrder.end(),
            [](const ISystem* a, const ISystem* b) {
                return a->GetPriority() < b->GetPriority();
            }
        );
    }

    inline void SystemManager::Reset()
    {
        m_Systems.clear();
        m_Signatures.clear();
        m_SystemOrder.clear();
        m_Initialized = false;
    }

} // namespace SM
