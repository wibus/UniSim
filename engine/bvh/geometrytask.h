#ifndef GEOMETRYTASK_H
#define GEOMETRYTASK_H

#include <GLM/glm.hpp>

#include "../taskgraph/pathtracerprovider.h"


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


class GeometryTask : public PathTracerProviderTask
{
public:
    GeometryTask();

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

#endif // GEOMETRYTASK_H
