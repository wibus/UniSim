#include "gpudevice.h"

#include "PilsCore/Utils/Assert.h"

#include "gpuresource.h"


namespace unisim
{

GpuDeviceGl::GpuDeviceGl()
{

}

GpuDeviceGl::~GpuDeviceGl()
{

}

void GpuDeviceGl::bindBuffer(const GpuConstantResource& resource, unsigned int bindPoint)
{
    PILS_ASSERT(resource.handle().bufferId > 0, "Invalid constant buffer index");

    glBindBufferBase(GL_UNIFORM_BUFFER, bindPoint, resource.handle().bufferId);
}

void GpuDeviceGl::bindBuffer(const GpuStorageResource& resource, unsigned int bindPoint)
{
    PILS_ASSERT(resource.handle().bufferId > 0, "Invalid storage buffer index");

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindPoint, resource.handle().bufferId);
}

void GpuDeviceGl::bindTexture(const GpuTextureResource& resource, unsigned int index)
{
    PILS_ASSERT(resource.handle().texId > 0, "Invalid texture index");

    glActiveTexture(GL_TEXTURE0 + index);
    glBindTexture(GL_TEXTURE_2D, resource.handle().texId);
}

void GpuDeviceGl::bindTexture(const GpuImageResource& resource, unsigned int index)
{
    PILS_ASSERT(resource.handle().texId > 0, "Invalid texture index");

    glActiveTexture(GL_TEXTURE0 + index);
    glBindTexture(GL_TEXTURE_2D, resource.handle().texId);
}

void GpuDeviceGl::bindImage(const GpuImageResource& resource, unsigned int index)
{
    PILS_ASSERT(resource.handle().texId > 0, "Invalid image index");

    glBindImageTexture(index, resource.handle().texId, 0, false, 0, GL_WRITE_ONLY, resource.handle().format);
}

}
