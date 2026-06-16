#ifndef GPURESOURCE_VK_H
#define GPURESOURCE_VK_H

#include "graphic_vk.h"


namespace unisim
{

class GpuTextureResourceHandle
{
public:
    GpuTextureResourceHandle() {}
};

class GpuImageResourceHandle
{
public:
    GpuImageResourceHandle() {}
};

class GpuBindlessResourceHandle
{
public:
    GpuBindlessResourceHandle() {}
};

class GpuBindlessTextureDescriptor
{
public:
    GpuBindlessTextureDescriptor() {}
    GpuBindlessTextureDescriptor(const GpuBindlessResourceHandle& bindless);
};

class GpuStorageResourceHandle
{
public:
    GpuStorageResourceHandle() {}
};

class GpuConstantResourceHandle
{
public:
    GpuConstantResourceHandle() {}
};

class GpuGeometryResourceHandle
{
public:
    GpuGeometryResourceHandle() {}
};

}

#endif // GPURESOURCE_VK_H
