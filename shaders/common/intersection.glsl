vec4 rayTriangleIntersection(
    Probe probe,
    vec3 A, vec3 B, vec3 C,
    float inv2Area)
{
    vec3 crossABC = cross(B-A, C-A);
    vec3 normal = crossABC * inv2Area;

    float t = dot(A-probe.origin, normal) / dot(normal, probe.direction);
    vec3 Q = probe.origin + probe.direction * t; // hit point

    float areaQBC = dot(cross(C-B, Q-B), normal);
    float areaAQC = dot(cross(A-C, Q-C), normal);
    float areaABQ = dot(cross(B-A, Q-A), normal);

    return
        areaQBC >= 0.0 && areaAQC >= 0.0 && areaABQ >= 0.0 ?
        vec4(vec3(areaQBC, areaAQC, areaABQ) * inv2Area, t) :
        vec4(0, 0, 0, -1);
}

bool rayAABBIntersection(Probe probe, vec3 aabbMin, vec3 aabbMax)
{
    vec3 tmin = (aabbMin - probe.origin) * probe.invDirection;
    vec3 tmax = (aabbMax - probe.origin) * probe.invDirection;

    vec3 trueMin = min(tmin, tmax);
    vec3 trueMax = max(tmin, tmax);

    float tNear = max(max(trueMin.x, trueMin.y), trueMin.y);
    float tFar  = min(min(trueMax.x, trueMax.y), trueMax.y);

    return (tNear <= tFar) && (tFar > 0);
}

bool intersectMesh(inout Intersection intersection, Probe probe, uint meshId, uint materialId)
{
    Mesh mesh = meshes[meshId];

    BvhNode bvhNode = bvhNodes[mesh.bvhNode];
    if(!rayAABBIntersection(probe,
        vec3(bvhNode.aabbMinX, bvhNode.aabbMinY, bvhNode.aabbMinZ),
        vec3(bvhNode.aabbMaxX, bvhNode.aabbMaxY, bvhNode.aabbMaxZ)))
        return false;

    bool intersected = false;

    uint triBegin = bvhNode.leftFirst;
    uint triEnd = triBegin + bvhNode.triCount;
    for(uint t = triBegin; t < triEnd; ++t)
    {
        Triangle tri = triangles[t];
        vec4 triHit = rayTriangleIntersection(
            probe,
            verticesPos[tri.v.x].position.xyz,
            verticesPos[tri.v.y].position.xyz,
            verticesPos[tri.v.z].position.xyz,
            asfloat(tri.v.w));

        if(triHit.w > 0 && triHit.w < intersection.t)
        {
            intersection.t = triHit.w;
            intersection.materialId = materialId;

            VertexData data0 = verticesData[tri.v.x];
            VertexData data1 = verticesData[tri.v.y];
            VertexData data2 = verticesData[tri.v.z];

            intersection.normal = normalize(
                triHit.x * data0.normal.xyz +
                triHit.y * data1.normal.xyz +
                triHit.z * data2.normal.xyz);

            intersection.uv =
                triHit.x * data0.uv +
                triHit.y * data1.uv +
                triHit.z * data2.uv;

            intersection.primitiveAreaPdf = triHit.w * triHit.w * asfloat(tri.v.w) * 2;

            intersected = true;
        }
    }

    return intersected;
}

bool intersectSphere(inout Intersection intersection, Probe probe, uint sphereId, uint materialId)
{
    Sphere sphere = spheres[sphereId];

    float t_ca = dot(-probe.origin, probe.direction);

    if(t_ca < 0)
        return false;

    float dSqr = (dot(probe.origin, probe.origin) - t_ca*t_ca);

    if(dSqr > sphere.radius * sphere.radius)
        return false;

    float t_hc = sqrt(sphere.radius * sphere.radius - dSqr);
    float t_0 = t_ca - t_hc;
    float t_1 = t_ca + t_hc;

    float t = (t_0 > 0) ? t_0 : (t_1 > 0 ? t_1 : -1);

    if(t > 0 && t < intersection.t)
    {
        // Ray origin is outside the sphere
        intersection.t = t;
        intersection.materialId = materialId;
        intersection.normal = normalize(probe.origin + probe.direction * t);

        if(t_0 == t)
        {
            float iRSqr = sphere.radius * sphere.radius;
            vec3 originToHit = (-probe.origin);
            float distanceSqr = dot(originToHit, originToHit);
            float sinThetaMaxSqr = iRSqr / distanceSqr;
            float cosThetaMax = sqrt(max(0, 1 - sinThetaMaxSqr));
            float solidAngle = 2 * PI * (1 - cosThetaMax);
            intersection.primitiveAreaPdf = 1 / solidAngle;
        }
        else
        {
            intersection.normal = -intersection.normal;
            intersection.primitiveAreaPdf = 1 / (4 * PI);
        }

        intersection.uv = findUV(intersection.normal);

        return true;

    }

    return false;
}

bool intersectPlane(inout Intersection intersection, Probe probe, uint planeId, uint materialId)
{
    float t = -probe.origin.z * probe.invDirection.z;

    if(t > 0 && t < intersection.t)
    {
        Plane plane = planes[planeId];

        intersection.t = t;
        intersection.materialId = materialId;
        intersection.normal = vec3(0, 0, -sign(probe.direction.z));
        intersection.uv = plane.invScale * (probe.origin + probe.direction * t).xy;
        intersection.primitiveAreaPdf = 1 / (2 * PI);

        return true;
    }

    return false;
}
