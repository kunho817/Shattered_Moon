#pragma once

/**
 * @file PipelineState.h
 * @brief DirectX 12 Pipeline State Object builder
 *
 * Provides a fluent interface for building graphics pipeline state objects
 * with proper defaults and common configurations.
 */

#include "renderer/DX12Core.h"

#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <vector>
#include <string>

namespace SM
{
    using Microsoft::WRL::ComPtr;

    // Forward declarations
    class DX12Core;
    class RootSignature;

    /**
     * @brief Shader bytecode wrapper
     */
    struct ShaderBytecode
    {
        ComPtr<ID3DBlob> Blob;
        D3D12_SHADER_BYTECODE GetBytecode() const
        {
            return { Blob ? Blob->GetBufferPointer() : nullptr,
                     Blob ? Blob->GetBufferSize() : 0 };
        }

        bool IsValid() const { return Blob != nullptr; }
    };

    /**
     * @brief Compile shader from file
     * @param filename Path to shader file
     * @param entryPoint Entry point function name
     * @param target Shader target (e.g., "vs_5_1", "ps_5_1")
     * @param bytecode Output shader bytecode
     * @return true if successful
     */
    bool CompileShaderFromFile(
        const std::wstring& filename,
        const char* entryPoint,
        const char* target,
        ShaderBytecode& bytecode
    );

    /**
     * @brief Compile shader from string
     * @param source Shader source code
     * @param entryPoint Entry point function name
     * @param target Shader target
     * @param bytecode Output shader bytecode
     * @return true if successful
     */
    bool CompileShaderFromString(
        const std::string& source,
        const char* entryPoint,
        const char* target,
        ShaderBytecode& bytecode
    );

    /**
     * @brief Load precompiled shader from file
     * @param filename Path to compiled shader (.cso)
     * @param bytecode Output shader bytecode
     * @return true if successful
     */
    bool LoadCompiledShader(const std::wstring& filename, ShaderBytecode& bytecode);

    /**
     * @brief Input element semantic names
     */
    namespace SemanticName
    {
        constexpr const char* Position = "POSITION";
        constexpr const char* Normal = "NORMAL";
        constexpr const char* TexCoord = "TEXCOORD";
        constexpr const char* Color = "COLOR";
        constexpr const char* Tangent = "TANGENT";
        constexpr const char* Binormal = "BINORMAL";
        constexpr const char* BlendIndices = "BLENDINDICES";
        constexpr const char* BlendWeight = "BLENDWEIGHT";
    }

    /**
     * @brief Common blend modes
     */
    enum class BlendMode
    {
        Opaque,
        AlphaBlend,
        Additive,
        Multiply,
        PremultipliedAlpha
    };

    /**
     * @brief Common cull modes
     */
    enum class CullMode
    {
        None = D3D12_CULL_MODE_NONE,
        Front = D3D12_CULL_MODE_FRONT,
        Back = D3D12_CULL_MODE_BACK
    };

    /**
     * @brief Common fill modes
     */
    enum class FillMode
    {
        Solid = D3D12_FILL_MODE_SOLID,
        Wireframe = D3D12_FILL_MODE_WIREFRAME
    };

    /**
     * @brief Common depth comparison functions
     */
    enum class DepthFunc
    {
        Less = D3D12_COMPARISON_FUNC_LESS,
        LessEqual = D3D12_COMPARISON_FUNC_LESS_EQUAL,
        Greater = D3D12_COMPARISON_FUNC_GREATER,
        GreaterEqual = D3D12_COMPARISON_FUNC_GREATER_EQUAL,
        Equal = D3D12_COMPARISON_FUNC_EQUAL,
        NotEqual = D3D12_COMPARISON_FUNC_NOT_EQUAL,
        Always = D3D12_COMPARISON_FUNC_ALWAYS,
        Never = D3D12_COMPARISON_FUNC_NEVER
    };

    /**
     * @brief Graphics Pipeline State builder
     *
     * Provides a fluent interface for building D3D12 graphics PSOs.
     */
    class GraphicsPipelineState
    {
    public:
        GraphicsPipelineState() = default;
        ~GraphicsPipelineState() = default;

        // Prevent copying
        GraphicsPipelineState(const GraphicsPipelineState&) = delete;
        GraphicsPipelineState& operator=(const GraphicsPipelineState&) = delete;

        // Allow moving
        GraphicsPipelineState(GraphicsPipelineState&&) noexcept = default;
        GraphicsPipelineState& operator=(GraphicsPipelineState&&) noexcept = default;

        /**
         * @brief Begin building a new pipeline state
         * @return Reference to this builder
         */
        GraphicsPipelineState& Begin();

        /**
         * @brief Set root signature
         * @param rootSignature Root signature to use
         * @return Reference to this builder
         */
        GraphicsPipelineState& SetRootSignature(ID3D12RootSignature* rootSignature);

        /**
         * @brief Set root signature from wrapper
         */
        GraphicsPipelineState& SetRootSignature(const RootSignature& rootSignature);

        /**
         * @brief Set vertex shader
         * @param bytecode Compiled vertex shader
         * @return Reference to this builder
         */
        GraphicsPipelineState& SetVertexShader(const ShaderBytecode& bytecode);

        /**
         * @brief Set pixel shader
         * @param bytecode Compiled pixel shader
         * @return Reference to this builder
         */
        GraphicsPipelineState& SetPixelShader(const ShaderBytecode& bytecode);

        /**
         * @brief Set geometry shader (optional)
         * @param bytecode Compiled geometry shader
         * @return Reference to this builder
         */
        GraphicsPipelineState& SetGeometryShader(const ShaderBytecode& bytecode);

        /**
         * @brief Set hull shader (optional)
         * @param bytecode Compiled hull shader
         * @return Reference to this builder
         */
        GraphicsPipelineState& SetHullShader(const ShaderBytecode& bytecode);

        /**
         * @brief Set domain shader (optional)
         * @param bytecode Compiled domain shader
         * @return Reference to this builder
         */
        GraphicsPipelineState& SetDomainShader(const ShaderBytecode& bytecode);

        /**
         * @brief Add input element to the input layout
         * @param semanticName Semantic name (e.g., "POSITION")
         * @param semanticIndex Semantic index
         * @param format DXGI format
         * @param inputSlot Input slot
         * @param alignedByteOffset Offset in bytes
         * @param inputSlotClass Per-vertex or per-instance
         * @param instanceDataStepRate Instance step rate
         * @return Reference to this builder
         */
        GraphicsPipelineState& AddInputElement(
            const char* semanticName,
            uint32_t semanticIndex,
            DXGI_FORMAT format,
            uint32_t inputSlot = 0,
            uint32_t alignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION inputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            uint32_t instanceDataStepRate = 0
        );

        /**
         * @brief Set standard vertex input layout (Position, Normal, TexCoord, Color)
         * @return Reference to this builder
         */
        GraphicsPipelineState& SetStandardInputLayout();

        /**
         * @brief Set position-only input layout
         * @return Reference to this builder
         */
        GraphicsPipelineState& SetPositionOnlyInputLayout();

        /**
         * @brief Set rasterizer state
         * @param fillMode Fill mode
         * @param cullMode Cull mode
         * @param frontCounterClockwise Front face winding
         * @return Reference to this builder
         */
        GraphicsPipelineState& SetRasterizer(
            FillMode fillMode = FillMode::Solid,
            CullMode cullMode = CullMode::Back,
            bool frontCounterClockwise = false
        );

        /**
         * @brief Enable/disable depth clipping
         */
        GraphicsPipelineState& SetDepthClip(bool enable);

        /**
         * @brief Set depth bias for shadow mapping
         */
        GraphicsPipelineState& SetDepthBias(int32_t depthBias, float depthBiasClamp, float slopeScaledDepthBias);

        /**
         * @brief Set blend state using preset
         * @param mode Blend mode preset
         * @param rtIndex Render target index
         * @return Reference to this builder
         */
        GraphicsPipelineState& SetBlendMode(BlendMode mode, uint32_t rtIndex = 0);

        /**
         * @brief Set custom blend state for a render target
         */
        GraphicsPipelineState& SetBlendState(
            uint32_t rtIndex,
            bool blendEnable,
            D3D12_BLEND srcBlend,
            D3D12_BLEND destBlend,
            D3D12_BLEND_OP blendOp,
            D3D12_BLEND srcBlendAlpha,
            D3D12_BLEND destBlendAlpha,
            D3D12_BLEND_OP blendOpAlpha,
            uint8_t writeMask = D3D12_COLOR_WRITE_ENABLE_ALL
        );

        /**
         * @brief Set depth stencil state
         * @param depthEnable Enable depth testing
         * @param depthWrite Enable depth writing
         * @param depthFunc Depth comparison function
         * @return Reference to this builder
         */
        GraphicsPipelineState& SetDepthStencil(
            bool depthEnable = true,
            bool depthWrite = true,
            DepthFunc depthFunc = DepthFunc::Less
        );

        /**
         * @brief Disable depth testing
         */
        GraphicsPipelineState& DisableDepth();

        /**
         * @brief Set primitive topology type
         * @param type Topology type
         * @return Reference to this builder
         */
        GraphicsPipelineState& SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE type);

        /**
         * @brief Set render target formats
         * @param numRenderTargets Number of render targets
         * @param formats Array of formats
         * @return Reference to this builder
         */
        GraphicsPipelineState& SetRenderTargetFormats(
            uint32_t numRenderTargets,
            const DXGI_FORMAT* formats
        );

        /**
         * @brief Set single render target format
         * @param format Render target format
         * @return Reference to this builder
         */
        GraphicsPipelineState& SetRenderTargetFormat(DXGI_FORMAT format);

        /**
         * @brief Set depth stencil format
         * @param format Depth stencil format
         * @return Reference to this builder
         */
        GraphicsPipelineState& SetDepthStencilFormat(DXGI_FORMAT format);

        /**
         * @brief Set sample description
         * @param count Sample count
         * @param quality Sample quality
         * @return Reference to this builder
         */
        GraphicsPipelineState& SetSampleDesc(uint32_t count, uint32_t quality = 0);

        /**
         * @brief Build and finalize the pipeline state
         * @param core DX12 core reference
         * @return true if successful
         */
        bool Build(DX12Core* core);

        /**
         * @brief Get the native pipeline state object
         */
        ID3D12PipelineState* GetNative() const { return m_PipelineState.Get(); }

        /**
         * @brief Check if PSO is valid
         */
        bool IsValid() const { return m_PipelineState != nullptr; }

    private:
        ComPtr<ID3D12PipelineState> m_PipelineState;
        D3D12_GRAPHICS_PIPELINE_STATE_DESC m_Desc = {};

        // Store input elements (must persist until Build)
        std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputElements;
        std::vector<std::string> m_SemanticNames;  // Store names to keep pointers valid
    };

    /**
     * @brief Create a basic opaque pipeline state
     * @param core DX12 core reference
     * @param rootSignature Root signature to use
     * @param vs Vertex shader bytecode
     * @param ps Pixel shader bytecode
     * @param pso Output pipeline state
     * @return true if successful
     */
    bool CreateBasicPipelineState(
        DX12Core* core,
        ID3D12RootSignature* rootSignature,
        const ShaderBytecode& vs,
        const ShaderBytecode& ps,
        GraphicsPipelineState& pso
    );

} // namespace SM
