#include "editor/ECSPanel.h"
#include "ecs/World.h"
#include "ecs/components/Components.h"

#include <imgui.h>
#include <algorithm>
#include <cstring>

namespace SM
{
    void ECSPanel::Draw(World& world)
    {
        if (!m_Visible)
        {
            return;
        }

        ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(680, 10), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("ECS Inspector", &m_Visible))
        {
            // Split into two columns: entity list and component inspector
            float panelWidth = ImGui::GetContentRegionAvail().x;

            // Entity List (left panel)
            ImGui::BeginChild("EntityList", ImVec2(panelWidth * 0.4f, 0), true);
            DrawEntityList(world);
            ImGui::EndChild();

            ImGui::SameLine();

            // Component Inspector (right panel)
            ImGui::BeginChild("ComponentInspector", ImVec2(0, 0), true);
            if (m_SelectedEntity != INVALID_ENTITY && world.IsAlive(m_SelectedEntity))
            {
                DrawComponentInspector(world, m_SelectedEntity);
            }
            else
            {
                ImGui::TextDisabled("Select an entity to inspect");
            }
            ImGui::EndChild();
        }
        ImGui::End();

        // Draw create entity dialog if open
        if (m_ShowCreateDialog)
        {
            DrawCreateEntityDialog(world);
        }
    }

    void ECSPanel::DrawEntityList(World& world)
    {
        ImGui::Text("Entities (%d)", world.GetEntityCount());
        ImGui::Separator();

        // Search filter
        ImGui::PushItemWidth(-1);
        if (ImGui::InputTextWithHint("##Search", "Search...", m_SearchBuffer, sizeof(m_SearchBuffer)))
        {
            m_SearchFilter = m_SearchBuffer;
        }
        ImGui::PopItemWidth();

        // Create entity button
        if (ImGui::Button("+ Create", ImVec2(-1, 0)))
        {
            m_ShowCreateDialog = true;
        }

        ImGui::Separator();

        // Entity list
        ImGui::BeginChild("EntityListScroll");

        for (EntityID entity = 0; entity < MAX_ENTITIES; ++entity)
        {
            if (!world.IsAlive(entity))
            {
                continue;
            }

            std::string displayName = GetEntityDisplayName(world, entity);

            // Apply search filter
            if (!m_SearchFilter.empty())
            {
                std::string lowerName = displayName;
                std::string lowerFilter = m_SearchFilter;
                std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
                std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);

                if (lowerName.find(lowerFilter) == std::string::npos)
                {
                    continue;
                }
            }

            // Selectable item
            bool isSelected = (m_SelectedEntity == entity);
            if (ImGui::Selectable(displayName.c_str(), isSelected))
            {
                EntityID oldSelection = m_SelectedEntity;
                m_SelectedEntity = entity;

                if (m_OnSelectionChanged && oldSelection != entity)
                {
                    m_OnSelectionChanged(entity);
                }
            }

            // Right-click context menu
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Delete Entity"))
                {
                    if (m_SelectedEntity == entity)
                    {
                        m_SelectedEntity = INVALID_ENTITY;
                    }
                    world.DestroyEntity(entity);
                }
                if (ImGui::MenuItem("Duplicate"))
                {
                    // TODO: Implement entity duplication
                }
                ImGui::EndPopup();
            }
        }

        ImGui::EndChild();
    }

    void ECSPanel::DrawComponentInspector(World& world, EntityID entity)
    {
        std::string displayName = GetEntityDisplayName(world, entity);
        ImGui::Text("Entity: %s", displayName.c_str());
        ImGui::Text("ID: %u", entity);
        ImGui::Separator();

        // Delete button
        if (ImGui::Button("Delete Entity"))
        {
            world.DestroyEntity(entity);
            m_SelectedEntity = INVALID_ENTITY;
            return;
        }

        ImGui::Separator();
        ImGui::Text("Components:");

        // Draw each component type if present
        DrawTagComponent(world, entity);
        DrawTransformComponent(world, entity);
        DrawMeshComponent(world, entity);
        DrawMaterialComponent(world, entity);
        DrawCameraComponent(world, entity);
        DrawLightComponent(world, entity);
        DrawVelocityComponent(world, entity);

        // Add component dropdown
        ImGui::Separator();
        if (ImGui::Button("+ Add Component"))
        {
            ImGui::OpenPopup("AddComponentPopup");
        }

        if (ImGui::BeginPopup("AddComponentPopup"))
        {
            if (!world.HasComponent<TransformComponent>(entity) && ImGui::MenuItem("Transform"))
            {
                world.AddComponent(entity, TransformComponent{});
            }
            if (!world.HasComponent<MeshComponent>(entity) && ImGui::MenuItem("Mesh"))
            {
                world.AddComponent(entity, MeshComponent{});
            }
            if (!world.HasComponent<MaterialComponent>(entity) && ImGui::MenuItem("Material"))
            {
                world.AddComponent(entity, MaterialComponent{});
            }
            if (!world.HasComponent<CameraComponent>(entity) && ImGui::MenuItem("Camera"))
            {
                world.AddComponent(entity, CameraComponent{});
            }
            if (!world.HasComponent<LightComponent>(entity) && ImGui::MenuItem("Light"))
            {
                world.AddComponent(entity, LightComponent{});
            }
            if (!world.HasComponent<VelocityComponent>(entity) && ImGui::MenuItem("Velocity"))
            {
                world.AddComponent(entity, VelocityComponent{});
            }
            ImGui::EndPopup();
        }
    }

    void ECSPanel::DrawTransformComponent(World& world, EntityID entity)
    {
        if (!world.HasComponent<TransformComponent>(entity))
        {
            return;
        }

        if (ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        {
            TransformComponent& transform = world.GetComponent<TransformComponent>(entity);

            // Position
            float pos[3] = { transform.Position.x, transform.Position.y, transform.Position.z };
            if (ImGui::DragFloat3("Position", pos, 0.1f))
            {
                transform.Position = Vector3(pos[0], pos[1], pos[2]);
            }

            // Scale
            float scale[3] = { transform.Scale.x, transform.Scale.y, transform.Scale.z };
            if (ImGui::DragFloat3("Scale", scale, 0.01f, 0.01f, 100.0f))
            {
                transform.Scale = Vector3(scale[0], scale[1], scale[2]);
            }

            // Rotation (quaternion displayed as euler for editing)
            // Note: This is a simplified display; proper euler<->quaternion conversion needed
            ImGui::Text("Rotation: (%.2f, %.2f, %.2f, %.2f)",
                transform.Rotation.x, transform.Rotation.y,
                transform.Rotation.z, transform.Rotation.w);

            // Remove component button
            if (ImGui::SmallButton("Remove##Transform"))
            {
                world.RemoveComponent<TransformComponent>(entity);
            }

            ImGui::TreePop();
        }
    }

    void ECSPanel::DrawMeshComponent(World& world, EntityID entity)
    {
        if (!world.HasComponent<MeshComponent>(entity))
        {
            return;
        }

        if (ImGui::TreeNode("Mesh"))
        {
            MeshComponent& mesh = world.GetComponent<MeshComponent>(entity);

            // Primitive mesh type selector
            const char* meshTypes[] = { "Cube", "Sphere", "Plane", "Cylinder", "Cone", "Quad", "Custom" };
            int currentType = static_cast<int>(mesh.PrimitiveType);
            if (currentType > 5) currentType = 6; // Custom

            if (ImGui::Combo("Primitive Type", &currentType, meshTypes, 7))
            {
                if (currentType <= 5)
                {
                    mesh.PrimitiveType = static_cast<PrimitiveMesh>(currentType);
                }
            }

            ImGui::Text("Mesh ID: %u", mesh.MeshId);

            // Remove component button
            if (ImGui::SmallButton("Remove##Mesh"))
            {
                world.RemoveComponent<MeshComponent>(entity);
            }

            ImGui::TreePop();
        }
    }

    void ECSPanel::DrawMaterialComponent(World& world, EntityID entity)
    {
        if (!world.HasComponent<MaterialComponent>(entity))
        {
            return;
        }

        if (ImGui::TreeNode("Material"))
        {
            MaterialComponent& material = world.GetComponent<MaterialComponent>(entity);

            // Base color
            float color[4] = { material.BaseColor.r, material.BaseColor.g,
                               material.BaseColor.b, material.BaseColor.a };
            if (ImGui::ColorEdit4("Base Color", color))
            {
                material.BaseColor = Color(color[0], color[1], color[2], color[3]);
            }

            // Metallic/Roughness
            ImGui::SliderFloat("Metallic", &material.Metallic, 0.0f, 1.0f);
            ImGui::SliderFloat("Roughness", &material.Roughness, 0.0f, 1.0f);

            // Remove component button
            if (ImGui::SmallButton("Remove##Material"))
            {
                world.RemoveComponent<MaterialComponent>(entity);
            }

            ImGui::TreePop();
        }
    }

    void ECSPanel::DrawTagComponent(World& world, EntityID entity)
    {
        if (!world.HasComponent<TagComponent>(entity))
        {
            return;
        }

        if (ImGui::TreeNode("Tag"))
        {
            TagComponent& tag = world.GetComponent<TagComponent>(entity);

            // Name editing
            char nameBuffer[128];
            strncpy_s(nameBuffer, tag.Name.c_str(), sizeof(nameBuffer) - 1);
            if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer)))
            {
                tag.Name = nameBuffer;
            }

            // Tag editing
            char tagBuffer[64];
            strncpy_s(tagBuffer, tag.Tag.c_str(), sizeof(tagBuffer) - 1);
            if (ImGui::InputText("Tag", tagBuffer, sizeof(tagBuffer)))
            {
                tag.Tag = tagBuffer;
            }

            // Active/Static flags
            ImGui::Checkbox("Active", &tag.IsActive);
            ImGui::Checkbox("Static", &tag.IsStatic);

            // Remove component button
            if (ImGui::SmallButton("Remove##Tag"))
            {
                world.RemoveComponent<TagComponent>(entity);
            }

            ImGui::TreePop();
        }
    }

    void ECSPanel::DrawCameraComponent(World& world, EntityID entity)
    {
        if (!world.HasComponent<CameraComponent>(entity))
        {
            return;
        }

        if (ImGui::TreeNode("Camera"))
        {
            CameraComponent& camera = world.GetComponent<CameraComponent>(entity);

            ImGui::SliderFloat("FOV", &camera.FieldOfView, 30.0f, 120.0f, "%.1f deg");
            ImGui::DragFloat("Near Plane", &camera.NearPlane, 0.01f, 0.001f, camera.FarPlane - 0.001f);
            ImGui::DragFloat("Far Plane", &camera.FarPlane, 1.0f, camera.NearPlane + 0.001f, 100000.0f);
            ImGui::Checkbox("Is Primary", &camera.IsPrimary);
            ImGui::Checkbox("Is Active", &camera.IsActive);

            // Remove component button
            if (ImGui::SmallButton("Remove##Camera"))
            {
                world.RemoveComponent<CameraComponent>(entity);
            }

            ImGui::TreePop();
        }
    }

    void ECSPanel::DrawLightComponent(World& world, EntityID entity)
    {
        if (!world.HasComponent<LightComponent>(entity))
        {
            return;
        }

        if (ImGui::TreeNode("Light"))
        {
            LightComponent& light = world.GetComponent<LightComponent>(entity);

            // Light type
            const char* lightTypes[] = { "Directional", "Point", "Spot" };
            int currentType = static_cast<int>(light.Type);
            if (ImGui::Combo("Type", &currentType, lightTypes, 3))
            {
                light.Type = static_cast<LightType>(currentType);
            }

            // Color
            float color[3] = { light.Color.r, light.Color.g, light.Color.b };
            if (ImGui::ColorEdit3("Color", color))
            {
                light.Color = Color(color[0], color[1], color[2], 1.0f);
            }

            // Intensity
            ImGui::DragFloat("Intensity", &light.Intensity, 0.1f, 0.0f, 100.0f);

            // Point/Spot light specific
            if (light.Type == LightType::Point || light.Type == LightType::Spot)
            {
                ImGui::DragFloat("Range", &light.Range, 1.0f, 0.0f, 1000.0f);
            }

            // Spot light specific
            if (light.Type == LightType::Spot)
            {
                ImGui::SliderFloat("Inner Angle", &light.InnerConeAngle, 0.0f, light.OuterConeAngle, "%.1f deg");
                ImGui::SliderFloat("Outer Angle", &light.OuterConeAngle, light.InnerConeAngle, 90.0f, "%.1f deg");
            }

            ImGui::Checkbox("Cast Shadows", &light.CastShadows);
            ImGui::Checkbox("Is Active", &light.IsActive);

            // Remove component button
            if (ImGui::SmallButton("Remove##Light"))
            {
                world.RemoveComponent<LightComponent>(entity);
            }

            ImGui::TreePop();
        }
    }

    void ECSPanel::DrawVelocityComponent(World& world, EntityID entity)
    {
        if (!world.HasComponent<VelocityComponent>(entity))
        {
            return;
        }

        if (ImGui::TreeNode("Velocity"))
        {
            VelocityComponent& velocity = world.GetComponent<VelocityComponent>(entity);

            // Linear velocity
            float linear[3] = { velocity.Linear.x, velocity.Linear.y, velocity.Linear.z };
            if (ImGui::DragFloat3("Linear", linear, 0.1f))
            {
                velocity.Linear = Vector3(linear[0], linear[1], linear[2]);
            }

            // Angular velocity
            float angular[3] = { velocity.Angular.x, velocity.Angular.y, velocity.Angular.z };
            if (ImGui::DragFloat3("Angular", angular, 0.01f))
            {
                velocity.Angular = Vector3(angular[0], angular[1], angular[2]);
            }

            // Remove component button
            if (ImGui::SmallButton("Remove##Velocity"))
            {
                world.RemoveComponent<VelocityComponent>(entity);
            }

            ImGui::TreePop();
        }
    }

    void ECSPanel::DrawCreateEntityDialog(World& world)
    {
        ImGui::OpenPopup("Create Entity");

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("Create Entity", &m_ShowCreateDialog, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::InputText("Name", m_NewEntityName, sizeof(m_NewEntityName));

            ImGui::Separator();

            if (ImGui::Button("Create", ImVec2(120, 0)))
            {
                EntityID newEntity = world.CreateEntity(m_NewEntityName);
                m_SelectedEntity = newEntity;
                m_ShowCreateDialog = false;

                // Reset name for next entity
                strcpy_s(m_NewEntityName, "New Entity");
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                m_ShowCreateDialog = false;
            }

            ImGui::EndPopup();
        }
    }

    std::string ECSPanel::GetEntityDisplayName(World& world, EntityID entity) const
    {
        if (world.HasComponent<TagComponent>(entity))
        {
            const TagComponent& tag = world.GetComponent<TagComponent>(entity);
            if (!tag.Name.empty())
            {
                return tag.Name + " (" + std::to_string(entity) + ")";
            }
        }

        return "Entity " + std::to_string(entity);
    }

} // namespace SM
