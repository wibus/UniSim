#include "gpudevice.h"

#include "PilsCore/Utils/Assert.h"

#include "gpuresource.h"
#include "gpuprograminterface.h"


namespace unisim
{

GpuDeviceGl::GpuDeviceGl()
{

}

GpuDeviceGl::~GpuDeviceGl()
{

}

void GpuDeviceGl::bindBuffer(const GpuConstantResource& resource, const GpuProgramConstantBindPoint& bindPoint)
{
    PILS_ASSERT(resource.handle().bufferId > 0, "Invalid constant buffer index");

    glBindBufferBase(GL_UNIFORM_BUFFER, bindPoint.bindPoint, resource.handle().bufferId);
}

void GpuDeviceGl::bindBuffer(const GpuStorageResource& resource, const GpuProgramStorageBindPoint& bindPoint)
{
    PILS_ASSERT(resource.handle().bufferId > 0, "Invalid storage buffer index");

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindPoint.bindPoint, resource.handle().bufferId);
}

void GpuDeviceGl::bindTexture(const GpuTextureResource& resource, const GpuProgramTextureUnit& unit)
{
    PILS_ASSERT(resource.handle().texId > 0, "Invalid texture index");

    glActiveTexture(GL_TEXTURE0 + unit.unit);
    glBindTexture(GL_TEXTURE_2D, resource.handle().texId);
}

void GpuDeviceGl::bindTexture(const GpuImageResource& resource, const GpuProgramTextureUnit& unit)
{
    PILS_ASSERT(resource.handle().texId > 0, "Invalid texture index");

    glActiveTexture(GL_TEXTURE0 + unit.unit);
    glBindTexture(GL_TEXTURE_2D, resource.handle().texId);
}

void GpuDeviceGl::bindImage(const GpuImageResource& resource, const GpuProgramImageUnit& unit)
{
    PILS_ASSERT(resource.handle().texId > 0, "Invalid image index");

    glBindImageTexture(unit.unit, resource.handle().texId, 0, false, 0, GL_WRITE_ONLY, resource.handle().format);
}

void GpuDeviceGl::dispatch(unsigned int workGroupCountX, unsigned int workGroupCountY, unsigned int workGroupCountZ)
{
    glDispatchCompute(workGroupCountX, workGroupCountY, workGroupCountZ);
}

void GpuDeviceGl::draw(const GpuGeometryResource& resource)
{
    glBindVertexArray(resource.handle().vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void GpuDeviceGl::clearSwapChain()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

}
