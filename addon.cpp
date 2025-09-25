#include <array>
#include <atomic>
#include <charconv>
#include <tuple>
#include <type_traits>
#include <utility>

#include <imgui.h>
#include <reshade.hpp>

#include <easylogging++.h>

#include "addon.h"
#include "eternal.h"
#include "fields.h"
#include "hook.h"

INITIALIZE_EASYLOGGINGPP

// =========================
// Constants / config
// =========================
static constexpr const char *kSection = "untitled";
static constexpr const char *kOverlay = "untitled"; // registered overlay window title
static bool gEnabled = false;                       // runtime-loaded: [untitled] Enabled=0/1

// =========================
// Tiny compile-time string (structural NTTP) and utilities
// =========================
template <std::size_t N> struct ct_string {
    char v[N]{};
    constexpr ct_string() = default;
    // constexpr ctor from literal
    constexpr ct_string(const char (&s)[N]) {
        for (std::size_t i = 0; i < N; ++i)
            v[i] = s[i];
    }
    static consteval std::size_t size() { return N; }
    static consteval std::size_t len() { return N ? N - 1 : 0; } // excluding trailing '\0'
    constexpr const char *c_str() const { return v; }
    // structural equality by member-wise comparison is implicit
};

// concat two ct_strings -> new ct_string
template <ct_string A, ct_string B> consteval auto ct_concat() {
    constexpr std::size_t NA = A.size();
    constexpr std::size_t NB = B.size();
    ct_string<NA + NB - 1> out{};
    // copy A (including its '\0')
    for (std::size_t i = 0; i < NA; ++i)
        out.v[i] = A.v[i];
    // overwrite '\0' and append B (including its '\0')
    for (std::size_t j = 0; j < NB; ++j)
        out.v[NA - 1 + j] = B.v[j];
    return out;
}

// suffix constants we need
static constexpr auto Suffix_Enabled = ct_string{".Enabled"};
static constexpr auto Suffix_Value = ct_string{".Value"};

// =========================
// Schema node tags (types only; no runtime storage)
// =========================
template <ct_string Title> struct Group {
    static constexpr auto title = Title;
};

template <ct_string Key, ct_string Info, typename T> struct ItemFree {
    using value_type = T;
    static_assert(std::is_same_v<T, int> || std::is_same_v<T, float>);
    static constexpr auto key = Key;
    static constexpr auto info = Info;
    static constexpr auto key_enabled = ct_concat<Key, Suffix_Enabled>();
    static constexpr auto key_value = ct_concat<Key, Suffix_Value>();
};

template <ct_string Key, ct_string Info, typename T, T Min, T Max> struct ItemRanged : ItemFree<Key, Info, T> {
    static constexpr T min = Min;
    static constexpr T max = Max;
};

template <typename T>
concept IsGroup = requires { T::title; };

template <typename T>
concept IsItem = requires { typename T::value_type; };

template <typename T>
concept IsRangedItem = IsItem<T> && requires {
    T::min;
    T::max;
};

// =========================
// Compile-time utilities over the tuple of types
// =========================
template <typename... Ts, typename F, std::size_t... I>
constexpr void for_each_type_impl(std::tuple<Ts...> *, F &&f, std::index_sequence<I...>) {
    // Expand f.template operator()<T>()... at compile-time
    (f.template operator()<std::tuple_element_t<I, std::tuple<Ts...>>>(), ...);
}

template <typename Tuple, typename F> constexpr void for_each_type(F &&f) {
    constexpr std::size_t N = std::tuple_size_v<Tuple>;
    for_each_type_impl(static_cast<Tuple *>(nullptr), std::forward<F>(f), std::make_index_sequence<N>{});
}

// Count number of items at compile time
template <typename Tuple> consteval std::size_t count_items() {
    std::size_t n = 0;
    for_each_type<Tuple>([&]<typename Node>() {
        if constexpr (IsItem<Node>)
            ++n;
    });
    return n;
}

// =========================
// Gui schema (compile-time)
// Generated from PPS_FIELDS (addon.h) and overrides_grouped.md; order matches groups then fields therein
// =========================
using Schema = std::tuple<
    Group<"Ambient Cubemap">,
    ItemRanged<"AmbientCubemapIntensity", "To scale the Ambient cubemap brightness >=0: off, 1(default), >1 brighter",
               float, 0.0f, 4.0f>,

    Group<"Ambient Occlusion">,
    ItemRanged<"AmbientOcclusionBias",
               "in unreal units, default (3.0) works well for flat surfaces but can reduce details", float, 0.0f,
               10.0f>,
    ItemFree<"AmbientOcclusionDistance", "", float>,
    ItemRanged<"AmbientOcclusionFadeDistance",
               "in unreal units, at what distance the AO effect disppears in the distance (avoding artifacts and AO "
               "effects on huge object)",
               float, 0.0f, 20000.0f>,
    ItemRanged<"AmbientOcclusionFadeRadius",
               "in unreal units, how many units before AmbientOcclusionFadeOutDistance it starts fading out", float,
               0.0f, 20000.0f>,
    ItemRanged<"AmbientOcclusionIntensity",
               "0..1 0=off/no ambient occlusion .. 1=strong ambient occlusion, defines how much it affects the non "
               "direct lighting after base pass",
               float, 0.0f, 1.0f>,
    ItemRanged<"AmbientOcclusionMipBlend",
               "Affects the blend over the multiple mips (lower resolution versions) , 0:fully use full resolution, "
               "1::fully use low resolution, around 0.6 seems to be a good value",
               float, 0.1f, 1.0f>,
    ItemRanged<"AmbientOcclusionMipScale",
               "Affects the radius AO radius scale over the multiple mips (lower resolution versions)", float, 0.5f,
               4.0f>,
    ItemRanged<"AmbientOcclusionMipThreshold",
               "to tweak the bilateral upsampling when using multiple mips (lower resolution versions)", float, 0.0f,
               0.1f>,
    ItemRanged<"AmbientOcclusionPower",
               "in unreal units, bigger values means even distant surfaces affect the ambient occlusion", float, 0.1f,
               8.0f>,
    ItemRanged<"AmbientOcclusionQuality",
               "0=lowest quality..100=maximum quality, only a few quality levels are implemented, no soft transition",
               float, 0.0f, 100.0f>,
    ItemRanged<"AmbientOcclusionRadius",
               "in unreal units, bigger values means even distant surfaces affect the ambient occlusion", float, 0.1f,
               500.0f>,
    ItemRanged<"AmbientOcclusionRadiusInWS",
               "true: AO radius is in world space units, false: AO radius is locked the view space in 400 units", int,
               0, 1>,
    ItemRanged<"AmbientOcclusionStaticFraction",
               "0..1 0=no effect on static lighting .. 1=AO affects the stat lighting, 0 is free meaning no extra "
               "rendering pass",
               float, 0.0f, 1.0f>,
    ItemRanged<"AmbientOcclusionTemporalBlendWeight",
               "How much to blend the current frame with previous frames when using GTAO with temporal accumulation",
               float, 0.0f, 0.5f>,

    Group<"Auto Exposure">,
    ItemRanged<"AutoExposureApplyPhysicalCameraExposure",
               "Enables physical camera exposure using ShutterSpeed/ISO/Aperture.", int, 0, 1>,
    ItemRanged<"AutoExposureBias",
               "Logarithmic adjustment for the exposure. Only used if a tonemapper is specified. 0: no adjustment, "
               "-1:2x darker, -2:4x darker, 1:2x brighter, 2:4x brighter, ...",
               float, -15.0f, 15.0f>,
    ItemFree<"AutoExposureBiasBackup",
             "With the auto exposure changes, we are changing the AutoExposureBias inside the serialization code. We "
             "are storing that value before conversion here as a backup.",
             float>,
    ItemFree<"AutoExposureCalibrationConstant", "", float>,
    ItemRanged<"AutoExposureHighPercent",
               "The eye adaptation will adapt to a value extracted from the luminance histogram... good values 80..95",
               float, 0.0f, 100.0f>,
    ItemRanged<"AutoExposureLowPercent",
               "The eye adaptation will adapt to a value extracted from the luminance histogram... good values 70..80",
               float, 0.0f, 100.0f>,
    ItemRanged<"AutoExposureMaxBrightness",
               "Auto-Exposure maximum adaptation. Eye Adaptation is disabled if Min = Max.", float, -10.0f, 20.0f>,
    ItemRanged<"AutoExposureMethod", "Luminance computation method (AEM_Histogram=0, AEM_Basic=1, AEM_Manual=2)", int,
               0, 2>,
    ItemRanged<"AutoExposureMinBrightness",
               "Auto-Exposure minimum adaptation. Eye Adaptation is disabled if Min = Max.", float, -10.0f, 20.0f>,
    ItemRanged<"AutoExposureSpeedDown", "", float, 0.02f, 20.0f>,
    ItemRanged<"AutoExposureSpeedUp", "", float, 0.02f, 20.0f>,
    ItemRanged<"HistogramLogMax",
               "Histogram Max value. Expressed in Log2(Luminance) or in EV100 when using ExtendDefaultLuminanceRange",
               float, 0.0f, 16.0f>,
    ItemRanged<"HistogramLogMin",
               "Histogram Min value. Expressed in Log2(Luminance) or in EV100 when using ExtendDefaultLuminanceRange",
               float, -16.0f, 0.0f>,

    Group<"Bloom">, ItemRanged<"Bloom1Size", "Diameter size for Bloom1 in % screen width (1/2 res)", float, 0.0f, 4.0f>,
    ItemRanged<"Bloom2Size", "Diameter size for Bloom2 in % screen width (1/4 res)", float, 0.0f, 8.0f>,
    ItemRanged<"Bloom3Size", "Diameter size for Bloom3 in % screen width (1/8 res)", float, 0.0f, 16.0f>,
    ItemRanged<"Bloom4Size", "Diameter size for Bloom4 in % screen width (1/16 res)", float, 0.0f, 32.0f>,
    ItemRanged<"Bloom5Size", "Diameter size for Bloom5 in % screen width (1/32 res)", float, 0.0f, 64.0f>,
    ItemRanged<"Bloom6Size", "Diameter size for Bloom6 in % screen width (1/64 res)", float, 0.0f, 128.0f>,
    ItemRanged<"BloomConvolutionBufferScale", "Implicit buffer region as a fraction of the screen size to avoid wrap",
               float, 0.0f, 1.0f>,
    ItemFree<"BloomConvolutionPreFilterMax",
             "Boost intensity of select pixels prior to convolution (Min, Max, Multiplier). Max < Min disables", float>,
    ItemFree<"BloomConvolutionPreFilterMin",
             "Boost intensity of select pixels prior to convolution (Min, Max, Multiplier). Max < Min disables", float>,
    ItemFree<"BloomConvolutionPreFilterMult",
             "Boost intensity of select pixels prior to convolution (Min, Max, Multiplier). Max < Min disables", float>,
    ItemRanged<"BloomConvolutionScatterDispersion",
               "Intensity multiplier on the scatter dispersion energy of the kernel", float, 0.0f, 20.0f>,
    ItemRanged<"BloomConvolutionSize", "Relative size of the convolution kernel image vs viewport minor axis", float,
               0.0f, 1.0f>,
    ItemRanged<"BloomDirtMaskIntensity", "BloomDirtMask intensity", float, 0.0f, 8.0f>,
    ItemRanged<"BloomIntensity", "Multiplier for all bloom contributions", float, 0.0f, 8.0f>,
    ItemRanged<"BloomMethod", "Bloom algorithm (BM_SOG=0, BM_FFT=1)", int, 0, 1>,
    ItemRanged<"BloomSizeScale", "Scale for all bloom sizes", float, 0.0f, 64.0f>,
    ItemRanged<"BloomThreshold", "Minimum brightness where bloom starts to have effect", float, -1.0f, 8.0f>,

    Group<"Camera & White Balance">, ItemFree<"CameraISO", "The camera sensor sensitivity in ISO.", float>,
    ItemRanged<"CameraShutterSpeed", "The camera shutter in seconds.", float, 1.0f, 2000.0f>,
    ItemRanged<"TemperatureType", "WhiteBalance=0, ColorTemperature=1", int, 0, 1>,
    ItemRanged<"WhiteTemp", "Controls the color temperature (Kelvin) considered as white", float, 1500.0f, 15000.0f>,
    ItemRanged<"WhiteTint", "Magenta-Green axis tint (orthogonal to temperature)", float, -1.0f, 1.0f>,

    Group<"Chromatic Aberration">,
    ItemRanged<"ChromaticAberrationStartOffset", "Normalized distance to framebuffer center where effect takes place",
               float, 0.0f, 1.0f>,
    ItemRanged<"SceneFringeIntensity", "% chromatic aberration to simulate lens artifact", float, 0.0f, 5.0f>,

    Group<"Color Grading & Tone Mapping">,
    ItemRanged<"BlueCorrection", "Correct electric blues in ACEScg (bright blue desaturates)", float, 0.0f, 1.0f>,
    ItemRanged<"ColorCorrectionHighlightsMax", "Upper threshold for highlights; should be > HighlightsMin", float, 1.0f,
               10.0f>,
    ItemRanged<"ColorCorrectionHighlightsMin", "Lower threshold for highlights", float, -1.0f, 1.0f>,
    ItemRanged<"ColorCorrectionShadowsMax", "Threshold for shadows", float, -1.0f, 1.0f>,
    ItemRanged<"ColorGradingIntensity", "LUT intensity (0..1)", float, 0.0f, 1.0f>,
    ItemRanged<"ExpandGamut", "Expand bright saturated colors outside sRGB to fake wide gamut", float, 0.0f, 1.0f>,
    ItemRanged<"FilmBlackClip", "Lowers the toe; increasing clips more to black", float, 0.0f, 1.0f>,
    ItemRanged<"FilmGrainHighlightsMax", "Upper bound for Film Grain Highlight Intensity (> Min)", float, 1.0f, 10.0f>,
    ItemRanged<"FilmGrainHighlightsMin", "Lower bound for Film Grain Highlight Intensity", float, 0.0f, 1.0f>,
    ItemRanged<"FilmGrainIntensity", "Film grain intensity (lerp with decoded texture)", float, 0.0f, 1.0f>,
    ItemRanged<"FilmGrainIntensityHighlights", "Grain intensity in highlight regions", float, 0.0f, 1.0f>,
    ItemRanged<"FilmGrainIntensityMidtones", "Grain intensity in mid-tones", float, 0.0f, 1.0f>,
    ItemRanged<"FilmGrainIntensityShadows", "Grain intensity in shadow regions", float, 0.0f, 1.0f>,
    ItemRanged<"FilmGrainShadowsMax", "Upper bound for Film Grain Shadow Intensity", float, 0.0f, 1.0f>,
    ItemRanged<"FilmGrainTexelSize", "Size of film grain texel on screen", float, 0.0f, 4.0f>,
    ItemRanged<"FilmShoulder", "", float, 0.0f, 1.0f>,
    ItemRanged<"FilmSlope", "Controls overall steepness of tonemapper curve (contrast)", float, 0.0f, 1.0f>,
    ItemRanged<"FilmToe", "Controls contrast of dark end of tonemapper curve", float, 0.0f, 1.0f>,
    ItemRanged<"FilmWhiteClip", "Controls height of tonemapper curve (white saturation)", float, 0.0f, 1.0f>,
    ItemRanged<"Sharpen", "Strength of sharpening applied during tonemapping", float, 0.0f, 10.0f>,
    ItemRanged<"ToneCurveAmount", "Reduce effect of Tone Curve (set with ExpandGamut=0 to disable)", float, 0.0f, 1.0f>,

    Group<"Depth of Field">, ItemRanged<"DepthOfFieldBladeCount", "Number of diaphragm blades (4..16)", int, 4, 16>,
    ItemRanged<"DepthOfFieldDepthBlurAmount", "CircleDOF only: Depth blur km for 50%", float, 1e-06f, 100.0f>,
    ItemRanged<"DepthOfFieldDepthBlurRadius", "CircleDOF only: Depth blur radius in pixels at 1920x", float, 0.0f,
               4.0f>,
    ItemRanged<"DepthOfFieldFarBlurSize", "Gaussian only: Max blur size (% of view width)", float, 0.0f, 32.0f>,
    ItemRanged<"DepthOfFieldFarTransitionRegion", "Width of transition region next to focal region (near side)", float,
               0.0f, 10000.0f>,
    ItemRanged<"DepthOfFieldFocalDistance", "Distance where DoF is sharp (cm)", float, 0.0f, 10000.0f>,
    ItemRanged<"DepthOfFieldFocalRegion", "Artificial region starting after focal distance where content is in focus",
               float, 0.0f, 10000.0f>,
    ItemRanged<"DepthOfFieldFstop", "Lens opening; larger opening -> stronger DoF", float, 1.0f, 32.0f>,
    ItemRanged<"DepthOfFieldMinFstop", "Max opening (controls blade curvature); 0 for straight blades", float, 0.0f,
               32.0f>,
    ItemRanged<"DepthOfFieldNearBlurSize", "Gaussian only: Max blur size (% of view width)", float, 0.0f, 32.0f>,
    ItemRanged<"DepthOfFieldNearTransitionRegion", "Width of transition region next to focal region (near side)", float,
               0.0f, 10000.0f>,
    ItemRanged<"DepthOfFieldOcclusion", "Occlusion tweak factor (e.g., 0.18 natural, 0.4 fix leaking)", float, 0.0f,
               1.0f>,
    ItemRanged<"DepthOfFieldScale", "SM5 BokehDOF: amplify DoF; ES3_1: used to blend DoF", float, 0.0f, 2.0f>,
    ItemRanged<"DepthOfFieldSensorWidth", "Camera sensor width (mm)", float, 0.1f, 1000.0f>,
    ItemRanged<"DepthOfFieldSkyFocusDistance", "Artificial distance to keep skybox in focus (GaussianDOF)", float, 0.0f,
               200000.0f>,
    ItemRanged<"DepthOfFieldSqueezeFactor", "Anamorphic lens squeeze factor", float, 1.0f, 2.0f>,
    ItemRanged<"DepthOfFieldUseHairDepth", "Use hair depth for CoC size (else interpolate with scene depth)", int, 0,
               1>,
    ItemRanged<"DepthOfFieldVignetteSize", "Circular mask to blur outside radius (GaussianDOF)", float, 0.0f, 100.0f>,
    ItemFree<"GbxDepthOfFieldMaxBackgroundCocRadiusScale", "", float>,

    Group<"Fog">, ItemFree<"GbxFogDensityMultiplier", "", float>,

    Group<"Global Illumination">,
    ItemRanged<"DynamicGlobalIlluminationMethod",
               "Chooses the Dynamic GI method (None=0, Lumen=1, ScreenSpace=2, Plugin=3)", int, 0, 3>,
    ItemRanged<"IndirectLightingIntensity", "Scales indirect lighting; 0 disables GI", float, 0.0f, 4.0f>,

    Group<"Lens Flare">,
    ItemRanged<"LensFlareBokehSize", "Size of Lens Blur (% of view width) using Bokeh texture", float, 0.0f, 32.0f>,
    ItemRanged<"LensFlareIntensity", "Brightness scale of image caused lens flares", float, 0.0f, 16.0f>,
    ItemRanged<"LensFlareThreshold", "Minimum brightness for lens flares (set high to avoid perf cost)", float, 0.1f,
               32.0f>,

    Group<"Local Exposure">,
    ItemRanged<"LocalExposureBlurredLuminanceBlend",
               "Blend between bilateral filtered and blurred luminance as base layer", float, 0.0f, 1.0f>,
    ItemRanged<"LocalExposureBlurredLuminanceKernelSizePercent", "Kernel size (% of screen) to blur luminance", float,
               0.0f, 100.0f>,
    ItemFree<"LocalExposureContrastScale", "", float>,
    ItemRanged<"LocalExposureDetailStrength", "Value != 1 enables local exposure; set 1 in most cases", float, 0.0f,
               4.0f>,
    ItemRanged<"LocalExposureHighlightContrastScale",
               "Reduce base layer contrast (highlights). <1 enables local exposure", float, 0.0f, 1.0f>,
    ItemRanged<"LocalExposureHighlightThreshold", "Threshold for highlight regions", float, 0.0f, 4.0f>,
    ItemRanged<"LocalExposureMethod", "Algorithm (Bilateral=0, Fusion=1)", int, 0, 1>,
    ItemRanged<"LocalExposureMiddleGreyBias", "Log adjustment for local exposure middle grey", float, -15.0f, 15.0f>,
    ItemRanged<"LocalExposureShadowContrastScale", "Reduce base layer contrast (shadows). <1 enables local exposure",
               float, 0.0f, 1.0f>,
    ItemRanged<"LocalExposureShadowThreshold", "Threshold for shadow regions", float, 0.0f, 4.0f>,

    Group<"Lumen">,
    ItemRanged<"LumenFinalGatherLightingUpdateSpeed",
               "How much Final Gather may cache/update lighting (perf vs propagation)", float, 0.5f, 4.0f>,
    ItemRanged<"LumenFinalGatherQuality", "Final Gather quality scale (higher -> less noise, more cost)", float, 0.25f,
               2.0f>,
    ItemRanged<"LumenFinalGatherScreenTraces", "Use screen space traces for GI (bypasses Lumen Scene)", int, 0, 1>,
    ItemRanged<"LumenFrontLayerTranslucencyReflections", "High quality mirror reflections on front translucent layer",
               int, 0, 1>,
    ItemRanged<"LumenFullSkylightLeakingDistance", "Distance where skylight leaking reaches full intensity", float,
               0.1f, 2000.0f>,
    ItemRanged<"LumenMaxReflectionBounces", "Max recursive reflection bounces (hardware RT with Hit Lighting)", int, 1,
               8>,
    ItemRanged<"LumenMaxRefractionBounces", "Max number of refraction events to trace", int, 0, 64>,
    ItemRanged<"LumenMaxRoughnessToTraceReflections", "Max roughness for dedicated reflection rays", float, 0.0f, 1.0f>,
    ItemRanged<"LumenMaxTraceDistance", "Max distance to trace while solving lighting", float, 1.0f, 2097152.0f>,
    ItemRanged<"LumenRayLightingMode",
               "Lighting mode for HWRT rays (Default=0, SurfaceCache=1, HitLightingForReflections=2, HitLighting=3)",
               int, 0, 3>,
    ItemRanged<"LumenReflectionQuality", "Reflection quality scale (higher -> less noise, more cost)", float, 0.25f,
               2.0f>,
    ItemRanged<"LumenReflectionsScreenTraces", "Use screen space traces for reflections", int, 0, 1>,
    ItemRanged<"LumenSceneDetail", "Size of instances represented in Lumen Scene", float, 0.25f, 4.0f>,
    ItemRanged<"LumenSceneLightingQuality", "Lumen Scene quality scale", float, 0.25f, 2.0f>,
    ItemRanged<"LumenSceneLightingUpdateSpeed", "How much Lumen Scene may cache/update lighting", float, 0.5f, 4.0f>,
    ItemRanged<"LumenSceneViewDistance", "Max view distance maintained for ray tracing", float, 1.0f, 2097152.0f>,
    ItemRanged<"LumenSkylightLeaking", "Fraction of skylight intensity allowed to leak", float, 0.0f, 0.02f>,
    ItemRanged<"LumenSurfaceCacheResolution", "Surface Cache resolution scale (Scene Capture)", float, 0.5f, 1.0f>,

    Group<"MegaLights">,
    ItemRanged<"bMegaLights", "Force MegaLights on/off for this volume; requires HW Ray Tracing & SM6", int, 0, 1>,

    Group<"Motion Blur">, ItemRanged<"MotionBlurAmount", "Strength (0: off)", float, 0.0f, 1.0f>,
    ItemRanged<"MotionBlurDisableCameraInfluence", "", int, 0, 1>,
    ItemRanged<"MotionBlurMax", "Max distortion in % of screen width (0: off)", float, 0.0f, 100.0f>,
    ItemRanged<"MotionBlurPerObjectSize", "Min projected screen radius (% of screen width) for velocity pass", float,
               0.0f, 100.0f>,
    ItemRanged<"MotionBlurTargetFPS", "Target FPS for motion blur; 0 = dependent on actual FPS", int, 0, 120>,

    Group<"Path Tracing">,
    ItemRanged<"PathTracingEnableDenoiser", "Run denoiser plugin on last sample (if loaded)", int, 0, 1>,
    ItemRanged<"PathTracingEnableEmissiveMaterials", "Should emissive materials contribute to lighting?", int, 0, 1>,
    ItemRanged<"PathTracingEnableReferenceAtmosphere", "Path trace the atmosphere (skylight ignored when enabled)", int,
               0, 1>,
    ItemRanged<"PathTracingEnableReferenceDOF", "Reference-quality DOF replaces post-process DOF", int, 0, 1>,
    ItemRanged<"PathTracingIncludeDiffuse", "Include diffuse lighting contributions", int, 0, 1>,
    ItemRanged<"PathTracingIncludeEmissive", "Include directly visible emissive elements", int, 0, 1>,
    ItemRanged<"PathTracingIncludeIndirectDiffuse", "Include indirect diffuse", int, 0, 1>,
    ItemRanged<"PathTracingIncludeIndirectSpecular", "Include indirect specular", int, 0, 1>,
    ItemRanged<"PathTracingIncludeIndirectVolume", "Include indirect volume lighting", int, 0, 1>,
    ItemRanged<"PathTracingIncludeSpecular", "Include specular", int, 0, 1>,
    ItemRanged<"PathTracingIncludeVolume", "Include volume lighting", int, 0, 1>,
    ItemRanged<"PathTracingMaxBounces", "Max path bounces", int, 0, 100>,
    ItemRanged<"PathTracingMaxPathIntensity", "Clamp maximum indirect sample intensity (fireflies)", float, 1.0f,
               65504.0f>,
    ItemRanged<"PathTracingSamplesPerPixel", "Samples per pixel", int, 1, 65536>,

    Group<"Ray Tracing">, ItemRanged<"RayTracingAO", "Enable ray tracing AO", int, 0, 1>,
    ItemRanged<"RayTracingAOIntensity", "Scalar factor on RT AO score", float, 0.0f, 1.0f>,
    ItemRanged<"RayTracingAORadius", "World-space search radius for occlusion rays", float, 0.0f, 10000.0f>,
    ItemRanged<"RayTracingAOSamplesPerPixel", "SPP for RT AO", int, 1, 65536>,
    ItemRanged<"RayTracingTranslucencyMaxRoughness", "Max roughness for visible RT translucency (fades near threshold)",
               float, 0.01f, 1.0f>,
    ItemRanged<"RayTracingTranslucencyRefraction", "Enable refraction (else rays continue in same direction)", int, 0,
               1>,
    ItemRanged<"RayTracingTranslucencyRefractionRays", "Max refraction rays", int, 0, 50>,
    ItemRanged<"RayTracingTranslucencySamplesPerPixel", "SPP for RT translucency", int, 1, 65536>,
    ItemRanged<"RayTracingTranslucencyShadows", "Translucency shadows type (Disabled=0, Hard=1, Area=2)", int, 0, 2>,

    Group<"Reflections">,
    ItemRanged<"ReflectionMethod", "Reflection method (None=0, Lumen=1, ScreenSpace=2)", int, 0, 2>,
    ItemRanged<"ScreenSpaceReflectionIntensity", "Enable/Fade/disable SSR (%); avoid 0..1 for consistency", float, 0.0f,
               100.0f>,
    ItemRanged<"ScreenSpaceReflectionMaxRoughness", "Until what roughness we fade SSR (e.g., 0.8)", float, 0.01f, 1.0f>,
    ItemRanged<"ScreenSpaceReflectionQuality", "0=lowest..100=maximum quality; 50 default for perf", float, 0.0f,
               100.0f>,

    Group<"Resolution">, ItemFree<"ScreenPercentage", "Rendering resolution scale (%)", float>,

    Group<"Translucency">, ItemRanged<"TranslucencyType", "Translucency type (Raster=0, RayTracing=1)", int, 0, 1>,

    Group<"Vignette">, ItemRanged<"VignetteIntensity", "0..1 0=off .. 1=strong vignette", float, 0.0f, 1.0f>>;

static constexpr std::size_t kItemCount = count_items<Schema>();

// Compile-time duplicate key check
template <typename Tuple> consteval bool check_unique_keys() {
    // Gather keys into a constexpr array of string views
    std::array<const char *, kItemCount> keys{};
    std::size_t idx = 0;
    for_each_type<Tuple>([&]<typename Node>() {
        if constexpr (IsItem<Node>) {
            keys[idx++] = Node::key.c_str();
        }
    });
    // Compare all pairs
    for (std::size_t i = 0; i < keys.size(); ++i) {
        for (std::size_t j = i + 1; j < keys.size(); ++j) {
            // simple C-string compare
            const char *a = keys[i];
            const char *b = keys[j];
            std::size_t k = 0;
            while (a[k] != '\0' && a[k] == b[k])
                ++k;
            if (a[k] == '\0' && b[k] == '\0')
                return false; // duplicate found
        }
    }
    return true;
}
static_assert(check_unique_keys<Schema>(), "Duplicate keys in schema");

union Value {
    template <typename T> Value &operator=(T v) {
        if constexpr (std::is_same_v<T, int>)
            i = v;
        else if constexpr (std::is_same_v<T, float>)
            f = v;
        return *this;
    }
    template <typename T> T &get() {
        if constexpr (std::is_same_v<T, int>)
            return i;
        else if constexpr (std::is_same_v<T, float>)
            return f;
        else
            static_assert(false, "Unsupported type");
    }

  private:
    int i;
    float f;
};

// =========================
// Runtime storage (enabled + value), aligned with schema item order
// =========================
static std::array<bool, kItemCount> gEnables{};
static std::array<Value, kItemCount> gValues{};
static std::array<bool, kItemCount> gChanges{};

// =========================
// Tiny helpers
// =========================
template <typename T> void set_config(reshade::api::effect_runtime *rt, const char *key, T v) {
    if constexpr (std::is_same_v<T, bool>) {
        reshade::set_config_value(rt, kSection, key, v ? "1" : "0");
    } else {
        char buf[64]{};
        auto res = std::to_chars(std::begin(buf), std::end(buf), v);
        const std::size_t len = res.ec == std::errc() ? static_cast<std::size_t>(res.ptr - buf) : 0;
        reshade::set_config_value(rt, kSection, key, buf, len); // overload with explicit size
    }
}
inline bool get_config(reshade::api::effect_runtime *rt, const char *key, char *out, std::size_t &inout_size) {
    return reshade::get_config_value(rt, kSection, key, out, &inout_size);
}
template <typename T> void parse(const char *s, std::size_t n, T &out) {
    if (n != 0)
        std::from_chars(s, s + n, out);
}

// =========================
// compile-time map of setters
// =========================
std::atomic<float> myPostProcessBlendWeight = 0.f;
SDK::FPostProcessSettings myPostProcessSettings = {};

using namespace mapbox;
using Setter = void (*)(SDK::FPostProcessSettings &, bool, float);
#define SETTER(Name)                                                                                                   \
    {#Name, [](SDK::FPostProcessSettings &s, bool o, float v) {                                                        \
         s.bOverride_##Name = o;                                                                                       \
         s.Name = static_cast<decltype(s.Name)>(v);                                                                    \
     }},
#define APPLY_SETTER(Name) SETTER(Name)
static constexpr auto gSetters = eternal::map<eternal::string, Setter>({FOR_EACH(APPLY_SETTER, PPS_FIELDS)});
#undef APPLY_SETTER
#undef SETTER

template <ct_string Key, typename T> constexpr void apply_setting(bool o, T v) {
    constexpr auto setter = gSetters.find(Key.c_str());
    if constexpr (setter != gSetters.end()) {
        setter->second(myPostProcessSettings, o, v);
    } else {
        static_assert(false, "missing setter");
    }
}

// =========================
// Loading current preset values into runtime arrays (no defaults; absent -> 0/false)
// =========================
static void load_all_from_preset(reshade::api::effect_runtime *runtime) {
    LOG(INFO) << "Loading all from preset";
    // Global Enabled gate
    char buf[64]{};
    std::size_t sz = sizeof(buf);
    if (reshade::get_config_value(runtime, kSection, "Enabled", buf, &sz) && sz > 0)
        gEnabled = buf[0] == '1';
    else
        gEnabled = false; // absent -> off

    myPostProcessBlendWeight.store(gEnabled ? 1.f : 0.f, std::memory_order_release);
    LOG(INFO) << "Untitled " << (gEnabled ? "enabled" : "disabled");
    // Per item: read "<Key>.Enabled" and "<Key>.Value" using compile-time expansion over schema
    std::size_t idx = 0;
    for_each_type<Schema>([&]<typename Node>() {
        if constexpr (IsItem<Node>) {
            // Enabled
            if (sz = sizeof(buf); get_config(runtime, Node::key_enabled.c_str(), buf, sz) && sz > 0)
                gEnables[idx] = buf[0] == '1';
            else
                gEnables[idx] = false; // absent -> off

            // Value
            if (sz = sizeof(buf); get_config(runtime, Node::key_value.c_str(), buf, sz) && sz > 0) {
                typename Node::value_type v{};
                parse(buf, sz - 1, v);
                gValues[idx] = v;
            }
            apply_setting<Node::key>(gEnables[idx], gValues[idx].get<typename Node::value_type>());
            ++idx;
        }
    });
    LOG(INFO) << "Loaded all from preset";
}

// =========================
// Overlay drawing (compile-time expanded per item kind)
// =========================
static void draw_overlay(reshade::api::effect_runtime *runtime) {
    runtime->block_input_next_frame(); // block input while overlay visible

    // Global master toggle
    bool changed = ImGui::Checkbox("##enabled", &gEnabled);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Global gate for all overrides");
    if (changed)
        set_config(runtime, "Enabled", gEnabled);

    myPostProcessBlendWeight.store(gEnabled ? 1.f : 0.f, std::memory_order_release);
    ImGui::SameLine();
    ImGui::TextUnformatted("<-- GLOBAL GATE (MUST ENABLED !!!)");

    std::size_t idx = 0;
    bool have_group = false;
    bool current_group_open = true; // if items appear before the first group, treat as visible
    bool prev_group_open = false;   // for managing indentation

    for_each_type<Schema>([&]<typename Node>() {
        if constexpr (IsGroup<Node>) {
            // If a previous group was open, close its indent before starting a new header
            if (prev_group_open)
                ImGui::Unindent();

            // Collapsible header for this group (default-open)
            current_group_open = ImGui::CollapsingHeader(Node::title.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
            have_group = true;
            prev_group_open = current_group_open;
            if (current_group_open) {
                ImGui::Spacing();
                ImGui::Indent();
            }
        } else if constexpr (IsItem<Node>) {
            // Determine whether to draw this item (open group or no groups at all)
            const bool draw = have_group ? current_group_open : true;

            if (draw) {
                ImGui::PushID(Node::key.c_str());
                bool enabled_changed = ImGui::Checkbox("##enabled", &gEnables[idx]);
                ImGui::SameLine();

                const bool disabled = !gEnables[idx] || !gEnabled;
                if (disabled)
                    ImGui::BeginDisabled();

                bool value_changed = false;
                auto &value = gValues[idx].get<typename Node::value_type>();
                if constexpr (std::is_same_v<typename Node::value_type, int>) {
                    if constexpr (IsRangedItem<Node>) {
                        value_changed = ImGui::SliderInt("##value", &value, Node::min, Node::max);
                    } else {
                        value_changed = ImGui::InputInt("##value", &value);
                    }
                } else { // float
                    if constexpr (IsRangedItem<Node>) {
                        value_changed = ImGui::SliderFloat("##value", &value, Node::min, Node::max, "%.3f");
                    } else {
                        value_changed = ImGui::InputFloat("##value", &value);
                    }
                }
                bool deactivated = ImGui::IsItemDeactivatedAfterEdit();

                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                    ImGui::SetTooltip("%s", Node::info.c_str());

                if (disabled)
                    ImGui::EndDisabled();

                ImGui::SameLine();
                ImGui::TextUnformatted(Node::key.c_str());
                ImGui::PopID();

                if (enabled_changed)
                    set_config(runtime, Node::key_enabled.c_str(), gEnables[idx]);
                if (value_changed)
                    set_config(runtime, Node::key_value.c_str(), gValues[idx].get<typename Node::value_type>());
                if (enabled_changed || value_changed)
                    gChanges[idx] = true;
                if ((deactivated || enabled_changed) && gChanges[idx]) {
                    gChanges[idx] = false;
                    apply_setting<Node::key>(gEnables[idx], gValues[idx].get<typename Node::value_type>());
                    if (gEnables[idx]) {
                        LOG(INFO) << "Enabled: " << Node::key.c_str() << " = " << value;
                    } else {
                        LOG(INFO) << "Disabled: " << Node::key.c_str();
                    }
                }
            }
            ++idx;
        }
    });

    // Close last group's indent if it is open
    if (prev_group_open)
        ImGui::Unindent();
}

// =========================
// ReShade add-on wiring
// =========================
static void on_init_runtime(reshade::api::effect_runtime *rt) { load_all_from_preset(rt); }
static void on_preset_changed(reshade::api::effect_runtime *rt, const char * /*path*/) { load_all_from_preset(rt); }
static void overlay_cb(reshade::api::effect_runtime *rt) { draw_overlay(rt); }

static reshade::log::level to_reshade_level(el::Level lvl) {
    using RL = reshade::log::level;
    switch (lvl) {
    case el::Level::Fatal:
    case el::Level::Error:
        return RL::error;
    case el::Level::Warning:
        return RL::warning;
    case el::Level::Info:
        return RL::info;
    default:
        return RL::debug; // Debug/Trace/Verbose -> debug
    }
}

class ReShadeSink final : public el::LogDispatchCallback {
  public:
    void handle(const el::LogDispatchData *data) override {
        const auto lvl = to_reshade_level(data->logMessage()->level());
        reshade::log::message(lvl, data->logMessage()->message().c_str());
    }
};

// Exported identifiers (optional, shown in Add-ons tab)
extern "C" __declspec(dllexport) const char *NAME = "untitled";
extern "C" __declspec(dllexport) const char *DESCRIPTION = "for overriding post process settings on the fly (bl4 only)";

// Entry points
extern "C" __declspec(dllexport) bool AddonInit(HMODULE addon_module, HMODULE reshade_module) {
    if (!reshade::register_addon(addon_module, reshade_module)) {
        return false;
    }

    el::Helpers::installLogDispatchCallback<ReShadeSink>("ReShadeSink");
    el::Configurations conf;
    conf.setToDefault();
    conf.setGlobally(el::ConfigurationType::ToStandardOutput, "false");
    conf.setGlobally(el::ConfigurationType::ToFile, "false");
    el::Loggers::reconfigureAllLoggers(conf);

    std::memset(&myPostProcessSettings, 0, sizeof(SDK::FPostProcessSettings));

    reshade::register_event<reshade::addon_event::init_effect_runtime>(on_init_runtime);
    reshade::register_event<reshade::addon_event::reshade_set_current_preset_path>(on_preset_changed);
    reshade::register_overlay(kOverlay, overlay_cb);

    InstallHook();

    return true;
}

extern "C" __declspec(dllexport) void AddonUninit(HMODULE addon_module) {
    UninstallHook();

    reshade::unregister_overlay(kOverlay, overlay_cb);
    reshade::unregister_event<reshade::addon_event::reshade_set_current_preset_path>(on_preset_changed);
    reshade::unregister_event<reshade::addon_event::init_effect_runtime>(on_init_runtime);

    el::Helpers::uninstallLogDispatchCallback<ReShadeSink>("ReShadeSink");

    reshade::unregister_addon(addon_module);
}