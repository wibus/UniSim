#ifndef GPUPROGRAMINTERFACE_GL_H
#define GPUPROGRAMINTERFACE_GL_H

#include "graphic_gl.h"


namespace unisim
{

struct GpuProgramConstantBindPoint
{
    GLuint bindPoint;

    static GpuProgramConstantBindPoint first();
    static GpuProgramConstantBindPoint invalid();
    static GpuProgramConstantBindPoint next(const GpuProgramConstantBindPoint& current);
};


struct GpuProgramStorageBindPoint
{
    GLuint bindPoint;

    static GpuProgramStorageBindPoint first();
    static GpuProgramStorageBindPoint invalid();
    static GpuProgramStorageBindPoint next(const GpuProgramStorageBindPoint& current);
};

struct GpuProgramTextureBindPoint
{
    GLuint bindPoint;

    static GpuProgramTextureBindPoint first();
    static GpuProgramTextureBindPoint invalid();
    static GpuProgramTextureBindPoint next(const GpuProgramTextureBindPoint& current);
};

struct GpuProgramImageBindPoint
{
    GLuint bindPoint;

    static GpuProgramImageBindPoint first();
    static GpuProgramImageBindPoint invalid();
    static GpuProgramImageBindPoint next(const GpuProgramImageBindPoint& current);
};

}

#endif // GPUPROGRAMINTERFACE_GL_H
