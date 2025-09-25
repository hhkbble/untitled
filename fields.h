#pragma once

#define PARENS ()

#define EXPAND(...) EXPAND4(EXPAND4(EXPAND4(EXPAND4(__VA_ARGS__))))
#define EXPAND4(...) EXPAND3(EXPAND3(EXPAND3(EXPAND3(__VA_ARGS__))))
#define EXPAND3(...) EXPAND2(EXPAND2(EXPAND2(EXPAND2(__VA_ARGS__))))
#define EXPAND2(...) EXPAND1(EXPAND1(EXPAND1(EXPAND1(__VA_ARGS__))))
#define EXPAND1(...) __VA_ARGS__

#define FOR_EACH(macro, ...) __VA_OPT__(EXPAND(FOR_EACH_HELPER(macro, __VA_ARGS__)))

#define FOR_EACH_HELPER(macro, a1, ...) macro(a1) __VA_OPT__(FOR_EACH_AGAIN PARENS(macro, __VA_ARGS__))

#define FOR_EACH_AGAIN() FOR_EACH_HELPER

// all fields from FPostProcessSettings (filtered out non-primitives)
#define PPS_FIELDS                                                                                                     \
    AmbientCubemapIntensity, AmbientOcclusionBias, AmbientOcclusionDistance, AmbientOcclusionFadeDistance,             \
        AmbientOcclusionFadeRadius, AmbientOcclusionIntensity, AmbientOcclusionMipBlend, AmbientOcclusionMipScale,     \
        AmbientOcclusionMipThreshold, AmbientOcclusionPower, AmbientOcclusionQuality, AmbientOcclusionRadius,          \
        AmbientOcclusionRadiusInWS, AmbientOcclusionStaticFraction, AmbientOcclusionTemporalBlendWeight,               \
        AutoExposureApplyPhysicalCameraExposure, AutoExposureBias, AutoExposureBiasBackup,                             \
        AutoExposureCalibrationConstant, AutoExposureHighPercent, AutoExposureLowPercent, AutoExposureMaxBrightness,   \
        AutoExposureMethod, AutoExposureMinBrightness, AutoExposureSpeedDown, AutoExposureSpeedUp, Bloom1Size,         \
        Bloom2Size, Bloom3Size, Bloom4Size, Bloom5Size, Bloom6Size, BloomConvolutionBufferScale,                       \
        BloomConvolutionPreFilterMax, BloomConvolutionPreFilterMin, BloomConvolutionPreFilterMult,                     \
        BloomConvolutionScatterDispersion, BloomConvolutionSize, BloomDirtMaskIntensity, BloomIntensity, BloomMethod,  \
        BloomSizeScale, BloomThreshold, BlueCorrection, CameraISO, CameraShutterSpeed, ChromaticAberrationStartOffset, \
        ColorCorrectionHighlightsMax, ColorCorrectionHighlightsMin, ColorCorrectionShadowsMax, ColorGradingIntensity,  \
        DepthOfFieldBladeCount, DepthOfFieldDepthBlurAmount, DepthOfFieldDepthBlurRadius, DepthOfFieldFarBlurSize,     \
        DepthOfFieldFarTransitionRegion, DepthOfFieldFocalDistance, DepthOfFieldFocalRegion, DepthOfFieldFstop,        \
        DepthOfFieldMinFstop, DepthOfFieldNearBlurSize, DepthOfFieldNearTransitionRegion, DepthOfFieldOcclusion,       \
        DepthOfFieldScale, DepthOfFieldSensorWidth, DepthOfFieldSkyFocusDistance, DepthOfFieldSqueezeFactor,           \
        DepthOfFieldUseHairDepth, DepthOfFieldVignetteSize, DynamicGlobalIlluminationMethod, ExpandGamut,              \
        FilmBlackClip, FilmGrainHighlightsMax, FilmGrainHighlightsMin, FilmGrainIntensity,                             \
        FilmGrainIntensityHighlights, FilmGrainIntensityMidtones, FilmGrainIntensityShadows, FilmGrainShadowsMax,      \
        FilmGrainTexelSize, FilmShoulder, FilmSlope, FilmToe, FilmWhiteClip,                                           \
        GbxDepthOfFieldMaxBackgroundCocRadiusScale, GbxFogDensityMultiplier, HistogramLogMax, HistogramLogMin,         \
        IndirectLightingIntensity, LensFlareBokehSize, LensFlareIntensity, LensFlareThreshold,                         \
        LocalExposureBlurredLuminanceBlend, LocalExposureBlurredLuminanceKernelSizePercent,                            \
        LocalExposureContrastScale, LocalExposureDetailStrength, LocalExposureHighlightContrastScale,                  \
        LocalExposureHighlightThreshold, LocalExposureMiddleGreyBias, LocalExposureMethod,                             \
        LocalExposureShadowContrastScale, LocalExposureShadowThreshold, LumenFinalGatherLightingUpdateSpeed,           \
        LumenFinalGatherQuality, LumenFinalGatherScreenTraces, LumenFrontLayerTranslucencyReflections,                 \
        LumenFullSkylightLeakingDistance, LumenMaxReflectionBounces, LumenMaxRefractionBounces,                        \
        LumenMaxRoughnessToTraceReflections, LumenMaxTraceDistance, LumenRayLightingMode, LumenReflectionQuality,      \
        LumenReflectionsScreenTraces, LumenSceneDetail, LumenSceneLightingQuality, LumenSceneLightingUpdateSpeed,      \
        LumenSceneViewDistance, LumenSkylightLeaking, LumenSurfaceCacheResolution, MotionBlurAmount,                   \
        MotionBlurDisableCameraInfluence, MotionBlurMax, MotionBlurPerObjectSize, MotionBlurTargetFPS,                 \
        PathTracingEnableDenoiser, PathTracingEnableEmissiveMaterials, PathTracingEnableReferenceAtmosphere,           \
        PathTracingEnableReferenceDOF, PathTracingIncludeDiffuse, PathTracingIncludeEmissive,                          \
        PathTracingIncludeIndirectDiffuse, PathTracingIncludeIndirectSpecular, PathTracingIncludeIndirectVolume,       \
        PathTracingIncludeSpecular, PathTracingIncludeVolume, PathTracingMaxBounces, PathTracingMaxPathIntensity,      \
        PathTracingSamplesPerPixel, RayTracingAO, RayTracingAOIntensity, RayTracingAORadius,                           \
        RayTracingAOSamplesPerPixel, RayTracingTranslucencyMaxRoughness, RayTracingTranslucencyRefraction,             \
        RayTracingTranslucencyRefractionRays, RayTracingTranslucencySamplesPerPixel, RayTracingTranslucencyShadows,    \
        ReflectionMethod, SceneFringeIntensity, ScreenPercentage, ScreenSpaceReflectionIntensity,                      \
        ScreenSpaceReflectionMaxRoughness, ScreenSpaceReflectionQuality, Sharpen, TemperatureType, ToneCurveAmount,    \
        TranslucencyType, VignetteIntensity, WhiteTemp, WhiteTint, bMegaLights
