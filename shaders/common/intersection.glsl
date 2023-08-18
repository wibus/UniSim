vec4 rayTriangleIntersection(
    vec3 ray, vec3 rayDir,
    vec3 A, vec3 B, vec3 C,
    out float inv2Area)
{
    vec3 crossABC = cross(B-A, C-A);
    inv2Area = 1 / length(crossABC);
    vec3 normal = crossABC * inv2Area;

    float t = dot(A-ray, normal) / dot(normal, rayDir);
    vec3 Q = ray + rayDir * t; // hit point

    float areaQBC = dot(cross(C-B, Q-B), normal);
    float areaAQC = dot(cross(A-C, Q-C), normal);
    float areaABQ = dot(cross(B-A, Q-A), normal);

    return
        areaQBC >= 0.0 && areaAQC >= 0.0 && areaABQ >= 0.0 ?
        vec4(vec3(areaQBC, areaAQC, areaABQ) * inv2Area, t) :
        vec4(0, 0, 0, -1);
}

bool rayAABBIntersection(Ray ray, vec3 aabbMin, vec3 aabbMax)
{
    vec3 tmin = (aabbMin - ray.origin) * ray.invDirection;
    vec3 tmax = (aabbMax - ray.origin) * ray.invDirection;

    vec3 trueMin = min(tmin, tmax);
    vec3 trueMax = max(tmin, tmax);

    float tNear = max(max(trueMin.x, trueMin.y), trueMin.y);
    float tFar  = min(min(trueMax.x, trueMax.y), trueMax.y);

    return (tNear <= tFar) && (tFar > 0);
}

Intersection intersectMesh(Ray ray, uint meshId, uint materialId)
{
    Mesh mesh = meshes[meshId];

    Intersection intersection;
    intersection.t = 1 / 0.0;
    intersection.materialId = materialId;

    BvhNode bvhNode = bvhNodes[mesh.bvhNode];
    if(!rayAABBIntersection(ray,
        vec3(bvhNode.aabbMinX, bvhNode.aabbMinY, bvhNode.aabbMinZ),
        vec3(bvhNode.aabbMaxX, bvhNode.aabbMaxY, bvhNode.aabbMaxZ)))
        return intersection;

    uint triBegin = bvhNode.leftFirst;
    uint triEnd = triBegin + bvhNode.triCount;
    for(uint t = triBegin; t < triEnd; ++t)
    {
        float inv2Area;
        vec4 triHit = rayTriangleIntersection(
            ray.origin,
            ray.direction,
            verticesPos[triangles[t].v0].position.xyz,
            verticesPos[triangles[t].v1].position.xyz,
            verticesPos[triangles[t].v2].position.xyz,
            inv2Area);

        if(triHit.w > 0 && triHit.w < intersection.t)
        {
            intersection.t = triHit.w;

            VertexData data0 = verticesData[triangles[t].v0];
            VertexData data1 = verticesData[triangles[t].v1];
            VertexData data2 = verticesData[triangles[t].v2];

            intersection.normal = normalize(
                triHit.x * data0.normal.xyz +
                triHit.y * data1.normal.xyz +
                triHit.z * data2.normal.xyz);

            intersection.uv =
                triHit.x * data0.uv +
                triHit.y * data1.uv +
                triHit.z * data2.uv;

            intersection.primitiveAreaPdf = triHit.w * triHit.w * inv2Area * 2;
        }
    }

    if(intersection.t == 1 / 0)
        intersection.t = -1;

    return intersection;
}

Intersection intersectSphere(Ray ray, uint sphereId, uint materialId)
{
    Sphere sphere = spheres[sphereId];

    Intersection intersection;
    intersection.t = -1.0;
    intersection.materialId = materialId;

    float t_ca = dot(-ray.origin, ray.direction);

    if(t_ca < 0)
        return intersection;

    float dSqr = (dot(ray.origin, ray.origin) - t_ca*t_ca);

    if(dSqr > sphere.radius * sphere.radius)
        return intersection;

    float t_hc = sqrt(sphere.radius * sphere.radius - dSqr);
    float t_0 = t_ca - t_hc;
    float t_1 = t_ca + t_hc;

    if(t_0 > 0)
    {
        // Ray origin is outside the sphere
        intersection.t = t_0;
        intersection.normal = normalize(ray.origin + ray.direction * t_0);
        intersection.uv = findUV(intersection.normal);

        float iRSqr = sphere.radius * sphere.radius;
        vec3 originToHit = (-ray.origin);
        float distanceSqr = dot(originToHit, originToHit);
        float sinThetaMaxSqr = iRSqr / distanceSqr;
        float cosThetaMax = sqrt(max(0, 1 - sinThetaMaxSqr));
        float solidAngle = 2 * PI * (1 - cosThetaMax);
        intersection.primitiveAreaPdf = 1 / solidAngle;

    }
    else if(t_1 > 0)
    {
        // Ray origin is inside the sphere
        intersection.t = t_1;
        intersection.normal = -normalize(ray.origin + ray.direction * t_1);
        intersection.uv = findUV(-intersection.normal);
        intersection.primitiveAreaPdf = 1 / (4 * PI);
    }

    return intersection;
}

Intersection intersectPlane(Ray ray, uint planeId, uint materialId)
{
    Plane plane = planes[planeId];

    Intersection intersection;
    intersection.t = -ray.origin.z * ray.invDirection.z;
    intersection.materialId = materialId;
    intersection.normal = vec3(0, 0, intersection.t >= 0 ? 1 : -1);
    intersection.uv = plane.invScale * (ray.origin + ray.direction * intersection.t).xy;
    intersection.primitiveAreaPdf = 1 / (2 * PI);

    return intersection;
}
