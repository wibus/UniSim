Ray genRay(uvec2 pixelPos)
{
    const uint rayDepth = 0;
    vec4 noise = sampleBlueNoise(rayDepth);

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

Intersection raycast(in Ray ray)
{
    Intersection bestIntersection;
    bestIntersection.t = 1 / 0.0;

    for(uint i = 0; i < instances.length(); ++i)
    {
        Instance instance = instances[i];

        Ray instanceRay;
        instanceRay.origin = rotate(instance.quaternion, ray.origin - instance.position.xyz);
        instanceRay.direction = rotate(instance.quaternion, ray.direction);

        for(uint p = instance.primitiveBegin; p < instance.primitiveEnd; ++p)
        {
            Intersection intersection;
            intersection.t = -1;

            Primitive primitive = primitives[p];

            if(primitive.type == PRIMITIVE_TYPE_MESH)
            {
                intersection = intersectMesh(instanceRay, primitive.index, primitive.material);
            }
            else if(primitive.type == PRIMITIVE_TYPE_SPHERE)
            {
                intersection = intersectSphere(instanceRay, primitive.index, primitive.material);
            }
            else if(primitive.type == PRIMITIVE_TYPE_PLANE)
            {
                intersection = intersectPlane(instanceRay, primitive.index, primitive.material);
            }

            if(intersection.t > 0 && intersection.t < bestIntersection.t)
            {
                bestIntersection = intersection;
                bestIntersection.normal = rotate(
                    quatConj(instance.quaternion),
                    bestIntersection.normal);
            }
        }
    }

    return bestIntersection;
}

bool shadowcast(in Ray ray, float tMax)
{
    for(uint i = 0; i < instances.length(); ++i)
    {
        Instance instance = instances[i];

        Ray instanceRay;
        instanceRay.origin = rotate(instance.quaternion, ray.origin - instance.position.xyz);
        instanceRay.direction = rotate(instance.quaternion, ray.direction);

        for(uint p = instance.primitiveBegin; p < instance.primitiveEnd; ++p)
        {
            Primitive primitive = primitives[p];

            Intersection intersection;
            intersection.t = -1;

            if(primitive.type == PRIMITIVE_TYPE_MESH)
            {
                intersection = intersectMesh(instanceRay, primitive.index, primitive.material);
            }
            else if(primitive.type == PRIMITIVE_TYPE_SPHERE)
            {
                intersection = intersectSphere(instanceRay, primitive.index, primitive.material);
            }
            else if(primitive.type == PRIMITIVE_TYPE_PLANE)
            {
                intersection = intersectPlane(instanceRay, primitive.index, primitive.material);
            }

            if(intersection.t > 0 && intersection.t < tMax * 0.99999)
            {
                return false;
            }
        }
    }

    return true;
}

HitInfo probe(in Ray ray, in Intersection intersection)
{
    Material material = materials[intersection.materialId];

    HitInfo hitInfo;

    hitInfo.position = ray.origin + intersection.t * ray.direction;
    hitInfo.normal = intersection.normal;

    hitInfo.position += hitInfo.normal * 1e-4 * max(intersection.t, maxV(abs(hitInfo.position)));

    vec2 uv = fract(vec2(intersection.uv.x, 1 - intersection.uv.y));

    vec3 albedo = material.albedo.rgb;
    if(material.albedoTexture != -1)
    {
        layout(rgba8) image2D img = textures[material.albedoTexture];
        ivec2 imgSize = imageSize(img);
        vec3 texel = imageLoad(img, ivec2(imgSize * uv)).rgb;

        albedo = toLinear(texel);
    }

    vec3 specular = material.specular.rgb;
    if(material.specularTexture != -1)
    {
        layout(rgba8) image2D img = textures[material.specularTexture];
        ivec2 imgSize = imageSize(img);
        vec3 texel = imageLoad(img, ivec2(imgSize * uv)).rgb;

        specular.r = toLinear(texel).r;
    }

    hitInfo.specularA = specular.r;
    hitInfo.specularA2 = specular.r * specular.r;

    float metalness = specular.g;
    float reflectance = specular.b;

    hitInfo.diffuseAlbedo = mix(albedo, vec3(0, 0, 0), metalness);
    hitInfo.specularF0 = mix(reflectance.xxx, albedo, metalness);
    hitInfo.emission = material.emission.rgb;

    hitInfo.NdotV = max(0.0f, -dot(hitInfo.normal, ray.direction));

    hitInfo.primitiveAreaPdf = intersection.primitiveAreaPdf;

    return hitInfo;
}

vec3 evaluateBSDF(HitInfo hitInfo, vec3 V, vec3 L, float solidAngle)
{
    vec3 H = normalize(L - V);
    float NdotL = max(0, dot(hitInfo.normal, L));
    float NdotH = dot(hitInfo.normal, H);
    float HdotV = -dot(H, V);
    float NdotV = hitInfo.NdotV;

    vec3 f = fresnel(HdotV, hitInfo.specularF0);
    float ndf = ggxNDF(hitInfo.specularA2, NdotH);

    float lightAreaPdf = 1 / solidAngle;

    float diffuseWeight = misHeuristic(1, lightAreaPdf, 1, cosineHemispherePdf(NdotL));
    vec3 diffuse = hitInfo.diffuseAlbedo / PI * (1 - f) * diffuseWeight;

    vec3 specularAlbedo = f * ndf * ggxSmithG2AndDenom(hitInfo.specularA2, NdotV, NdotL);
    float specularPdf = ggxVisibleNormalsDistributionFunction(hitInfo.specularA2, NdotV, NdotH, HdotV) / (4 * HdotV);
    float specularWeight = hitInfo.specularA != 0 ? misHeuristic(1, lightAreaPdf, 1, specularPdf) : 0;
    vec3 specular = (hitInfo.specularA2 != 0) ? specularAlbedo * specularWeight : vec3(0, 0, 0);

    return (NdotL > 0 && NdotV > 0) ? NdotL * (diffuse + specular) * solidAngle : vec3(0, 0, 0);
}

LightSample sampleMesh(uint meshId, uint materialId, vec3 position, vec4 noise)
{
    // TODO
    Mesh mesh = meshes[meshId];
    Material material = materials[materialId];

    LightSample lightSample;
    lightSample.direction = vec3(0, 0, 1);
    lightSample.distance = 1.0;
    lightSample.emission = vec3(0, 0, 0);
    lightSample.solidAngle = 0.0;

    return lightSample;
}

LightSample sampleSphere(uint sphereId, uint materialId, vec3 position, vec4 noise)
{
    Sphere sphere = spheres[sphereId];
    Material material = materials[materialId];

    float lRSqr = sphere.radius * sphere.radius;

    vec3 hitToCenter = - position;
    float distanceSqr = dot(hitToCenter, hitToCenter);

    float sinThetaMaxSqr = lRSqr / distanceSqr;
    float cosThetaMax = sqrt(max(0, 1 - sinThetaMaxSqr));

    float solidAngle = 2 * PI * (1 - cosThetaMax);

    // Importance sample cone
    float cosTheta = (1 - noise.b) + noise.b * cosThetaMax;
    float sinTheta = sqrt(max(0, 1 - cosTheta*cosTheta));
    float phi = noise.a * 2 * PI;

    float dc = sqrt(distanceSqr);
    float ds = dc * cosTheta - sqrt(max(0, lRSqr - dc * dc * sinTheta * sinTheta));
    float cosAlpha = (dc * dc + lRSqr - ds * ds) / (2 * dc * sphere.radius);
    float sinAlpha = sqrt(max(0, 1 - cosAlpha * cosAlpha));

    vec3 lT, lB, lN = -normalize(hitToCenter);
    makeOrthBase(lN, lT, lB);

    vec3 lightN = sphere.radius * vec3(sinAlpha * cos(phi), sinAlpha * sin(phi), cosAlpha);
    vec3 lightP = lightN.x * lT + lightN.y * lB + lightN.z * lN;
    vec3 hitToSample = hitToCenter + lightP;
    float sampleDist = length(hitToSample);
    vec3 L = hitToSample / sampleDist;

    LightSample lightSample;
    lightSample.direction = L;
    lightSample.distance = sampleDist;
    lightSample.emission = material.emission.rgb;
    lightSample.solidAngle = solidAngle;

    return lightSample;
}

LightSample samplePlane(uint planeId, uint materialId, vec3 position, vec4 noise)
{
    Plane plane = planes[planeId];
    Material material = materials[materialId];

    float r = noise.b;
    float theta = 2 * PI * noise.a;

    vec2 Lxy = vec2(cos(theta), sin(theta)) * r;
    vec3 L = vec3(Lxy, sqrt(1 - dot(Lxy, Lxy)));
    L.z = L.z * sign(-position.z);
    L.z = L.z == 0 ? 1 : L.z;

    LightSample lightSample;
    lightSample.direction = L;
    lightSample.distance = -position.z / L.z;
    lightSample.emission = material.emission.rgb;
    lightSample.solidAngle = 2 * PI;

    return lightSample;
}

LightSample sampleDirectionalLight(uint lightId, vec3 position, vec4 noise)
{
    DirectionalLight light = directionalLights[lightId];

    vec3 T, B, N = light.directionCosThetaMax.xyz;
    makeOrthBase(N, T, B);

    vec3 l = sampleUniformCone(noise.b, noise.a, light.directionCosThetaMax.w);
    vec3 L = normalize(l.x * T + l.y * B + l.z * N);

    vec3 skyLuminance;
    vec3 skyTransmittance;
    SampleSkyLuminance(
        skyLuminance,
        skyTransmittance,
        position,
        L);

    vec3 emission = SampleDirectionalLightLuminance(L, lightId);

    LightSample lightSample;
    lightSample.direction = L;
    lightSample.distance = 1 / 0.0;
    lightSample.emission = emission * skyTransmittance;
    lightSample.solidAngle = light.emissionSolidAngle.a;

    return lightSample;
}

vec3 shadeHit(Ray ray, HitInfo hitInfo)
{
    vec3 L_out = vec3(0);

    vec4 noise = sampleBlueNoise(ray.depth + PATH_LENGTH);

    for(uint e = 0; e < emitters.length(); ++e)
    {
        Emitter emitter = emitters[e];
        Instance instance = instances[emitter.instance];
        uint primitiveId = instance.primitiveBegin + emitter.primitive;
        Primitive primitive = primitives[primitiveId];

        vec3 objSpacePosition = rotate(instance.quaternion, hitInfo.position - instance.position.xyz);

        LightSample lightSample;

        if(primitive.type == PRIMITIVE_TYPE_MESH)
        {
            lightSample = sampleMesh(primitive.index, primitive.material, objSpacePosition, noise);
        }
        else if(primitive.type == PRIMITIVE_TYPE_SPHERE)
        {
            lightSample = sampleSphere(primitive.index, primitive.material, objSpacePosition, noise);
        }
        else if(primitive.type == PRIMITIVE_TYPE_PLANE)
        {
            lightSample = samplePlane(primitive.index, primitive.material, objSpacePosition, noise);
        }

        lightSample.direction = rotate(quatConj(instance.quaternion), lightSample.direction);

        if(dot(lightSample.direction, hitInfo.normal) <= 0)
            continue;

        Ray shadowRay;
        shadowRay.origin = hitInfo.position;
        shadowRay.direction = lightSample.direction;

        if(shadowcast(shadowRay, lightSample.distance))
        {
            L_out += lightSample.emission * evaluateBSDF(
                        hitInfo,
                        ray.direction,
                        lightSample.direction,
                        lightSample.solidAngle);
        }
    }

    for(uint dl = 0; dl < directionalLights.length(); ++dl)
    {
        LightSample lightSample = sampleDirectionalLight(dl, hitInfo.position, noise);

        Ray shadowRay;
        shadowRay.origin = hitInfo.position;
        shadowRay.direction = lightSample.direction;

        if(shadowcast(shadowRay, lightSample.distance))
        {
            L_out += lightSample.emission * evaluateBSDF(
                        hitInfo,
                        ray.direction,
                        lightSample.direction,
                        lightSample.solidAngle);
        }
    }

    // Emission
    float weight = misHeuristic(1, ray.bsdfPdf, 1, hitInfo.primitiveAreaPdf);
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
#ifndef IS_UNBIASED
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
#ifndef IS_UNBIASED
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
