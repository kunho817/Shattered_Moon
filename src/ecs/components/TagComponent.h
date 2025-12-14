#pragma once

/**
 * @file TagComponent.h
 * @brief Tag component for entity naming and identification
 *
 * Provides a name and optional tags for entities.
 */

#include <string>
#include <vector>
#include <algorithm>

namespace SM
{
    // ============================================================================
    // TagComponent
    // ============================================================================

    /**
     * @brief Component for entity identification and categorization
     *
     * Provides a human-readable name and a list of tags for filtering
     * and organizing entities.
     */
    struct TagComponent
    {
        /** Primary name of the entity */
        std::string Name;

        /** Additional tags for categorization */
        std::vector<std::string> Tags;

        TagComponent() = default;

        explicit TagComponent(const std::string& name)
            : Name(name)
        {
        }

        TagComponent(const std::string& name, const std::vector<std::string>& tags)
            : Name(name)
            , Tags(tags)
        {
        }

        TagComponent(const std::string& name, std::initializer_list<std::string> tags)
            : Name(name)
            , Tags(tags)
        {
        }

        /**
         * @brief Check if entity has a specific tag
         * @param tag The tag to check for
         * @return true if the entity has the tag
         */
        bool HasTag(const std::string& tag) const
        {
            return std::find(Tags.begin(), Tags.end(), tag) != Tags.end();
        }

        /**
         * @brief Add a tag to the entity
         * @param tag The tag to add
         */
        void AddTag(const std::string& tag)
        {
            if (!HasTag(tag))
            {
                Tags.push_back(tag);
            }
        }

        /**
         * @brief Remove a tag from the entity
         * @param tag The tag to remove
         * @return true if the tag was found and removed
         */
        bool RemoveTag(const std::string& tag)
        {
            auto it = std::find(Tags.begin(), Tags.end(), tag);
            if (it != Tags.end())
            {
                Tags.erase(it);
                return true;
            }
            return false;
        }

        /**
         * @brief Clear all tags
         */
        void ClearTags()
        {
            Tags.clear();
        }

        /**
         * @brief Check if entity has all specified tags
         */
        bool HasAllTags(const std::vector<std::string>& tagsToCheck) const
        {
            for (const auto& tag : tagsToCheck)
            {
                if (!HasTag(tag))
                {
                    return false;
                }
            }
            return true;
        }

        /**
         * @brief Check if entity has any of the specified tags
         */
        bool HasAnyTag(const std::vector<std::string>& tagsToCheck) const
        {
            for (const auto& tag : tagsToCheck)
            {
                if (HasTag(tag))
                {
                    return true;
                }
            }
            return false;
        }
    };

    // ============================================================================
    // Common Tag Constants
    // ============================================================================

    namespace Tags
    {
        // Entity types
        constexpr const char* Player = "Player";
        constexpr const char* Enemy = "Enemy";
        constexpr const char* NPC = "NPC";
        constexpr const char* Prop = "Prop";
        constexpr const char* Trigger = "Trigger";
        constexpr const char* Light = "Light";
        constexpr const char* Camera = "Camera";
        constexpr const char* Audio = "Audio";

        // States
        constexpr const char* Active = "Active";
        constexpr const char* Inactive = "Inactive";
        constexpr const char* Static = "Static";
        constexpr const char* Dynamic = "Dynamic";

        // Layers
        constexpr const char* Background = "Background";
        constexpr const char* Foreground = "Foreground";
        constexpr const char* UI = "UI";
    }

} // namespace SM
