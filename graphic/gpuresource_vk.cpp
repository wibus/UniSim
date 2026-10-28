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

// HANDLES //

GpuBindlessTextureResourceHandle::GpuBindlessTextureResourceHandle()
{
}

GpuTextureResourceHandle::GpuTextureResourceHandle()
{
}

GpuImageResourceHandle::GpuImageResourceHandle()
{
}

GpuStorageResourceHandle::GpuStorageResourceHandle()
{
}

GpuConstantResourceHandle::GpuConstantResourceHandle()
{
}

GpuGeometryResourceHandle::GpuGeometryResourceHandle()
{
}

GpuTextureResourceHandle::GpuTextureResourceHandle(
        const pils::gpu::ImageView& textureView,
        const pils::gpu::Sampler& sampler) :
    descriptor(new pils::gpu::Descriptor(textureView, sampler))
{

}

GpuImageResourceHandle::GpuImageResourceHandle(
        const pils::gpu::ImageView& imageView,
        const pils::gpu::Sampler& sampler) :
    descriptor(new pils::gpu::Descriptor(imageView, sampler))
{
}

GpuStorageResourceHandle::GpuStorageResourceHandle(
        const pils::gpu::StorageBuffer& buffer,
        uint64_t offset,
        uint64_t size) :
    descriptor(new pils::gpu::Descriptor(buffer, offset, size))
{
}

GpuConstantResourceHandle::GpuConstantResourceHandle(
        const pils::gpu::ConstantBuffer& buffer,
        uint64_t offset,
        uint64_t size) :
    descriptor(new pils::gpu::Descriptor(buffer, offset, size))
{
}

GpuGeometryResourceHandle::GpuGeometryResourceHandle(
        const pils::gpu::VertexBuffer& buffer,
        uint64_t offset,
        uint64_t size) :
    descriptor(new pils::gpu::Descriptor(buffer, offset, size))
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
    _manager.gpuDevice().commandList().imageBarrier(*_texture, vk::ImageLayout::eTransferDstOptimal);
    _manager.gpuDevice().commandList().copyImageBuffer(*_texture, imgOffset, imgExtent, bufferSet.transferBuffer(), transferMapping.offset);
    _manager.gpuDevice().commandList().imageBarrier(*_texture, vk::ImageLayout::eReadOnlyOptimal);
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
    GpuResource(manager, id),
    _device(_manager.gpuDevice().context().device())
{
    _sampler.reset(new pils::gpu::Sampler(_device));

    _image.reset(
        new pils::gpu::Texture(
            _device,
            def.width,
            def.height,
            def.depth,
            def.depth > 1 ? pils::gpu::ImageType::Img3D : pils::gpu::ImageType::Img2D,
            toPils(def.format),
            1)
        );

    _imageView.reset(new pils::gpu::ImageView(_device, *_image));
    _handle.reset(new GpuImageResourceHandle(*_imageView, *_sampler));

    _manager.gpuDevice().commandList().imageBarrier(*_image, vk::ImageLayout::eGeneral);
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

    _imageView.reset(new pils::gpu::ImageView(_device, *_image));
    _handle.reset(new GpuImageResourceHandle(*_imageView, *_sampler));

    _manager.gpuDevice().commandList().imageBarrier(*_image, vk::ImageLayout::eGeneral);
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
    PILS_ASSERT(dataSize == _handle->descriptor->vkBufferInfo().range, "Storage resource resize not implemented");

    pils::gpu::BufferSet& bufferSet = _manager.bufferSet();

    if (def.data != nullptr && dataSize > 0)
    {
        pils::gpu::Memory::Mapping transferMapping = bufferSet.registerTransfer(def.elemSize, def.elemCount);

        void* dst = bufferSet.transferBuffer().map(transferMapping);
        memcpy(dst, def.data, dataSize);
        bufferSet.transferBuffer().unmap(transferMapping);

        pils::gpu::Memory::Mapping mapping = {_handle->descriptor->vkBufferInfo().offset,_handle->descriptor->vkBufferInfo().range};
        PILS_ASSERT(mapping.size == transferMapping.size, "Constant and Transfert buffer mapping sizes do not match");
        _manager.gpuDevice().commandList().copyBuffer(bufferSet.constantBuffer(), mapping.offset, bufferSet.transferBuffer(), transferMapping.offset, transferMapping.size);
    }
}


// CONSTANT //
GpuConstantResource::GpuConstantResource(GpuResourceManager& manager, ResourceId id, Definition def) :
    GpuResource(manager, id)
{
    // Nothing to do, only transfer during updates
    if (def.data != nullptr)
        PILS_ERROR("Providing data when declaring constant buffer is not supported. Data needs to be provided each frame.");
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

    pils::gpu::Memory::Mapping mapping = bufferSet.registerConstant(def.size);

    _handle.reset(new GpuConstantResourceHandle(bufferSet.constantBuffer(), mapping.offset, mapping.size));

    void* dst = ((char*)bufferSet.constantBufferCpuAddress()) + mapping.offset;
    memcpy(dst, def.data, def.size);
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


// BINDLESS //

GpuBindlessTexture::GpuBindlessTexture()
{
    _handle.reset(new GpuBindlessTextureResourceHandle());
}

GpuBindlessTexture::GpuBindlessTexture(
        const GpuTextureResource& textureResource,
        const Texture& texture)
{
    _handle.reset(new GpuBindlessTextureResourceHandle());
    _handle->descriptor = textureResource.handle().descriptor;
}

GpuBindlessTexture::~GpuBindlessTexture()
{
}


// MANAGER //

bool GpuResourceManager::compileAndSetPassGpi(const GraphicProgram& program)
{
    bool ok = true;
    return ok;
}

void GpuResourceManager::initializeNative()
{
    pils::gpu::BufferSet::Settings bufferSettings{};
    bufferSettings.constantBufferSize = pils::pilsMB(1);
    bufferSettings.transferBufferSize = pils::pilsGB(2);
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