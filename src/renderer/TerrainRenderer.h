#pragma once

/**
 * @file TerrainRenderer.h
 * @brief Specialized renderer for procedural terrain
 *
 * Provides optimized rendering for terrain chunks with:
 * - Terrain-specific shaders
 * - Height-based texture blending
 * - LOD-aware rendering
 */

#include "renderer/DX12Core.h"
#include "renderer/RootSignature.h"
#include "renderer/PipelineState.h"
#include "renderer/GPUBuffer.h"

#include <DirectXMath.h>
#include <memory>

namespace SM
{
    class Renderer;
    class Mesh;
}

namespace PCG
{
    class Chunk;
    class ChunkManager;

    /**
     * @brief Per-frame terrain constant buffer
     */
    struct TerrainPerFrameData
    {
        DirectX::XMFLOAT4X4 ViewProjection;
        DirectX::XMFLOAT3 CameraPosition;
        float HeightScale;
        DirectX::XMFLOAT3 LightDirection;
        float Time;
        DirectX::XMFLOAT4 FogColor;
        float FogStart;
        float FogEnd;
        float Padding[2];
    };

    /**
     * @brief Per-chunk terrain constant buffer
     */
    struct TerrainPerChunkData
    {
        DirectX::XMFLOAT4X4 World;
        DirectX::XMFLOAT2 ChunkOffset;
        float TextureScale;
        float MinHeight;
        float MaxHeight;
        float Padding[3];
    };

    /**
     * @brief Terrain rendering configuration
     */
    struct TerrainRenderConfig
    {
        float HeightScale = 50.0f;        ///< World-space height multiplier
        float TextureScale = 10.0f;       ///< Texture UV tiling
        bool EnableFog = true;            ///< Enable distance fog
        float FogStart = 100.0f;          ///< Distance where fog starts
        float FogEnd = 500.0f;            ///< Distance where fog is fully opaque
        DirectX::XMFLOAT4 FogColor = { 0.6f, 0.7f, 0.8f, 1.0f }; ///< Fog color
        bool EnableWireframe = false;     ///< Render in wireframe mode
    };

    /**
     * @brief Specialized terrain renderer
     *
     * Handles terrain-specific rendering with optimizations for:
     * - Large terrain meshes
     * - Height-based coloring
     * - Distance fog
     * - LOD transitions
     */
    class TerrainRenderer
    {
    public:
        TerrainRenderer();
        ~TerrainRenderer();

        // Prevent copying
        TerrainRenderer(const TerrainRenderer&) = delete;
        TerrainRenderer& operator=(const TerrainRenderer&) = delete;

        // ====================================================================
        // Initialization
        // ====================================================================

        /**
         * @brief Initialize the terrain renderer
         * @param renderer Main renderer reference
         * @return true if initialization succeeded
         */
        bool Initialize(SM::Renderer& renderer);

        /**
         * @brief Shutdown and release all resources
         */
        void Shutdown();

        /**
         * @brief Check if renderer is initialized
         */
        bool IsInitialized() const { return m_Initialized; }

        // ====================================================================
        // Configuration
        // ====================================================================

        /**
         * @brief Set terrain height scale
         * @param scale World-space height multiplier
         */
        void SetHeightScale(float scale);

        /**
         * @brief Set texture tiling scale
         * @param scale UV tiling factor
         */
        void SetTextureScale(float scale);

        /**
         * @brief Set fog parameters
         * @param start Distance where fog starts
         * @param end Distance where fog is fully opaque
         * @param color Fog color
         */
        void SetFog(float start, float end, const DirectX::XMFLOAT4& color);

        /**
         * @brief Enable/disable fog
         */
        void SetFogEnabled(bool enabled);

        /**
         * @brief Enable/disable wireframe rendering
         */
        void SetWireframe(bool enabled);

        /**
         * @brief Get current configuration
         */
        const TerrainRenderConfig& GetConfig() const { return m_Config; }

        // ====================================================================
        // Rendering
        // ====================================================================

        /**
         * @brief Render a single terrain chunk
         * @param chunk Chunk to render
         * @param viewProjection View-projection matrix
         */
        void RenderChunk(const Chunk& chunk, const DirectX::XMMATRIX& viewProjection);

        /**
         * @brief Render all terrain from chunk manager
         * @param chunkManager ChunkManager containing terrain data
         * @param viewProjection View-projection matrix
         * @param cameraPosition Camera world position
         */
        void RenderTerrain(ChunkManager& chunkManager,
                           const DirectX::XMMATRIX& viewProjection,
                           const DirectX::XMFLOAT3& cameraPosition);

        /**
         * @brief Begin terrain rendering pass
         *
         * Sets up shaders, root signature, and pipeline state.
         * Must be called before RenderChunk/RenderTerrain.
         */
        void BeginTerrainPass();

        /**
         * @brief End terrain rendering pass
         */
        void EndTerrainPass();

        /**
         * @brief Update per-frame constants
         * @param viewProjection View-projection matrix
         * @param cameraPosition Camera world position
         * @param time Total elapsed time
         */
        void UpdateFrameConstants(const DirectX::XMMATRIX& viewProjection,
                                  const DirectX::XMFLOAT3& cameraPosition,
                                  float time);

        // ====================================================================
        // Statistics
        // ====================================================================

        /**
         * @brief Get number of chunks rendered last frame
         */
        uint32_t GetRenderedChunkCount() const { return m_RenderedChunkCount; }

        /**
         * @brief Get total triangles rendered last frame
         */
        uint32_t GetRenderedTriangleCount() const { return m_RenderedTriangleCount; }

        /**
         * @brief Reset rendering statistics
         */
        void ResetStats();

    private:
        /**
         * @brief Create terrain shaders
         */
        bool CreateShaders();

        /**
         * @brief Create terrain root signature
         */
        bool CreateRootSignature();

        /**
         * @brief Create terrain pipeline state
         */
        bool CreatePipelineState();

        /**
         * @brief Create constant buffers
         */
        bool CreateConstantBuffers();

    private:
        // Initialization state
        bool m_Initialized = false;

        // Renderer reference
        SM::Renderer* m_Renderer = nullptr;
        SM::DX12Core* m_Core = nullptr;

        // Configuration
        TerrainRenderConfig m_Config;

        // Shaders
        SM::ShaderBytecode m_VertexShader;
        SM::ShaderBytecode m_PixelShader;

        // Pipeline resources
        SM::RootSignature m_RootSignature;
        SM::GraphicsPipelineState m_TerrainPSO;
        SM::GraphicsPipelineState m_WireframePSO;

        // Constant buffers
        std::unique_ptr<SM::ConstantBuffer> m_PerFrameCB;
        std::unique_ptr<SM::ConstantBuffer> m_PerChunkCB;

        // Frame data
        TerrainPerFrameData m_FrameData;

        // Statistics
        uint32_t m_RenderedChunkCount = 0;
        uint32_t m_RenderedTriangleCount = 0;

        // Light direction (could be made configurable)
        DirectX::XMFLOAT3 m_LightDirection = { 0.5f, -0.8f, 0.3f };
    };

} // namespace PCG
