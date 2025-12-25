#include "gpudevice.h"

#include "PilsCore/Utils/Assert.h"

#include "gpuresource.h"
#include "gpuprograminterface.h"


namespace unisim
{

GpuDevice::GpuDevice()
{

}

GpuDevice::~GpuDevice()
{

}

void GpuDevice::bindBuffer(const GpuConstantResource& resource, const GpuProgramConstantBindPoint& bindPoint)
{
    PILS_ASSERT(resource.handle().bufferId > 0, "Invalid constant buffer index");

    glBindBufferBase(GL_UNIFORM_BUFFER, bindPoint.bindPoint, resource.handle().bufferId);
}

void GpuDevice::bindBuffer(const GpuStorageResource& resource, const GpuProgramStorageBindPoint& bindPoint)
{
    PILS_ASSERT(resource.handle().bufferId > 0, "Invalid storage buffer index");

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindPoint.bindPoint, resource.handle().bufferId);
}

void GpuDevice::bindTexture(const GpuTextureResource& resource, const GpuProgramTextureBindPoint& bindPoint)
{
    PILS_ASSERT(resource.handle().texId > 0, "Invalid texture index");

    glActiveTexture(GL_TEXTURE0 + bindPoint.bindPoint);
    glBindTexture(resource.handle().dimension, resource.handle().texId);
}

void GpuDevice::bindTexture(const GpuImageResource& resource, const GpuProgramTextureBindPoint& unit)
{
    PILS_ASSERT(resource.handle().texId > 0, "Invalid texture index");

    glActiveTexture(GL_TEXTURE0 + unit.bindPoint);
    glBindTexture(resource.handle().dimension, resource.handle().texId);
}

void GpuDevice::bindImage(const GpuImageResource& resource, const GpuProgramImageBindPoint& unit)
{
    PILS_ASSERT(resource.handle().texId > 0, "Invalid image index");

    glBindImageTexture(unit.bindPoint, resource.handle().texId, 0, false, 0, GL_WRITE_ONLY, resource.handle().format);
}

void GpuDevice::dispatch(unsigned int workGroupCountX, unsigned int workGroupCountY, unsigned int workGroupCountZ)
{
    glDispatchCompute(workGroupCountX, workGroupCountY, workGroupCountZ);
}

void GpuDevice::draw(const GpuGeometryResource& resource)
{
    glBindVertexArray(resource.handle().vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void GpuDevice::clearSwapChain()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

}
