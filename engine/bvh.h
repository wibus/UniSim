#ifndef BVH_H
#define BVH_H

#include <GLM/glm.hpp>

#include "pathtracer.h"


namespace unisim
{

struct GpuPrimitive;
struct GpuMesh;
struct GpuSphere;
struct GpuPlane;
struct GpuInstance;
struct GpuBvhNode;
struct GpuTriangle;
struct GpuVertexPos;
struct GpuVertexData;


class BVH : public PathTracerProvider
{
public:
    BVH();

    bool defineResources(GraphicContext& context) override;

    bool definePathTracerModules(
        GraphicContext& context,
        std::vector<std::shared_ptr<PathTracerModule>>& modules) override;
    bool definePathTracerInterface(
        GraphicContext& context,
        PathTracerInterface& interface) override;
    void bindPathTracerResources(
        GraphicContext& context,
        CompiledGpuProgramInterface& compiledGpi) const override;
    
    void update(GraphicContext& context) override;
    void render(GraphicContext& context) override;

private:
    uint64_t toGpu(
        const GraphicContext& context,
            std::vector<GpuPrimitive>& gpuPrimitives,
            std::vector<GpuMesh>& gpuMeshes,
            std::vector<GpuSphere>& gpuSpheres,
            std::vector<GpuPlane>& gpuPlanes,
            std::vector<GpuInstance>& gpuInstances,
            std::vector<GpuBvhNode>& gpuBvhNodes,
            std::vector<GpuTriangle>& gpuTriangles,
            std::vector<GpuVertexPos>& gpuVertPos,
            std::vector<GpuVertexData>& gpuVertData);
};


}

#endif // BVH_H
