#include "render.h"
#include "texture.h"
#include <cmath>
#include <fmt/core.h>
#include <glm/geometric.hpp>
#include <glm/gtx/string_cast.hpp>
#include <shading.h>

// This function is provided as-is. You do not have to implement it (unless
// you need to for some extra feature).
// Given render state and an intersection, based on render settings, sample
// the underlying material data in the expected manner.
glm::vec3 sampleMaterialKd(RenderState& state, const HitInfo& hitInfo)
{
    if (state.features.enableTextureMapping && hitInfo.material.kdTexture) {
        if (state.features.enableBilinearTextureFiltering) {
            return sampleTextureBilinear(*hitInfo.material.kdTexture, hitInfo.texCoord);
        } else {
            return sampleTextureNearest(*hitInfo.material.kdTexture, hitInfo.texCoord);
        }
    } else {
        return hitInfo.material.kd;
    }
}

// This function is provided as-is. You do not have to implement it.
// Given a camera direction, a light direction, a relevant intersection, and a color coming in
// from the light, evaluate the scene-selected shading model, returning the reflected light towards the target.
glm::vec3 computeShading(RenderState& state, const glm::vec3& cameraDirection, const glm::vec3& lightDirection, const glm::vec3& lightColor, const HitInfo& hitInfo)
{
    // Hardcoded linear gradient. Feel free to modify this
    static LinearGradient gradient = {
        .components = {
            { 0.1f, glm::vec3(215.f / 256.f, 210.f / 256.f, 203.f / 256.f) },
            { 0.22f, glm::vec3(250.f / 256.f, 250.f / 256.f, 240.f / 256.f) },
            { 0.5f, glm::vec3(145.f / 256.f, 170.f / 256.f, 175.f / 256.f) },
            { 0.78f, glm::vec3(255.f / 256.f, 250.f / 256.f, 205.f / 256.f) },
            { 0.9f, glm::vec3(170.f / 256.f, 170.f / 256.f, 170.f / 256.f) },
        }
    };

    if (state.features.enableShading) {
        switch (state.features.shadingModel) {
        case ShadingModel::Lambertian:
            return computeLambertianModel(state, cameraDirection, lightDirection, lightColor, hitInfo);
        case ShadingModel::Phong:
            return computePhongModel(state, cameraDirection, lightDirection, lightColor, hitInfo);
        case ShadingModel::BlinnPhong:
            return computeBlinnPhongModel(state, cameraDirection, lightDirection, lightColor, hitInfo);
        case ShadingModel::LinearGradient:
            return computeLinearGradientModel(state, cameraDirection, lightDirection, lightColor, hitInfo, gradient);
        };
    }

    return lightColor * sampleMaterialKd(state, hitInfo);
}

// Given a camera direction, a light direction, a relevant intersection, and a color coming in
// from the light, evaluate a Lambertian diffuse shading, returning the reflected light towards the target.
glm::vec3 computeLambertianModel(RenderState& state, const glm::vec3& cameraDirection, const glm::vec3& lightDirection, const glm::vec3& lightColor, const HitInfo& hitInfo)
{
    // Normalize the surface normal and light direction
    glm::vec3 N = glm::normalize(hitInfo.normal);
    glm::vec3 L = glm::normalize(lightDirection);

    // Compute the Lambertian reflection intensity
    float lambertian = glm::max(0.0f, glm::dot(N, L));

    // Get the diffuse material color either from the texture or the hitInfo's material property
    glm::vec3 kd = sampleMaterialKd(state, hitInfo);

    // Compute the final reflected light color
    glm::vec3 reflectedLight = kd * lightColor * lambertian;

    return reflectedLight;
}

// TODO: Standard feature
// Given a camera direction, a light direction, a relevant intersection, and a color coming in
// from the light, evaluate the Phong Model returning the reflected light towards the target.
// Note: materials do not have an ambient component, so you can ignore this.
// Note: use `sampleMaterialKd` instead of material.kd to automatically forward to texture
//       sampling if a material texture is available!
//
// - state;           the active scene, feature config, and the bvh
// - cameraDirection; exitant vector towards the camera (or secondary position)
// - lightDirection;  exitant vector towards the light
// - lightColor;      the color of light along the lightDirection vector
// - hitInfo;         hit object describing the intersection point
// - return;          the result of shading along the cameraDirection vector
//
// This method is unit-tested, so do not change the function signature.
glm::vec3 computePhongModel(RenderState& state, const glm::vec3& cameraDirection, const glm::vec3& lightDirection, const glm::vec3& lightColor, const HitInfo& hitInfo)
{
    // TODO: Implement phong shading
    glm::vec3 N = glm::normalize(hitInfo.normal);
    glm::vec3 L = glm::normalize(lightDirection);
    glm::vec3 V = glm::normalize(cameraDirection);

    // Calculate the reflection vector
    glm::vec3 R = glm::reflect(-L, N);

    // Diffuse component
    float NdotL = glm::max(glm::dot(N, L), 0.f);
    glm::vec3 kd = sampleMaterialKd(state, hitInfo) * NdotL;
    glm::vec3 diffuse = kd * lightColor * NdotL;

    // Specular component
    float RdotV = glm::max(glm::dot(R, V), 0.f);
    glm::vec3 ks = hitInfo.material.ks;
    glm::vec3 specular = ks * lightColor * RdotV;

    // Combine diffuse and specular components
    glm::vec3 reflectedLight = diffuse + specular;

    return reflectedLight;
}

// TODO: Standard feature
// Given a camera direction, a light direction, a relevant intersection, and a color coming in
// from the light, evaluate the Blinn-Phong Model returning the reflected light towards the target.
// Note: materials do not have an ambient component, so you can ignore this.
// Note: use `sampleMaterialKd` instead of material.kd to automatically forward to texture
//       sampling if a material texture is available!
//
// - state;           the active scene, feature config, and the bvh
// - cameraDirection; exitant vector towards the camera (or secondary position)
// - lightDirection;  exitant vector towards the light
// - lightColor;      the color of light along the lightDirection vector
// - hitInfo;         hit object describing the intersection point
// - return;          the result of shading along the cameraDirection vector
//
// This method is unit-tested, so do not change the function signature.
glm::vec3 computeBlinnPhongModel(RenderState& state, const glm::vec3& cameraDirection, const glm::vec3& lightDirection, const glm::vec3& lightColor, const HitInfo& hitInfo)
{
    // TODO: Implement blinn-phong shading
    glm::vec3 N = glm::normalize(hitInfo.normal);
    glm::vec3 L = glm::normalize(lightDirection);
    glm::vec3 V = glm::normalize(cameraDirection);

    // Compute the half-way vector between L and V
    glm::vec3 H = glm::normalize(L + V);

    // Diffuse component
    float NdotL = glm::max(glm::dot(N, L), 0.f);
    glm::vec3 kd = sampleMaterialKd(state, hitInfo);
    glm::vec3 diffuse = kd * lightColor * NdotL;

    // Specular component
    float NdotH = glm::pow(glm::max(0.0f, glm::dot(N, H)), hitInfo.material.shininess);
    glm::vec3 ks = hitInfo.material.ks;
    glm::vec3 specular = ks * lightColor * NdotH;

    // Combine diffuse and specular components
    glm::vec3 reflectedLight = diffuse + specular;

    return reflectedLight;
}

// TODO: Standard feature
// Given a number ti between [-1, 1], sample from the gradient's components and return the
// linearly interpolated color, for which ti lies in the interval between the t-values of two
// components, or on a boundary. If ti falls outside the gradient's smallest/largest components,
// the nearest component must be sampled.
// - ti; a number between [-1, 1]
// This method is unit-tested, so do not change the function signature.
glm::vec3 LinearGradient::sample(float ti) const
{
    // Ensure components are sorted by t-values
    assert(components.size() >= 2); // Ensure there are at least two components for interpolation.

    // Check if ti is outside the boundaries of the gradient components
    if (ti <= components.front().t)
        return components.front().color;

    if (ti >= components.back().t)
        return components.back().color;

    // Search for the two gradient components a and b such that a.t <= ti <= b.t
    for (size_t i = 0; i < components.size() - 1; ++i) {
        float ta = components[i].t;
        float tb = components[i + 1].t;

        if (ta <= ti && ti <= tb) {
            glm::vec3 colorA = components[i].color;
            glm::vec3 colorB = components[i + 1].color;

            // Calculate the relative position of ti between ta and tb
            float alpha = (ti - ta) / (tb - ta);

            // Linearly interpolate between colorA and colorB based on alpha
            return glm::mix(colorA, colorB, alpha);
        }
    }

    // This shouldn't be reached as ti should fall within the components' boundaries or be clamped.
    assert(false);
    return glm::vec3(0.f);
}

// TODO: Standard feature
// Given a camera direction, a light direction, a relevant intersection, and a color coming in
// from the light, evaluate a diffuse shading model, such that the diffuse component is sampled not
// from the intersected material, but a provided linear gradient, based on the cosine of theta
// as defined in the diffuse shading part of the Phong model.
//
// - state;           the active scene, feature config, and the bvh
// - cameraDirection; exitant vector towards the camera (or secondary position)
// - lightDirection;  exitant vector towards the light
// - lightColor;      the color of light along the lightDirection vector
// - hitInfo;         hit object describing the intersection point
// - gradient;        the linear gradient object
// - return;          the result of shading
//
// This method is unit-tested, so do not change the function signature.
glm::vec3 computeLinearGradientModel(RenderState& state, const glm::vec3& cameraDirection, const glm::vec3& lightDirection, const glm::vec3& lightColor, const HitInfo& hitInfo, const LinearGradient& gradient)
{
    // Calculate cosine of the angle between light direction and the normal
    float cos_theta = glm::dot(glm::normalize(lightDirection), glm::normalize(hitInfo.normal));

    // Clamp the value of cos_theta between -1 and 1 (just in case due to precision issues)
    cos_theta = glm::clamp(cos_theta, -1.0f, 1.0f);

    // Sample color from the gradient based on cos_theta
    glm::vec3 sampledColor = gradient.sample(cos_theta);

    // Multiply the sampled color with the light color to get the final color
    return sampledColor * lightColor;
}
