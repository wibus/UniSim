#ifndef GPUDEVICE_GL_H
#define GPUDEVICE_GL_H

#include "gpuprograminterface_gl.h"


namespace unisim
{

class GpuConstantResource;
class GpuStorageResource;
class GpuTextureResource;
class GpuImageResource;
class GpuGeometryResource;

struct GpuProgramConstantBindPoint;
struct GpuProgramStorageBindPoint;
struct GpuProgramTextureBindPoint;
struct GpuProgramImageBindPoint;


class GpuDevice
{
public:
    GpuDevice();
    ~GpuDevice();
    
    void bindBuffer(const GpuConstantResource& resource, const GpuProgramConstantBindPoint& bindPoint);
    void bindBuffer(const GpuStorageResource& resource, const GpuProgramStorageBindPoint& bindPoint);
    void bindTexture(const GpuTextureResource& resource, const GpuProgramTextureBindPoint& unit);
    void bindTexture(const GpuImageResource& resource, const GpuProgramTextureBindPoint& unit);
    void bindImage(const GpuImageResource& resource, const GpuProgramImageBindPoint& unit);

    void dispatch(unsigned int workGroupCountX, unsigned int workGroupCountY = 1, unsigned int workGroupCountZ = 1);
    void draw(const GpuGeometryResource& resource);

    void clearSwapChain();
private:

};

}

#endif // GPUDEVICE_GL_H
