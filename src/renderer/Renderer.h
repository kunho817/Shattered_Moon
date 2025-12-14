#pragma once

/**
 * @file Renderer.h
 * @brief Main Renderer class integrating all DX12 components
 *
 * Provides a high-level interface for rendering operations,
 * managing the render pipeline, and drawing meshes.
 */

#include "renderer/DX12Core.h"
#include "renderer/CommandList.h"
#include "renderer/GPUBuffer.h"
#include "renderer/RootSignature.h"
#include "renderer/PipelineState.h"
#include "renderer/Mesh.h"
#include "renderer/Texture.h"

#include <DirectXMath.h>
#include <memory>
#include <string>
#include <unordered_map>

namespace SM
{
    // Forward declarations
    class Window;

    /**
     * @brief Per-frame constant buffer data
     */
    struct PerFrameData
    {
        DirectX::XMFLOAT4X4 View;
        DirectX::XMFLOAT4X4 Projection;
        DirectX::XMFLOAT4X4 ViewProjection;
        DirectX::XMFLOAT3 CameraPosition;
        float Time;
    };

    /**
     * @brief Per-object constant buffer data
     */
    struct PerObjectData
    {
        DirectX::XMFLOAT4X4 World;
        DirectX::XMFLOAT4X4 WorldInverseTranspose;
    };

    /**
     * @brief Material constant buffer data
     */
    struct MaterialData
    {
        DirectX::XMFLOAT4 BaseColor;
        DirectX::XMFLOAT4 EmissiveColor;
        float Metallic;
        float Roughness;
        float AmbientOcclusion;
        float EmissiveIntensity;
        DirectX::XMFLOAT2 UVScale;
        DirectX::XMFLOAT2 UVOffset;
    };

    /**
     * @brief Simple camera for rendering
     */
    struct Camera
    {
        DirectX::XMFLOAT3 Position = { 0.0f, 2.0f, -5.0f };
        DirectX::XMFLOAT3 Target = { 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT3 Up = { 0.0f, 1.0f, 0.0f };

        float FieldOfView = DirectX::XM_PIDIV4;  // 45 degrees
        float NearPlane = 0.1f;
        float FarPlane = 1000.0f;

        DirectX::XMMATRIX GetViewMatrix() const
        {
            return DirectX::XMMatrixLookAtLH(
                DirectX::XMLoadFloat3(&Position),
                DirectX::XMLoadFloat3(&Target),
                DirectX::XMLoadFloat3(&Up)
            );
        }

        DirectX::XMMATRIX GetProjectionMatrix(float aspectRatio) const
        {
            return DirectX::XMMatrixPerspectiveFovLH(
                FieldOfView,
                aspectRatio,
                NearPlane,
                FarPlane
            );
        }
    };

    /**
     * @brief Render item for batched rendering
     */
    struct RenderItem
    {
        Mesh* MeshPtr = nullptr;
        DirectX::XMFLOAT4X4 WorldMatrix;
        MaterialData Material;
    };

    /**
     * @brief Main Renderer class
     *
     * Manages all rendering operations and provides a simple API
     * for drawing meshes with materials.
     */
    class Renderer
    {
    public:
        Renderer() = default;
        ~Renderer();

        // Prevent copying
        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        /**
         * @brief Initialize the renderer
         * @param hwnd Window handle
         * @param width Window width
         * @param height Window height
         * @param vsync Enable vertical sync
         * @return true if successful
         */
        bool Initialize(HWND hwnd, uint32_t width, uint32_t height, bool vsync = true);

        /**
         * @brief Shutdown and release all resources
         */
        void Shutdown();

        /**
         * @brief Handle window resize
         * @param width New width
         * @param height New height
         */
        void Resize(uint32_t width, uint32_t height);

        /**
         * @brief Begin a new frame
         */
        void BeginFrame();

        /**
         * @brief End the current frame
         */
        void EndFrame();

        /**
         * @brief Present the frame to the screen
         */
        void Present();

        /**
         * @brief Clear the render target and depth buffer
         * @param r Red component (0-1)
         * @param g Green component (0-1)
         * @param b Blue component (0-1)
         * @param a Alpha component (0-1)
         */
        void Clear(float r, float g, float b, float a = 1.0f);

        /**
         * @brief Set the viewport
         * @param width Viewport width
         * @param height Viewport height
         */
        void SetViewport(uint32_t width, uint32_t height);

        /**
         * @brief Set the camera
         * @param camera Camera to use for rendering
         */
        void SetCamera(const Camera& camera);

        /**
         * @brief Update per-frame constant buffer
         * @param totalTime Total time since start
         */
        void UpdateFrameConstants(float totalTime);

        /**
         * @brief Draw a mesh with a material and transform
         * @param mesh Mesh to draw
         * @param material Material data
         * @param worldMatrix World transform matrix
         */
        void DrawMesh(
            const Mesh& mesh,
            const MaterialData& material,
            const DirectX::XMMATRIX& worldMatrix
        );

        /**
         * @brief Draw a mesh with default material
         * @param mesh Mesh to draw
         * @param worldMatrix World transform matrix
         */
        void DrawMesh(const Mesh& mesh, const DirectX::XMMATRIX& worldMatrix);

        // Accessors
        DX12Core* GetCore() { return &m_Core; }
        const DX12Core* GetCore() const { return &m_Core; }

        uint32_t GetWidth() const { return m_Core.GetWidth(); }
        uint32_t GetHeight() const { return m_Core.GetHeight(); }
        float GetAspectRatio() const { return m_Core.GetAspectRatio(); }

        Camera& GetCamera() { return m_Camera; }
        const Camera& GetCamera() const { return m_Camera; }

        /**
         * @brief Get a primitive mesh by type
         * @param type Primitive type (Cube, Sphere, etc.)
         * @return Pointer to mesh, nullptr if invalid
         */
        Mesh* GetPrimitiveMesh(uint32_t type);

        /**
         * @brief Check if renderer is initialized
         */
        bool IsInitialized() const { return m_Initialized; }

        /**
         * @brief Get the native command list for external rendering
         * @return Native DX12 command list, nullptr if not recording
         * @note Only valid between BeginFrame and EndFrame
         */
        ID3D12GraphicsCommandList* GetCommandList() const { return m_CommandList.GetNative(); }

        /**
         * @brief Get the command list wrapper
         * @return Reference to command list wrapper
         */
        CommandList& GetCommandListWrapper() { return m_CommandList; }

    private:
        /**
         * @brief Create shaders
         */
        bool CreateShaders();

        /**
         * @brief Create root signature
         */
        bool CreateRootSignature();

        /**
         * @brief Create pipeline states
         */
        bool CreatePipelineStates();

        /**
         * @brief Create constant buffers
         */
        bool CreateConstantBuffers();

        /**
         * @brief Create primitive meshes
         */
        bool CreatePrimitiveMeshes();

        /**
         * @brief Create default textures
         */
        bool CreateDefaultTextures();

    private:
        // Initialization state
        bool m_Initialized = false;

        // Core DX12 resources
        DX12Core m_Core;
        CommandList m_CommandList;

        // Shaders
        ShaderBytecode m_VertexShader;
        ShaderBytecode m_PixelShader;

        // Pipeline resources
        RootSignature m_RootSignature;
        GraphicsPipelineState m_OpaquePSO;
        GraphicsPipelineState m_WireframePSO;

        // Constant buffers
        std::unique_ptr<ConstantBuffer> m_PerFrameCB;
        std::unique_ptr<ConstantBuffer> m_PerObjectCB;
        std::unique_ptr<ConstantBuffer> m_MaterialCB;

        // Upload heap for dynamic allocations
        std::unique_ptr<UploadHeap> m_UploadHeap;

        // Camera
        Camera m_Camera;
        PerFrameData m_FrameData;

        // Primitive meshes
        std::unique_ptr<Mesh> m_CubeMesh;
        std::unique_ptr<Mesh> m_SphereMesh;
        std::unique_ptr<Mesh> m_PlaneMesh;
        std::unique_ptr<Mesh> m_CylinderMesh;
        std::unique_ptr<Mesh> m_ConeMesh;
        std::unique_ptr<Mesh> m_QuadMesh;

        // Default textures
        std::unique_ptr<Texture> m_WhiteTexture;

        // Default material
        MaterialData m_DefaultMaterial;

        // Frame state
        bool m_FrameStarted = false;
    };

    /**
     * @brief Create a default material data structure
     */
    MaterialData CreateDefaultMaterial();

    /**
     * @brief Create a colored material
     */
    MaterialData CreateColoredMaterial(float r, float g, float b, float a = 1.0f);

    /**
     * @brief Create a metallic material
     */
    MaterialData CreateMetallicMaterial(float r, float g, float b, float metallic, float roughness);

} // namespace SM
