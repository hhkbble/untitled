#pragma once

#define PARENS ()

#define EXPAND(...) EXPAND4(EXPAND4(EXPAND4(EXPAND4(__VA_ARGS__))))
#define EXPAND4(...) EXPAND3(EXPAND3(EXPAND3(EXPAND3(__VA_ARGS__))))
#define EXPAND3(...) EXPAND2(EXPAND2(EXPAND2(EXPAND2(__VA_ARGS__))))
#define EXPAND2(...) EXPAND1(EXPAND1(EXPAND1(EXPAND1(__VA_ARGS__))))
#define EXPAND1(...) __VA_ARGS__

#define PROBE(x) x, 1
#define SECOND(a, b, ...) b
#define IS_PAREN_PROBE(...) PROBE(~)
#define IS_PAREN_CHECK(...) SECOND(__VA_ARGS__, 0)
#define IS_PAREN(x) IS_PAREN_CHECK(IS_PAREN_PROBE x)

#define IIF(c) _IIF(c)
#define _IIF(c) __IIF_##c
#define __IIF_0(t, f) f
#define __IIF_1(t, f) t

#define APPLY(m, x) IIF(IS_PAREN(x))(m x, m(x))
#define FOR_EACH(m, ...) __VA_OPT__(EXPAND(FE_HELPER(m, __VA_ARGS__)))
#define FE_HELPER(m, a1, ...) APPLY(m, a1) __VA_OPT__(FE_AGAIN PARENS(m, __VA_ARGS__))
#define FE_AGAIN() FE_HELPER

#define COMMA() ,
#define FOR_EACH_SEP(sep, m, ...) __VA_OPT__(EXPAND(FE_SEP_HELPER(sep, m, __VA_ARGS__)))
#define FE_SEP_HELPER(sep, m, a1, ...) APPLY(m, a1) __VA_OPT__(sep() FE_SEP_AGAIN PARENS(sep, m, __VA_ARGS__))
#define FE_SEP_AGAIN() FE_SEP_HELPER

#define FOR_EACH_CTX(m, ctx, ...) __VA_OPT__(EXPAND(FE_CTX_HELPER(m, ctx, __VA_ARGS__)))
#define FE_CTX_HELPER(m, ctx, a1, ...) m(ctx, a1) __VA_OPT__(FE_CTX_AGAIN PARENS(m, ctx, __VA_ARGS__))
#define FE_CTX_AGAIN() FE_CTX_HELPER

#define FOR_EACH_SEP_CTX(sep, m, ctx, ...) __VA_OPT__(EXPAND(FE_SEP_CTX_HELPER(sep, m, ctx, __VA_ARGS__)))
#define FE_SEP_CTX_HELPER(sep, m, ctx, a1, ...)                                                                        \
    m(ctx, a1) __VA_OPT__(sep() FE_SEP_CTX_AGAIN PARENS(sep, m, ctx, __VA_ARGS__))
#define FE_SEP_CTX_AGAIN() FE_SEP_CTX_HELPER

#define MAKE_TUPLE(ctx, ...) (ctx, __VA_ARGS__)
#define MAKE_TUPLES(ctx, ...) FOR_EACH_SEP_CTX(COMMA, MAKE_TUPLE, ctx, __VA_ARGS__)

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

// =========================
// fields in FGbxEdgeDetection2PostProcessSettings
// =========================
#define GBX_ED2_FIELD GbxEdgeDetection2PostProcessSettings
#define GBX_ED2_PPS_FIELDS                                                                                             \
    EdgeDetectionType, EdgeDetection2StartFade, EdgeDetection2FadeDistance, EdgeDetection2SobelThickness,              \
        EdgeDetection2SobelThicknessOffset, EdgeDetection2SobelThinessOffset, EdgeDetection2FarDistance,               \
        EdgeDetection2DarkThreshold, EdgeDetection2HighlightThreshold, EdgeDetection2SobelDarkEdgeFadePower,           \
        EdgeDetection2GlobalInkChannelStrength, EdgeDetection2ThresholdRampStartDistance,                              \
        EdgeDetection2ThresholdRampTransitionDistance, EdgeDetection2SobelHighlightEdgeFadePower,                      \
        EdgeDetection2EvCurveExponent, EdgeDetection2EdgeHighlightThreshLow, EdgeDetection2EdgeHighlightThreshHigh,    \
        EdgeDetection2EdgeHighlightMaskExponent, EdgeDetection2HighlightThreshLow, EdgeDetection2HighlightThreshHigh,  \
        EdgeDetection2HighlightMaskExponent, EdgeDetection2HotspotThreshLow, EdgeDetection2HotspotThreshHigh,          \
        EdgeDetection2HotspotMaskExponent, EdgeDetection2EdgeHighlightDiffuseFactor,                                   \
        EdgeDetection2EdgeHighlightSourceIntensityLow, EdgeDetection2EdgeHighlightSourceIntensityHigh,                 \
        EdgeDetection2EdgeHighlightSupersaturation, EdgeDetection2HighlightDiffuseFactor,                              \
        EdgeDetection2HighlightSourceIntensity, EdgeDetection2HotspotDiffuseFactor,                                    \
        EdgeDetection2HotspotsSourceIntensity, EdgeDetection2EdgeHighlightChannelStrength,                             \
        EdgeDetection2HotspotChannelStrength, EdgeDetection2HighlightDesat, EdgeDetection2HighlightHueShift,           \
        EdgeDetection2ExteriorDepthCutoff, EdgeDetection2HighlightChannelStrength, EdgeDetection2VisualizeInks
#define GBX_ED2_PPS_TUPLES MAKE_TUPLES(GBX_ED2_FIELD, GBX_ED2_PPS_FIELDS)

// =========================
// fields in FGbxEdgeDetectionPostProcessSettings
// =========================
#define GBX_ED_FIELD GbxEdgeDetectionPostProcessSettings
#define GBX_ED_PPS_FIELDS                                                                                              \
    EdgeDetectionEnable, EdgeDetectionHFilterAxisCoeff, EdgeDetectionHFilterDiagCoeff, EdgeDetectionVFilterAxisCoeff,  \
        EdgeDetectionVFilterDiagCoeff, EdgeDetectionFarDistance, EdgeDetectionNearDistance, EdgeDetectionSobelPower,   \
        EdgeDetectionTexelOffset, EdgeDetectionTransitionDistance, EdgeDetectionTransitionDistanceFar,                 \
        EdgeDetectionApplyThreshold, EdgeDerivativeCheckLimit, EdgeDerivativeDeltaThreshold
#define GBX_ED_PPS_TUPLES MAKE_TUPLES(GBX_ED_FIELD, GBX_ED_PPS_FIELDS)

// =========================
// fields in FGbxOutlinePostProcessSettings
// =========================
#define GBX_OUTLINE_FIELD GbxOutlinePostProcessSettings
#define GBX_OUTLINE_PPS_FIELDS                                                                                         \
    OutlineTechEnable, OutlineStencilTestEnable, OutlineThickness, OutlineMinDistance, OutlineFadeDistance,            \
        OutlineDistanceThreshold, OutlineColorThreshold, OutlineAlphaClipValue
#define GBX_OUTLINE_PPS_TUPLES MAKE_TUPLES(GBX_OUTLINE_FIELD, GBX_OUTLINE_PPS_FIELDS)

// =========================
// fields in FGbxKuwaharaPostProcessSettings
// =========================
#define GBX_KUWAHARA_FIELD GbxKuwaharaPostProcessSettings
#define GBX_KUWAHARA_PPS_FIELDS                                                                                        \
    KuwaharaEnable, Hardness, GaussianBlurEnable, IgnoreLowerKernelSize, KernelDiffLimitRatio
#define GBX_KUWAHARA_PPS_TUPLES MAKE_TUPLES(GBX_KUWAHARA_FIELD, GBX_KUWAHARA_PPS_FIELDS)

// =========================
// fields in FGbxRenderPostProcessSettings
// =========================
#define GBX_RENDER_FIELD GbxRenderPostProcessSettings
#define GBX_RENDER_PPS_FIELDS                                                                                          \
    WPODisableDistanceScale, ShadowWPODisableDistanceScale, WPOFading_PercentageDistance,                              \
        AccelerateVirtualTextureStreaming, ForegroundShadowProjectionMode
#define GBX_RENDER_PPS_TUPLES MAKE_TUPLES(GBX_RENDER_FIELD, GBX_RENDER_PPS_FIELDS)

#define GBX_PPS_TUPLES GBX_ED2_PPS_TUPLES, GBX_ED_PPS_TUPLES, GBX_OUTLINE_PPS_TUPLES, GBX_KUWAHARA_PPS_TUPLES, GBX_RENDER_PPS_TUPLES
