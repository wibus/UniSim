#include "bvh.h"

#include <iostream>

#include "../system/profiler.h"
#include "../system/units.h"

#include "../resource/body.h"
#include "../resource/instance.h"
#include "../resource/primitive.h"

#include "../graphic/gpudevice.h"

#include "scene.h"
#include "materialdatabase.h"


namespace unisim
{

DefineProfilePoint(BVH);

DefineResource(Primitives);
DefineResource(Meshes);
DefineResource(Spheres);
DefineResource(Planes);
DefineResource(Instances);
DefineResource(BvhNodes);
DefineResource(Triangles);
DefineResource(VerticesPos);
DefineResource(VerticesData);

struct GpuPrimitive
{
    GLuint type;
    GLuint index;
    GLuint material;
    GLuint pad1;
};

struct GpuMesh
{
    GLuint bvhNode;
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

struct GpuBvhNode
{
    glm::vec3 aabbMin;
    unsigned int leftFirst;
    glm::vec3 aabbMax;
    unsigned int triCount;
};

struct GpuTriangle
{
    uint v0;
    uint v1;
    uint v2;
    GLfloat inv2Area;
};

struct GpuVertexPos
{
    glm::vec4 position;
};

struct GpuVertexData
{
    glm::vec4 normal;
    glm::vec2 uv;
    glm::vec2 pad1;
};


BVH::BVH() :
    PathTracerProvider("BVH")
{
}

bool BVH::defineResources(GraphicContext& context)
{
    std::vector<GpuPrimitive> gpuPrimitives;
    std::vector<GpuMesh> gpuMeshes;
    std::vector<GpuSphere> gpuSpheres;
    std::vector<GpuPlane> gpuPlanes;
    std::vector<GpuInstance> gpuInstances;
    std::vector<GpuBvhNode> gpuBvhNodes;
    std::vector<GpuTriangle> gpuTriangles;
    std::vector<GpuVertexPos> gpuVerticesPos;
    std::vector<GpuVertexData> gpuVerticesData;

    _hash = toGpu(context,
                  gpuPrimitives,
                  gpuMeshes,
                  gpuSpheres,
                  gpuPlanes,
                  gpuInstances,
                  gpuBvhNodes,
                  gpuTriangles,
                  gpuVerticesPos,
                  gpuVerticesData);

    GpuResourceManager& resources = context.resources;

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

    ok = ok && resources.define<GpuStorageResource>(
             ResourceName(BvhNodes), {
              sizeof(GpuBvhNode),
              gpuBvhNodes.size(),
              gpuBvhNodes.data()});

    ok = ok && resources.define<GpuStorageResource>(
             ResourceName(Triangles), {
              sizeof(GpuTriangle),
              gpuTriangles.size(),
              gpuTriangles.data()});

    ok = ok && resources.define<GpuStorageResource>(
             ResourceName(VerticesPos), {
              sizeof(GpuVertexPos),
              gpuVerticesPos.size(),
              gpuVerticesPos.data()});

    ok = ok && resources.define<GpuStorageResource>(
             ResourceName(VerticesData), {
              sizeof(GpuVertexData),
              gpuVerticesData.size(),
              gpuVerticesData.data()});

    return ok;
}

bool BVH::definePathTracerModules(GraphicContext& context, std::vector<std::shared_ptr<PathTracerModule>>& modules)
{
    if(!addPathTracerModule(modules, "Instersection", context.settings, "shaders/common/intersection.glsl"))
        return false;

    return true;
}

bool BVH::definePathTracerInterface(GraphicContext& context, PathTracerInterface& interface)
{
    bool ok = true;

    ok = ok && interface.declareStorage({"Primitives"});
    ok = ok && interface.declareStorage({"Meshes"});
    ok = ok && interface.declareStorage({"Spheres"});
    ok = ok && interface.declareStorage({"Planes"});
    ok = ok && interface.declareStorage({"Instances"});
    ok = ok && interface.declareStorage({"BvhNodes"});
    ok = ok && interface.declareStorage({"Triangles"});
    ok = ok && interface.declareStorage({"VerticesPos"});
    ok = ok && interface.declareStorage({"VerticesData"});

    return ok;
}

void BVH::bindPathTracerResources(
    GraphicContext& context,
    CompiledGpuProgramInterface& compiledGpi) const
{
    GpuResourceManager& resources = context.resources;

    context.device.bindBuffer(resources.get<GpuStorageResource>(ResourceName(Primitives)),      compiledGpi.getStorageBindPoint("Primitives"));
    context.device.bindBuffer(resources.get<GpuStorageResource>(ResourceName(Meshes)),          compiledGpi.getStorageBindPoint("Meshes"));
    context.device.bindBuffer(resources.get<GpuStorageResource>(ResourceName(Spheres)),         compiledGpi.getStorageBindPoint("Spheres"));
    context.device.bindBuffer(resources.get<GpuStorageResource>(ResourceName(Planes)),          compiledGpi.getStorageBindPoint("Planes"));
    context.device.bindBuffer(resources.get<GpuStorageResource>(ResourceName(Instances)),       compiledGpi.getStorageBindPoint("Instances"));
    context.device.bindBuffer(resources.get<GpuStorageResource>(ResourceName(BvhNodes)),        compiledGpi.getStorageBindPoint("BvhNodes"));
    context.device.bindBuffer(resources.get<GpuStorageResource>(ResourceName(Triangles)),       compiledGpi.getStorageBindPoint("Triangles"));
    context.device.bindBuffer(resources.get<GpuStorageResource>(ResourceName(VerticesPos)),     compiledGpi.getStorageBindPoint("VerticesPos"));
    context.device.bindBuffer(resources.get<GpuStorageResource>(ResourceName(VerticesData)),    compiledGpi.getStorageBindPoint("VerticesData"));
}

void BVH::update(GraphicContext& context)
{
    Profile(BVH);

    std::vector<GpuPrimitive> gpuPrimitives;
    std::vector<GpuMesh> gpuMeshes;
    std::vector<GpuSphere> gpuSpheres;
    std::vector<GpuPlane> gpuPlanes;
    std::vector<GpuInstance> gpuInstances;
    std::vector<GpuBvhNode> gpuBvhNodes;
    std::vector<GpuTriangle> gpuTriangles;
    std::vector<GpuVertexPos> gpuVerticesPos;
    std::vector<GpuVertexData> gpuVerticesData;

    uint64_t hash = toGpu(context,
          gpuPrimitives,
          gpuMeshes,
          gpuSpheres,
          gpuPlanes,
          gpuInstances,
          gpuBvhNodes,
          gpuTriangles,
          gpuVerticesPos,
          gpuVerticesData);

    if(_hash == hash)
        return;

    _hash = hash;
    
    GpuResourceManager& resources = context.resources;

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

    resources.get<GpuStorageResource>(
        ResourceName(BvhNodes)).update({
            sizeof(GpuBvhNode),
            gpuBvhNodes.size(),
            gpuBvhNodes.data()});

    resources.get<GpuStorageResource>(
        ResourceName(Triangles)).update({
            sizeof(GpuTriangle),
            gpuTriangles.size(),
            gpuTriangles.data()});

    resources.get<GpuStorageResource>(
        ResourceName(VerticesPos)).update({
            sizeof(GpuVertexPos),
            gpuVerticesPos.size(),
            gpuVerticesPos.data()});

    resources.get<GpuStorageResource>(
        ResourceName(VerticesData)).update({
            sizeof(GpuVertexData),
            gpuVerticesData.size(),
            gpuVerticesData.data()});
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
        std::vector<GpuInstance>& gpuInstances,
        std::vector<GpuBvhNode>& gpuBvhNodes,
        std::vector<GpuTriangle>& gpuTriangles,
        std::vector<GpuVertexPos>& gpuVerticesPos,
        std::vector<GpuVertexData>& gpuVerticesData)
{
    const auto& instances = context.scene.instances();
    for(const std::shared_ptr<Instance>& instance : instances)
    {
        GpuInstance& gpuInstance = gpuInstances.emplace_back();
        gpuInstance.position = glm::vec4(instance->body()->position(), 0.0);
        gpuInstance.quaternion = glm::vec4(quatConjugate(instance->body()->quaternion()));

        gpuInstance.primitiveBegin = gpuPrimitives.size();
        for(const std::shared_ptr<Primitive>& primitive : instance->primitives())
        {
            GpuPrimitive& gpuPrimitive = gpuPrimitives.emplace_back();
            gpuPrimitive.type = primitive->type();
            gpuPrimitive.material = context.scene.materials()->materialId(primitive->material());
            gpuPrimitive.pad1 = 0;

            switch(primitive->type())
            {
            case Primitive::Mesh :
            {
                gpuPrimitive.index = gpuMeshes.size();

                const Mesh& mesh = static_cast<Mesh&>(*primitive.get());
                GpuMesh& gpuMesh = gpuMeshes.emplace_back();
                gpuMesh.bvhNode = gpuBvhNodes.size();

                GpuBvhNode& gpuBvhNode = gpuBvhNodes.emplace_back();
                gpuBvhNode.leftFirst = gpuTriangles.size();
                gpuBvhNode.triCount = mesh.triangles().size();
                gpuBvhNode.aabbMin = glm::vec3(MAXFLOAT);
                gpuBvhNode.aabbMax = -glm::vec3(MAXFLOAT);

                uint32_t vertexOffset = gpuVerticesPos.size();
                for(const Vertex& vert : mesh.vertices())
                {
                    gpuVerticesPos.push_back({glm::vec4(vert.position, 1.0f)});
                    gpuVerticesData.push_back({glm::vec4(vert.nornal, 0.0), {vert.uv}, {0, 0}});
                    gpuBvhNode.aabbMin = glm::min(vert.position, gpuBvhNode.aabbMin);
                    gpuBvhNode.aabbMax = glm::max(vert.position, gpuBvhNode.aabbMax);
                }

                for(const Triangle& tri : mesh.triangles())
                {
                    const glm::vec3& A = mesh.vertices()[tri.v[0]].position;
                    const glm::vec3& B = mesh.vertices()[tri.v[1]].position;
                    const glm::vec3& C = mesh.vertices()[tri.v[2]].position;

                    float inv2Area = 1.0f / glm::length(glm::cross(B-A, C-A));

                    gpuTriangles.push_back({
                        vertexOffset + tri.v[0],
                        vertexOffset + tri.v[1],
                        vertexOffset + tri.v[2],
                        inv2Area
                    });
                }
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
    hash = hashVec(gpuBvhNodes, hash);
    hash = hashVec(gpuTriangles, hash);
    hash = hashVec(gpuVerticesPos, hash);
    hash = hashVec(gpuVerticesData, hash);

    return hash;
}


}
