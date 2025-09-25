# Post Process Settings (Grouped by Semantics)

## Ambient Cubemap

| name                    | type  | min | max | info                                                                      |
|-------------------------|-------|-----|-----|---------------------------------------------------------------------------|
| AmbientCubemapIntensity | float | 0   | 4   | To scale the Ambient cubemap brightness >=0: off, 1(default), >1 brighter |

## Ambient Occlusion

| name                                | type  | min | max   | info                                                                                                                                                                 |
|-------------------------------------|-------|-----|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| AmbientOcclusionBias                | float | 0   | 10    | in unreal units, default (3.0) works well for flat surfaces but can reduce details                                                                                   |
| AmbientOcclusionDistance            | float |     |       |                                                                                                                                                                      |
| AmbientOcclusionFadeDistance        | float | 0   | 20000 | in unreal units, at what distance the AO effect disppears in the distance (avoding artifacts and AO effects on huge object)                                          |
| AmbientOcclusionFadeRadius          | float | 0   | 20000 | in unreal units, how many units before AmbientOcclusionFadeOutDistance it starts fading out                                                                          |
| AmbientOcclusionIntensity           | float | 0   | 1     | 0..1 0=off/no ambient occlusion .. 1=strong ambient occlusion, defines how much it affects the non direct lighting after base pass                                   |
| AmbientOcclusionMipBlend            | float | 0.1 | 1     | Affects the blend over the multiple mips (lower resolution versions) , 0:fully use full resolution, 1::fully use low resolution, around 0.6 seems to be a good value |
| AmbientOcclusionMipScale            | float | 0.5 | 4     | Affects the radius AO radius scale over the multiple mips (lower resolution versions)                                                                                |
| AmbientOcclusionMipThreshold        | float | 0   | 0.1   | to tweak the bilateral upsampling when using multiple mips (lower resolution versions)                                                                               |
| AmbientOcclusionPower               | float | 0.1 | 8     | in unreal units, bigger values means even distant surfaces affect the ambient occlusion                                                                              |
| AmbientOcclusionQuality             | float | 0   | 100   | 0=lowest quality..100=maximum quality, only a few quality levels are implemented, no soft transition                                                                 |
| AmbientOcclusionRadius              | float | 0.1 | 500   | in unreal units, bigger values means even distant surfaces affect the ambient occlusion                                                                              |
| AmbientOcclusionRadiusInWS          | uint8 | 0   | 1     | true: AO radius is in world space units, false: AO radius is locked the view space in 400 units                                                                      |
| AmbientOcclusionStaticFraction      | float | 0   | 1     | 0..1 0=no effect on static lighting .. 1=AO affects the stat lighting, 0 is free meaning no extra rendering pass                                                     |
| AmbientOcclusionTemporalBlendWeight | float | 0   | 0.5   | How much to blend the current frame with previous frames when using GTAO with temporal accumulation                                                                  |

## Auto Exposure

| name                                    | type  | min  | max | info                                                                                                                                                              |
|-----------------------------------------|-------|------|-----|-------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| AutoExposureApplyPhysicalCameraExposure | uint8 | 0    | 1   | Enables physical camera exposure using ShutterSpeed/ISO/Aperture.                                                                                                 |
| AutoExposureBias                        | float | -15  | 15  | Logarithmic adjustment for the exposure. Only used if a tonemapper is specified. 0: no adjustment, -1:2x darker, -2:4x darker, 1:2x brighter, 2:4x brighter, ...  |
| AutoExposureBiasBackup                  | float |      |     | With the auto exposure changes, we are changing the AutoExposureBias inside the serialization code. We are storing that value before conversion here as a backup. |
| AutoExposureCalibrationConstant         | float |      |     |                                                                                                                                                                   |
| AutoExposureHighPercent                 | float | 0    | 100 | The eye adaptation will adapt to a value extracted from the luminance histogram... good values 80..95                                                             |
| AutoExposureLowPercent                  | float | 0    | 100 | The eye adaptation will adapt to a value extracted from the luminance histogram... good values 70..80                                                             |
| AutoExposureMaxBrightness               | float | -10  | 20  | Auto-Exposure maximum adaptation. Eye Adaptation is disabled if Min = Max.                                                                                        |
| AutoExposureMethod                      | uint8 | 0    | 2   | Luminance computation method (AEM_Histogram=0, AEM_Basic=1, AEM_Manual=2)                                                                                         |
| AutoExposureMinBrightness               | float | -10  | 20  | Auto-Exposure minimum adaptation. Eye Adaptation is disabled if Min = Max.                                                                                        |
| AutoExposureSpeedDown                   | float | 0.02 | 20  |                                                                                                                                                                   |
| AutoExposureSpeedUp                     | float | 0.02 | 20  |                                                                                                                                                                   |
| HistogramLogMax                         | float | 0    | 16  | Histogram Max value. Expressed in Log2(Luminance) or in EV100 when using ExtendDefaultLuminanceRange                                                              |
| HistogramLogMin                         | float | -16  | 0   | Histogram Min value. Expressed in Log2(Luminance) or in EV100 when using ExtendDefaultLuminanceRange                                                              |

## Bloom

| name                              | type  | min | max | info                                                                                             |
|-----------------------------------|-------|-----|-----|--------------------------------------------------------------------------------------------------|
| Bloom1Size                        | float | 0   | 4   | Diameter size for Bloom1 in % screen width (1/2 res)                                             |
| Bloom2Size                        | float | 0   | 8   | Diameter size for Bloom2 in % screen width (1/4 res)                                             |
| Bloom3Size                        | float | 0   | 16  | Diameter size for Bloom3 in % screen width (1/8 res)                                             |
| Bloom4Size                        | float | 0   | 32  | Diameter size for Bloom4 in % screen width (1/16 res)                                            |
| Bloom5Size                        | float | 0   | 64  | Diameter size for Bloom5 in % screen width (1/32 res)                                            |
| Bloom6Size                        | float | 0   | 128 | Diameter size for Bloom6 in % screen width (1/64 res)                                            |
| BloomConvolutionBufferScale       | float | 0   | 1   | Implicit buffer region as a fraction of the screen size to avoid wrap                            |
| BloomConvolutionPreFilterMax      | float |     |     | Boost intensity of select pixels prior to convolution (Min, Max, Multiplier). Max < Min disables |
| BloomConvolutionPreFilterMin      | float |     |     | Boost intensity of select pixels prior to convolution (Min, Max, Multiplier). Max < Min disables |
| BloomConvolutionPreFilterMult     | float |     |     | Boost intensity of select pixels prior to convolution (Min, Max, Multiplier). Max < Min disables |
| BloomConvolutionScatterDispersion | float | 0   | 20  | Intensity multiplier on the scatter dispersion energy of the kernel                              |
| BloomConvolutionSize              | float | 0   | 1   | Relative size of the convolution kernel image vs viewport minor axis                             |
| BloomDirtMaskIntensity            | float | 0   | 8   | BloomDirtMask intensity                                                                          |
| BloomIntensity                    | float | 0   | 8   | Multiplier for all bloom contributions                                                           |
| BloomMethod                       | uint8 | 0   | 1   | Bloom algorithm (BM_SOG=0, BM_FFT=1)                                                             |
| BloomSizeScale                    | float | 0   | 64  | Scale for all bloom sizes                                                                        |
| BloomThreshold                    | float | -1  | 8   | Minimum brightness where bloom starts to have effect                                             |

## Camera & White Balance

| name               | type  | min  | max   | info                                                        |
|--------------------|-------|------|-------|-------------------------------------------------------------|
| CameraISO          | float | 1    |       | The camera sensor sensitivity in ISO.                       |
| CameraShutterSpeed | float | 1    | 2000  | The camera shutter in seconds.                              |
| TemperatureType    | uint8 | 0    | 1     | WhiteBalance=0, ColorTemperature=1                          |
| WhiteTemp          | float | 1500 | 15000 | Controls the color temperature (Kelvin) considered as white |
| WhiteTint          | float | -1   | 1     | Magenta-Green axis tint (orthogonal to temperature)         |

## Chromatic Aberration

| name                           | type  | min | max | info                                                               |
|--------------------------------|-------|-----|-----|--------------------------------------------------------------------|
| ChromaticAberrationStartOffset | float | 0   | 1   | Normalized distance to framebuffer center where effect takes place |
| SceneFringeIntensity           | float | 0   | 5   | % chromatic aberration to simulate lens artifact                   |

## Color Grading & Tone Mapping

| name                         | type  | min | max | info                                                            |
|------------------------------|-------|-----|-----|-----------------------------------------------------------------|
| BlueCorrection               | float | 0   | 1   | Correct electric blues in ACEScg (bright blue desaturates)      |
| ColorCorrectionHighlightsMax | float | 1   | 10  | Upper threshold for highlights; should be > HighlightsMin       |
| ColorCorrectionHighlightsMin | float | -1  | 1   | Lower threshold for highlights                                  |
| ColorCorrectionShadowsMax    | float | -1  | 1   | Threshold for shadows                                           |
| ColorGradingIntensity        | float | 0   | 1   | LUT intensity (0..1)                                            |
| ExpandGamut                  | float | 0   | 1   | Expand bright saturated colors outside sRGB to fake wide gamut  |
| FilmBlackClip                | float | 0   | 1   | Lowers the toe; increasing clips more to black                  |
| FilmGrainHighlightsMax       | float | 1   | 10  | Upper bound for Film Grain Highlight Intensity (> Min)          |
| FilmGrainHighlightsMin       | float | 0   | 1   | Lower bound for Film Grain Highlight Intensity                  |
| FilmGrainIntensity           | float | 0   | 1   | Film grain intensity (lerp with decoded texture)                |
| FilmGrainIntensityHighlights | float | 0   | 1   | Grain intensity in highlight regions                            |
| FilmGrainIntensityMidtones   | float | 0   | 1   | Grain intensity in mid-tones                                    |
| FilmGrainIntensityShadows    | float | 0   | 1   | Grain intensity in shadow regions                               |
| FilmGrainShadowsMax          | float | 0   | 1   | Upper bound for Film Grain Shadow Intensity                     |
| FilmGrainTexelSize           | float | 0   | 4   | Size of film grain texel on screen                              |
| FilmShoulder                 | float | 0   | 1   |                                                                 |
| FilmSlope                    | float | 0   | 1   | Controls overall steepness of tonemapper curve (contrast)       |
| FilmToe                      | float | 0   | 1   | Controls contrast of dark end of tonemapper curve               |
| FilmWhiteClip                | float | 0   | 1   | Controls height of tonemapper curve (white saturation)          |
| Sharpen                      | float | 0   | 10  | Strength of sharpening applied during tonemapping               |
| ToneCurveAmount              | float | 0   | 1   | Reduce effect of Tone Curve (set with ExpandGamut=0 to disable) |

## Depth of Field

| name                                       | type  | min   | max    | info                                                                      |
|--------------------------------------------|-------|-------|--------|---------------------------------------------------------------------------|
| DepthOfFieldBladeCount                     | int32 | 4     | 16     | Number of diaphragm blades (4..16)                                        |
| DepthOfFieldDepthBlurAmount                | float | 1e-06 | 100    | CircleDOF only: Depth blur km for 50%                                     |
| DepthOfFieldDepthBlurRadius                | float | 0     | 4      | CircleDOF only: Depth blur radius in pixels at 1920x                      |
| DepthOfFieldFarBlurSize                    | float | 0     | 32     | Gaussian only: Max blur size (% of view width)                            |
| DepthOfFieldFarTransitionRegion            | float | 0     | 10000  | Width of transition region next to focal region (near side)               |
| DepthOfFieldFocalDistance                  | float | 0     | 10000  | Distance where DoF is sharp (cm)                                          |
| DepthOfFieldFocalRegion                    | float | 0     | 10000  | Artificial region starting after focal distance where content is in focus |
| DepthOfFieldFstop                          | float | 1     | 32     | Lens opening; larger opening -> stronger DoF                              |
| DepthOfFieldMinFstop                       | float | 0     | 32     | Max opening (controls blade curvature); 0 for straight blades             |
| DepthOfFieldNearBlurSize                   | float | 0     | 32     | Gaussian only: Max blur size (% of view width)                            |
| DepthOfFieldNearTransitionRegion           | float | 0     | 10000  | Width of transition region next to focal region (near side)               |
| DepthOfFieldOcclusion                      | float | 0     | 1      | Occlusion tweak factor (e.g., 0.18 natural, 0.4 fix leaking)              |
| DepthOfFieldScale                          | float | 0     | 2      | SM5 BokehDOF: amplify DoF; ES3_1: used to blend DoF                       |
| DepthOfFieldSensorWidth                    | float | 0.1   | 1000   | Camera sensor width (mm)                                                  |
| DepthOfFieldSkyFocusDistance               | float | 0     | 200000 | Artificial distance to keep skybox in focus (GaussianDOF)                 |
| DepthOfFieldSqueezeFactor                  | float | 1     | 2      | Anamorphic lens squeeze factor                                            |
| DepthOfFieldUseHairDepth                   | uint8 | 0     | 1      | Use hair depth for CoC size (else interpolate with scene depth)           |
| DepthOfFieldVignetteSize                   | float | 0     | 100    | Circular mask to blur outside radius (GaussianDOF)                        |
| GbxDepthOfFieldMaxBackgroundCocRadiusScale | float |       |        |                                                                           |

## Fog

| name                    | type  | min | max | info |
|-------------------------|-------|-----|-----|------|
| GbxFogDensityMultiplier | float |     |     |      |

## Global Illumination

| name                            | type  | min | max | info                                                                     |
|---------------------------------|-------|-----|-----|--------------------------------------------------------------------------|
| DynamicGlobalIlluminationMethod | uint8 | 0   | 3   | Chooses the Dynamic GI method (None=0, Lumen=1, ScreenSpace=2, Plugin=3) |
| IndirectLightingIntensity       | float | 0   | 4   | Scales indirect lighting; 0 disables GI                                  |

## Lens Flare

| name               | type  | min | max | info                                                             |
|--------------------|-------|-----|-----|------------------------------------------------------------------|
| LensFlareBokehSize | float | 0   | 32  | Size of Lens Blur (% of view width) using Bokeh texture          |
| LensFlareIntensity | float | 0   | 16  | Brightness scale of image caused lens flares                     |
| LensFlareThreshold | float | 0.1 | 32  | Minimum brightness for lens flares (set high to avoid perf cost) |

## Local Exposure

| name                                           | type  | min | max | info                                                                 |
|------------------------------------------------|-------|-----|-----|----------------------------------------------------------------------|
| LocalExposureBlurredLuminanceBlend             | float | 0   | 1   | Blend between bilateral filtered and blurred luminance as base layer |
| LocalExposureBlurredLuminanceKernelSizePercent | float | 0   | 100 | Kernel size (% of screen) to blur luminance                          |
| LocalExposureContrastScale                     | float |     |     |                                                                      |
| LocalExposureDetailStrength                    | float | 0   | 4   | Value != 1 enables local exposure; set 1 in most cases               |
| LocalExposureHighlightContrastScale            | float | 0   | 1   | Reduce base layer contrast (highlights). <1 enables local exposure   |
| LocalExposureHighlightThreshold                | float | 0   | 4   | Threshold for highlight regions                                      |
| LocalExposureMethod                            | uint8 | 0   | 1   | Algorithm (Bilateral=0, Fusion=1)                                    |
| LocalExposureMiddleGreyBias                    | float | -15 | 15  | Log adjustment for local exposure middle grey                        |
| LocalExposureShadowContrastScale               | float | 0   | 1   | Reduce base layer contrast (shadows). <1 enables local exposure      |
| LocalExposureShadowThreshold                   | float | 0   | 4   | Threshold for shadow regions                                         |

## Lumen

| name                                   | type  | min  | max     | info                                                                                                |
|----------------------------------------|-------|------|---------|-----------------------------------------------------------------------------------------------------|
| LumenFinalGatherLightingUpdateSpeed    | float | 0.5  | 4       | How much Final Gather may cache/update lighting (perf vs propagation)                               |
| LumenFinalGatherQuality                | float | 0.25 | 2       | Final Gather quality scale (higher -> less noise, more cost)                                        |
| LumenFinalGatherScreenTraces           | uint8 | 0    | 1       | Use screen space traces for GI (bypasses Lumen Scene)                                               |
| LumenFrontLayerTranslucencyReflections | uint8 | 0    | 1       | High quality mirror reflections on front translucent layer                                          |
| LumenFullSkylightLeakingDistance       | float | 0.1  | 2000    | Distance where skylight leaking reaches full intensity                                              |
| LumenMaxReflectionBounces              | int32 | 1    | 8       | Max recursive reflection bounces (hardware RT with Hit Lighting)                                    |
| LumenMaxRefractionBounces              | int32 | 0    | 64      | Max number of refraction events to trace                                                            |
| LumenMaxRoughnessToTraceReflections    | float | 0    | 1       | Max roughness for dedicated reflection rays                                                         |
| LumenMaxTraceDistance                  | float | 1    | 2097152 | Max distance to trace while solving lighting                                                        |
| LumenRayLightingMode                   | uint8 | 0    | 3       | Lighting mode for HWRT rays (Default=0, SurfaceCache=1, HitLightingForReflections=2, HitLighting=3) |
| LumenReflectionQuality                 | float | 0.25 | 2       | Reflection quality scale (higher -> less noise, more cost)                                          |
| LumenReflectionsScreenTraces           | uint8 | 0    | 1       | Use screen space traces for reflections                                                             |
| LumenSceneDetail                       | float | 0.25 | 4       | Size of instances represented in Lumen Scene                                                        |
| LumenSceneLightingQuality              | float | 0.25 | 2       | Lumen Scene quality scale                                                                           |
| LumenSceneLightingUpdateSpeed          | float | 0.5  | 4       | How much Lumen Scene may cache/update lighting                                                      |
| LumenSceneViewDistance                 | float | 1    | 2097152 | Max view distance maintained for ray tracing                                                        |
| LumenSkylightLeaking                   | float | 0    | 0.02    | Fraction of skylight intensity allowed to leak                                                      |
| LumenSurfaceCacheResolution            | float | 0.5  | 1       | Surface Cache resolution scale (Scene Capture)                                                      |

## MegaLights

| name        | type  | min | max | info                                                                   |
|-------------|-------|-----|-----|------------------------------------------------------------------------|
| bMegaLights | uint8 | 0   | 1   | Force MegaLights on/off for this volume; requires HW Ray Tracing & SM6 |

## Motion Blur

| name                             | type  | min | max | info                                                              |
|----------------------------------|-------|-----|-----|-------------------------------------------------------------------|
| MotionBlurAmount                 | float | 0   | 1   | Strength (0: off)                                                 |
| MotionBlurDisableCameraInfluence | uint8 | 0   | 1   |                                                                   |
| MotionBlurMax                    | float | 0   | 100 | Max distortion in % of screen width (0: off)                      |
| MotionBlurPerObjectSize          | float | 0   | 100 | Min projected screen radius (% of screen width) for velocity pass |
| MotionBlurTargetFPS              | int32 | 0   | 120 | Target FPS for motion blur; 0 = dependent on actual FPS           |

## Path Tracing

| name                                 | type  | min | max   | info                                                      |
|--------------------------------------|-------|-----|-------|-----------------------------------------------------------|
| PathTracingEnableDenoiser            | uint8 | 0   | 1     | Run denoiser plugin on last sample (if loaded)            |
| PathTracingEnableEmissiveMaterials   | uint8 | 0   | 1     | Should emissive materials contribute to lighting?         |
| PathTracingEnableReferenceAtmosphere | uint8 | 0   | 1     | Path trace the atmosphere (skylight ignored when enabled) |
| PathTracingEnableReferenceDOF        | uint8 | 0   | 1     | Reference-quality DOF replaces post-process DOF           |
| PathTracingIncludeDiffuse            | uint8 | 0   | 1     | Include diffuse lighting contributions                    |
| PathTracingIncludeEmissive           | uint8 | 0   | 1     | Include directly visible emissive elements                |
| PathTracingIncludeIndirectDiffuse    | uint8 | 0   | 1     | Include indirect diffuse                                  |
| PathTracingIncludeIndirectSpecular   | uint8 | 0   | 1     | Include indirect specular                                 |
| PathTracingIncludeIndirectVolume     | uint8 | 0   | 1     | Include indirect volume lighting                          |
| PathTracingIncludeSpecular           | uint8 | 0   | 1     | Include specular                                          |
| PathTracingIncludeVolume             | uint8 | 0   | 1     | Include volume lighting                                   |
| PathTracingMaxBounces                | int32 | 0   | 100   | Max path bounces                                          |
| PathTracingMaxPathIntensity          | float | 1   | 65504 | Clamp maximum indirect sample intensity (fireflies)       |
| PathTracingSamplesPerPixel           | int32 | 1   | 65536 | Samples per pixel                                         |

## Ray Tracing

| name                                  | type  | min  | max   | info                                                             |
|---------------------------------------|-------|------|-------|------------------------------------------------------------------|
| RayTracingAO                          | uint8 | 0    | 1     | Enable ray tracing AO                                            |
| RayTracingAOIntensity                 | float | 0    | 1     | Scalar factor on RT AO score                                     |
| RayTracingAORadius                    | float | 0    | 10000 | World-space search radius for occlusion rays                     |
| RayTracingAOSamplesPerPixel           | int32 | 1    | 65536 | SPP for RT AO                                                    |
| RayTracingTranslucencyMaxRoughness    | float | 0.01 | 1     | Max roughness for visible RT translucency (fades near threshold) |
| RayTracingTranslucencyRefraction      | uint8 | 0    | 1     | Enable refraction (else rays continue in same direction)         |
| RayTracingTranslucencyRefractionRays  | int32 | 0    | 50    | Max refraction rays                                              |
| RayTracingTranslucencySamplesPerPixel | int32 | 1    | 65536 | SPP for RT translucency                                          |
| RayTracingTranslucencyShadows         | uint8 | 0    | 2     | Translucency shadows type (Disabled=0, Hard=1, Area=2)           |

## Reflections

| name                              | type  | min  | max | info                                                    |
|-----------------------------------|-------|------|-----|---------------------------------------------------------|
| ReflectionMethod                  | uint8 | 0    | 2   | Reflection method (None=0, Lumen=1, ScreenSpace=2)      |
| ScreenSpaceReflectionIntensity    | float | 0    | 100 | Enable/Fade/disable SSR (%); avoid 0..1 for consistency |
| ScreenSpaceReflectionMaxRoughness | float | 0.01 | 1   | Until what roughness we fade SSR (e.g., 0.8)            |
| ScreenSpaceReflectionQuality      | float | 0    | 100 | 0=lowest..100=maximum quality; 50 default for perf      |

## Resolution

| name             | type  | min | max | info                           |
|------------------|-------|-----|-----|--------------------------------|
| ScreenPercentage | float |     |     | Rendering resolution scale (%) |

## Translucency

| name             | type  | min | max | info                                       |
|------------------|-------|-----|-----|--------------------------------------------|
| TranslucencyType | uint8 | 0   | 1   | Translucency type (Raster=0, RayTracing=1) |

## Vignette

| name              | type  | min | max | info                            |
|-------------------|-------|-----|-----|---------------------------------|
| VignetteIntensity | float | 0   | 1   | 0..1 0=off .. 1=strong vignette |

