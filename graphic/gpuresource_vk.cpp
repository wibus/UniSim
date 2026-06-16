#ifdef UNISIM_GRAPHIC_BACKEND_VK

#include "gpuresource.h"

#include <PilsCore/Utils/Assert.h>

#include "../resource/texture.h"


namespace unisim
{

// TEXTURE //

GpuTextureResource::GpuTextureResource(ResourceId id, Definition def) :
    GpuResource(id)
{
    PILS_ASSERT(false, "Not implemented!");
}

GpuTextureResource::GpuTextureResource(GpuTextureResourceHandle&& handle) :
    GpuResource(0),
    _handle(new GpuTextureResourceHandle(std::move(handle)))
{
    PILS_ASSERT(false, "Not implemented!");
}

GpuTextureResource::~GpuTextureResource()
{
    PILS_ASSERT(false, "Not implemented!");
}


// IMAGE //
GpuImageResource::GpuImageResource(ResourceId id, Definition def) :
    GpuResource(id)
{
    PILS_ASSERT(false, "Not implemented!");
}

GpuImageResource::~GpuImageResource()
{
    PILS_ASSERT(false, "Not implemented!");
}

void GpuImageResource::update(const Definition& def) const
{
    PILS_ASSERT(false, "Not implemented!");
}


// BINDLESS //
GpuBindlessResource::GpuBindlessResource(ResourceId id, Definition def) :
    GpuResource(id)
{
    PILS_ASSERT(false, "Not implemented!");
}

GpuBindlessResource::~GpuBindlessResource()
{
    PILS_ASSERT(false, "Not implemented!");
}


// BINDLESS DESCRIPTOR //
GpuBindlessTextureDescriptor::GpuBindlessTextureDescriptor(const unisim::GpuBindlessResourceHandle& bindless)
{
    PILS_ASSERT(false, "Not implemented!");
}


// STORAGE //
GpuStorageResource::GpuStorageResource(ResourceId id, Definition def) :
    GpuResource(id)
{
    PILS_ASSERT(false, "Not implemented!");
}

GpuStorageResource::~GpuStorageResource()
{
    PILS_ASSERT(false, "Not implemented!");
}

void GpuStorageResource::update(const Definition& def) const
{
    PILS_ASSERT(false, "Not implemented!");
}


// CONSTANT //
GpuConstantResource::GpuConstantResource(ResourceId id, Definition def) :
    GpuResource(id)
{
    PILS_ASSERT(false, "Not implemented!");
}

GpuConstantResource::~GpuConstantResource()
{
    PILS_ASSERT(false, "Not implemented!");
}

void GpuConstantResource::update(const Definition& def) const
{
    PILS_ASSERT(false, "Not implemented!");
}


// VERTEX ARRAY //
GpuGeometryResource::GpuGeometryResource(ResourceId id, Definition def) :
    GpuResource(id)
{
    PILS_ASSERT(false, "Not implemented!");
}

GpuGeometryResource::~GpuGeometryResource()
{
    PILS_ASSERT(false, "Not implemented!");
}

}

#endif // UNISIM_GRAPHIC_BACKEND_VK