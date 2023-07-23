uniform float terrainHeight;

Intersection intersectTerrain(in Ray ray, in uint instanceId)
{
    Intersection intersection;
    intersection.t = (terrainHeight - ray.origin.z) / ray.direction.z;
    intersection.instanceId = instanceId;

    return intersection;
}
