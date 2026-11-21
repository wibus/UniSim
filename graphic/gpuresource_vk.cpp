#ifdef UNISIM_GRAPHIC_BACKEND_VK

#include "gpuresource.h"

#include <PilsCore/Utils/Assert.h>

#include <PilsCore/Gpu/Command.h>
#include <PilsCore/Gpu/Memory.h>
#include <PilsCore/Gpu/Context.h>
#include <PilsCore/Gpu/Layout.h>
#include <PilsCore/Gpu/Synchronization.h>

#include "../resource/texture.h"

#include "gpudevice.h"
#include "programinterface.h"


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

GpuStorageResourceHandle::GpuStorageResourceHandle(
        pils::gpu::StorageBuffer* buffer,
        const std::shared_ptr<pils::gpu::Mapping>& mapping) :
    buffer(buffer),
    mapping(mapping),
    descriptor(new pils::gpu::Descriptor(*buffer, *mapping))
{
}

GpuConstantResourceHandle::GpuConstantResourceHandle(
        pils::gpu::ConstantBuffer* buffer,
        const std::shared_ptr<pils::gpu::Mapping>& mapping) :
    buffer(buffer),
    mapping(mapping),
    descriptor(new pils::gpu::Descriptor(*buffer, *mapping))
{
}

GpuGeometryResourceHandle::GpuGeometryResourceHandle(
        pils::gpu::VertexBuffer* buffer,
        const std::shared_ptr<pils::gpu::Mapping>& mapping) :
    buffer(buffer),
    mapping(mapping),
    descriptor(new pils::gpu::Descriptor(*buffer, *mapping))
{
}


// TEXTURE //

GpuTextureResource::GpuTextureResource(GpuResourceManager& manager, ResourceId id, Definition def) :
    GpuResource(manager, id, ResourceState::ReadOnly)
{
    const pils::gpu::Device& device = _manager.gpuDevice().context().device();

    _handle.reset(new GpuTextureResourceHandle());
    _handle->state = _state;

    _handle->image.reset(
        new pils::gpu::Texture(
            device,
            def.texture.width,
            def.texture.height,
            def.texture.depth,
            def.texture.depth > 1 ? pils::gpu::ImageType::Img3D : pils::gpu::ImageType::Img2D,
            toPils(def.texture.format),
            1)
        );

    _handle->sampler.reset(new pils::gpu::Sampler(device));
    _handle->imageView.reset(new pils::gpu::ImageView(device, *_handle->image));
    _handle->descriptor.reset(new pils::gpu::Descriptor(*_handle->imageView, *_handle->sampler));

    // Upload data to memory
    pils::gpu::BufferSet& bufferSet = _manager.bufferSet();

    pils::pilsU64 texelSize = toTexelSize(def.texture.format);
    pils::pilsU64 texelCount = def.texture.width * def.texture.height * def.texture.depth;
    pils::gpu::Mapping transferMapping = bufferSet.registerTransfer(texelSize, texelCount);

    void* dst = bufferSet.transferBuffer().map(transferMapping);
    memcpy(dst, &def.texture.data[0], texelSize * texelCount);
    bufferSet.transferBuffer().unmapAndFlush(transferMapping);

    glm::uvec3 imgOffset = glm::uvec3(0, 0, 0);
    glm::uvec3 imgExtent = glm::uvec3(def.texture.width, def.texture.height, def.texture.depth);

    auto& cmdList = _manager.gpuDevice().commandList();
    cmdList.imageBarrier(*_handle->image, vk::ImageLayout::eTransferDstOptimal);
    cmdList.copyImageBuffer(*_handle->image, imgOffset, imgExtent, bufferSet.transferBuffer(), transferMapping.offset);
    cmdList.imageBarrier(*_handle->image, vk::ImageLayout::eReadOnlyOptimal);
}

GpuTextureResource::GpuTextureResource(GpuResourceManager& manager, GpuTextureResourceHandle&& handle) :
    GpuResource(manager, Invalid_ResourceId, ResourceState::ReadOnly),
    _handle(new GpuTextureResourceHandle(std::move(handle)))
{
}

GpuTextureResource::~GpuTextureResource()
{
}


// IMAGE //
GpuImageResource::GpuImageResource(GpuResourceManager& manager, ResourceId id, Definition def) :
    GpuResource(manager, id, ResourceState::ReadWrite)
{
    createHandles(def);
}

GpuImageResource::~GpuImageResource()
{
}

void GpuImageResource::update(const Definition& def)
{
    _manager.gpuDevice().dispose(_imageHandle);
    _manager.gpuDevice().dispose(_textureHandle);

    createHandles(def);
}

void GpuImageResource::createHandles(const Definition& def)
{
    const pils::gpu::Device& device = _manager.gpuDevice().context().device();

    std::shared_ptr<pils::gpu::Image> image(
        new pils::gpu::Image(
            device,
            def.width,
            def.height,
            def.depth,
            def.depth > 1 ? pils::gpu::ImageType::Img3D : pils::gpu::ImageType::Img2D,
            toPils(def.format),
            1
        )
    );

    _imageHandle.reset(new GpuImageResourceHandle());
    _imageHandle->state = _state;
    _imageHandle->image = image;
    _imageHandle->imageView.reset(new pils::gpu::ImageView(device, *image));
    _imageHandle->descriptor.reset(new pils::gpu::Descriptor(*_imageHandle->imageView));

    _textureHandle.reset(new GpuTextureResourceHandle());
    _textureHandle->state = _state;
    _textureHandle->image = image;
    _textureHandle->sampler.reset(new pils::gpu::Sampler(device));
    _textureHandle->imageView.reset(new pils::gpu::ImageView(device, *image));
    _textureHandle->descriptor.reset(new pils::gpu::Descriptor(*_textureHandle->imageView, *_textureHandle->sampler));

    _manager.gpuDevice().commandList().imageBarrier(*image, vk::ImageLayout::eGeneral);
}


// STORAGE //
GpuStorageResource::GpuStorageResource(GpuResourceManager& manager, ResourceId id, Definition def) :
    GpuResource(manager, id, ResourceState::ReadWrite)
{
    createHandle(def);
}

GpuStorageResource::~GpuStorageResource()
{
}

void GpuStorageResource::update(const Definition& def)
{
    _manager.gpuDevice().dispose(_handle);

    createHandle(def);
}

void GpuStorageResource::createHandle(const Definition& def)
{
    pils::pilsU64 dataSize = def.elemSize * def.elemCount;
    PILS_ASSERT(dataSize > 0, "Cannot allocate 0 size storage");

    pils::gpu::BufferSet& bufferSet = _manager.bufferSet();

    std::shared_ptr<pils::gpu::Mapping> storageMapping(new pils::gpu::Mapping());
    *storageMapping = bufferSet.registerStorage(def.elemSize, def.elemCount);

    _handle.reset(new GpuStorageResourceHandle(&bufferSet.storageBuffer(), storageMapping));
    _handle->state = _state;

    if (def.data != nullptr && dataSize > 0)
    {
        pils::gpu::Mapping transferMapping = bufferSet.registerTransfer(def.elemSize, def.elemCount);

        void* dst = bufferSet.transferBuffer().map(transferMapping);
        memcpy(dst, def.data, def.elemSize * def.elemCount);
        bufferSet.transferBuffer().unmap(transferMapping);

        auto& cmdList = _manager.gpuDevice().commandList();

        PILS_ASSERT(storageMapping->size == transferMapping.size, "Storage and Transfert buffer mapping sizes do not match");
        cmdList.copyBuffer(bufferSet.storageBuffer(), storageMapping->offset, bufferSet.transferBuffer(), transferMapping.offset, transferMapping.size);
        cmdList.bufferBarrier(bufferSet.storageBuffer(), *storageMapping);
    }
}


// CONSTANT //
GpuConstantResource::GpuConstantResource(GpuResourceManager& manager, ResourceId id, Definition def) :
    GpuResource(manager, id, ResourceState::ReadOnly)
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
    PILS_ASSERT(def.data != nullptr, "Constant data must be provided at update time");
    PILS_ASSERT(def.size > 0, "Constant data block cannot be empty");

    pils::gpu::BufferSet& bufferSet = _manager.bufferSet();

    std::shared_ptr<pils::gpu::Mapping> mapping(new pils::gpu::Mapping());
    *mapping = bufferSet.registerConstant(def.size);

    _handle.reset(new GpuConstantResourceHandle(&bufferSet.constantBuffer(), mapping));
    _handle->state = _state;

    void* dst = ((char*)bufferSet.constantBufferCpuAddress()) + mapping->offset;
    memcpy(dst, def.data, def.size);
}

void GpuConstantResource::invalidate()
{
    _handle.reset();
}


// VERTEX ARRAY //
GpuGeometryResource::GpuGeometryResource(GpuResourceManager& manager, ResourceId id, Definition def) :
    GpuResource(manager, id, ResourceState::ReadOnly),
    _vertexCount(def.vertices.size())
{
    pils::pilsU64 elemSize = sizeof(decltype(def.vertices[0]));
    pils::pilsU64 elemCount = def.vertices.size();
    pils::pilsU64 dataSize = elemSize * elemCount;
    PILS_ASSERT(dataSize > 0, "Cannot allocate 0 size vertex data");

    pils::gpu::BufferSet& bufferSet = _manager.bufferSet();

    std::shared_ptr<pils::gpu::Mapping> mapping(new pils::gpu::Mapping());
    *mapping = bufferSet.registerVertex(elemSize, elemCount);

    _handle.reset(new GpuGeometryResourceHandle(&bufferSet.vertexBuffer(), mapping));
    _handle->state = _state;

    if (!def.vertices.empty())
    {
        pils::gpu::Mapping transferMapping = bufferSet.registerTransfer(elemSize, elemCount);

        void* dst = bufferSet.transferBuffer().map(transferMapping);
        memcpy(dst, def.vertices.data(), dataSize);
        bufferSet.transferBuffer().unmap(transferMapping);

        auto& cmdList = _manager.gpuDevice().commandList();

        PILS_ASSERT(mapping->size == transferMapping.size, "Vertex and Transfert buffer mapping sizes do not match");
        cmdList.copyBuffer(bufferSet.vertexBuffer(), mapping->offset, bufferSet.transferBuffer(), transferMapping.offset, transferMapping.size);
        cmdList.bufferBarrier(bufferSet.vertexBuffer(), *mapping);
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
    const auto& roHandle = textureResource.roHandle();
    _handle->state = roHandle.state;
    _handle->descriptor = roHandle.descriptor;
}

GpuBindlessTexture::~GpuBindlessTexture()
{
}


// MANAGER //

bool GpuResourceManager::compileAndSetPassPi(const Program& program)
{
    // TODO 2026-11-15: not optimal. bind once per frame.
    bool ok = true;
    ok = ok && _persistentPi->compileAndSet(_gpuDevice, program);
    ok = ok && _framePi->compileAndSet(_gpuDevice, program);
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

void GpuResourceManager::unregisterBindlessTexture(BindlessTextureIndex index)
{
    PILS_ASSERT(_bindlessTextures[int32_t(index)].get() != nullptr, "Trying to unregister bindless texture that was never registered at index ", int(index));
    _persistentPi->setBindless("BindlessTextures", GpuBindlessTexture(), int32_t(index));
    _bindlessTextures[int32_t(index)].reset();
    _bindlessTextureFreeList.push_back(index);
    _isBindlessTextureDirty = true;
}

}

#endif // UNISIM_GRAPHIC_BACKEND_VK