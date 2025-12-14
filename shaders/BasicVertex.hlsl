/**
 * @file BasicVertex.hlsl
 * @brief Basic vertex shader with standard transformations
 *
 * Transforms vertices from object space to clip space and passes
 * data to the pixel shader for lighting calculations.
 */

// ============================================================================
// Constant Buffers
// ============================================================================

// Per-frame constants (b0)
cbuffer PerFrame : register(b0)
{
    float4x4 View;
    float4x4 Projection;
    float4x4 ViewProjection;
    float3 CameraPosition;
    float Time;
};

// Per-object constants (b1)
cbuffer PerObject : register(b1)
{
    float4x4 World;
    float4x4 WorldInverseTranspose;
};

// ============================================================================
// Input/Output Structures
// ============================================================================

struct VS_INPUT
{
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD;
    float4 Color    : COLOR;
};

struct VS_OUTPUT
{
    float4 Position      : SV_POSITION;  // Clip space position
    float3 WorldPosition : TEXCOORD0;    // World space position (for lighting)
    float3 WorldNormal   : TEXCOORD1;    // World space normal (for lighting)
    float2 TexCoord      : TEXCOORD2;    // Texture coordinates
    float4 Color         : COLOR;        // Vertex color
};

// ============================================================================
// Main Vertex Shader
// ============================================================================

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    // Transform position to world space
    float4 worldPosition = mul(float4(input.Position, 1.0f), World);
    output.WorldPosition = worldPosition.xyz;

    // Transform position to clip space
    output.Position = mul(worldPosition, ViewProjection);

    // Transform normal to world space (use inverse transpose for correct normal transformation)
    output.WorldNormal = normalize(mul(input.Normal, (float3x3)WorldInverseTranspose));

    // Pass through texture coordinates
    output.TexCoord = input.TexCoord;

    // Pass through vertex color
    output.Color = input.Color;

    return output;
}

// ============================================================================
// Alternative Entry Points
// ============================================================================

// Simple vertex shader for position-only rendering (shadows, depth-only pass)
float4 PositionOnlyVS(float3 Position : POSITION) : SV_POSITION
{
    float4 worldPosition = mul(float4(Position, 1.0f), World);
    return mul(worldPosition, ViewProjection);
}

// Vertex shader for full-screen quad (post-processing)
struct FullscreenVS_Output
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
};

FullscreenVS_Output FullscreenVS(uint VertexID : SV_VertexID)
{
    FullscreenVS_Output output;

    // Generate fullscreen triangle from vertex ID
    // This creates a triangle that covers the entire screen
    // VertexID 0: (-1, -1), VertexID 1: (3, -1), VertexID 2: (-1, 3)
    output.TexCoord = float2((VertexID << 1) & 2, VertexID & 2);
    output.Position = float4(output.TexCoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);

    return output;
}
