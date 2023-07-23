Ray genRay(uvec2 pixelPos)
{
    vec4 noise = sampleBlueNoise(0);

    vec4 pixelClip = vec4(
        float(pixelPos.x) + noise.x,
        float(pixelPos.y) + noise.y,
        0, 1);

    vec4 unprojRay = rayMatrix * pixelClip;
    vec3 primaryDir = normalize(unprojRay.xyz / unprojRay.w);
    float RoD = dot(primaryDir, lenseDirection.xyz);
    float focusT = focusDistance / RoD;

    vec3 T, B, N = lenseDirection.xyz;
    makeOrthBase(N, T, B);

    vec2 offset = sampleUniformDisk(noise.z, noise.w) * apertureRadius;
    vec3 offsetPos = T * offset.x + B * offset.y;
    vec3 rayOrigin = lensePosition.xyz + offsetPos;
    vec3 rayDirection = normalize(primaryDir * focusT - offsetPos);

    Ray ray;
    ray.direction = rayDirection;
    ray.origin = rayOrigin;
    ray.throughput = vec3(1, 1, 1);
#ifndef IS_UNBIASED
    ray.diffusivity = 0;
#endif
    ray.depth = 0;
    ray.bsdfPdf = DELTA;

    return ray;
}

Intersection intersectSphere(in Ray ray, in uint instanceId)
{
    Intersection intersection;
    intersection.t = -1.0;
    intersection.instanceId = instanceId;

    Instance instance = instances[instanceId];

    vec3 O = ray.origin;
    vec3 D = ray.direction;
    vec3 C = instance.position.xyz;

    vec3 L = C - O;
    float t_ca = dot(L, D);

    if(t_ca < 0)
        return intersection;

    float dSqr = (dot(L, L) - t_ca*t_ca);

    if(dSqr > instance.radius * instance.radius)
        return intersection;

    float t_hc = sqrt(instance.radius * instance.radius - dSqr);
    float t_0 = t_ca - t_hc;
    float t_1 = t_ca + t_hc;

    if(t_0 > 0)
        intersection.t = t_0;
    else if(t_1 > 0)
        intersection.t = t_1;

    return intersection;
}

Intersection raycast(in Ray ray)
{
    Intersection bestIntersection;
    bestIntersection.t = 1 / 0.0;

    for(uint b = 0; b < instances.length(); ++b)
    {
        Intersection intersection;
        intersection.t = -1;
        intersection.instanceId = -1;

        if(instances[b].primitiveType == PRIMITIVE_TYPE_SPHERE)
        {
            intersection = intersectSphere(ray, b);
        }
        else if(instances[b].primitiveType == PRIMITIVE_TYPE_TERRAIN)
        {
            intersection = intersectTerrain(ray, b);
        }

        if(intersection.t > 0 && intersection.t < bestIntersection.t)
        {
            bestIntersection = intersection;
        }
    }

    return bestIntersection;
}

HitInfo probe(in Ray ray, in Intersection intersection)
{
    Instance instance = instances[intersection.instanceId];

    HitInfo hitInfo;

    hitInfo.instanceId = intersection.instanceId;

    hitInfo.position = ray.origin + intersection.t * ray.direction;

    if(instance.primitiveType == PRIMITIVE_TYPE_SPHERE)
        hitInfo.normal = normalize(hitInfo.position - instance.position.xyz);
    else if(instance.primitiveType == PRIMITIVE_TYPE_TERRAIN)
        hitInfo.normal = vec3(0, 0, ray.direction.z != 0 ? -sign(ray.direction.z): 1);
    else
        hitInfo.normal = vec3(0, 0, 1);

    hitInfo.position += hitInfo.normal * 1e-6 * max(1, maxV(abs(hitInfo.position)));

    vec3 albedo = instance.albedo.rgb;
    if(instance.materialId > 0)
    {
        vec2 uv = findUV(instance.quaternion, hitInfo.normal);
        layout(rgba8) image2D albedoImg = materials[instance.materialId-1].albedo;
        ivec2 imgSize = imageSize(albedoImg);
        vec3 texel = imageLoad(albedoImg, ivec2(imgSize * vec2(uv.x, 1 - uv.y))).rgb;

        albedo = toLinear(texel);
    }

    hitInfo.specularA = instance.specular.r;
    hitInfo.specularA2 = hitInfo.specularA * hitInfo.specularA;

    float metalness = instance.specular.g;
    float reflectance = instance.specular.b;

    hitInfo.diffuseAlbedo = mix(albedo, vec3(0, 0, 0), metalness);
    hitInfo.specularF0 = mix(reflectance.xxx, albedo, metalness);
    hitInfo.emission = instance.emission.rgb;

    hitInfo.NdotV = max(0.0f, -dot(hitInfo.normal, ray.direction));

    if(instance.primitiveType == PRIMITIVE_TYPE_SPHERE)
    {
        float iRSqr = instance.radius * instance.radius;
        vec3 originToHit = (instance.position.xyz - ray.origin);
        float distanceSqr = dot(originToHit, originToHit);
        float sinThetaMaxSqr = iRSqr / distanceSqr;
        float cosThetaMax = sqrt(max(0, 1 - sinThetaMaxSqr));
        float solidAngle = 2 * PI * (1 - cosThetaMax);
        hitInfo.instanceAreaPdf = 1 / solidAngle;
    }
    else
    {
        hitInfo.instanceAreaPdf = 1 / (2 * PI);
    }

    return hitInfo;
}

vec3 evaluateBSDF(Ray ray, HitInfo hitInfo, vec3 L, float solidAngle)
{
    float lightAreaPdf = 1 / solidAngle;

    vec3 H = normalize(L - ray.direction);
    float NdotL = max(0, dot(hitInfo.normal, L));
    float NdotH = dot(hitInfo.normal, H);
    float HdotV = -dot(H, ray.direction);
    float NdotV = hitInfo.NdotV;

    vec3 f = fresnel(HdotV, hitInfo.specularF0);
    float ndf = ggxNDF(hitInfo.specularA2, NdotH);

    float diffuseWeight = misHeuristic(1, lightAreaPdf, 1, cosineHemispherePdf(NdotL));
    vec3 diffuse = hitInfo.diffuseAlbedo / PI * (1 - f) * diffuseWeight;

    vec3 specularAlbedo = f * ndf * ggxSmithG2AndDenom(hitInfo.specularA2, NdotV, NdotL);
    float specularPdf = ggxVisibleNormalsDistributionFunction(hitInfo.specularA2, NdotV, NdotH, HdotV) / (4 * HdotV);
    float specularWeight = hitInfo.specularA != 0 ? misHeuristic(1, lightAreaPdf, 1, specularPdf) : 0;
    vec3 specular = (NdotL > 0 && NdotV > 0 && hitInfo.specularA2 != 0) ? specularAlbedo * specularWeight : vec3(0, 0, 0);

    return NdotL * (diffuse + specular) * solidAngle;
}

vec3 sampleSphereLight(uint emitterId, Ray ray, HitInfo hitInfo, vec4 noise)
{
    Instance emitter = instances[emitterId];

    float lRSqr = emitter.radius * emitter.radius;

    vec3 hitToLight = (emitter.position.xyz - hitInfo.position);
    float distanceSqr = dot(hitToLight, hitToLight);

    float sinThetaMaxSqr = lRSqr / distanceSqr;
    float cosThetaMax = sqrt(max(0, 1 - sinThetaMaxSqr));

    float solidAngle = 2 * PI * (1 - cosThetaMax);

    // Importance sample cone
    float cosTheta = (1 - noise.b) + noise.b * cosThetaMax;
    float sinTheta = sqrt(max(0, 1 - cosTheta*cosTheta));
    float phi = noise.a * 2 * PI;

    float dc = sqrt(distanceSqr);
    float ds = dc * cosTheta - sqrt(max(0, lRSqr - dc * dc * sinTheta * sinTheta));
    float cosAlpha = (dc * dc + lRSqr - ds * ds) / (2 * dc * emitter.radius);
    float sinAlpha = sqrt(max(0, 1 - cosAlpha * cosAlpha));

    vec3 lT, lB, lN = -normalize(hitToLight);
    makeOrthBase(lN, lT, lB);

    vec3 lightN = emitter.radius * vec3(sinAlpha * cos(phi), sinAlpha * sin(phi), cosAlpha);
    vec3 lightP = lightN.x * lT + lightN.y * lB + lightN.z * lN;
    vec3 L = normalize(hitToLight + lightP);

    Ray shadowRay;
    shadowRay.origin = hitInfo.position;
    shadowRay.direction = L;

    Intersection shadowIntersection = raycast(shadowRay);
    if(shadowIntersection.instanceId == emitterId)
        return evaluateBSDF(ray, hitInfo, L, solidAngle) * emitter.emission.rgb;
    else
        return vec3(0);
}

vec3 sampleDirectionalLight(uint lightId, Ray ray, HitInfo hitInfo, vec4 noise)
{
    DirectionalLight light = directionalLights[lightId];

    vec3 T, B, N = light.directionCosThetaMax.xyz;
    makeOrthBase(N, T, B);

    vec3 l = sampleUniformCone(noise.b, noise.a, light.directionCosThetaMax.w);
    vec3 L = normalize(l.x * T + l.y * B + l.z * N);

    Ray shadowRay;
    shadowRay.origin = hitInfo.position;
    shadowRay.direction = L;

    vec3 skyLuminance;
    vec3 skyTransmittance;
    SampleSkyLuminance(
        skyLuminance,
        skyTransmittance,
        shadowRay.origin,
        shadowRay.direction);

    vec3 emission = SampleDirectionalLightLuminance(shadowRay.direction, lightId);

    Intersection shadowIntersection = raycast(shadowRay);
    if(shadowIntersection.t == 1 / 0.0)
        return evaluateBSDF(ray, hitInfo, L, light.emissionSolidAngle.a)
                * emission
                * skyTransmittance;
    else
        return vec3(0);
}

vec3 shadeHit(Ray ray, HitInfo hitInfo)
{
    vec3 L_out = vec3(0);

    vec4 noise = sampleBlueNoise(ray.depth + PATH_LENGTH);

    for(uint e = 0; e < emitters.length(); ++e)
    {
        uint emitterId = emitters[e];

        if(emitterId == hitInfo.instanceId)
            continue;

        if(instances[emitterId].primitiveType == PRIMITIVE_TYPE_SPHERE)
            L_out += sampleSphereLight(emitterId, ray, hitInfo, noise);
    }

    for(uint dl = 0; dl < directionalLights.length(); ++dl)
    {
        L_out += sampleDirectionalLight(dl, ray, hitInfo, noise);
    }

    // Emission
    float weight = misHeuristic(1, ray.bsdfPdf, 1, hitInfo.instanceAreaPdf);
    L_out += weight * hitInfo.emission;

    return ray.throughput * L_out;
}

vec3 shadeSky(in Ray ray)
{
    vec3 skyLuminance;
    vec3 skyTransmittance;
    SampleSkyLuminance(
        skyLuminance,
        skyTransmittance,
        ray.origin,
        ray.direction);

    vec3 L_in = skyLuminance;

    for(uint dl = 0; dl < directionalLights.length(); ++dl)
    {
        DirectionalLight light = directionalLights[dl];
        float cosTheta = dot(light.directionCosThetaMax.xyz, ray.direction);

        if(cosTheta >= light.directionCosThetaMax.w)
        {
            vec3 emission = SampleDirectionalLightLuminance(ray.direction, dl);
            float lightAreaPdf = 1 / light.emissionSolidAngle.a;
            float weight = misHeuristic(1, ray.bsdfPdf, 1, lightAreaPdf);
            L_in += weight * emission * skyTransmittance;
        }
    }

    return ray.throughput * L_in;
}

Ray scatter(Ray rayIn, HitInfo hitInfo)
{
    Ray rayOut = rayIn;
    rayOut.origin = hitInfo.position;
    rayOut.depth = rayIn.depth + 1;

    vec4 noise = sampleBlueNoise(rayOut.depth);

    vec3 T, B, N = hitInfo.normal;
    makeOrthBase(N, T, B);

    // Specular
    vec3 V = -vec3(dot(rayIn.direction, T), dot(rayIn.direction, B), dot(rayIn.direction, N));
    vec3 H = sampleGGXVNDF(V, hitInfo.specularA, hitInfo.specularA, noise.x, noise.y);
    vec3 L = 2.0 * dot(H, V) * H - V;
    vec3 reflectionDirection = normalize(L.x * T + L.y * B + L.z * N);

    vec3 f = fresnel(max(0, dot(H, V)), hitInfo.specularF0);
    vec3 specularAlbedo = f
                * ggxSmithG2(hitInfo.specularA2, V.z, L.z)
                / ggxSmithG1(hitInfo.specularA2, V.z);
    float specularLuminance = L.z > 0 ? toLuminance(specularAlbedo) : 0;
    float diffuseLuminance = toLuminance(hitInfo.diffuseAlbedo);

    float r = specularLuminance != 0 ? specularLuminance / (specularLuminance + diffuseLuminance) : 0;

    if(noise.z < r)
    {
        float HdotV = dot(V, H);
        float pdf = ggxVisibleNormalsDistributionFunction(hitInfo.specularA2, V.z, H.z, HdotV) / (4 * HdotV);

        rayOut.direction = reflectionDirection;
        rayOut.bsdfPdf = hitInfo.specularA == 0 ? DELTA : pdf;
        rayOut.throughput = rayIn.throughput * specularAlbedo / r;
#ifndef UNBIASED
        rayOut.throughput *= mix(1 - rayIn.diffusivity, 1, min(1, hitInfo.specularA));
        rayOut.diffusivity = mix(rayIn.diffusivity, 1, min(1, hitInfo.specularA));
#endif
    }
    else if(noise.z >= r && r != 1)
    {
        // Diffuse
        vec3 dir = sampleCosineHemisphere(noise.r, noise.g);
        rayOut.direction = normalize(dir.x * T + dir.y * B + dir.z * N);
        rayOut.bsdfPdf = cosineHemispherePdf(dir.z);
        rayOut.throughput = rayIn.throughput * hitInfo.diffuseAlbedo * (1 - f) / (1 - r);
#ifndef UNBIASED
        rayOut.diffusivity = 1;
#endif
    }
    else
    {
        rayOut.bsdfPdf = 0;
        rayOut.throughput = vec3(0, 0, 0);
    }

    return rayOut;
}

layout(local_size_x = 8, local_size_y = 4, local_size_z = 1) in;

void main()
{
    vec3 colorAccum = vec3(0, 0, 0);

    Ray ray = genRay(gl_GlobalInvocationID.xy);

    for(uint depthId = 0; depthId < PATH_LENGTH; ++depthId)
    {
        Intersection bestIntersection = raycast(ray);

        if (bestIntersection.t != 1 / 0.0)
        {
            HitInfo hitInfo = probe(ray, bestIntersection);
            colorAccum += shadeHit(ray, hitInfo);
            ray = scatter(ray, hitInfo);
        }
        else
        {
            colorAccum += shadeSky(ray);
            break;
        }
    }

    vec3 finalLinear = exposure * colorAccum;

    if(frameIndex != 0)
    {
        vec4 prevFrameSRGB = imageLoad(result, ivec2(gl_GlobalInvocationID));
        vec3 prevFrameLinear = toLinear(prevFrameSRGB.rgb);

        float blend = frameIndex / float(frameIndex+1);
        finalLinear = mix(finalLinear, prevFrameLinear, blend);
    }

    vec4 finalSRGB = vec4(sRGB(finalLinear), 0);
    imageStore(result, ivec2(gl_GlobalInvocationID), finalSRGB);
}
