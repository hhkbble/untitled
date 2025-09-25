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
    static consteval std::size_t size() { return N ? N - 1 : 0; }
    constexpr const char *c_str() const { return v; }
    // structural equality by member-wise comparison is implicit
};

// concat two ct_strings -> new ct_string
template <ct_string A, ct_string B> consteval auto ct_concat() {
    constexpr std::size_t NA = A.size();
    constexpr std::size_t NB = B.size();
    ct_string<NA + NB + 1> out{};
    for (std::size_t i = 0; i < NA; ++i)
        out.v[i] = A.v[i];
    for (std::size_t j = 0; j < NB; ++j)
        out.v[NA - 1 + j] = B.v[j];
    out.v[NA + NB - 1] = '\0';
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

// Count by predicate at compile time
template <typename Tuple, auto Pred> consteval std::size_t count_by() {
    std::size_t n = 0;
    for_each_type<Tuple>([&]<typename Node>() {
        if constexpr (Pred.template operator()<Node>())
            ++n;
    });
    return n;
}
template <typename Tuple> consteval std::size_t count_items() {
    return count_by<Tuple, []<typename Node>() { return IsItem<Node>; }>();
}
template <typename Tuple> consteval std::size_t count_groups() {
    return count_by<Tuple, []<typename Node>() { return IsGroup<Node>; }>();
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
               "in unreal units, at what distance the AO effect disppears in the distance (avoding artifacts and AO\n"
               "effects on huge object)",
               float, 0.0f, 20000.0f>,
    ItemRanged<"AmbientOcclusionFadeRadius",
               "in unreal units, how many units before AmbientOcclusionFadeOutDistance it starts fading out", float,
               0.0f, 20000.0f>,
    ItemRanged<"AmbientOcclusionIntensity",
               "0..1 0=off/no ambient occlusion .. 1=strong ambient occlusion, defines how much it affects the non\n"
               "direct lighting after base pass",
               float, 0.0f, 1.0f>,
    ItemRanged<"AmbientOcclusionMipBlend",
               "Affects the blend over the multiple mips (lower resolution versions) , 0:fully use full resolution,\n"
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
               "0..1 0=no effect on static lighting .. 1=AO affects the stat lighting, 0 is free meaning no extra\n"
               "rendering pass",
               float, 0.0f, 1.0f>,
    ItemRanged<"AmbientOcclusionTemporalBlendWeight",
               "How much to blend the current frame with previous frames when using GTAO with temporal accumulation",
               float, 0.0f, 0.5f>,

    Group<"Auto Exposure">,
    ItemRanged<"AutoExposureApplyPhysicalCameraExposure",
               "Enables physical camera exposure using ShutterSpeed/ISO/Aperture.", int, 0, 1>,
    ItemRanged<"AutoExposureBias",
               "Logarithmic adjustment for the exposure. Only used if a tonemapper is specified. 0: no adjustment,\n"
               "-1:2x darker, -2:4x darker, 1:2x brighter, 2:4x brighter, ...",
               float, -15.0f, 15.0f>,
    ItemFree<"AutoExposureBiasBackup",
             "With the auto exposure changes, we are changing the AutoExposureBias inside the serialization code. We\n"
             "are storing that value before conversion here as a backup. Hopefully it will not be needed, and removed\n"
             "in the next engine revision.",
             float>,
    ItemFree<"AutoExposureCalibrationConstant", "", float>,
    ItemRanged<
        "AutoExposureHighPercent",
        "The eye adaptation will adapt to a value extracted from the luminance histogram of the scene color. The "
        "value\n"
        "is defined as having x percent below this brightness. Higher values give bright spots on the screen more\n"
        "priority but can lead to less stable results. Lower values give the medium and darker values more priority\n"
        "but might cause burn out of bright spots. >0, <100, good values are in the range 80 .. 95",
        float, 0.0f, 100.0f>,
    ItemRanged<
        "AutoExposureLowPercent",
        "The eye adaptation will adapt to a value extracted from the luminance histogram of the scene color. The "
        "value\n"
        "is defined as having x percent below this brightness. Higher values give bright spots on the screen more\n"
        "priority but can lead to less stable results. Lower values give the medium and darker values more priority\n"
        "but might cause burn out of bright spots. >0, <100, good values are in the range 70 .. 80",
        float, 0.0f, 100.0f>,
    ItemRanged<
        "AutoExposureMaxBrightness",
        "Auto-Exposure maximum adaptation. Eye Adaptation is disabled if Min = Max. Auto-exposure is\n"
        "implemented by choosing an exposure value for which the average luminance generates a pixel brightness\n"
        "equal to the Constant Calibration value. The Min/Max are expressed in pixel luminance (cd/m2) or in\n"
        "EV100 when using ExtendDefaultLuminanceRange (see project settings).",
        float, -10.0f, 20.0f>,
    ItemRanged<"AutoExposureMethod", "Luminance computation method (AEM_Histogram=0, AEM_Basic=1, AEM_Manual=2)", int,
               0, 2>,
    ItemRanged<
        "AutoExposureMinBrightness",
        "Auto-Exposure minimum adaptation. Eye Adaptation is disabled if Min = Max. Auto-exposure is\n"
        "implemented by choosing an exposure value for which the average luminance generates a pixel brightness\n"
        "equal to the Constant Calibration value. The Min/Max are expressed in pixel luminance (cd/m2) or in\n"
        "EV100 when using ExtendDefaultLuminanceRange (see project settings).",
        float, -10.0f, 20.0f>,
    ItemRanged<"AutoExposureSpeedDown", "", float, 0.02f, 20.0f>,
    ItemRanged<"AutoExposureSpeedUp", "", float, 0.02f, 20.0f>,
    ItemRanged<"HistogramLogMax",
               "Histogram Max value. Expressed in Log2(Luminance) or in EV100 when using ExtendDefaultLuminanceRange",
               float, 0.0f, 16.0f>,
    ItemRanged<"HistogramLogMin",
               "Histogram Min value. Expressed in Log2(Luminance) or in EV100 when using ExtendDefaultLuminanceRange",
               float, -16.0f, 0.0f>,

    Group<"Bloom">,
    ItemRanged<
        "Bloom1Size",
        "Diameter size for the Bloom1 in percent of the screen width (is done in 1/2 resolution, larger values cost\n"
        "more performance, good for high frequency details) >=0: can be clamped because of shader limitations",
        float, 0.0f, 4.0f>,
    ItemRanged<
        "Bloom2Size",
        "Diameter size for Bloom2 in percent of the screen width (is done in 1/4 resolution, larger values cost\n"
        "more performance) >=0: can be clamped because of shader limitations",
        float, 0.0f, 8.0f>,
    ItemRanged<
        "Bloom3Size",
        "Diameter size for Bloom3 in percent of the screen width (is done in 1/8 resolution, larger values cost\n"
        "more performance) >=0: can be clamped because of shader limitations",
        float, 0.0f, 16.0f>,
    ItemRanged<"Bloom4Size",
               "Diameter size for Bloom4 in percent of the screen width (is done in 1/16 resolution, larger values\n"
               "cost more performance, best for wide contributions) >=0: can be clamped because of shader limitations",
               float, 0.0f, 32.0f>,
    ItemRanged<"Bloom5Size",
               "Diameter size for Bloom5 in percent of the screen width (is done in 1/32 resolution, larger values\n"
               "cost more performance, best for wide contributions) >=0: can be clamped because of shader limitations",
               float, 0.0f, 64.0f>,
    ItemRanged<"Bloom6Size",
               "Diameter size for Bloom6 in percent of the screen width (is done in 1/64 resolution, larger values\n"
               "cost more performance, best for wide contributions) >=0: can be clamped because of shader limitations",
               float, 0.0f, 128.0f>,
    ItemRanged<"BloomConvolutionBufferScale",
               "Implicit buffer region as a fraction of the screen size to insure the bloom does not wrap across the\n"
               "screen. Larger sizes have perf impact.",
               float, 0.0f, 1.0f>,
    ItemFree<"BloomConvolutionPreFilterMax",
             "Boost intensity of select pixels prior to computing bloom convolution (Min, Max, Multiplier). Max < Min\n"
             "disables",
             float>,
    ItemFree<"BloomConvolutionPreFilterMin",
             "Boost intensity of select pixels prior to computing bloom convolution (Min, Max, Multiplier). Max < Min\n"
             "disables",
             float>,
    ItemFree<"BloomConvolutionPreFilterMult",
             "Boost intensity of select pixels prior to computing bloom convolution (Min, Max, Multiplier). Max < Min\n"
             "disables",
             float>,
    ItemRanged<"BloomConvolutionScatterDispersion",
               "Intensity multiplier on the scatter dispersion energy of the kernel. 1.0 means exactly use the same\n"
               "energy as the kernel scatter dispersion.",
               float, 0.0f, 20.0f>,
    ItemRanged<"BloomConvolutionSize",
               "Relative size of the convolution kernel image compared to the minor axis of the viewport", float, 0.0f,
               1.0f>,
    ItemRanged<"BloomDirtMaskIntensity", "BloomDirtMask intensity", float, 0.0f, 8.0f>,
    ItemRanged<"BloomIntensity", "Multiplier for all bloom contributions >=0: off, 1(default), >1 brighter", float,
               0.0f, 8.0f>,
    ItemRanged<"BloomMethod", "Bloom algorithm (BM_SOG=0, BM_FFT=1)", int, 0, 1>,
    ItemRanged<"BloomSizeScale", "Scale for all bloom sizes", float, 0.0f, 64.0f>,
    ItemRanged<
        "BloomThreshold",
        "minimum brightness the bloom starts having effect -1:all pixels affect bloom equally (physically correct,\n"
        "faster as a threshold pass is omitted), 0:all pixels affect bloom brights more, 1(default), >1 brighter",
        float, -1.0f, 8.0f>,

    Group<"Camera & White Balance">, ItemFree<"CameraISO", "The camera sensor sensitivity in ISO.", float>,
    ItemRanged<"CameraShutterSpeed", "The camera shutter in seconds.", float, 1.0f, 2000.0f>,
    ItemRanged<"TemperatureType",
               "Selects the type of temperature calculation. White Balance uses the Temperature value to control the\n"
               "virtual camera's White Balance. This is the default selection. Color Temperature uses the Temperature\n"
               "value to adjust the color temperature of the scene, which is the inverse of the White Balance\n"
               "operation. (TEMP_WhiteBalance=0, TEMP_ColorTemperature=1)",
               int, 0, 1>,
    ItemRanged<"WhiteTemp", "Controls the color temperature (Kelvin) considered as white", float, 1500.0f, 15000.0f>,
    ItemRanged<"WhiteTint", "Magenta-Green axis tint (orthogonal to temperature)", float, -1.0f, 1.0f>,

    Group<"Chromatic Aberration">,
    ItemRanged<"ChromaticAberrationStartOffset",
               "A normalized distance to the center of the framebuffer where the effect takes place.", float, 0.0f,
               1.0f>,
    ItemRanged<"SceneFringeIntensity",
               "in percent, Scene chromatic aberration / color fringe (camera imperfection) to simulate an artifact\n"
               "that happens in real-world lens, mostly visible in the image corners.",
               float, 0.0f, 5.0f>,

    Group<"Color Grading & Tone Mapping">,
    ItemRanged<"BlueCorrection",
               "Correct for artifacts with \"electric\" blues due to the ACEScg color space. Bright blue desaturates\n"
               "instead of going to violet.",
               float, 0.0f, 1.0f>,
    ItemRanged<"ColorCorrectionHighlightsMax",
               "This value sets the upper threshold for what is considered to be the highlight region of the image.\n"
               "This value should be larger than HighlightsMin. Default is 1.0, for backwards compatibility",
               float, 1.0f, 10.0f>,
    ItemRanged<"ColorCorrectionHighlightsMin",
               "This value sets the lower threshold for what is considered to be the highlight region of the image.",
               float, -1.0f, 1.0f>,
    ItemRanged<"ColorCorrectionShadowsMax",
               "This value sets the threshold for what is considered to be the shadow region of the image.", float,
               -1.0f, 1.0f>,
    ItemRanged<"ColorGradingIntensity", "Color grading lookup table intensity. 0 = no intensity, 1=full intensity",
               float, 0.0f, 1.0f>,
    ItemRanged<"ExpandGamut", "Expand bright saturated colors outside the sRGB gamut to fake wide gamut rendering.",
               float, 0.0f, 1.0f>,
    ItemRanged<"FilmBlackClip",
               "Lowers the toe of the tonemapper curve by this amount. Increasing this value causes more of the scene\n"
               "to clip to black. For most purposes, this property should remain 0",
               float, 0.0f, 1.0f>,
    ItemRanged<"FilmGrainHighlightsMax",
               "Sets the upper bound used for Film Grain Highlight Intensity. This value should be larger than\n"
               "HighlightsMin.. Default is 1.0, for backwards compatibility",
               float, 1.0f, 10.0f>,
    ItemRanged<"FilmGrainHighlightsMin", "Sets the lower bound used for Film Grain Highlight Intensity.", float, 0.0f,
               1.0f>,
    ItemRanged<"FilmGrainIntensity",
               "0..1 Film grain intensity to apply. LinearSceneColor *= lerp(1.0, DecodedFilmGrainTexture,\n"
               "FilmGrainIntensity)",
               float, 0.0f, 1.0f>,
    ItemRanged<"FilmGrainIntensityHighlights",
               "Control over the grain intensity in the regions of the image considered highlight areas.", float, 0.0f,
               1.0f>,
    ItemRanged<"FilmGrainIntensityMidtones", "Control over the grain intensity in the mid-tone region of the image.",
               float, 0.0f, 1.0f>,
    ItemRanged<"FilmGrainIntensityShadows",
               "Control over the grain intensity in the regions of the image considered shadow areas.", float, 0.0f,
               1.0f>,
    ItemRanged<"FilmGrainShadowsMax", "Sets the upper bound used for Film Grain Shadow Intensity.", float, 0.0f, 1.0f>,
    ItemRanged<"FilmGrainTexelSize",
               "Controls the size of the film grain. Size of texel of FilmGrainTexture on screen.", float, 0.0f, 4.0f>,
    ItemRanged<"FilmShoulder",
               "Sometimes referred to as highlight rolloff. Controls the contrast of the bright end of the tonemapper\n"
               "curve. Larger values increase contrast and smaller values decrease contrast.",
               float, 0.0f, 1.0f>,
    ItemRanged<"FilmSlope",
               "Controls the overall steepness of the tonemapper curve. Larger values increase scene contrast and\n"
               "smaller values reduce contrast.",
               float, 0.0f, 1.0f>,
    ItemRanged<"FilmToe",
               "Controls the contrast of the dark end of the tonemapper curve. Larger values increase contrast and\n"
               "smaller values decrease contrast.",
               float, 0.0f, 1.0f>,
    ItemRanged<"FilmWhiteClip",
               "Controls the height of the tonemapper curve. Raising this value can cause bright values to more\n"
               "quickly approach fully-saturated white.",
               float, 0.0f, 1.0f>,
    ItemRanged<"Sharpen", "Controls the strength of image sharpening applied during tonemapping.", float, 0.0f, 10.0f>,
    ItemRanged<"ToneCurveAmount",
               "Allow effect of Tone Curve to be reduced (Set ToneCurveAmount and ExpandGamut to 0.0 to fully disable\n"
               "tone curve)",
               float, 0.0f, 1.0f>,

    Group<"Depth of Field">,
    ItemRanged<"DepthOfFieldBladeCount",
               "Defines the number of blades of the diaphragm within the lens (between 4 and 16).", int, 4, 16>,
    ItemRanged<"DepthOfFieldDepthBlurAmount", "CircleDOF only: Depth blur km for 50%", float, 1e-06f, 100.0f>,
    ItemRanged<"DepthOfFieldDepthBlurRadius", "CircleDOF only: Depth blur radius in pixels at 1920x", float, 0.0f,
               4.0f>,
    ItemRanged<"DepthOfFieldFarBlurSize",
               "Gaussian only: Maximum size of the Depth of Field blur (in percent of the view width) (note:\n"
               "performance cost scales with size)",
               float, 0.0f, 32.0f>,
    ItemRanged<"DepthOfFieldFarTransitionRegion",
               "To define the width of the transition region next to the focal region on the near side (cm)", float,
               0.0f, 10000.0f>,
    ItemRanged<"DepthOfFieldFocalDistance",
               "Distance in which the Depth of Field effect should be sharp, in unreal units (cm)", float, 0.0f,
               10000.0f>,
    ItemRanged<"DepthOfFieldFocalRegion",
               "Artificial region where all content is in focus, starting after DepthOfFieldFocalDistance, in unreal\n"
               "units (cm)",
               float, 0.0f, 10000.0f>,
    ItemRanged<"DepthOfFieldFstop",
               "Defines the opening of the camera lens, Aperture is 1/fstop, typical lens go down to f/1.2 (large\n"
               "opening), larger numbers reduce the DOF effect",
               float, 1.0f, 32.0f>,
    ItemRanged<"DepthOfFieldMinFstop",
               "Defines the maximum opening of the camera lens to control the curvature of blades of the diaphragm.\n"
               "Set it to 0 to get straight blades.",
               float, 0.0f, 32.0f>,
    ItemRanged<"DepthOfFieldNearBlurSize",
               "Gaussian only: Maximum size of the Depth of Field blur (in percent of the view width) (note:\n"
               "performance cost scales with size)",
               float, 0.0f, 32.0f>,
    ItemRanged<"DepthOfFieldNearTransitionRegion",
               "To define the width of the transition region next to the focal region on the near side (cm)", float,
               0.0f, 10000.0f>,
    ItemRanged<"DepthOfFieldOcclusion",
               "Occlusion tweak factor 1 (0.18 to get natural occlusion, 0.4 to solve layer color leaking issues)",
               float, 0.0f, 1.0f>,
    ItemRanged<"DepthOfFieldScale",
               "SM5: BokehDOF only: To amplify the depth of field effect (like aperture) 0=off ES3_1: Used to blend\n"
               "DoF. 0=off",
               float, 0.0f, 2.0f>,
    ItemRanged<"DepthOfFieldSensorWidth", "Width of the camera sensor to assume, in mm.", float, 0.1f, 1000.0f>,
    ItemRanged<"DepthOfFieldSkyFocusDistance",
               "Artificial distance to allow the skybox to be in focus (e.g. 200000), <=0 to switch the feature off,\n"
               "only for GaussianDOF, can cost performance",
               float, 0.0f, 200000.0f>,
    ItemRanged<"DepthOfFieldSqueezeFactor",
               "This is the squeeze factor for the DOF, which emulates the properties of anamorphic lenses.", float,
               1.0f, 2.0f>,
    ItemRanged<"DepthOfFieldUseHairDepth",
               "For depth of field to use the hair depth for computing circle of confusion size. Otherwise use an\n"
               "interpolated distance between the hair depth and the scene depth based on the hair coverage (default).",
               int, 0, 1>,
    ItemRanged<
        "DepthOfFieldVignetteSize",
        "Artificial circular mask to (near) blur content outside the radius, only for GaussianDOF, diameter in "
        "percent\n"
        "of screen width, costs performance if the mask is used, keep Feather can Radius on default to keep it off",
        float, 0.0f, 100.0f>,
    ItemFree<"GbxDepthOfFieldMaxBackgroundCocRadiusScale", "", float>,

    Group<"Fog">, ItemFree<"GbxFogDensityMultiplier", "", float>,

    Group<"Global Illumination">,
    ItemRanged<
        "DynamicGlobalIlluminationMethod",
        "Chooses the Dynamic Global Illumination method. Not compatible with Forward Shading. (None=0, Lumen=1,\n"
        "ScreenSpace=2, Plugin=3)",
        int, 0, 3>,
    ItemRanged<"IndirectLightingIntensity",
               "Scales the indirect lighting contribution. A value of 0 disables GI. Default is 1. The show flag\n"
               "'Global Illumination' must be enabled to use this property.",
               float, 0.0f, 4.0f>,

    Group<"Lens Flare">,
    ItemRanged<"LensFlareBokehSize",
               "Size of the Lens Blur (in percent of the view width) that is done with the Bokeh texture (note:\n"
               "performance cost is radius*radius)",
               float, 0.0f, 32.0f>,
    ItemRanged<"LensFlareIntensity", "Brightness scale of the image cased lens flares (linear)", float, 0.0f, 16.0f>,
    ItemRanged<"LensFlareThreshold",
               "Minimum brightness the lens flare starts having effect (this should be as high as possible to avoid\n"
               "the performance cost of blurring content that is too dark too see)",
               float, 0.1f, 32.0f>,

    Group<"Local Exposure">,
    ItemRanged<"LocalExposureBlurredLuminanceBlend",
               "Local Exposure decomposes luminance of the frame into a base layer and a detail layer. Blend between\n"
               "bilateral filtered and blurred luminance as the base layer. Blurred luminance helps preserve image "
               "appearance\n"
               "and specular highlights, and reduce ringing. Good values are usually in the range 0.4 .. 0.6",
               float, 0.0f, 1.0f>,
    ItemRanged<"LocalExposureBlurredLuminanceKernelSizePercent",
               "Kernel size (percentage of screen) used to blur frame luminance.", float, 0.0f, 100.0f>,
    ItemFree<"LocalExposureContrastScale", "", float>,
    ItemRanged<
        "LocalExposureDetailStrength",
        "Local Exposure decomposes luminance of the frame into a base layer and a detail layer. Value different\n"
        "than 1 will enable local exposure. This value should be set to 1 in most cases.",
        float, 0.0f, 4.0f>,
    ItemRanged<
        "LocalExposureHighlightContrastScale",
        "Local Exposure decomposes luminance of the frame into a base layer and a detail layer. Contrast of the\n"
        "base layer is reduced based on this value. Value less than 1 will enable local exposure. Good values\n"
        "are usually in the range 0.6 .. 1.0.",
        float, 0.0f, 1.0f>,
    ItemRanged<"LocalExposureHighlightThreshold",
               "Threshold used to determine which regions of the screen are considered highlights.", float, 0.0f, 4.0f>,
    ItemRanged<"LocalExposureMethod", "Local Exposure algorithm (Bilateral=0, Fusion=1)", int, 0, 1>,
    ItemRanged<"LocalExposureMiddleGreyBias",
               "Logarithmic adjustment for the local exposure middle grey. 0: no adjustment, -1:2x darker, -2:4x\n"
               "darker, 1:2x brighter, 2:4x brighter, ...",
               float, -15.0f, 15.0f>,
    ItemRanged<
        "LocalExposureShadowContrastScale",
        "Local Exposure decomposes luminance of the frame into a base layer and a detail layer. Contrast of the\n"
        "base layer is reduced based on this value. Value less than 1 will enable local exposure. Good values\n"
        "are usually in the range 0.6 .. 1.0.",
        float, 0.0f, 1.0f>,
    ItemRanged<"LocalExposureShadowThreshold",
               "Threshold used to determine which regions of the screen are considered shadows.", float, 0.0f, 4.0f>,

    Group<"Lumen">,
    ItemRanged<"LumenFinalGatherLightingUpdateSpeed",
               "Controls how much Lumen Final Gather is allowed to cache lighting results to improve performance.\n"
               "Larger scales cause lighting changes to propagate faster, but increase GPU cost and noise.",
               float, 0.5f, 4.0f>,
    ItemRanged<"LumenFinalGatherQuality",
               "Scales Lumen's Final Gather quality. Larger scales reduce noise, but greatly increase GPU cost.", float,
               0.25f, 2.0f>,
    ItemRanged<"LumenFinalGatherScreenTraces",
               "Whether to use screen space traces for Lumen Global Illumination. Screen space traces bypass Lumen\n"
               "Scene and instead sample Scene Depth and Scene Color. This improves quality, as it bypasses Lumen\n"
               "Scene, but causes view dependent lighting.",
               int, 0, 1>,
    ItemRanged<"LumenFrontLayerTranslucencyReflections",
               "Whether to use high quality mirror reflections on the front layer of translucent surfaces. Other\n"
               "layers will use the lower quality Radiance Cache method that can only produce glossy reflections.\n"
               "Increases GPU cost when enabled.",
               int, 0, 1>,
    ItemRanged<
        "LumenFullSkylightLeakingDistance",
        "Controls the distance from a receiving surface where skylight leaking reaches its full intensity. Smaller\n"
        "values make the skylight leaking flatter, while larger values create an Ambient Occlusion effect.",
        float, 0.1f, 2000.0f>,
    ItemRanged<
        "LumenMaxReflectionBounces",
        "Sets the maximum number of recursive reflection bounces. 1 means a single reflection ray (no secondary\n"
        "reflections in mirrors). Currently only supported by Hardware Ray Tracing with Hit Lighting.",
        int, 1, 8>,
    ItemRanged<"LumenMaxRefractionBounces",
               "The maximum count of refraction event to trace. When hit lighting is used, Translucent meshes will be\n"
               "traced when LumenMaxRefractionBounces > 0, making the reflection tracing more expenssive.",
               int, 0, 64>,
    ItemRanged<
        "LumenMaxRoughnessToTraceReflections",
        "Sets the maximum roughness value for which Lumen still traces dedicated reflection rays. Higher values\n"
        "improve reflection quality, but greatly increase GPU cost.",
        float, 0.0f, 1.0f>,
    ItemRanged<
        "LumenMaxTraceDistance",
        "Controls the maximum distance that Lumen should trace while solving lighting. Values that are too small will\n"
        "cause lighting to leak into large caves, while values that are large will increase GPU cost.",
        float, 1.0f, 2097152.0f>,
    ItemRanged<"LumenRayLightingMode",
               "Controls how Lumen rays are lit when Lumen is using Hardware Ray Tracing. By default, Lumen uses the\n"
               "Surface Cache for best performance, but can be set to 'Hit Lighting' for higher quality. (Default=0,\n"
               "SurfaceCache=1, HitLightingForReflections=2, HitLighting=3)",
               int, 0, 3>,
    ItemRanged<"LumenReflectionQuality",
               "Scales the Reflection quality. Larger scales reduce noise in reflections, but increase GPU cost.",
               float, 0.25f, 2.0f>,
    ItemRanged<"LumenReflectionsScreenTraces",
               "Whether to use screen space traces for Lumen Reflections. Screen space traces bypass Lumen Scene and\n"
               "instead sample Scene Depth and Scene Color. This improves quality, as it bypasses Lumen Scene, but\n"
               "causes view dependent lighting.",
               int, 0, 1>,
    ItemRanged<
        "LumenSceneDetail",
        "Controls the size of instances that can be represented in Lumen Scene. Larger values will ensure small\n"
        "objects are represented, but increase GPU cost.",
        float, 0.25f, 4.0f>,
    ItemRanged<
        "LumenSceneLightingQuality",
        "Scales Lumen Scene's quality. Larger scales cause Lumen Scene to be calculated with a higher fidelity,\n"
        "which can be visible in reflections, but increase GPU cost.",
        float, 0.25f, 2.0f>,
    ItemRanged<"LumenSceneLightingUpdateSpeed",
               "Controls how much Lumen Scene is allowed to cache lighting results to improve performance. Larger\n"
               "scales cause lighting changes to propagate faster, but increase GPU cost.",
               float, 0.5f, 4.0f>,
    ItemRanged<
        "LumenSceneViewDistance",
        "Sets the maximum view distance of the scene that Lumen maintains for ray tracing against. Larger values will\n"
        "increase the effective range of sky shadowing and Global Illumination, but increase GPU cost.",
        float, 1.0f, 2097152.0f>,
    ItemRanged<"LumenSkylightLeaking",
               "Controls what fraction of the skylight intensity should be allowed to leak. This can be useful as an\n"
               "art direction knob (non-physically based) to keep indoor areas from going fully black.",
               float, 0.0f, 0.02f>,
    ItemRanged<
        "LumenSurfaceCacheResolution",
        "Scale factor for Lumen Surface Cache resolution, for Scene Capture. Smaller values save GPU memory, at\n"
        "a cost in quality. Defaults to 0.5 if not overridden.",
        float, 0.5f, 1.0f>,

    Group<"MegaLights">,
    ItemRanged<
        "bMegaLights",
        "Allows forcing MegaLights on or off for this volume, regardless of the project setting for MegaLights.\n"
        "MegaLights will stochastically sample lights, which allows many shadow casting lights to be rendered\n"
        "efficiently, with a consistent and low GPU cost. When MegaLights is enabled, other direct lighting\n"
        "algorithms like Deferred Shading will no longer be used, and other shadowing methods like Ray Traced\n"
        "Shadows, Distance Field Shadows and Shadow Maps will no longer be used. MegaLights requires Hardware\n"
        "Ray Tracing and Shader Model 6.",
        int, 0, 1>,

    Group<"Motion Blur">, ItemRanged<"MotionBlurAmount", "Strength of motion blur, 0:off", float, 0.0f, 1.0f>,
    ItemRanged<"MotionBlurDisableCameraInfluence", "", int, 0, 1>,
    ItemRanged<"MotionBlurMax", "max distortion caused by motion blur, in percent of the screen width, 0:off", float,
               0.0f, 100.0f>,
    ItemRanged<"MotionBlurPerObjectSize",
               "The minimum projected screen radius for a primitive to be drawn in the velocity pass, percentage of\n"
               "screen width. smaller numbers cause more draw calls, default: 4%",
               float, 0.0f, 100.0f>,
    ItemRanged<
        "MotionBlurTargetFPS",
        "Defines the target FPS for motion blur. Makes motion blur independent of actual frame rate and\n"
        "relative to the specified target FPS instead. Higher target FPS results in shorter frames, which means\n"
        "shorter shutter times and less motion blur. Lower FPS means more motion blur. A value of zero makes\n"
        "the motion blur dependent on the actual frame rate.",
        int, 0, 120>,

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
    ItemRanged<"ReflectionMethod",
               "Chooses the Reflection method. Not compatible with Forward Shading. (None=0, Lumen=1, ScreenSpace=2)",
               int, 0, 2>,
    ItemRanged<"ScreenSpaceReflectionIntensity",
               "Enable/Fade/disable the Screen Space Reflection feature, in percent, avoid numbers between 0 and 1 fo\n"
               "consistency",
               float, 0.0f, 100.0f>,
    ItemRanged<"ScreenSpaceReflectionMaxRoughness",
               "Until what roughness we fade the screen space reflections, 0.8 works well, smaller can run faster",
               float, 0.01f, 1.0f>,
    ItemRanged<"ScreenSpaceReflectionQuality",
               "0=lowest quality..100=maximum quality, only a few quality levels are implemented, no soft transition,\n"
               "50 is the default for better performance.",
               float, 0.0f, 100.0f>,

    Group<"Resolution">, ItemFree<"ScreenPercentage", "Rendering resolution scale (%)", float>,

    Group<"Translucency">, ItemRanged<"TranslucencyType", "Translucency type (Raster=0, RayTracing=1)", int, 0, 1>,

    Group<"Vignette">, ItemRanged<"VignetteIntensity", "0..1 0=off .. 1=strong vignette", float, 0.0f, 1.0f>>;

static constexpr std::size_t kItemCount = count_items<Schema>();
static constexpr std::size_t kGroupCount = count_groups<Schema>();

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

// without checking for performance
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
// global state for MyBlueprintModifyPostProcess
// =========================
std::atomic<float> myPostProcessBlendWeight = 0.f;
SDK::FPostProcessSettings myPostProcessSettings = {};

// =========================
// compile-time map of setters
// =========================
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
        static_assert(false, "Missing setter");
    }
}

// =========================
// Runtime storage item(enabled + value + changed) / group(opened), aligned with schema item order
// =========================
static std::array<bool, kItemCount> gEnables{};
static std::array<Value, kItemCount> gValues{};
static std::array<bool, kItemCount> gChanges{};
static std::array<bool, kGroupCount> gOpens{};

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
    bool opened = ImGui::CollapsingHeader("GLOBAL", ImGuiTreeNodeFlags_DefaultOpen);
    if (opened) {
        ImGui::Spacing();
        ImGui::Indent();
    }
    bool changed = ImGui::Checkbox("##enabled", &gEnabled);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Global gate for all overrides");
    if (changed)
        set_config(runtime, "Enabled", gEnabled);

    myPostProcessBlendWeight.store(gEnabled ? 1.f : 0.f, std::memory_order_release);

    ImGui::SameLine();
    ImGui::TextUnformatted("<-- ENABLED");
    if (opened)
        ImGui::Unindent();

    if (!gEnabled)
        return;

    std::size_t idx = 0;
    std::size_t gidx = 0;
    bool have_group = false;
    bool current_group_open = true; // if items appear before the first group, treat as visible
    bool prev_group_open = false;   // for managing indentation

    for_each_type<Schema>([&]<typename Node>() {
        if constexpr (IsGroup<Node>) {
            have_group = true;
            // If a previous group was open, close its indent before starting a new header
            if (prev_group_open)
                ImGui::Unindent();

            // Collapsible header for this group
            ImGui::SetNextItemOpen(gOpens[gidx], ImGuiCond_Always);
            gOpens[gidx] = ImGui::CollapsingHeader(Node::title.c_str());
            current_group_open = gOpens[gidx];
            prev_group_open = current_group_open;
            if (current_group_open) {
                ImGui::Spacing();
                ImGui::Indent();
            }
            ++gidx;
        } else if constexpr (IsItem<Node>) {
            // Determine whether to draw this item (open group or no groups at all)
            const bool draw = have_group ? current_group_open : true;

            if (draw) {
                ImGui::PushID(Node::key.c_str());
                bool enabled_changed = ImGui::Checkbox("##enabled", &gEnables[idx]);
                ImGui::SameLine();
                ImGui::TextUnformatted(Node::key.c_str());

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

                if constexpr (Node::info.size() > 0) {
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                        ImGui::SetTooltip("%s", Node::info.c_str());
                }

                if (disabled)
                    ImGui::EndDisabled();

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

// =========================
// ReShade logging sink
// =========================
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