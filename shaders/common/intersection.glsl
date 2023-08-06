Intersection intersectMesh(Ray ray, uint meshId, uint materialId)
{
    Intersection intersection;
    intersection.t = -1.0;
    intersection.materialId = materialId;
    intersection.normal = vec3(0, 0, 1);
    intersection.uv = vec2(0, 0);
    intersection.primitiveAreaPdf = 1 / 0.0;

    // TODO

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
