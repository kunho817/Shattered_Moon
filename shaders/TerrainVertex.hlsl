/**
 * @file TerrainVertex.hlsl
 * @brief Terrain vertex shader with height-based transformations
 *
 * Transforms terrain vertices and passes height/normal data
 * to the pixel shader for terrain-specific rendering.
 */

// ============================================================================
// Constant Buffers
// ============================================================================

// Per-frame constants (b0)
cbuffer PerFrame : register(b0)
{
    float4x4 ViewProjection;
    float3 CameraPosition;
    float HeightScale;
    float3 LightDirection;
    float Time;
    float4 FogColor;
    float FogStart;
    float FogEnd;
    float2 Padding;
};

// Per-chunk constants (b1)
cbuffer PerChunk : register(b1)
{
    float4x4 World;
    float2 ChunkOffset;
    float TextureScale;
    float MinHeight;
    float MaxHeight;
    float3 ChunkPadding;
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
    float4 Position       : SV_POSITION;  // Clip space position
    float3 WorldPosition  : TEXCOORD0;    // World space position
    float3 WorldNormal    : TEXCOORD1;    // World space normal
    float2 TexCoord       : TEXCOORD2;    // Texture coordinates
    float4 VertexColor    : COLOR0;       // Vertex color (height-based)
    float Height          : TEXCOORD3;    // Normalized height [0, 1]
    float FogFactor       : TEXCOORD4;    // Fog interpolation factor
};

// ============================================================================
// Main Vertex Shader
// ============================================================================

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    // The vertex position is already in world space from chunk generation
    float4 worldPos = float4(input.Position, 1.0f);
    output.WorldPosition = worldPos.xyz;

    // Transform to clip space
    output.Position = mul(worldPos, ViewProjection);

    // Normal is already in world space
    output.WorldNormal = normalize(input.Normal);

    // Pass through texture coordinates (scaled for tiling)
    output.TexCoord = input.TexCoord * TextureScale;

    // Pass through vertex color (contains height-based coloring from CPU)
    output.VertexColor = input.Color;

    // Calculate normalized height for shader use
    float heightRange = MaxHeight - MinHeight;
    if (heightRange > 0.001f)
    {
        output.Height = saturate((input.Position.y - MinHeight) / heightRange);
    }
    else
    {
        output.Height = 0.5f;
    }

    // Calculate fog factor based on distance from camera
    float distToCamera = length(CameraPosition - worldPos.xyz);
    output.FogFactor = saturate((distToCamera - FogStart) / (FogEnd - FogStart));

    return output;
}

// ============================================================================
// Alternative Entry Points
// ============================================================================

// Simple depth-only vertex shader (for shadow mapping)
float4 DepthOnlyVS(VS_INPUT input) : SV_POSITION
{
    float4 worldPos = float4(input.Position, 1.0f);
    return mul(worldPos, ViewProjection);
}

// Vertex shader with wind animation (for grass/vegetation on terrain)
VS_OUTPUT WindAnimatedVS(VS_INPUT input)
{
    VS_OUTPUT output;

    // Base world position
    float4 worldPos = float4(input.Position, 1.0f);

    // Simple wind animation based on height and time
    float windStrength = 0.1f;
    float windFrequency = 2.0f;
    float heightFactor = saturate((input.Position.y - MinHeight) / max(MaxHeight - MinHeight, 0.001f));

    // Wind offset (only affects vertices above a certain height)
    float windOffset = sin(Time * windFrequency + worldPos.x * 0.5f + worldPos.z * 0.3f) * windStrength;
    worldPos.x += windOffset * heightFactor * heightFactor;
    worldPos.z += windOffset * 0.5f * heightFactor * heightFactor;

    output.WorldPosition = worldPos.xyz;
    output.Position = mul(worldPos, ViewProjection);
    output.WorldNormal = normalize(input.Normal);
    output.TexCoord = input.TexCoord * TextureScale;
    output.VertexColor = input.Color;

    float heightRange = MaxHeight - MinHeight;
    output.Height = heightRange > 0.001f ? saturate((input.Position.y - MinHeight) / heightRange) : 0.5f;

    float distToCamera = length(CameraPosition - worldPos.xyz);
    output.FogFactor = saturate((distToCamera - FogStart) / (FogEnd - FogStart));

    return output;
}
