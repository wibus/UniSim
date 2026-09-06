#ifdef UNISIM_GRAPHIC_BACKEND_VK

#include "gpuresource.h"

#include <PilsCore/Utils/Assert.h>

#include <PilsCore/Gpu/Command.h>
#include <PilsCore/Gpu/Memory.h>
#include <PilsCore/Gpu/Context.h>
#include <PilsCore/Gpu/Layout.h>

#include "../resource/texture.h"

#include "gpudevice.h"


namespace unisim
{

pils::gpu::ImageFormat toPils(const TextureFormat format)
{
    switch(format)
    {
    case TextureFormat::R8G8B8A8_UNORM:
        return pils::gpu::ImageFormat::RGBA_UNORM8;
        break;
    case TextureFormat::R32G32B32A32_FLOAT:
        return pils::gpu::ImageFormat::RGBA_32F;
        break;
    default:
        PILS_FATAL("Texture format not supported");
        return pils::gpu::ImageFormat::RGBA_UNORM8;
    }
}

pils::pilsU64 toTexelSize(const TextureFormat format)
{
    switch(format)
    {
    case TextureFormat::R8G8B8A8_UNORM:
        return 4;
        break;
    case TextureFormat::R32G32B32A32_FLOAT:
        return 16;
        break;
    default:
        PILS_FATAL("Texture format not supported");
        return 4;
    }
}


// HANDLES //
GpuTextureResourceHandle::GpuTextureResourceHandle(
        const pils::gpu::ImageView& textureView,
        const pils::gpu::Sampler& sampler) :
    textureView(textureView),
    sampler(sampler)
{

}

GpuImageResourceHandle::GpuImageResourceHandle(
        const pils::gpu::ImageView& imageView) :
    imageView(imageView)
{
}

GpuBindlessResourceHandle::GpuBindlessResourceHandle()
{
    PILS_ASSERT(false, "Not implemented!");
}

GpuBindlessTextureDescriptor::GpuBindlessTextureDescriptor()
{
    PILS_ASSERT(false, "Not implemented!");
}

GpuBindlessTextureDescriptor::GpuBindlessTextureDescriptor(const unisim::GpuBindlessResourceHandle& bindless)
{
    PILS_ASSERT(false, "Not implemented!");
}

GpuStorageResourceHandle::GpuStorageResourceHandle(
        const pils::gpu::StorageBuffer& buffer,
        uint64_t offset,
        uint64_t size) :
    buffer(buffer),
    offset(offset),
    size(size)
{
}

GpuConstantResourceHandle::GpuConstantResourceHandle(
        const pils::gpu::ConstantBuffer& buffer,
        uint64_t offset,
        uint64_t size) :
    buffer(buffer),
    offset(offset),
    size(size)
{
}

GpuGeometryResourceHandle::GpuGeometryResourceHandle(
        const pils::gpu::VertexBuffer& buffer,
        uint64_t offset,
        uint64_t size) :
    buffer(buffer),
    offset(offset),
    size(size)
{
}


// TEXTURE //

GpuTextureResource::GpuTextureResource(GpuResourceManager& manager, ResourceId id, Definition def) :
    GpuResource(manager, id)
{
    const pils::gpu::Device& device = _manager.gpuDevice().context().device();

    _texture.reset(
        new pils::gpu::Texture(
            device,
            def.texture.width,
            def.texture.height,
            def.texture.depth,
            def.texture.depth > 1 ? pils::gpu::ImageType::Img3D : pils::gpu::ImageType::Img2D,
            toPils(def.texture.format),
            1)
        );

    _textureView.reset(new pils::gpu::ImageView(device, *_texture));
    _sampler.reset(new pils::gpu::Sampler(device));
    _handle.reset(new GpuTextureResourceHandle(*_textureView, *_sampler));

    // Upload data to memory
    pils::gpu::BufferSet& bufferSet = _manager.bufferSet();

    pils::pilsU64 texelSize = toTexelSize(def.texture.format);
    pils::pilsU64 texelCount = def.texture.width * def.texture.height * def.texture.depth;
    pils::gpu::Memory::Mapping transferMapping = bufferSet.registerTransfer(texelSize, texelCount);

    void* dst = bufferSet.transferBuffer().map(transferMapping);
    memcpy(dst, &def.texture.data[0], texelSize * texelCount);
    bufferSet.transferBuffer().unmapAndFlush(transferMapping);

    glm::uvec3 imgOffset = glm::uvec3(0, 0, 0);
    glm::uvec3 imgExtent = glm::uvec3(def.texture.width, def.texture.height, def.texture.depth);
    _manager.gpuDevice().commandList().copyImageBuffer(*_texture, imgOffset, imgExtent, bufferSet.transferBuffer(), transferMapping.offset);
}

GpuTextureResource::GpuTextureResource(GpuResourceManager& manager, GpuTextureResourceHandle&& handle) :
    GpuResource(manager, Invalid_ResourceId),
    _handle(new GpuTextureResourceHandle(std::move(handle)))
{
}

GpuTextureResource::~GpuTextureResource()
{
}


// IMAGE //
GpuImageResource::GpuImageResource(GpuResourceManager& manager, ResourceId id, Definition def) :
    GpuResource(manager, id)
{
    const pils::gpu::Device& device = _manager.gpuDevice().context().device();

    _image.reset(
        new pils::gpu::Texture(
            device,
            def.width,
            def.height,
            def.depth,
            def.depth > 1 ? pils::gpu::ImageType::Img3D : pils::gpu::ImageType::Img2D,
            toPils(def.format),
            1)
        );

    _imageView.reset(new pils::gpu::ImageView(device, *_image));
    _handle.reset(new GpuImageResourceHandle(*_imageView));
}

GpuImageResource::~GpuImageResource()
{
}

void GpuImageResource::update(const Definition& def)
{
    _image.reset(
        new pils::gpu::Texture(
            _manager.gpuDevice().context().device(),
            def.width,
            def.height,
            def.depth,
            def.depth > 1 ? pils::gpu::ImageType::Img3D : pils::gpu::ImageType::Img2D,
            toPils(def.format),
            1)
        );
}


// BINDLESS //
GpuBindlessResource::GpuBindlessResource(GpuResourceManager& manager, ResourceId id, Definition def) :
    GpuResource(manager, id)
{
    PILS_ASSERT(false, "Not implemented!");
}

GpuBindlessResource::~GpuBindlessResource()
{
    PILS_ASSERT(false, "Not implemented!");
}


// STORAGE //
GpuStorageResource::GpuStorageResource(GpuResourceManager& manager, ResourceId id, Definition def) :
    GpuResource(manager, id)
{
    pils::pilsU64 dataSize = def.elemSize * def.elemCount;
    PILS_ASSERT(dataSize > 0, "Cannot allocate 0 size storage");

    pils::gpu::BufferSet& bufferSet = _manager.bufferSet();

    pils::gpu::Memory::Mapping mapping = bufferSet.registerStorage(def.elemSize, def.elemCount);

    _handle.reset(new GpuStorageResourceHandle(bufferSet.storageBuffer(), mapping.offset, mapping.size));

    if (def.data != nullptr && dataSize > 0)
    {
        pils::gpu::Memory::Mapping transferMapping = bufferSet.registerTransfer(def.elemSize, def.elemCount);

        void* dst = bufferSet.transferBuffer().map(transferMapping);
        memcpy(dst, def.data, def.elemSize * def.elemCount);
        bufferSet.transferBuffer().unmap(transferMapping);

        PILS_ASSERT(mapping.size == transferMapping.size, "Storage and Transfert buffer mapping sizes do not match");
        _manager.gpuDevice().commandList().copyBuffer(bufferSet.storageBuffer(), mapping.offset, bufferSet.transferBuffer(), transferMapping.offset, transferMapping.size);
    }
}

GpuStorageResource::~GpuStorageResource()
{
}

void GpuStorageResource::update(const Definition& def)
{
    pils::pilsU64 dataSize = def.elemSize * def.elemCount;
    PILS_ASSERT(dataSize == _handle->size, "Storage resource resize not implemented");

    pils::gpu::BufferSet& bufferSet = _manager.bufferSet();

    if (def.data != nullptr && dataSize > 0)
    {
        pils::gpu::Memory::Mapping transferMapping = bufferSet.registerTransfer(def.elemSize, def.elemCount);

        void* dst = bufferSet.transferBuffer().map(transferMapping);
        memcpy(dst, def.data, dataSize);
        bufferSet.transferBuffer().unmap(transferMapping);

        pils::gpu::Memory::Mapping mapping = {_handle->offset, _handle->size};
        PILS_ASSERT(mapping.size == transferMapping.size, "Constant and Transfert buffer mapping sizes do not match");
        _manager.gpuDevice().commandList().copyBuffer(bufferSet.constantBuffer(), mapping.offset, bufferSet.transferBuffer(), transferMapping.offset, transferMapping.size);
    }
}


// CONSTANT //
GpuConstantResource::GpuConstantResource(GpuResourceManager& manager, ResourceId id, Definition def) :
    GpuResource(manager, id)
{

}

GpuConstantResource::~GpuConstantResource()
{
}

void GpuConstantResource::update(const Definition& def)
{
    PILS_ASSERT(_handle.get() == nullptr, "Constant resource updated twice in the same frame");
    PILS_ASSERT(def.data != nullptr, "Constant data must be provided at update time");
    PILS_ASSERT(def.size > 0, "Constant data block cannot be empty");

    pils::gpu::BufferSet& bufferSet = _manager.bufferSet();

    pils::gpu::Memory::Mapping mapping = bufferSet.registerConstant(def.size, 1);

    _handle.reset(new GpuConstantResourceHandle(bufferSet.constantBuffer(), mapping.offset, mapping.size));

    if (def.data != nullptr && def.size > 0)
    {
        void* dst = bufferSet.constantBuffer().map(mapping);
        memcpy(dst, def.data, def.size);
        bufferSet.transferBuffer().unmap(mapping);
    }
}

void GpuConstantResource::invalidate()
{
    _handle.reset();
}


// VERTEX ARRAY //
GpuGeometryResource::GpuGeometryResource(GpuResourceManager& manager, ResourceId id, Definition def) :
    GpuResource(manager, id)
{
    pils::pilsU64 elemSize = sizeof(decltype(def.vertices[0]));
    pils::pilsU64 elemCount = def.vertices.size();
    pils::pilsU64 dataSize = elemSize * elemCount;
    PILS_ASSERT(dataSize > 0, "Cannot allocate 0 size vertex data");

    pils::gpu::BufferSet& bufferSet = _manager.bufferSet();

    pils::gpu::Memory::Mapping mapping = bufferSet.registerVertex(elemSize, elemCount);

    _handle.reset(new GpuGeometryResourceHandle(bufferSet.vertexBuffer(), mapping.offset, mapping.size));

    if (!def.vertices.empty())
    {
        pils::gpu::Memory::Mapping transferMapping = bufferSet.registerTransfer(elemSize, elemCount);

        void* dst = bufferSet.transferBuffer().map(transferMapping);
        memcpy(dst, def.vertices.data(), dataSize);
        bufferSet.transferBuffer().unmap(transferMapping);

        PILS_ASSERT(mapping.size == transferMapping.size, "Vertex and Transfert buffer mapping sizes do not match");
        _manager.gpuDevice().commandList().copyBuffer(bufferSet.vertexBuffer(), mapping.offset, bufferSet.transferBuffer(), transferMapping.offset, transferMapping.size);
    }
}

GpuGeometryResource::~GpuGeometryResource()
{
}


// MANAGER //

void GpuResourceManager::initializeNative()
{
    pils::gpu::BufferSet::Settings bufferSettings{};
    bufferSettings.constantBufferSize = pils::pilsMB(1);
    bufferSettings.transferBufferSize = pils::pilsMB(10);
    bufferSettings.storageBufferSize = pils::pilsMB(10);
    bufferSettings.vertexBufferSize = pils::pilsMB(1);

    _bufferSet.reset(new pils::gpu::BufferSet(_gpuDevice.context().device(), bufferSettings));

    // We need the memory allocated during resource definition to upload data to buffers
    _bufferSet->allocate();
}

void GpuResourceManager::garbageCollect()
{
    for (const auto& resource : _resources)
    {
        if (GpuConstantResource* constant = dynamic_cast<GpuConstantResource*>(resource.get()))
        {
            constant->invalidate();
        }
    }

    _bufferSet->garbageCollect();
}

void GpuResourceManager::commitAllocations()
{
    _bufferSet->finalizeFrame(_gpuDevice.frameFence());
}

}

#endif // UNISIM_GRAPHIC_BACKEND_VK