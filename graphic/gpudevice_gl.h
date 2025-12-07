#ifndef GPUDEVICE_GL_H
#define GPUDEVICE_GL_H


namespace unisim
{

class GpuConstantResource;
class GpuStorageResource;
class GpuTextureResource;
class GpuImageResource;
class GpuGeometryResource;


class GpuDeviceGl
{
public:
    GpuDeviceGl();
    ~GpuDeviceGl();

    void bindBuffer(const GpuConstantResource& resource, unsigned int bindPoint);
    void bindBuffer(const GpuStorageResource& resource, unsigned int bindPoint);
    void bindTexture(const GpuTextureResource& resource, unsigned int index);
    void bindTexture(const GpuImageResource& resource, unsigned int index);
    void bindImage(const GpuImageResource& resource, unsigned int index);

    void dispatch(unsigned int workGroupCountX, unsigned int workGroupCountY = 1, unsigned int workGroupCountZ = 1);
    void draw(const GpuGeometryResource& resource);

    void clearSwapChain();
private:

};


typedef GpuDeviceGl GpuDevice;

}

#endif // GPUDEVICE_GL_H
