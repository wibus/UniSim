#ifndef GPUPROGRAMINTERFACE_VK_H
#define GPUPROGRAMINTERFACE_VK_H

#include "graphic_vk.h"


namespace unisim
{

struct GpuProgramConstantBindPoint
{
    static GpuProgramConstantBindPoint first();
    static GpuProgramConstantBindPoint invalid();
    static GpuProgramConstantBindPoint next(const GpuProgramConstantBindPoint& current);
};


struct GpuProgramStorageBindPoint
{
    static GpuProgramStorageBindPoint first();
    static GpuProgramStorageBindPoint invalid();
    static GpuProgramStorageBindPoint next(const GpuProgramStorageBindPoint& current);
};

struct GpuProgramTextureBindPoint
{
    static GpuProgramTextureBindPoint first();
    static GpuProgramTextureBindPoint invalid();
    static GpuProgramTextureBindPoint next(const GpuProgramTextureBindPoint& current);
};

struct GpuProgramImageBindPoint
{
    static GpuProgramImageBindPoint first();
    static GpuProgramImageBindPoint invalid();
    static GpuProgramImageBindPoint next(const GpuProgramImageBindPoint& current);
};

}

#endif // GPUPROGRAMINTERFACE_VK_H
