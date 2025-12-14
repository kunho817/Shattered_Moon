#pragma once

/**
 * @file MaterialComponent.h
 * @brief Material component for entity rendering
 *
 * Contains material reference and rendering properties.
 */

#include <cstdint>
#include <string>

namespace SM
{
    /**
     * @brief Identifier for material assets
     */
    using MaterialID = std::uint32_t;

    /** Invalid material identifier */
    constexpr MaterialID INVALID_MATERIAL_ID = 0;

    /** Default material identifier */
    constexpr MaterialID DEFAULT_MATERIAL_ID = 1;

    // ============================================================================
    // Color Structure
    // ============================================================================

    /**
     * @brief RGBA color representation
     */
    struct Color
    {
        float r = 1.0f;
        float g = 1.0f;
        float b = 1.0f;
        float a = 1.0f;

        Color() = default;
        Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}

        // Predefined colors
        static Color White() { return Color(1.0f, 1.0f, 1.0f, 1.0f); }
        static Color Black() { return Color(0.0f, 0.0f, 0.0f, 1.0f); }
        static Color Red() { return Color(1.0f, 0.0f, 0.0f, 1.0f); }
        static Color Green() { return Color(0.0f, 1.0f, 0.0f, 1.0f); }
        static Color Blue() { return Color(0.0f, 0.0f, 1.0f, 1.0f); }
        static Color Yellow() { return Color(1.0f, 1.0f, 0.0f, 1.0f); }
        static Color Cyan() { return Color(0.0f, 1.0f, 1.0f, 1.0f); }
        static Color Magenta() { return Color(1.0f, 0.0f, 1.0f, 1.0f); }
        static Color Gray() { return Color(0.5f, 0.5f, 0.5f, 1.0f); }
        static Color Transparent() { return Color(0.0f, 0.0f, 0.0f, 0.0f); }

        /**
         * @brief Create color from hex value (0xRRGGBB or 0xRRGGBBAA)
         */
        static Color FromHex(std::uint32_t hex, bool hasAlpha = false)
        {
            if (hasAlpha)
            {
                return Color(
                    ((hex >> 24) & 0xFF) / 255.0f,
                    ((hex >> 16) & 0xFF) / 255.0f,
                    ((hex >> 8) & 0xFF) / 255.0f,
                    (hex & 0xFF) / 255.0f
                );
            }
            else
            {
                return Color(
                    ((hex >> 16) & 0xFF) / 255.0f,
                    ((hex >> 8) & 0xFF) / 255.0f,
                    (hex & 0xFF) / 255.0f,
                    1.0f
                );
            }
        }
    };

    // ============================================================================
    // MaterialComponent
    // ============================================================================

    /**
     * @brief Component for material/shader properties
     *
     * Associates an entity with material properties for rendering.
     * Can reference a material asset or use inline properties.
     */
    struct MaterialComponent
    {
        /** ID of the material resource */
        MaterialID MaterialId = DEFAULT_MATERIAL_ID;

        /** Base/albedo color (tint multiplied with texture) */
        Color BaseColor = Color::White();

        /** Emissive color (for glow effects) */
        Color EmissiveColor = Color::Black();

        /** Metallic factor (0 = dielectric, 1 = metal) */
        float Metallic = 0.0f;

        /** Roughness factor (0 = smooth, 1 = rough) */
        float Roughness = 0.5f;

        /** Ambient occlusion factor */
        float AmbientOcclusion = 1.0f;

        /** Emissive intensity multiplier */
        float EmissiveIntensity = 0.0f;

        /** Whether to use alpha blending */
        bool UseAlphaBlending = false;

        /** Whether this material is two-sided */
        bool TwoSided = false;

        /** Optional debug name */
        std::string DebugName;

        MaterialComponent() = default;

        explicit MaterialComponent(MaterialID materialId)
            : MaterialId(materialId)
        {
        }

        MaterialComponent(const Color& baseColor)
            : BaseColor(baseColor)
        {
        }

        MaterialComponent(MaterialID materialId, const Color& baseColor)
            : MaterialId(materialId)
            , BaseColor(baseColor)
        {
        }

        /**
         * @brief Check if the material is valid
         */
        bool IsValid() const
        {
            return MaterialId != INVALID_MATERIAL_ID;
        }

        /**
         * @brief Check if the material has emissive properties
         */
        bool HasEmission() const
        {
            return EmissiveIntensity > 0.0f &&
                   (EmissiveColor.r > 0.0f || EmissiveColor.g > 0.0f || EmissiveColor.b > 0.0f);
        }
    };

    // ============================================================================
    // Material Presets
    // ============================================================================

    namespace MaterialPresets
    {
        inline MaterialComponent Metal(const Color& color = Color::Gray())
        {
            MaterialComponent mat;
            mat.BaseColor = color;
            mat.Metallic = 1.0f;
            mat.Roughness = 0.3f;
            return mat;
        }

        inline MaterialComponent Plastic(const Color& color = Color::White())
        {
            MaterialComponent mat;
            mat.BaseColor = color;
            mat.Metallic = 0.0f;
            mat.Roughness = 0.4f;
            return mat;
        }

        inline MaterialComponent Rubber(const Color& color = Color::Black())
        {
            MaterialComponent mat;
            mat.BaseColor = color;
            mat.Metallic = 0.0f;
            mat.Roughness = 0.9f;
            return mat;
        }

        inline MaterialComponent Emissive(const Color& color, float intensity = 1.0f)
        {
            MaterialComponent mat;
            mat.BaseColor = Color::Black();
            mat.EmissiveColor = color;
            mat.EmissiveIntensity = intensity;
            return mat;
        }
    }

} // namespace SM
