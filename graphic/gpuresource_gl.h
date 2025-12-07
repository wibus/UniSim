#ifndef GPURESOURCE_GL_H
#define GPURESOURCE_GL_H

#include "graphic_gl.h"


namespace unisim
{

class GpuTextureResourceHandle
{
public:
    GpuTextureResourceHandle() : texId(0) {}

    GLuint texId;
};

class GpuImageResourceHandle
{
public:
    GpuImageResourceHandle() : texId(0), format(-1) {}

    GLuint texId;
    GLint format;
};

class GpuBindlessResourceHandle
{
public:
    GpuBindlessResourceHandle() : handle(0) {}

    GLuint64 handle;
};

class GpuBindlessTextureDescriptor
{
public:
    GpuBindlessTextureDescriptor() : texture(0), padding(0) {}
    GpuBindlessTextureDescriptor(const GpuBindlessResourceHandle& bindless);

    GLuint64 texture;
    GLuint64 padding;
};

class GpuStorageResourceHandle
{
public:
    GpuStorageResourceHandle() : bufferId(0) {}

    GLuint bufferId;
};

class GpuConstantResourceHandle
{
public:
    GpuConstantResourceHandle() : bufferId(0) {}

    GLuint bufferId;
};

}

#endif // GPURESOURCE_GL_H
