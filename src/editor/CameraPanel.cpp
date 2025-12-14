#include "editor/CameraPanel.h"
#include "gameplay/Camera.h"
#include "gameplay/CameraController.h"

#include <imgui.h>
#include <cmath>

namespace SM
{
    void CameraPanel::Draw(GameCamera& camera, FPSCameraController* fpsController, OrbitCameraController* orbitController)
    {
        if (!m_Visible)
        {
            return;
        }

        ImGui::SetNextWindowSize(ImVec2(320, 400), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(10, 380), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Camera", &m_Visible))
        {
            // Camera type indicator
            if (fpsController && fpsController->IsMouseCaptured())
            {
                ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Mode: FPS (Captured)");
            }
            else
            {
                ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Mode: Orbit");
            }

            ImGui::Separator();

            DrawPositionControls(camera);
            DrawRotationControls(camera);
            DrawProjectionSettings(camera);

            if (fpsController && ImGui::CollapsingHeader("FPS Controller"))
            {
                DrawFPSControllerSettings(fpsController);
            }

            if (orbitController && ImGui::CollapsingHeader("Orbit Controller"))
            {
                DrawOrbitControllerSettings(orbitController);
            }

            DrawPresets(camera);
        }
        ImGui::End();
    }

    void CameraPanel::Draw(GameCamera& camera)
    {
        Draw(camera, nullptr, nullptr);
    }

    void CameraPanel::DrawPositionControls(GameCamera& camera)
    {
        if (ImGui::CollapsingHeader("Position", ImGuiTreeNodeFlags_DefaultOpen))
        {
            Vector3 position = camera.GetPosition();
            float pos[3] = { position.x, position.y, position.z };

            if (ImGui::DragFloat3("Position", pos, 1.0f, -10000.0f, 10000.0f, "%.2f"))
            {
                camera.SetPosition(pos[0], pos[1], pos[2]);
                m_PositionChanged = true;
            }

            // Individual axis controls for fine-tuning
            ImGui::Indent();
            if (ImGui::DragFloat("X##pos", &pos[0], 0.5f, -10000.0f, 10000.0f, "%.2f"))
            {
                camera.SetPosition(pos[0], pos[1], pos[2]);
            }
            if (ImGui::DragFloat("Y##pos", &pos[1], 0.5f, -10000.0f, 10000.0f, "%.2f"))
            {
                camera.SetPosition(pos[0], pos[1], pos[2]);
            }
            if (ImGui::DragFloat("Z##pos", &pos[2], 0.5f, -10000.0f, 10000.0f, "%.2f"))
            {
                camera.SetPosition(pos[0], pos[1], pos[2]);
            }
            ImGui::Unindent();

            // Reset position button
            if (ImGui::Button("Reset Position"))
            {
                camera.SetPosition(0.0f, 30.0f, -80.0f);
            }
        }
    }

    void CameraPanel::DrawRotationControls(GameCamera& camera)
    {
        if (ImGui::CollapsingHeader("Rotation", ImGuiTreeNodeFlags_DefaultOpen))
        {
            float pitch = camera.GetPitch();
            float yaw = camera.GetYaw();

            if (ImGui::SliderFloat("Pitch", &pitch, -89.0f, 89.0f, "%.1f deg"))
            {
                camera.SetRotation(pitch, yaw);
                m_RotationChanged = true;
            }

            if (ImGui::SliderFloat("Yaw", &yaw, -180.0f, 180.0f, "%.1f deg"))
            {
                camera.SetRotation(pitch, yaw);
                m_RotationChanged = true;
            }

            // Direction vectors (read-only)
            ImGui::Text("Direction Vectors:");
            ImGui::Indent();

            const Vector3& forward = camera.GetForward();
            const Vector3& right = camera.GetRight();
            const Vector3& up = camera.GetUp();

            ImGui::Text("Forward: (%.2f, %.2f, %.2f)", forward.x, forward.y, forward.z);
            ImGui::Text("Right:   (%.2f, %.2f, %.2f)", right.x, right.y, right.z);
            ImGui::Text("Up:      (%.2f, %.2f, %.2f)", up.x, up.y, up.z);

            ImGui::Unindent();

            // Reset rotation button
            if (ImGui::Button("Reset Rotation"))
            {
                camera.SetRotation(0.0f, -90.0f);
            }
        }
    }

    void CameraPanel::DrawProjectionSettings(GameCamera& camera)
    {
        if (ImGui::CollapsingHeader("Projection"))
        {
            // Projection type
            const char* projTypes[] = { "Perspective", "Orthographic" };
            int currentProj = (camera.GetProjectionType() == ProjectionType::Perspective) ? 0 : 1;

            if (ImGui::Combo("Type", &currentProj, projTypes, 2))
            {
                if (currentProj == 0)
                {
                    camera.SetPerspective(
                        camera.GetFOV(),
                        16.0f / 9.0f,
                        camera.GetNearZ(),
                        camera.GetFarZ()
                    );
                }
                else
                {
                    camera.SetOrthographic(100.0f, 56.25f, camera.GetNearZ(), camera.GetFarZ());
                }
            }

            // FOV (for perspective)
            if (camera.GetProjectionType() == ProjectionType::Perspective)
            {
                float fov = camera.GetFOV();
                if (ImGui::SliderFloat("FOV", &fov, 30.0f, 120.0f, "%.1f deg"))
                {
                    camera.SetPerspective(fov, 16.0f / 9.0f, camera.GetNearZ(), camera.GetFarZ());
                }
            }

            // Near/Far planes
            float nearZ = camera.GetNearZ();
            float farZ = camera.GetFarZ();

            if (ImGui::DragFloat("Near Plane", &nearZ, 0.01f, 0.001f, farZ - 0.001f, "%.3f"))
            {
                if (camera.GetProjectionType() == ProjectionType::Perspective)
                {
                    camera.SetPerspective(camera.GetFOV(), 16.0f / 9.0f, nearZ, farZ);
                }
            }

            if (ImGui::DragFloat("Far Plane", &farZ, 1.0f, nearZ + 0.001f, 100000.0f, "%.1f"))
            {
                if (camera.GetProjectionType() == ProjectionType::Perspective)
                {
                    camera.SetPerspective(camera.GetFOV(), 16.0f / 9.0f, nearZ, farZ);
                }
            }
        }
    }

    void CameraPanel::DrawFPSControllerSettings(FPSCameraController* controller)
    {
        if (!controller)
        {
            return;
        }

        ImGui::Indent();

        // Movement speed
        float moveSpeed = controller->GetMoveSpeed();
        if (ImGui::SliderFloat("Move Speed", &moveSpeed, 1.0f, 100.0f, "%.1f"))
        {
            controller->SetMoveSpeed(moveSpeed);
        }

        // Sprint multiplier
        float sprintMult = controller->GetSprintMultiplier();
        if (ImGui::SliderFloat("Sprint Multiplier", &sprintMult, 1.0f, 5.0f, "%.1f"))
        {
            controller->SetSprintMultiplier(sprintMult);
        }

        // Mouse sensitivity
        float sensitivity = controller->GetMouseSensitivity();
        if (ImGui::SliderFloat("Mouse Sensitivity", &sensitivity, 0.01f, 1.0f, "%.3f"))
        {
            controller->SetMouseSensitivity(sensitivity);
        }

        // Mouse capture status
        if (controller->IsMouseCaptured())
        {
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Mouse: Captured");
            ImGui::Text("(Press ESC to release)");
        }
        else
        {
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Mouse: Free");
            ImGui::Text("(Right-click to capture)");
        }

        ImGui::Unindent();
    }

    void CameraPanel::DrawOrbitControllerSettings(OrbitCameraController* controller)
    {
        if (!controller)
        {
            return;
        }

        ImGui::Indent();

        // Target position
        Vector3 target = controller->GetTarget();
        float targetPos[3] = { target.x, target.y, target.z };
        if (ImGui::DragFloat3("Target", targetPos, 1.0f, -10000.0f, 10000.0f, "%.2f"))
        {
            controller->SetTarget(targetPos[0], targetPos[1], targetPos[2]);
        }

        // Distance
        float distance = controller->GetDistance();
        if (ImGui::SliderFloat("Distance", &distance, 5.0f, 500.0f, "%.1f"))
        {
            controller->SetDistance(distance);
        }

        // Azimuth (horizontal rotation)
        float azimuth = controller->GetAzimuth();
        if (ImGui::SliderFloat("Azimuth", &azimuth, -180.0f, 180.0f, "%.1f deg"))
        {
            controller->SetAzimuth(azimuth);
        }

        // Elevation (vertical rotation)
        float elevation = controller->GetElevation();
        if (ImGui::SliderFloat("Elevation", &elevation, -89.0f, 89.0f, "%.1f deg"))
        {
            controller->SetElevation(elevation);
        }

        // Rotation speed
        float rotSpeed = controller->GetRotationSpeed();
        if (ImGui::SliderFloat("Rotation Speed", &rotSpeed, 0.1f, 1.0f, "%.2f"))
        {
            controller->SetRotationSpeed(rotSpeed);
        }

        // Zoom speed
        float zoomSpeed = controller->GetZoomSpeed();
        if (ImGui::SliderFloat("Zoom Speed", &zoomSpeed, 1.0f, 50.0f, "%.1f"))
        {
            controller->SetZoomSpeed(zoomSpeed);
        }

        ImGui::Unindent();
    }

    void CameraPanel::DrawPresets(GameCamera& camera)
    {
        if (ImGui::CollapsingHeader("Presets"))
        {
            ImGui::Text("Quick View Positions:");

            if (ImGui::Button("Top Down"))
            {
                camera.SetPosition(0.0f, 100.0f, 0.1f);
                camera.SetRotation(-89.0f, -90.0f);
            }

            ImGui::SameLine();
            if (ImGui::Button("Front"))
            {
                camera.SetPosition(0.0f, 30.0f, -80.0f);
                camera.SetRotation(-15.0f, -90.0f);
            }

            ImGui::SameLine();
            if (ImGui::Button("Side"))
            {
                camera.SetPosition(-80.0f, 30.0f, 0.0f);
                camera.SetRotation(-15.0f, 0.0f);
            }

            if (ImGui::Button("Isometric"))
            {
                camera.SetPosition(-60.0f, 60.0f, -60.0f);
                camera.SetRotation(-35.0f, -45.0f);
            }

            ImGui::SameLine();
            if (ImGui::Button("Origin"))
            {
                camera.SetPosition(0.0f, 30.0f, -80.0f);
                camera.LookAt(0.0f, 10.0f, 0.0f);
            }
        }
    }

} // namespace SM
