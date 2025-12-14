/**
 * @file TerrainPixel.hlsl
 * @brief Terrain pixel shader with height-based coloring and lighting
 *
 * Implements:
 * - Height-based color blending (water, sand, grass, rock, snow)
 * - Slope-based texture blending
 * - Simple directional lighting
 * - Distance fog
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
// Input Structure
// ============================================================================

struct PS_INPUT
{
    float4 Position       : SV_POSITION;
    float3 WorldPosition  : TEXCOORD0;
    float3 WorldNormal    : TEXCOORD1;
    float2 TexCoord       : TEXCOORD2;
    float4 VertexColor    : COLOR0;
    float Height          : TEXCOORD3;
    float FogFactor       : TEXCOORD4;
};

// ============================================================================
// Terrain Colors
// ============================================================================

// Define terrain layer colors
static const float3 DeepWaterColor = float3(0.02f, 0.08f, 0.25f);
static const float3 ShallowWaterColor = float3(0.10f, 0.30f, 0.50f);
static const float3 SandColor = float3(0.76f, 0.70f, 0.50f);
static const float3 GrassColor = float3(0.20f, 0.45f, 0.12f);
static const float3 ForestColor = float3(0.12f, 0.35f, 0.10f);
static const float3 RockColor = float3(0.45f, 0.42f, 0.40f);
static const float3 SnowColor = float3(0.95f, 0.95f, 0.98f);

// Height thresholds (normalized 0-1)
static const float WaterLevel = 0.15f;
static const float ShallowWaterLevel = 0.20f;
static const float SandLevel = 0.25f;
static const float GrassLevel = 0.50f;
static const float ForestLevel = 0.65f;
static const float RockLevel = 0.80f;
static const float SnowLevel = 0.90f;

// ============================================================================
// Lighting Constants
// ============================================================================

static const float3 AmbientColor = float3(0.20f, 0.22f, 0.28f);
static const float3 SunColor = float3(1.0f, 0.95f, 0.85f);
static const float SunIntensity = 1.2f;

// ============================================================================
// Helper Functions
// ============================================================================

// Smooth interpolation for terrain blending
float SmoothBlend(float value, float minVal, float maxVal)
{
    float t = saturate((value - minVal) / (maxVal - minVal));
    return t * t * (3.0f - 2.0f * t); // Smoothstep
}

// Calculate terrain color based on height
float3 GetHeightBasedColor(float height)
{
    float3 color = DeepWaterColor;

    if (height < WaterLevel)
    {
        // Deep water
        color = DeepWaterColor;
    }
    else if (height < ShallowWaterLevel)
    {
        // Shallow water blend
        float t = SmoothBlend(height, WaterLevel, ShallowWaterLevel);
        color = lerp(DeepWaterColor, ShallowWaterColor, t);
    }
    else if (height < SandLevel)
    {
        // Sand blend
        float t = SmoothBlend(height, ShallowWaterLevel, SandLevel);
        color = lerp(ShallowWaterColor, SandColor, t);
    }
    else if (height < GrassLevel)
    {
        // Grass blend
        float t = SmoothBlend(height, SandLevel, GrassLevel);
        color = lerp(SandColor, GrassColor, t);
    }
    else if (height < ForestLevel)
    {
        // Forest blend
        float t = SmoothBlend(height, GrassLevel, ForestLevel);
        color = lerp(GrassColor, ForestColor, t);
    }
    else if (height < RockLevel)
    {
        // Rock blend
        float t = SmoothBlend(height, ForestLevel, RockLevel);
        color = lerp(ForestColor, RockColor, t);
    }
    else if (height < SnowLevel)
    {
        // Snow blend
        float t = SmoothBlend(height, RockLevel, SnowLevel);
        color = lerp(RockColor, SnowColor, t);
    }
    else
    {
        // Full snow
        color = SnowColor;
    }

    return color;
}

// Calculate slope factor (0 = flat, 1 = vertical)
float GetSlopeFactor(float3 normal)
{
    return 1.0f - saturate(normal.y);
}

// Blend rock color on steep slopes
float3 ApplySlopeBlending(float3 baseColor, float3 normal, float height)
{
    float slope = GetSlopeFactor(normal);

    // Cliffs should show rock regardless of height
    float cliffThreshold = 0.4f;
    float cliffBlend = smoothstep(cliffThreshold, cliffThreshold + 0.2f, slope);

    // Don't apply cliff rock to snow areas as strongly
    float snowProtection = smoothstep(RockLevel, SnowLevel, height);
    cliffBlend *= (1.0f - snowProtection * 0.5f);

    return lerp(baseColor, RockColor, cliffBlend);
}

// Simple procedural detail variation
float GetDetailVariation(float2 texCoord)
{
    // Simple pseudo-random variation based on position
    float2 p = texCoord * 10.0f;
    float variation = frac(sin(dot(p, float2(12.9898f, 78.233f))) * 43758.5453f);
    return 0.9f + variation * 0.2f; // Range: 0.9 to 1.1
}

// Calculate diffuse lighting
float3 CalculateDiffuse(float3 normal, float3 lightDir, float3 lightColor, float intensity)
{
    float NdotL = max(dot(normal, -lightDir), 0.0f);
    return lightColor * NdotL * intensity;
}

// Calculate specular (Blinn-Phong) for wet surfaces
float3 CalculateSpecular(float3 normal, float3 lightDir, float3 viewDir, float wetness)
{
    if (wetness < 0.01f)
        return float3(0.0f, 0.0f, 0.0f);

    float3 halfDir = normalize(-lightDir + viewDir);
    float NdotH = max(dot(normal, halfDir), 0.0f);
    float shininess = 32.0f * wetness;
    float spec = pow(NdotH, shininess) * wetness;

    return SunColor * spec * 0.5f;
}

// ============================================================================
// Main Pixel Shader
// ============================================================================

float4 main(PS_INPUT input) : SV_TARGET
{
    // Normalize interpolated normal
    float3 normal = normalize(input.WorldNormal);

    // Calculate view direction
    float3 viewDir = normalize(CameraPosition - input.WorldPosition);

    // Get base terrain color from height
    float3 heightColor = GetHeightBasedColor(input.Height);

    // Apply slope-based rock blending
    float3 terrainColor = ApplySlopeBlending(heightColor, normal, input.Height);

    // Add subtle detail variation
    float detailVar = GetDetailVariation(input.TexCoord);
    terrainColor *= detailVar;

    // Optionally blend with vertex color (from CPU calculation)
    // This allows for smoother gradients from the mesh generation
    terrainColor = lerp(terrainColor, input.VertexColor.rgb, 0.3f);

    // Calculate lighting
    float3 diffuse = CalculateDiffuse(normal, LightDirection, SunColor, SunIntensity);

    // Add specular for water/wet areas
    float wetness = smoothstep(WaterLevel, ShallowWaterLevel, input.Height);
    wetness = 1.0f - wetness; // Invert so lower = more wet
    float3 specular = CalculateSpecular(normal, LightDirection, viewDir, wetness * 0.5f);

    // Combine lighting
    float3 ambient = AmbientColor;

    // Add simple ambient occlusion based on slope (crevices are darker)
    float ao = 1.0f - GetSlopeFactor(normal) * 0.3f;
    ambient *= ao;

    float3 litColor = terrainColor * (ambient + diffuse) + specular;

    // Apply fog
    float3 finalColor = lerp(litColor, FogColor.rgb, input.FogFactor);

    // Gamma correction (assuming sRGB output)
    finalColor = pow(abs(finalColor), 1.0f / 2.2f);

    return float4(finalColor, 1.0f);
}

// ============================================================================
// Alternative Entry Points
// ============================================================================

// Unlit terrain (for debugging)
float4 UnlitPS(PS_INPUT input) : SV_TARGET
{
    float3 terrainColor = GetHeightBasedColor(input.Height);
    terrainColor = ApplySlopeBlending(terrainColor, normalize(input.WorldNormal), input.Height);

    // Apply fog
    float3 finalColor = lerp(terrainColor, FogColor.rgb, input.FogFactor);

    return float4(finalColor, 1.0f);
}

// Height visualization (debug)
float4 DebugHeightPS(PS_INPUT input) : SV_TARGET
{
    // Visualize height as grayscale
    float h = input.Height;
    return float4(h, h, h, 1.0f);
}

// Normal visualization (debug)
float4 DebugNormalsPS(PS_INPUT input) : SV_TARGET
{
    float3 normal = normalize(input.WorldNormal);
    return float4(normal * 0.5f + 0.5f, 1.0f);
}

// Slope visualization (debug)
float4 DebugSlopePS(PS_INPUT input) : SV_TARGET
{
    float slope = GetSlopeFactor(normalize(input.WorldNormal));
    return float4(slope, 1.0f - slope, 0.0f, 1.0f);
}

// Vertex color passthrough (debug CPU-side coloring)
float4 VertexColorPS(PS_INPUT input) : SV_TARGET
{
    return input.VertexColor;
}

// Water depth visualization
float4 DebugWaterPS(PS_INPUT input) : SV_TARGET
{
    if (input.Height < ShallowWaterLevel)
    {
        // Water areas shown in blue
        float depth = 1.0f - (input.Height / ShallowWaterLevel);
        return float4(0.0f, 0.2f, 0.5f + depth * 0.5f, 1.0f);
    }
    else
    {
        // Land shown in gray
        return float4(0.5f, 0.5f, 0.5f, 1.0f);
    }
}
