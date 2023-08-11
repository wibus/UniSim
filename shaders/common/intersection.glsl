vec4 rayTriangleIntersection(
    vec3 ray, vec3 rayDir,
    vec3 A, vec3 B, vec3 C,
    out float areaABC)
{
    vec3 normal = normalize(cross(B-A, C-A));

    float t = dot(A-ray, normal) / dot(normal, rayDir);
    vec3 Q = ray + rayDir*t; // hit point

    areaABC = dot(cross(B-A, C-A), normal);
    float areaQBC = dot(cross(C-B, Q-B), normal);
    float areaAQC = dot(cross(A-C, Q-C), normal);
    float areaABQ = dot(cross(B-A, Q-A), normal);

    return
        areaQBC >= 0.0 && areaAQC >= 0.0 && areaABQ >= 0.0 ?
        vec4(vec3(areaQBC, areaAQC, areaABQ) / areaABC, t) :
        vec4(0, 0, 0, -1);
}

bool rayAABBIntersection(Ray ray, vec3 aabbMin, vec3 aabbMax)
{
    float tmin = (aabbMin.x - ray.origin.x) / ray.direction.x;
    float tmax = (aabbMax.x - ray.origin.x) / ray.direction.x;

    if (tmin > tmax) swap(tmin, tmax);

    float tymin = (aabbMin.y - ray.origin.y) / ray.direction.y;
    float tymax = (aabbMax.y - ray.origin.y) / ray.direction.y;

    if (tymin > tymax) swap(tymin, tymax);

    if ((tmin > tymax) || (tymin > tmax))
        return false;

    if (tymin > tmin)
        tmin = tymin;

    if (tymax < tmax)
        tmax = tymax;

    float tzmin = (aabbMin.z - ray.origin.z) / ray.direction.z;
    float tzmax = (aabbMax.z - ray.origin.z) / ray.direction.z;

    if (tzmin > tzmax) swap(tzmin, tzmax);

    if ((tmin > tzmax) || (tzmin > tmax))
        return false;

    if (tzmin > tmin)
        tmin = tzmin;

    if (tzmax < tmax)
        tmax = tzmax;

    return true;
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
        float areaABC;
        vec4 triHit = rayTriangleIntersection(
            ray.origin,
            ray.direction,
            verticesPos[triangles[t].v0].position.xyz,
            verticesPos[triangles[t].v1].position.xyz,
            verticesPos[triangles[t].v2].position.xyz,
            areaABC);

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

            intersection.primitiveAreaPdf = triHit.w * triHit.w / areaABC;
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
    intersection.t = -ray.origin.z / ray.direction.z;
    intersection.materialId = materialId;
    intersection.normal = vec3(0, 0, intersection.t >= 0 ? 1 : -1);
    intersection.uv = plane.invScale * (ray.origin + ray.direction * intersection.t).xy;
    intersection.primitiveAreaPdf = 1 / (2 * PI);

    return intersection;
}
