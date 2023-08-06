#ifndef BVH_H
#define BVH_H

#include <GLM/glm.hpp>

#include "graphic.h"


namespace unisim
{

struct GpuPrimitive;
struct GpuMesh;
struct GpuSphere;
struct GpuPlane;
struct GpuInstance;


struct BVHNode
{
    glm::vec3 aabbMin;
    unsigned int leftFirst;
    glm::vec3 aabbMax;
    unsigned int triCount;
};


class BVH : public GraphicTask
{
public:
    BVH();

    void registerDynamicResources(GraphicContext& context) override;
    bool definePathTracerModules(GraphicContext& context) override;
    bool defineResources(GraphicContext& context) override;

    void setPathTracerResources(
            GraphicContext& context,
            PathTracerInterface& interface) const override;

    void update(GraphicContext& context) override;
    void render(GraphicContext& context) override;

private:
    uint64_t toGpu(
            const GraphicContext& context,
            std::vector<GpuPrimitive>& gpuPrimitives,
            std::vector<GpuMesh>& gpuMeshes,
            std::vector<GpuSphere>& gpuSpheres,
            std::vector<GpuPlane>& gpuPlanes,
            std::vector<GpuInstance>& gpuInstances);

    GLuint _primitivesSSBO;
    GLuint _meshesSSBO;
    GLuint _spheresSSBO;
    GLuint _planesSSBO;
    GLuint _instanceSSBO;
};


}

#endif // BVH_H
