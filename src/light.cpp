#include "light.h"
#include "bvh_interface.h"
#include "config.h"
#include "draw.h"
#include "intersect.h"
#include "render.h"
#include "scene.h"
#include "shading.h"
#include <iostream>
// Suppress warnings in third-party code.
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/geometric.hpp>
DISABLE_WARNINGS_POP()


// TODO: Standard feature
// Given a single segment light, transform a uniformly distributed 1d sample in [0, 1),
// into a uniformly sampled position and an interpolated color on the segment light,
// and write these into the reference return values.
// - sample;    a uniformly distributed 1d sample in [0, 1)
// - light;     the SegmentLight object, see `common.h`
// - position;  reference return value of the sampled position on the light
// - color;     reference return value of the color emitted by the light at the sampled position
// This method is unit-tested, so do not change the function signature.
void sampleSegmentLight(const float& sample, const SegmentLight& light, glm::vec3& position, glm::vec3& color)
{
    // Calculate the sampled position along the segment
    position = light.endpoint0 + sample * (light.endpoint1 - light.endpoint0);

    // Interpolate the color along the segment
    color = glm::mix(light.color0, light.color1, sample);
}

// TODO: Standard feature
// Given a single paralellogram light, transform a uniformly distributed 2d sample in [0, 1),
// into a uniformly sampled position and interpolated color on the paralellogram light,
// and write these into the reference return values.
// - sample;   a uniformly distributed 2d sample in [0, 1)
// - light;    the ParallelogramLight object, see `common.h`
// - position; reference return value of the sampled position on the light
// - color;    reference return value of the color emitted by the light at the sampled position
// This method is unit-tested, so do not change the function signature.
void sampleParallelogramLight(const glm::vec2& sample, const ParallelogramLight& light, glm::vec3& position, glm::vec3& color)
{
    // Calculate the sampled position within the parallelogram
    glm::vec3 u = light.edge01;
    glm::vec3 v = light.edge02;
    position = light.v0 + (u * sample.x + v * sample.y);

    // Interpolate the color within the parallelogram
    glm::vec3 color0 = light.color0;
    glm::vec3 color1 = light.color1;
    glm::vec3 color2 = light.color2;
    glm::vec3 color3 = light.color3;

    color = color0 * (1.0f - sample.x) * (1.0f - sample.y) + color1 * sample.x * (1.0f - sample.y) + color2 * (1.0f - sample.x) * sample.y + color3 * sample.x * sample.y;
}

// TODO: Standard feature
// Given a sampled position on some light, and the emitted color at this position, return whether
// or not the light is visible from the provided ray/intersection.
// For a description of the method's arguments, refer to 'light.cpp'
// - state;         the active scene, feature config, and the bvh
// - lightPosition; the sampled position on some light source
// - lightColor;    the sampled color emitted at lightPosition
// - ray;           the incident ray to the current intersection
// - hitInfo;       information about the current intersection
// - return;        whether the light is visible (true) or not (false)
// This method is unit-tested, so do not change the function signature.
bool visibilityOfLightSampleBinary(RenderState& state, const glm::vec3& lightPosition, const glm::vec3 &lightColor, const Ray& ray, const HitInfo& hitInfo)
{
        // Shadows are enabled in the renderer
        // Create a shadow ray from the intersection point to the light sample
        Ray shadowRay;
        shadowRay.origin = ray.origin + ray.direction * ray.t + 0.0001f; // Offset from the intersection point
        shadowRay.direction = glm::normalize(lightPosition - shadowRay.origin);
        // Adjust the shadow bias here (e.g., 0.001f to 0.01f) and observe the result
        const float shadowBias = 0.0001f;

        // Check for intersections along the shadow ray
        HitInfo shadowHitInfo;
        bool isShadowed = state.bvh.intersect(state, shadowRay, shadowHitInfo);
        //std::cout << "isFalse: " << isShadowed << std::endl;

        // Ensure that the shadow ray's t value is within a valid range
        if (isShadowed)
            {
            
            drawRay(shadowRay, glm::vec3 { 0, 0.0f, 1.0f });
            // Shadowed if the ray hits an object between the intersection point and the light source
            return false;
        } else {
            drawRay(shadowRay, glm::vec3 { 0, 1.0f, 0.0f });
        }

        // Not shadowed if the shadow ray doesn't hit an object or hits an object beyond the light source
        return true;
}

// TODO: Standard feature
// Given a sampled position on some light, and the emitted color at this position, return the actual
// light that is visible from the provided ray/intersection, or 0 if this is not the case.
// Use the following blending operation: lightColor = lightColor * kd * (1 - alpha)
// Please reflect within 50 words in your report on why this is incorrect, and illustrate
// two examples of what is incorrect.
//
// - state;         the active scene, feature config, and the bvh
// - lightPosition; the sampled position on some light source
// - lightColor;    the sampled color emitted at lightPosition
// - ray;           the incident ray to the current intersection
// - hitInfo;       information about the current intersection
// - return;        the visible light color that reaches the intersection
//
// This method is unit-tested, so do not change the function signature.
glm::vec3 visibilityOfLightSampleTransparency(RenderState& state, const glm::vec3& lightPosition, const glm::vec3& lightColor, const Ray& ray, const HitInfo& hitInfo)
{
  
        glm::vec3 intersectionPoint = ray.origin + ray.direction * ray.t + 0.0001f;

        // Shadows are enabled in the renderer
        // Create a shadow ray from the intersection point to the light sample
        Ray shadowRay;
        shadowRay.origin = ray.origin + ray.direction * ray.t + 0.001f; // Offset from the intersection point
        shadowRay.direction = glm::normalize(lightPosition - shadowRay.origin);
        glm::vec3 lightDirection = glm::normalize(lightPosition - intersectionPoint);

        // Check for intersections along the shadow ray
        HitInfo shadowHitInfo;
        bool isShadowed = state.bvh.intersect(state, shadowRay, shadowHitInfo);

        if (isShadowed){

           return lightColor * hitInfo.material.kd * (1 - hitInfo.material.transparency);
        } else {
            // If the shadow ray doesn't hit an object, return the light color
            return lightColor;
        }
    
}

// TODO: Standard feature
// Given a single point light, compute its contribution towards an incident ray at an intersection point.
//
// Hint: you should use `visibilityOfLightSample()` to account for shadows, and if the light is visible, use
//       the result of `computeShading()`, whose submethods you should probably implement first in `shading.cpp`.
//
// - state;   the active scene, feature config, bvh, and a thread-safe sampler
// - light;   the PointLight object, see `common.h`
// - ray;     the incident ray to the current intersection
// - hitInfo; information about the current intersection
// - return;  reflected light along the incident ray, based on `computeShading()`
//
// This method is unit-tested, so do not change the function signature.
glm::vec3 computeContributionPointLight(RenderState& state, const PointLight& light, const Ray& ray, const HitInfo& hitInfo)
{
    // Calculate the intersection point
    glm::vec3 intersectionPoint = ray.origin + ray.direction * ray.t + 0.0001f;

    // Calculate the light direction from the intersection point to the light source
    glm::vec3 lightDirection = glm::normalize(light.position - intersectionPoint);
    Ray shadowRay;
    shadowRay.origin = intersectionPoint + 0.001f; // Add a small bias
    shadowRay.direction = glm::normalize(light.position - shadowRay.origin);
    // Check if the light is visible from the intersection point
    bool isLightVisible = visibilityOfLightSampleBinary(state, light.position, light.color, ray, hitInfo);

   if (isLightVisible) {
            if (hitInfo.material.transparency < 1.0f) {
                // If the material is transparent, compute transparent shadows
                glm::vec3 transparentShadowColor = visibilityOfLightSampleTransparency(state, light.position, light.color, shadowRay, hitInfo);

                // Calculate shading with the Phong model
                glm::vec3 viewDirection = -ray.direction;
                glm::vec3 normal = hitInfo.normal;

                // Compute the shading result with the Phong model
                glm::vec3 shadingResult = computeShading(state, viewDirection, lightDirection, light.color, hitInfo);

                // Final contribution is the blending of shading and transparent shadows
                return (shadingResult * (1.0f - hitInfo.material.transparency) + transparentShadowColor);
            } else {
                // If the material is not transparent, calculate shading with the Phong model
                glm::vec3 viewDirection = -ray.direction;
                glm::vec3 normal = hitInfo.normal;

                // Compute the shading result with the Phong model
                glm::vec3 shadingResult = computeShading(state, viewDirection, lightDirection, light.color, hitInfo);

                // Final contribution is the shading result multiplied by the light intensity
                return shadingResult;
            }
    } else {
            // If the light is not visible, there is no contribution
            return glm::vec3(0.0f);
    }

}

// TODO: Standard feature
// Given a single segment light, compute its contribution towards an incident ray at an intersection point
// by integrating over the segment, taking `numSamples` samples from the light source.
//
// Hint: you can sample the light by using `sampleSegmentLight(state.sampler.next_1d(), ...);`, which
//       you should implement first.
// Hint: you should use `visibilityOfLightSample()` to account for shadows, and if the sample is visible, use
//       the result of `computeShading()`, whose submethods you should probably implement first in `shading.cpp`.
//
// - state;      the active scene, feature config, bvh, and a thread-safe sampler
// - light;      the SegmentLight object, see `common.h`
// - ray;        the incident ray to the current intersection
// - hitInfo;    information about the current intersection
// - numSamples; the number of samples you need to take
// - return;     accumulated light along the incident ray, based on `computeShading()`
//
// This method is unit-tested, so do not change the function signature.
glm::vec3 computeContributionSegmentLight(RenderState& state, const SegmentLight& light, const Ray& ray, const HitInfo& hitInfo, uint32_t numSamples)
{
    glm::vec3 accumulatedLight = glm::vec3(0);

    for (uint32_t i = 0; i < numSamples; ++i) {
        // Sample the segment light to get a position and color
        glm::vec3 lightPosition, lightColor;
        sampleSegmentLight(state.sampler.next_1d(), light, lightPosition, lightColor);

        // Calculate the direction from the intersection point to the light
        glm::vec3 intersectionPoint = ray.origin + ray.direction * ray.t;
        glm::vec3 lightDirection = glm::normalize(lightPosition - intersectionPoint);

        // Create a shadow ray from the intersection point to the sampled light position
        Ray shadowRay;
        shadowRay.origin = intersectionPoint + 0.001f * hitInfo.normal; // Add a small bias
        shadowRay.direction = lightDirection;
        // Test the visibility of the light sample
        bool isLightVisible = visibilityOfLightSampleBinary(state, lightPosition, lightColor, shadowRay, hitInfo);

        if (isLightVisible) {
            // Calculate shading using the Phong model and accumulate it
            glm::vec3 viewDirection = -ray.direction;
            glm::vec3 normal = hitInfo.normal;
            glm::vec3 shadingResult = computeShading(state, viewDirection, lightDirection, lightColor, hitInfo);
            accumulatedLight += shadingResult;
        } else {
        }
    }

    // Normalize the accumulated light by the number of samples
    if (numSamples > 0) {
        accumulatedLight /= static_cast<float>(numSamples);
    }

    return accumulatedLight;
}

// TODO: Standard feature
// Given a single parralelogram light, compute its contribution towards an incident ray at an intersection point
// by integrating over the parralelogram, taking `numSamples` samples from the light source, and applying
// shading.
//
// Hint: you can sample the light by using `sampleParallelogramLight(state.sampler.next_1d(), ...);`, which
//       you should implement first.
// Hint: you should use `visibilityOfLightSample()` to account for shadows, and if the sample is visible, use
//       the result of `computeShading()`, whose submethods you should probably implement first in `shading.cpp`.
//
// - state;      the active scene, feature config, bvh, and a thread-safe sampler
// - light;      the ParallelogramLight object, see `common.h`
// - ray;        the incident ray to the current intersection
// - hitInfo;    information about the current intersection
// - numSamples; the number of samples you need to take
// - return;     accumulated light along the incident ray, based on `computeShading()`
//
// This method is unit-tested, so do not change the function signature.
glm::vec3 computeContributionParallelogramLight(RenderState& state, const ParallelogramLight& light, const Ray& ray, const HitInfo& hitInfo, uint32_t numSamples)
{

    glm::vec3 accumulatedLight = glm::vec3(0);

    for (uint32_t i = 0; i < numSamples; ++i) {
        // Sample the parallelogram light to get a position and color
        glm::vec3 lightPosition, lightColor;
        sampleParallelogramLight(state.sampler.next_2d(), light, lightPosition, lightColor);

        // Calculate the direction from the intersection point to the light
        glm::vec3 intersectionPoint = ray.origin + ray.direction * ray.t + 0.001f;
        glm::vec3 lightDirection = glm::normalize(lightPosition - intersectionPoint);

        // Create a shadow ray from the intersection point to the sampled light position
        Ray shadowRay;
        shadowRay.origin = intersectionPoint; // Add a small bias
        shadowRay.direction = glm::normalize(lightPosition - shadowRay.origin);

        // Test the visibility of the light sample
        bool isLightVisible = visibilityOfLightSampleBinary(state, lightPosition, lightColor, shadowRay, hitInfo);

        if (isLightVisible) {
            drawRay(shadowRay, glm::vec3 { 0, 0, 1.0f });
            // Calculate shading using the Phong model and accumulate it
            glm::vec3 viewDirection = -ray.direction;
            glm::vec3 normal = hitInfo.normal;
            glm::vec3 shadingResult = computeShading(state, viewDirection, lightDirection, lightColor, hitInfo);
            accumulatedLight += shadingResult;
        } else {
            drawRay(shadowRay, glm::vec3 { 0, 1, 0.0f });

        }
    }

    // Normalize the accumulated light by the number of samples
    if (numSamples > 0) {
        accumulatedLight /= static_cast<float>(numSamples);
    }

    return accumulatedLight;
}

// This function is provided as-is. You do not have to implement it.
// Given a sampled position on some light, and the emitted color at this position, return the actual
// light that is visible from the provided ray/intersection, or 0 if this is not the case.
// This forowards to `visibilityOfLightSampleBinary`/`visibilityOfLightSampleTransparency` based on settings.
//
// - state;         the active scene, feature config, and the bvh
// - lightPosition; the sampled position on some light source
// - lightColor;    the sampled color emitted at lightPosition
// - ray;           the incident ray to the current intersection
// - hitInfo;       information about the current intersection
// - return;        the visible light color that reaches the intersection
//
// This method is unit-tested, so do not change the function signature.
glm::vec3 visibilityOfLightSample(RenderState& state, const glm::vec3& lightPosition, const glm::vec3& lightColor, const Ray& ray, const HitInfo& hitInfo)
{
    if (!state.features.enableShadows) {
        // Shadows are disabled in the renderer
        return lightColor;
    } else if (!state.features.enableTransparency) {
        // Shadows are enabled but transparency is disabled
        return visibilityOfLightSampleBinary(state, lightPosition, lightColor, ray, hitInfo) ? lightColor : glm::vec3(0);
    } else {
        // Shadows and transparency are enabled
        return visibilityOfLightSampleTransparency(state, lightPosition, lightColor, ray, hitInfo);
    }
}

// This function is provided as-is. You do not have to implement it.
glm::vec3 computeLightContribution(RenderState& state, const Ray& ray, const HitInfo& hitInfo)
{
    // Iterate over all lights
    glm::vec3 Lo { 0.0f };
    for (const auto& light : state.scene.lights) {
        if (std::holds_alternative<PointLight>(light)) {
            Lo += computeContributionPointLight(state, std::get<PointLight>(light), ray, hitInfo);
        } else if (std::holds_alternative<SegmentLight>(light)) {
            Lo += computeContributionSegmentLight(state, std::get<SegmentLight>(light), ray, hitInfo, state.features.numShadowSamples);
        } else if (std::holds_alternative<ParallelogramLight>(light)) {
            Lo += computeContributionParallelogramLight(state, std::get<ParallelogramLight>(light), ray, hitInfo, state.features.numShadowSamples);
        }
    }
    return Lo;
}