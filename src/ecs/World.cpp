#include "World.h"
#include "components/Components.h"

namespace SM
{
    EntityID World::CreateEntity(const std::string& name)
    {
        EntityID entity = CreateEntity();
        if (entity != INVALID_ENTITY)
        {
            // Ensure TagComponent is registered
            if (!m_ComponentManager.IsComponentRegistered<TagComponent>())
            {
                RegisterComponent<TagComponent>();
            }
            AddComponent(entity, TagComponent{ name });
        }
        return entity;
    }

    EntityID World::FindEntityByName(const std::string& name) const
    {
        if (!m_ComponentManager.IsComponentRegistered<TagComponent>())
        {
            return INVALID_ENTITY;
        }

        for (EntityID entity = 0; entity < MAX_ENTITIES; ++entity)
        {
            if (!m_EntityManager.IsAlive(entity))
            {
                continue;
            }

            const TagComponent* tag = m_ComponentManager.TryGetComponent<TagComponent>(entity);
            if (tag && tag->Name == name)
            {
                return entity;
            }
        }

        return INVALID_ENTITY;
    }

    std::vector<EntityID> World::FindEntitiesByTag(const std::string& tag) const
    {
        std::vector<EntityID> result;

        if (!m_ComponentManager.IsComponentRegistered<TagComponent>())
        {
            return result;
        }

        for (EntityID entity = 0; entity < MAX_ENTITIES; ++entity)
        {
            if (!m_EntityManager.IsAlive(entity))
            {
                continue;
            }

            const TagComponent* tagComp = m_ComponentManager.TryGetComponent<TagComponent>(entity);
            if (tagComp && tagComp->HasTag(tag))
            {
                result.push_back(entity);
            }
        }

        return result;
    }

} // namespace SM
