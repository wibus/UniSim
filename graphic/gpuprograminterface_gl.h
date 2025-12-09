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

struct GpuProgramTextureUnit
{
    GLuint unit;

    static GpuProgramTextureUnit first();
    static GpuProgramTextureUnit invalid();
    static GpuProgramTextureUnit next(const GpuProgramTextureUnit& current);
};

struct GpuProgramImageUnit
{
    GLuint unit;

    static GpuProgramImageUnit first();
    static GpuProgramImageUnit invalid();
    static GpuProgramImageUnit next(const GpuProgramImageUnit& current);
};

}

#endif // GPUPROGRAMINTERFACE_GL_H
