/**
 * @file BasicPixel.hlsl
 * @brief Basic pixel shader with directional lighting
 *
 * Implements simple Lambertian diffuse lighting with ambient term
 * and optional texture sampling.
 */

// ============================================================================
// Constant Buffers
// ============================================================================

// Per-frame constants (b0) - shared with vertex shader
cbuffer PerFrame : register(b0)
{
    float4x4 View;
    float4x4 Projection;
    float4x4 ViewProjection;
    float3 CameraPosition;
    float Time;
};

// Material constants (b2)
cbuffer Material : register(b2)
{
    float4 BaseColor;       // Base/albedo color
    float4 EmissiveColor;   // Emissive color
    float Metallic;         // Metallic factor (0-1)
    float Roughness;        // Roughness factor (0-1)
    float AmbientOcclusion; // AO factor
    float EmissiveIntensity;
    float2 UVScale;         // Texture tiling
    float2 UVOffset;        // Texture offset
};

// ============================================================================
// Lighting Constants
// ============================================================================

// Simple directional light
static const float3 LightDirection = normalize(float3(0.5f, -0.8f, 0.3f));
static const float3 LightColor = float3(1.0f, 0.98f, 0.95f);
static const float LightIntensity = 1.2f;

// Ambient light
static const float3 AmbientColor = float3(0.15f, 0.18f, 0.25f);

// ============================================================================
// Textures and Samplers
// ============================================================================

Texture2D<float4> BaseTexture : register(t0);
SamplerState LinearSampler : register(s0);

// ============================================================================
// Input Structure
// ============================================================================

struct PS_INPUT
{
    float4 Position      : SV_POSITION;
    float3 WorldPosition : TEXCOORD0;
    float3 WorldNormal   : TEXCOORD1;
    float2 TexCoord      : TEXCOORD2;
    float4 Color         : COLOR;
};

// ============================================================================
// Lighting Functions
// ============================================================================

// Simple Lambertian diffuse
float3 CalculateDiffuse(float3 normal, float3 lightDir, float3 lightColor, float intensity)
{
    float NdotL = max(dot(normal, -lightDir), 0.0f);
    return lightColor * NdotL * intensity;
}

// Simple specular (Blinn-Phong)
float3 CalculateSpecular(float3 normal, float3 lightDir, float3 viewDir, float3 lightColor, float roughness)
{
    float3 halfDir = normalize(-lightDir + viewDir);
    float NdotH = max(dot(normal, halfDir), 0.0f);

    // Convert roughness to shininess (simple approximation)
    float shininess = max(2.0f / (roughness * roughness) - 2.0f, 1.0f);
    shininess = min(shininess, 256.0f);

    float specPower = pow(NdotH, shininess);
    return lightColor * specPower * (1.0f - roughness);
}

// Fresnel approximation (Schlick)
float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
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

    // Sample texture (if available) and apply UV transform
    float2 uv = input.TexCoord * UVScale + UVOffset;
    float4 texColor = BaseTexture.Sample(LinearSampler, uv);

    // Combine base color with texture and vertex color
    float4 albedo = BaseColor * texColor * input.Color;

    // Calculate diffuse lighting
    float3 diffuse = CalculateDiffuse(normal, LightDirection, LightColor, LightIntensity);

    // Calculate specular lighting
    float3 specular = CalculateSpecular(normal, LightDirection, viewDir, LightColor, Roughness);

    // Apply metallic influence to specular
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo.rgb, Metallic);
    float NdotV = max(dot(normal, viewDir), 0.0f);
    float3 fresnel = FresnelSchlick(NdotV, F0);
    specular *= fresnel;

    // Calculate ambient
    float3 ambient = AmbientColor * AmbientOcclusion;

    // Combine lighting
    float3 finalColor = albedo.rgb * (ambient + diffuse) + specular;

    // Add emissive
    finalColor += EmissiveColor.rgb * EmissiveIntensity;

    // Apply simple tone mapping (Reinhard)
    finalColor = finalColor / (finalColor + 1.0f);

    // Gamma correction (assuming sRGB output)
    finalColor = pow(finalColor, 1.0f / 2.2f);

    return float4(finalColor, albedo.a);
}

// ============================================================================
// Alternative Entry Points
// ============================================================================

// Unlit pixel shader (for debug/UI)
float4 UnlitPS(PS_INPUT input) : SV_TARGET
{
    float2 uv = input.TexCoord * UVScale + UVOffset;
    float4 texColor = BaseTexture.Sample(LinearSampler, uv);
    return BaseColor * texColor * input.Color;
}

// Solid color output (for depth-only passes that write color)
float4 SolidColorPS(PS_INPUT input) : SV_TARGET
{
    return BaseColor;
}

// Normal visualization (debug)
float4 DebugNormalsPS(PS_INPUT input) : SV_TARGET
{
    float3 normal = normalize(input.WorldNormal);
    return float4(normal * 0.5f + 0.5f, 1.0f);
}

// UV visualization (debug)
float4 DebugUVsPS(PS_INPUT input) : SV_TARGET
{
    return float4(input.TexCoord, 0.0f, 1.0f);
}

// Depth visualization (debug)
float4 DebugDepthPS(PS_INPUT input) : SV_TARGET
{
    float depth = input.Position.z;
    return float4(depth, depth, depth, 1.0f);
}
