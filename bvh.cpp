#include "bvh.h"

#include "scene.h"
#include "object.h"
#include "body.h"
#include "primitive.h"
#include "material.h"


namespace unisim
{

DefineResource(Primitives);
DefineResource(Meshes);
DefineResource(Spheres);
DefineResource(Planes);
DefineResource(Instances);

struct GpuPrimitive
{
    GLuint type;
    GLuint index;
    GLuint material;
    GLuint pad1;
};

struct GpuMesh
{
    GLuint vertexStart;
    GLuint vertexEnd;
    GLuint triangleStart;
    GLuint triangleEnd;
    GLuint submeshStart;
    GLuint submeshEnd;
};

struct GpuSphere
{
    GLfloat radius;
};

struct GpuPlane
{
    GLfloat invScale;
};

struct GpuInstance
{
    glm::vec4 position;
    glm::vec4 quaternion;

    uint primitiveBegin;
    uint primitiveEnd;

    int pad1;
    int pad2;
};


BVH::BVH() :
    GraphicTask("BVH")
{

}

void BVH::registerDynamicResources(GraphicContext& context)
{

}

bool BVH::definePathTracerModules(GraphicContext& context)
{
    GLuint moduleId = 0;
    if(!addPathTracerModule(moduleId, context.settings, "shaders/common/intersection.glsl"))
        return false;

    return true;
}

bool BVH::defineResources(GraphicContext& context)
{
    std::vector<GpuPrimitive> gpuPrimitives;
    std::vector<GpuMesh> gpuMeshes;
    std::vector<GpuSphere> gpuSpheres;
    std::vector<GpuPlane> gpuPlanes;
    std::vector<GpuInstance> gpuInstances;

    _hash = toGpu(context,
          gpuPrimitives,
          gpuMeshes,
          gpuSpheres,
          gpuPlanes,
          gpuInstances);

    ResourceManager& resources = context.resources;

    bool ok = true;

    ok = ok && resources.define<GpuStorageResource>(
        ResourceName(Primitives), {
            sizeof(GpuPrimitive),
            gpuPrimitives.size(),
            gpuPrimitives.data()});

    ok = ok && resources.define<GpuStorageResource>(
        ResourceName(Meshes), {
            sizeof(GpuMesh),
            gpuMeshes.size(),
            gpuMeshes.data()});

    ok = ok && resources.define<GpuStorageResource>(
        ResourceName(Spheres), {
            sizeof(GpuSphere),
            gpuSpheres.size(),
            gpuSpheres.data()});

    ok = ok && resources.define<GpuStorageResource>(
        ResourceName(Planes), {
            sizeof(GpuPlane),
            gpuPlanes.size(),
            gpuPlanes.data()});

    ok = ok && resources.define<GpuStorageResource>(
        ResourceName(Instances), {
            sizeof(GpuInstance),
            gpuInstances.size(),
            gpuInstances.data()});

    return ok;
}

void BVH::setPathTracerResources(
        GraphicContext& context,
        PathTracerInterface& interface) const
{
    ResourceManager& resources = context.resources;

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, interface.getSsboBindPoint("Primitives"),
                     resources.get<GpuStorageResource>(ResourceName(Primitives)).bufferId);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, interface.getSsboBindPoint("Meshes"),
                     resources.get<GpuStorageResource>(ResourceName(Meshes)).bufferId);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, interface.getSsboBindPoint("Spheres"),
                     resources.get<GpuStorageResource>(ResourceName(Spheres)).bufferId);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, interface.getSsboBindPoint("Planes"),
                     resources.get<GpuStorageResource>(ResourceName(Planes)).bufferId);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, interface.getSsboBindPoint("Instances"),
                     resources.get<GpuStorageResource>(ResourceName(Instances)).bufferId);
}

void BVH::update(GraphicContext& context)
{
    std::vector<GpuPrimitive> gpuPrimitives;
    std::vector<GpuMesh> gpuMeshes;
    std::vector<GpuSphere> gpuSpheres;
    std::vector<GpuPlane> gpuPlanes;
    std::vector<GpuInstance> gpuInstances;

    uint64_t hash = toGpu(context,
          gpuPrimitives,
          gpuMeshes,
          gpuSpheres,
          gpuPlanes,
          gpuInstances);

    if(_hash == hash)
        return;

    _hash = hash;

    ResourceManager& resources = context.resources;

    resources.get<GpuStorageResource>(
        ResourceName(Primitives)).update({
            sizeof(GpuPrimitive),
            gpuPrimitives.size(),
            gpuPrimitives.data()});

    resources.get<GpuStorageResource>(
        ResourceName(Meshes)).update({
            sizeof(GpuMesh),
            gpuMeshes.size(),
            gpuMeshes.data()});

    resources.get<GpuStorageResource>(
        ResourceName(Spheres)).update({
            sizeof(GpuSphere),
            gpuSpheres.size(),
            gpuSpheres.data()});

    resources.get<GpuStorageResource>(
        ResourceName(Planes)).update({
            sizeof(GpuPlane),
            gpuPlanes.size(),
            gpuPlanes.data()});

    resources.get<GpuStorageResource>(
        ResourceName(Instances)).update({
            sizeof(GpuInstance),
            gpuInstances.size(),
            gpuInstances.data()});
}

void BVH::render(GraphicContext& context)
{
}

uint64_t BVH::toGpu(
        const GraphicContext& context,
        std::vector<GpuPrimitive>& gpuPrimitives,
        std::vector<GpuMesh>& gpuMeshes,
        std::vector<GpuSphere>& gpuSpheres,
        std::vector<GpuPlane>& gpuPlanes,
        std::vector<GpuInstance>& gpuInstances)
{
    const auto& objects = context.scene.objects();
    for(const std::shared_ptr<Object>& object : objects)
    {
        GpuInstance& gpuInstance = gpuInstances.emplace_back();
        gpuInstance.position = glm::vec4(object->body()->position(), 0.0);
        gpuInstance.quaternion = glm::vec4(quatConjugate(object->body()->quaternion()));

        gpuInstance.primitiveBegin = gpuPrimitives.size();
        for(const std::shared_ptr<Primitive>& primitive : object->primitives())
        {
            GpuPrimitive& gpuPrimitive = gpuPrimitives.emplace_back();
            gpuPrimitive.type = primitive->type();
            gpuPrimitive.material =  context.materials.materialId(primitive->material());
            gpuPrimitive.pad1 = 0;

            switch(primitive->type())
            {
            case Primitive::Mesh :
            {
                gpuPrimitive.index = gpuMeshes.size();

                const Mesh& mesh = static_cast<Mesh&>(*primitive.get());
                GpuMesh& gpuMesh = gpuMeshes.emplace_back();
                gpuMesh.vertexStart = 0;
                gpuMesh.vertexEnd = 0;
                gpuMesh.triangleStart = 0;
                gpuMesh.triangleEnd = 0;
                gpuMesh.submeshStart = 0;
                gpuMesh.submeshEnd = 0;
            }
                break;
            case Primitive::Sphere :
            {
                gpuPrimitive.index = gpuSpheres.size();

                const Sphere& sphere = static_cast<Sphere&>(*primitive.get());
                GpuSphere& gpuSphere = gpuSpheres.emplace_back();
                gpuSphere.radius = sphere.radius();
            }
                break;
            case Primitive::Plane :
            {
                gpuPrimitive.index = gpuPlanes.size();

                const Plane& plane = static_cast<Plane&>(*primitive.get());
                GpuPlane& gpuPlane = gpuPlanes.emplace_back();
                gpuPlane.invScale = 1 / plane.scale();
            }
                break;
            default:
                assert(false /* Unknown primitive type */);
                std::cerr << "Unknown primitive type " << primitive->type() << std::endl;
            }
        }
        gpuInstance.primitiveEnd = gpuPrimitives.size();

        gpuInstance.pad1 = 0;
        gpuInstance.pad2 = 0;
    }

    uint64_t hash = 0;
    hash = hashVec(gpuPrimitives, hash);
    hash = hashVec(gpuMeshes, hash);
    hash = hashVec(gpuSpheres, hash);
    hash = hashVec(gpuPlanes, hash);
    hash = hashVec(gpuInstances, hash);

    return hash;
}


}
