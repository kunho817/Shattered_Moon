#pragma once

/**
 * @file ECSPanel.h
 * @brief ECS Inspector panel for the editor
 *
 * Provides UI for inspecting and editing entities and their components
 * in the ECS world.
 */

#include "ecs/Entity.h"
#include <string>
#include <functional>

namespace SM
{
    // Forward declarations
    class World;

    /**
     * @brief ECS Inspector panel
     *
     * Displays:
     * - List of all entities with names/tags
     * - Component list for selected entity
     * - Component property editors
     * - Entity creation/deletion controls
     */
    class ECSPanel
    {
    public:
        ECSPanel() = default;
        ~ECSPanel() = default;

        /**
         * @brief Draw the ECS panel using ImGui
         * @param world Reference to the ECS world
         */
        void Draw(World& world);

        // ====================================================================
        // Selection
        // ====================================================================

        /**
         * @brief Get the currently selected entity
         * @return Selected entity ID, or INVALID_ENTITY if none
         */
        EntityID GetSelectedEntity() const { return m_SelectedEntity; }

        /**
         * @brief Set the selected entity
         * @param entity Entity to select
         */
        void SetSelectedEntity(EntityID entity) { m_SelectedEntity = entity; }

        /**
         * @brief Clear the selection
         */
        void ClearSelection() { m_SelectedEntity = INVALID_ENTITY; }

        /**
         * @brief Check if an entity is selected
         */
        bool HasSelection() const { return m_SelectedEntity != INVALID_ENTITY; }

        // ====================================================================
        // Visibility
        // ====================================================================

        /**
         * @brief Set panel visibility
         */
        void SetVisible(bool visible) { m_Visible = visible; }

        /**
         * @brief Get panel visibility
         */
        bool IsVisible() const { return m_Visible; }

        /**
         * @brief Toggle visibility
         */
        void ToggleVisible() { m_Visible = !m_Visible; }

        // ====================================================================
        // Callbacks
        // ====================================================================

        /**
         * @brief Set callback for entity selection changes
         */
        void SetSelectionChangedCallback(std::function<void(EntityID)> callback)
        {
            m_OnSelectionChanged = callback;
        }

    private:
        /**
         * @brief Draw the entity hierarchy/list panel
         */
        void DrawEntityList(World& world);

        /**
         * @brief Draw the component inspector panel
         */
        void DrawComponentInspector(World& world, EntityID entity);

        /**
         * @brief Draw transform component editor
         */
        void DrawTransformComponent(World& world, EntityID entity);

        /**
         * @brief Draw mesh component editor
         */
        void DrawMeshComponent(World& world, EntityID entity);

        /**
         * @brief Draw material component editor
         */
        void DrawMaterialComponent(World& world, EntityID entity);

        /**
         * @brief Draw tag component editor
         */
        void DrawTagComponent(World& world, EntityID entity);

        /**
         * @brief Draw camera component editor
         */
        void DrawCameraComponent(World& world, EntityID entity);

        /**
         * @brief Draw light component editor
         */
        void DrawLightComponent(World& world, EntityID entity);

        /**
         * @brief Draw velocity component editor
         */
        void DrawVelocityComponent(World& world, EntityID entity);

        /**
         * @brief Draw entity creation dialog
         */
        void DrawCreateEntityDialog(World& world);

        /**
         * @brief Get display name for an entity
         */
        std::string GetEntityDisplayName(World& world, EntityID entity) const;

    private:
        bool m_Visible = true;
        EntityID m_SelectedEntity = INVALID_ENTITY;

        // UI state
        bool m_ShowCreateDialog = false;
        char m_NewEntityName[128] = "New Entity";
        std::string m_SearchFilter;
        char m_SearchBuffer[128] = "";

        // Callbacks
        std::function<void(EntityID)> m_OnSelectionChanged;
    };

} // namespace SM
