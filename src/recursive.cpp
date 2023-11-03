#include "recursive.h"
#include "draw.h"
#include "bvh_interface.h"
#include "intersect.h"
#include "extra.h"
#include "light.h"

// This function is provided as-is. You do not have to implement it.
// Given a range of rays, render out all rays and average the result
glm::vec3 renderRays(RenderState& state, std::span<const Ray> rays, int rayDepth)
{
    glm::vec3 L { 0.f };
    for (const auto& ray : rays) {
        L += renderRay(state, ray, rayDepth);
    }
    return L / static_cast<float>(rays.size());
}

// This method is provided as-is. You do not have to implement it.
// Given a camera ray (or secondary ray), tests for a scene intersection, and
// dependent on the results, evaluates the following functions which you must
// implement yourself:
// - `computeLightContribution()` and its submethods
// - `renderRaySpecularComponent()`, `renderRayTransparentComponent()`, `renderRayGlossyComponent()`
glm::vec3 renderRay(RenderState& state, Ray ray, int rayDepth)
{
    // Trace the ray into the scene. If nothing was hit, return early
    HitInfo hitInfo;
    if (!state.bvh.intersect(state, ray, hitInfo)) {
        drawRay(ray, glm::vec3(1, 0, 0));
        return sampleEnvironmentMap(state, ray);
    }

    // Return value: the light along the ray
    // Given an intersection, estimate the contribution of scene lights at this intersection
    glm::vec3 Lo = computeLightContribution(state, ray, hitInfo);

    // Draw an example debug ray for the incident ray (feel free to modify this for yourself)
    drawRay(ray, glm::vec3(1.0f));

    // Given that recursive components are enabled, and we have not exceeded maximum depth,
    // estimate the contribution along these components
    if (rayDepth < 6) {
        bool isReflective = glm::any(glm::notEqual(hitInfo.material.ks, glm::vec3(0.0f)));
        bool isTransparent = hitInfo.material.transparency != 1.f;

        // Default, specular reflections
        if (state.features.enableReflections && !state.features.extra.enableGlossyReflection && isReflective) {
            renderRaySpecularComponent(state, ray, hitInfo, Lo, rayDepth);
        }

        // Alternative, glossy reflections
        if (state.features.enableReflections && state.features.extra.enableGlossyReflection && isReflective) {
            renderRayGlossyComponent(state, ray, hitInfo, Lo, rayDepth);
        }

        // Transparency passthrough
        if (state.features.enableTransparency && isTransparent) {
            renderRayTransparentComponent(state, ray, hitInfo, Lo, rayDepth);
        }
    }

    return Lo;
}

// TODO: Standard feature
// Given an incident ray and a intersection point, generate a mirrored ray
// - Ray;     the indicent ray
// - HitInfo; hit struct for the intersection point
// - return;  a reflected ray
// This method is unit-tested, so do not change the function signature.
Ray generateReflectionRay(Ray ray, HitInfo hitInfo)
{
    glm::vec3 incidentDirection = glm::normalize(ray.direction);
    glm::vec3 normal = glm::normalize(hitInfo.normal);

    glm::vec3 reflectedDirection = incidentDirection - 2.0f * glm::dot(incidentDirection, normal) * normal;

    glm::vec3 intersectionPoint = ray.origin + ray.direction * ray.t + 0.001f * normal;

    
    Ray reflectedRay;
    reflectedRay.origin = intersectionPoint; 
    reflectedRay.direction = reflectedDirection;
    
    Ray normalRay; 
    normalRay.origin = intersectionPoint;
    normalRay.direction = hitInfo.normal;
    glm::vec3 visualizationColor = glm::vec3(0.0f, 0.0f, 1.0f); 
    glm::vec3 visualizationColor2 = glm::vec3(1.0f, 0.0f, 0.0f); 

    drawRay(reflectedRay, visualizationColor);
    drawRay(normalRay, visualizationColor2);
    return reflectedRay;
}

// TODO: Standard feature
// Given an incident ray and a intersection point, generate a passthrough ray for transparency,
// starting at the intersection point and continuing in the same direction.
// - Ray;     the indicent ray
// - HitInfo; hit struct for the intersection point
// - return;  a passthrough ray for transparency
// This method is unit-tested, so do not change the function signature.
Ray generatePassthroughRay(Ray ray, HitInfo hitInfo)
{ 
    Ray passthroughRay;

    passthroughRay.origin = ray.origin + ray.direction * ray.t + 0.001f;

    passthroughRay.direction = ray.direction;

    passthroughRay.t =  1.0f; 

    drawRay(passthroughRay, glm::vec3(0.0f, 0.0f, 1.0f)); 

    
    return passthroughRay;
}

// TODO: standard feature
// Given a camera ray (or secondary ray) and an intersection, evaluates the contribution
// of a mirrored ray, recursively evaluating renderRay(..., depth + 1) along this ray,
// and adding the result times material.ks to the current intersection's hit color.
// - state;    the active scene, feature config, bvh, and sampler
// - ray;      camera ray
// - hitInfo;  intersection object
// - hitColor; current color at the current intersection, which this function modifies
// - rayDepth; current recursive ray depth
// This method is unit-tested, so do not change the function signature.
void renderRaySpecularComponent(RenderState& state, Ray ray, const HitInfo& hitInfo, glm::vec3& hitColor, int rayDepth)
{
    if (state.features.enableReflections) {
        Ray reflectedRay = generateReflectionRay(ray, hitInfo);

        if (reflectedRay.direction != glm::vec3(0.0f)) {
            glm::vec3 reflectedColor = renderRay(state, reflectedRay, rayDepth + 1); // Recursively trace the reflected ray.

            const Material& material = hitInfo.material;
            glm::vec3 specularColor = material.ks * reflectedColor;
            hitColor += specularColor;
        }
    }

}

// TODO: standard feature
// Given a camera ray (or secondary ray) and an intersection, evaluates the contribution
// of a passthrough transparent ray, recursively evaluating renderRay(..., depth + 1) along this ray,
// and correctly alpha blending the result with the current intersection's hit color
// - state;    the active scene, feature config, bvh, and sampler
// - ray;      camera ray
// - hitInfo;  intersection object
// - hitColor; current color at the current intersection, which this function modifies
// - rayDepth; current recursive ray depth
// This method is unit-tested, so do not change the function signature.
void renderRayTransparentComponent(RenderState& state, Ray ray, const HitInfo& hitInfo, glm::vec3& hitColor, int rayDepth)
{
    if (state.features.enableTransparency) {
        Ray passthroughRay = generatePassthroughRay(ray, hitInfo);

        if (passthroughRay.direction != glm::vec3(0.0f)) {
            glm::vec3 passthroughColor = renderRay(state, passthroughRay, rayDepth + 1);

            const Material& material = hitInfo.material;
            hitColor = glm::mix(hitColor, passthroughColor, material.transparency);
        }
    }
}