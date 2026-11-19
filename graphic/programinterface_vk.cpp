#ifdef UNISIM_GRAPHIC_BACKEND_VK

#include "programinterface.h"

#include <PilsCore/Utils/Assert.h>

#include <PilsCore/Gpu/Context.h>
#include <PilsCore/Gpu/Command.h>
#include <PilsCore/Gpu/Layout.h>
#include <PilsCore/Gpu/Synchronization.h>

#include "gpuresource.h"
#include "gpudevice.h"


namespace unisim
{

// Template //
ProgramInterfaceTemplate::ProgramInterfaceTemplate(ProgramType programType) :
    _programType(programType),
    _setIndex(0),
    _shaderStages(toShaderStage(programType)),
    _isSetLayoutDirty(true),
    _parent(nullptr)
{
}

ProgramInterfaceTemplate::ProgramInterfaceTemplate(ProgramType programType, const ProgramInterfaceTemplate& parent) :
    _programType(programType),
    _setIndex(parent._setIndex+1),
    _shaderStages(toShaderStage(programType)),
    _isSetLayoutDirty(true),
    _parent(&parent)
{
}

void ProgramInterfaceTemplate::declareBindless(uint32_t bindingIndex, const std::string& name, uint32_t descriptorCount)
{
    _isSetLayoutDirty = true;
    _bindless.push_back({name, std::make_unique<pils::gpu::DescriptorBinding>(
        bindingIndex, descriptorCount, pils::gpu::DescriptorType::ImageSampler, _shaderStages, true)});
}

void ProgramInterfaceTemplate::declareConstant(uint32_t bindingIndex, const std::string& name, uint32_t descriptorCount)
{
    _isSetLayoutDirty = true;
    _constants.push_back({name, std::make_unique<pils::gpu::DescriptorBinding>(
        bindingIndex, descriptorCount, pils::gpu::DescriptorType::Constant, _shaderStages)});
}

void ProgramInterfaceTemplate::declareStorage(uint32_t bindingIndex, const std::string& name, uint32_t descriptorCount)
{
    _isSetLayoutDirty = true;
    _storages.push_back({name, std::make_unique<pils::gpu::DescriptorBinding>(
        bindingIndex, descriptorCount, pils::gpu::DescriptorType::Storage, _shaderStages)});
}

void ProgramInterfaceTemplate::declareTexture(uint32_t bindingIndex, const std::string& name, uint32_t descriptorCount)
{
    _isSetLayoutDirty = true;
    _textures.push_back({name, std::make_unique<pils::gpu::DescriptorBinding>(
        bindingIndex, descriptorCount, pils::gpu::DescriptorType::ImageSampler, _shaderStages)});
}

void ProgramInterfaceTemplate::declareImage(uint32_t bindingIndex, const std::string& name, uint32_t descriptorCount)
{
    _isSetLayoutDirty = true;
    _images.push_back({name, std::make_unique<pils::gpu::DescriptorBinding>(
        bindingIndex, descriptorCount, pils::gpu::DescriptorType::Image, _shaderStages)});
}

std::shared_ptr<pils::gpu::DescriptorSetLayout> ProgramInterfaceTemplate::computeSetLayout(const GpuDevice& gpuDevice) const
{
    if (_cachedSetLayout && !_isSetLayoutDirty)
        return _cachedSetLayout;

    std::vector<pils::gpu::DescriptorBinding> _descriptorBindings;
    _descriptorBindings.reserve(32);

    for(const auto& binding : bindless())
        _descriptorBindings.push_back(*binding.binding);

    for(const auto& binding : constants())
        _descriptorBindings.push_back(*binding.binding);

    for(const auto& binding : storages())
        _descriptorBindings.push_back(*binding.binding);

    for(const auto& binding : textures())
        _descriptorBindings.push_back(*binding.binding);

    for(const auto& binding : images())
        _descriptorBindings.push_back(*binding.binding);

    const pils::gpu::Device& device = gpuDevice.context().device();
    _cachedSetLayout.reset(new pils::gpu::DescriptorSetLayout(device, _descriptorBindings));

    _isSetLayoutDirty = false;

    return _cachedSetLayout;
}

std::shared_ptr<pils::gpu::PipelineLayout> ProgramInterfaceTemplate::computePipelineLayout(const GpuDevice& gpuDevice) const
{
    std::vector<std::shared_ptr<pils::gpu::DescriptorSetLayout>> descriptorLayouts(_setIndex + 1);

    const ProgramInterfaceTemplate* piTemplate = this;
    while(piTemplate != nullptr)
    {
        descriptorLayouts[piTemplate->setIndex()] = piTemplate->computeSetLayout(gpuDevice);
        piTemplate = piTemplate->_parent;
    }

    const pils::gpu::Device& device = gpuDevice.context().device();
    std::shared_ptr<pils::gpu::PipelineLayout> pipelineLayout(new pils::gpu::PipelineLayout(device, descriptorLayouts));

    return pipelineLayout;
}


// Interface //

ProgramInterface::ProgramInterface(const GpuDevice& gpuDevice, const ProgramInterfaceTemplate& piTemplate) :
    _programType(piTemplate.programType()),
    _isDescriptorSetDirty(true),
    _isAlive(new bool(true))
{
    _setIndex = piTemplate.setIndex();
    _layout = piTemplate.computeSetLayout(gpuDevice);

    for(const auto& binding : piTemplate.bindless())
    {
        auto& data = _bindlessData[binding.name];
        data.binding = binding;
        data.descriptors.resize(binding.binding->vkBinding().descriptorCount);
    }

    for(const auto& binding : piTemplate.constants())
    {
        auto& data = _constantData[binding.name];
        data.binding = binding;
        data.descriptors.resize(binding.binding->vkBinding().descriptorCount);
    }

    for(const auto& binding : piTemplate.storages())
    {
        auto& data = _storageData[binding.name];
        data.binding = binding;
        data.descriptors.resize(binding.binding->vkBinding().descriptorCount);
    }

    for(const auto& binding : piTemplate.textures())
    {
        auto& data = _textureData[binding.name];
        data.binding = binding;
        data.descriptors.resize(binding.binding->vkBinding().descriptorCount);
    }

    for(const auto& binding : piTemplate.images())
    {
        auto& data = _imageData[binding.name];
        data.binding = binding;
        data.descriptors.resize(binding.binding->vkBinding().descriptorCount);
    }
}

ProgramInterface::~ProgramInterface()
{
    *_isAlive = false;
}

void ProgramInterface::setBindless(const std::string& name, const GpuBindlessTexture& resource, uint32_t arrayIndex)
{
    auto dataIt = _bindlessData.find(name);
    if (dataIt == _bindlessData.end())
    {
        PILS_ERROR("No bindless descriptor declared for ", name);
        return;
    }

    auto& descriptors = dataIt->second.descriptors;
    if (arrayIndex >= descriptors.size())
    {
        PILS_ERROR("Bindless descriptor array '", name,"' out of bounds at ", arrayIndex);
        return;
    }

    const auto& handle = resource.roHandle();

    if (descriptors[arrayIndex].descriptor != handle.descriptor)
    {
        _dirtyBindlessDescriptors[name].insert(arrayIndex);
        descriptors[arrayIndex] = handle;
    }
}

void ProgramInterface::setConstant(const std::string& name, const GpuConstantResource& resource, uint32_t arrayIndex)
{
    auto dataIt = _constantData.find(name);
    if (dataIt == _constantData.end())
    {
        PILS_ERROR("No constant descriptor declared for ", name);
        return;
    }

    auto& descriptors = dataIt->second.descriptors;
    if (arrayIndex >= descriptors.size())
    {
        PILS_ERROR("Constant descriptor array '", name,"' out of bounds at ", arrayIndex);
        return;
    }

    const auto& handle = resource.roHandle();

    if (descriptors[arrayIndex].descriptor != handle.descriptor)
    {
        _isDescriptorSetDirty = true;
        descriptors[arrayIndex] = handle;
    }
}

void ProgramInterface::setStorage(const std::string& name, const GpuStorageResource& resource, uint32_t arrayIndex)
{
    auto dataIt = _storageData.find(name);
    if (dataIt == _storageData.end())
    {
        PILS_ERROR("No storage descriptor declared for ", name);
        return;
    }

    auto& descriptors = dataIt->second.descriptors;
    if (arrayIndex >= descriptors.size())
    {
        PILS_ERROR("Storage descriptor array '", name,"' out of bounds at ", arrayIndex);
        return;
    }

    const auto& handle = resource.rwHandle();

    if (descriptors[arrayIndex].descriptor != handle.descriptor)
    {
        _isDescriptorSetDirty = true;
        descriptors[arrayIndex] = handle;
    }
}

void ProgramInterface::setTexture(const std::string& name, const GpuTextureResource& resource, uint32_t arrayIndex)
{
    auto dataIt = _textureData.find(name);
    if (dataIt == _textureData.end())
    {
        PILS_ERROR("No texture descriptor declared for ", name);
        return;
    }

    auto& descriptors = dataIt->second.descriptors;
    if (arrayIndex >= descriptors.size())
    {
        PILS_ERROR("Texture descriptor array '", name,"' out of bounds at ", arrayIndex);
        return;
    }

    const auto& roHandle = resource.roHandle();

    if (descriptors[arrayIndex].descriptor != roHandle.descriptor)
    {
        _isDescriptorSetDirty = true;
        descriptors[arrayIndex] = roHandle;
    }
}

void ProgramInterface::setTexture(const std::string& name, const GpuImageResource& resource, uint32_t arrayIndex)
{
    auto dataIt = _textureData.find(name);
    if (dataIt == _textureData.end())
    {
        PILS_ERROR("No texture descriptor declared for ", name);
        return;
    }

    auto& descriptors = dataIt->second.descriptors;
    if (arrayIndex >= descriptors.size())
    {
        PILS_ERROR("Texture descriptor array '", name,"' out of bounds at ", arrayIndex);
        return;
    }

    const auto& roHandle = resource.roHandle();

    if (descriptors[arrayIndex].descriptor != roHandle.descriptor)
    {
        _isDescriptorSetDirty = true;
        descriptors[arrayIndex] = roHandle;
    }
}

void ProgramInterface::setImage(const std::string& name, const GpuImageResource& resource, uint32_t arrayIndex)
{
    auto dataIt = _imageData.find(name);
    if (dataIt == _imageData.end())
    {
        PILS_ERROR("No image descriptor declared for ", name);
        return;
    }

    auto& descriptors = dataIt->second.descriptors;
    if (arrayIndex >= descriptors.size())
    {
        PILS_ERROR("Image descriptor array '", name,"' out of bounds at ", arrayIndex);
        return;
    }

    const auto& rwHandle = resource.rwHandle();

    if (descriptors[arrayIndex].descriptor != rwHandle.descriptor)
    {
        _isDescriptorSetDirty = true;
        descriptors[arrayIndex] = rwHandle;
    }
}

bool ProgramInterface::compileAndSet(GpuDevice &gpuDevice, const Program& program)
{
    bool isCompatible = (uint32_t(program.type()) & uint32_t(_programType)) == uint32_t(program.type());
    PILS_ASSERT(isCompatible, "This program interface cannot be used with program ", program.name());

    bool ok = isCompatible;

    // Create descriptor set

    if (ok && (_isDescriptorSetDirty || !_cachedDescriptorSet))
    {
        recycleDescriptorSet(gpuDevice);

        std::vector<pils::gpu::Descriptor> pilsDescriptors;
        pilsDescriptors.reserve(
            _bindlessData.size() +
            _constantData.size() +
            _storageData.size() +
            _textureData.size() +
            _imageData.size());

        auto write = [&](const auto& descTypeData) -> void
        {
            for(const auto& [name, data] : descTypeData)
            {
                std::size_t start = pilsDescriptors.size();

                for (const auto& desc : data.descriptors)
                {
                    if (desc.descriptor)
                    {
                        pilsDescriptors.push_back(*desc.descriptor);
                    }
                    else if (pilsDescriptors.size() - start > 0)
                    {
                        _cachedDescriptorSet->write(*data.binding.binding, &pilsDescriptors[start], 0, pilsDescriptors.size() - start);
                        start = pilsDescriptors.size();
                    }
                }

                if (pilsDescriptors.size() - start > 0)
                    _cachedDescriptorSet->write(*data.binding.binding, &pilsDescriptors[start], 0, pilsDescriptors.size() - start);
            }
        };

        write(_bindlessData);
        write(_constantData);
        write(_storageData);
        write(_textureData);
        write(_imageData);

        _cachedDescriptorSet->flushWrites();

        _isDescriptorSetDirty = false;
        _dirtyBindlessDescriptors.clear();
    }
    else if (ok && !_dirtyBindlessDescriptors.empty())
    {
        for (const auto& [name, indices] : _dirtyBindlessDescriptors)
        {
            auto dataIt = _bindlessData.find(name);
            PILS_ASSERT(dataIt != _bindlessData.end(), "Unkown dirty bindless binding: ", name);

            const auto& descriptors = dataIt->second.descriptors;
            for (uint32_t index : indices)
            {
                _cachedDescriptorSet->write(*dataIt->second.binding.binding, *descriptors[index].descriptor, index);
            }
        }

        _cachedDescriptorSet->flushWrites();

        _dirtyBindlessDescriptors.clear();
    }

    auto& cmdList = gpuDevice.commandList();

    for (const auto& [name, data] : _textureData)
    {
        for (const auto& desc : data.descriptors)
        {
            if (*desc.state != ResourceState::ReadOnly)
            {
                cmdList.imageBarrier(*desc.image, vk::ImageLayout::eReadOnlyOptimal);
                *desc.state = ResourceState::ReadOnly;
            }
        }
    }

    for (const auto& [name, data] : _imageData)
    {
        for (const auto& desc : data.descriptors)
        {
            cmdList.imageBarrier(*desc.image, vk::ImageLayout::eGeneral);
            *desc.state = ResourceState::ReadWrite;
        }
    }

    for (const auto& [name, data] : _storageData)
    {
        for (const auto& desc : data.descriptors)
        {
            cmdList.bufferBarrier(*desc.buffer, *desc.mapping);
            *desc.state = ResourceState::ReadWrite;
        }
    }

    cmdList.bindDescriptorSet(
        toPipelineType(program.type()),
        program.layout(),
        *_cachedDescriptorSet);

    return ok;
}

void ProgramInterface::recycleDescriptorSet(GpuDevice& gpuDevice)
{
    if (_cachedDescriptorSet)
    {
        gpuDevice.dispose([descriptorSet = _cachedDescriptorSet, isAlive = _isAlive, self = this]()
        {
            if (*isAlive)
            {
                self->_recycledDesciptorSets.push_back(descriptorSet);
            }
        });
    }

    if (!_recycledDesciptorSets.empty())
    {
        _cachedDescriptorSet = _recycledDesciptorSets.back();
        _recycledDesciptorSets.pop_back();
    }
    else
    {
        _cachedDescriptorSet.reset(new pils::gpu::DescriptorSet(gpuDevice.context().device(), gpuDevice.descriptorPool(), *_layout, _setIndex));
    }
}

}

#endif // UNISIM_GRAPHIC_BACKEND_VK
