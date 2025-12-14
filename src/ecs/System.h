#pragma once

/**
 * @file System.h
 * @brief System interface for Shattered Moon ECS
 *
 * Systems contain the logic that operates on entities with specific
 * component signatures. Each system defines which components it requires
 * and processes entities that have all required components.
 */

#include "Entity.h"

#include <set>
#include <string>

namespace SM
{
    // Forward declaration
    class World;

    // ============================================================================
    // ISystem Interface
    // ============================================================================

    /**
     * @brief Base interface for all ECS systems
     *
     * Systems are responsible for processing entities with specific component
     * combinations. Each system maintains a set of entities it should process
     * based on component signatures.
     */
    class ISystem
    {
    public:
        virtual ~ISystem() = default;

        /**
         * @brief Called once when the system is initialized
         * @param world Reference to the ECS world
         */
        virtual void Initialize([[maybe_unused]] World& world) {}

        /**
         * @brief Update the system
         * @param world Reference to the ECS world
         * @param deltaTime Time since last frame in seconds
         */
        virtual void Update(World& world, float deltaTime) = 0;

        /**
         * @brief Called when the system is shutting down
         * @param world Reference to the ECS world
         */
        virtual void Shutdown([[maybe_unused]] World& world) {}

        /**
         * @brief Get the name of this system (for debugging)
         * @return System name string
         */
        virtual const char* GetName() const = 0;

        /**
         * @brief Get the required component signature for this system
         * @return The signature representing required components
         *
         * Override this to specify which components the system requires.
         * By default, returns an empty signature (processes no entities).
         */
        virtual Signature GetRequiredSignature() const { return Signature(); }

        /**
         * @brief Check if the system is enabled
         * @return true if the system should be updated
         */
        bool IsEnabled() const { return m_Enabled; }

        /**
         * @brief Enable or disable the system
         * @param enabled New enabled state
         */
        void SetEnabled(bool enabled) { m_Enabled = enabled; }

        /**
         * @brief Get the priority of this system (lower = earlier execution)
         * @return System priority
         */
        int GetPriority() const { return m_Priority; }

        /**
         * @brief Set the system priority
         * @param priority New priority value
         */
        void SetPriority(int priority) { m_Priority = priority; }

        /**
         * @brief Get the set of entities this system processes
         * @return Set of entity IDs
         */
        const std::set<EntityID>& GetEntities() const { return m_Entities; }

        /**
         * @brief Called internally when an entity matches the system's signature
         * @param entity The entity that was added
         */
        void OnEntityAdded(EntityID entity)
        {
            m_Entities.insert(entity);
        }

        /**
         * @brief Called internally when an entity no longer matches the signature
         * @param entity The entity that was removed
         */
        void OnEntityRemoved(EntityID entity)
        {
            m_Entities.erase(entity);
        }

    protected:
        /** Set of entities this system processes */
        std::set<EntityID> m_Entities;

        /** Whether the system is currently enabled */
        bool m_Enabled = true;

        /** System priority (lower = earlier execution) */
        int m_Priority = 0;
    };

    // ============================================================================
    // System CRTP Base
    // ============================================================================

    /**
     * @brief CRTP base class for systems with automatic signature setup
     *
     * Derive from this class and implement SetupSignature() to automatically
     * configure the system's required component signature.
     *
     * @tparam Derived The derived system class
     */
    template<typename Derived>
    class System : public ISystem
    {
    public:
        const char* GetName() const override
        {
            // Default implementation returns the type name
            // Derived classes should override for cleaner names
            return typeid(Derived).name();
        }
    };

} // namespace SM
