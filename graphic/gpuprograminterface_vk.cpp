#ifdef UNISIM_GRAPHIC_BACKEND_VK

#include "gpuprograminterface.h"

#include <PilsCore/Utils/Assert.h>

#include <PilsCore/Gpu/Context.h>
#include <PilsCore/Gpu/Command.h>
#include <PilsCore/Gpu/Layout.h>
#include <PilsCore/Gpu/Synchronization.h>

#include "graphic.h"
#include "gpuresource.h"
#include "gpudevice.h"


namespace unisim
{

// Template //
GpuProgramInterfaceTemplate::GpuProgramInterfaceTemplate(ProgramType programType) :
    _programType(programType),
    _setIndex(0),
    _nextBinding(0),
    _shaderStages(toShaderStage(programType)),
    _isSetLayoutDirty(true),
    _parent(nullptr)
{
}

GpuProgramInterfaceTemplate::GpuProgramInterfaceTemplate(ProgramType programType, const GpuProgramInterfaceTemplate& parent) :
    _programType(programType),
    _setIndex(parent._setIndex+1),
    _nextBinding(0),
    _shaderStages(toShaderStage(programType)),
    _isSetLayoutDirty(true),
    _parent(&parent)
{
}

void GpuProgramInterfaceTemplate::declareBindless(const std::string& name, uint32_t descriptorCount)
{
    _isSetLayoutDirty = true;
    _bindless.push_back({name, std::make_unique<pils::gpu::DescriptorBinding>(
        _nextBinding++, descriptorCount, pils::gpu::DescriptorType::ImageSampler, _shaderStages, true)});
}

void GpuProgramInterfaceTemplate::declareConstant(const std::string& name, uint32_t descriptorCount)
{
    _isSetLayoutDirty = true;
    _constants.push_back({name, std::make_unique<pils::gpu::DescriptorBinding>(
        _nextBinding++, descriptorCount, pils::gpu::DescriptorType::Constant, _shaderStages)});
}

void GpuProgramInterfaceTemplate::declareStorage(const std::string& name, uint32_t descriptorCount)
{
    _isSetLayoutDirty = true;
    _storages.push_back({name, std::make_unique<pils::gpu::DescriptorBinding>(
        _nextBinding++, descriptorCount, pils::gpu::DescriptorType::Storage, _shaderStages)});
}

void GpuProgramInterfaceTemplate::declareTexture(const std::string& name, uint32_t descriptorCount)
{
    _isSetLayoutDirty = true;
    _textures.push_back({name, std::make_unique<pils::gpu::DescriptorBinding>(
        _nextBinding++, descriptorCount, pils::gpu::DescriptorType::ImageSampler, _shaderStages)});
}

void GpuProgramInterfaceTemplate::declareImage(const std::string& name, uint32_t descriptorCount)
{
    _isSetLayoutDirty = true;
    _images.push_back({name, std::make_unique<pils::gpu::DescriptorBinding>(
        _nextBinding++, descriptorCount, pils::gpu::DescriptorType::Image, _shaderStages)});
}

std::shared_ptr<pils::gpu::DescriptorSetLayout> GpuProgramInterfaceTemplate::computeSetLayout(const GpuDevice& gpuDevice) const
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

std::shared_ptr<pils::gpu::PipelineLayout> GpuProgramInterfaceTemplate::computePipelineLayout(const GpuDevice& gpuDevice) const
{
    std::vector<std::shared_ptr<pils::gpu::DescriptorSetLayout>> descriptorLayouts(_setIndex + 1);

    const GpuProgramInterfaceTemplate* gpiTemplate = this;
    while(gpiTemplate != nullptr)
    {
        descriptorLayouts[gpiTemplate->setIndex()] = gpiTemplate->computeSetLayout(gpuDevice);
        gpiTemplate = gpiTemplate->_parent;
    }

    const pils::gpu::Device& device = gpuDevice.context().device();
    std::shared_ptr<pils::gpu::PipelineLayout> pipelineLayout(new pils::gpu::PipelineLayout(device, descriptorLayouts));

    return pipelineLayout;
}


// Interface //

GpuProgramInterface::GpuProgramInterface(const GpuDevice& gpuDevice, const GpuProgramInterfaceTemplate& gpiTemplate) :
    _programType(gpiTemplate.programType()),
    _isDirty(true)
{
    _setIndex = gpiTemplate.setIndex();
    _layout = gpiTemplate.computeSetLayout(gpuDevice);

    for(const auto& binding : gpiTemplate.bindless())
    {
        auto& data = _bindlessData[binding.name];
        data.binding = binding;
        data.descriptors.resize(binding.binding->vkBinding().descriptorCount);
    }

    for(const auto& binding : gpiTemplate.constants())
    {
        auto& data = _constantData[binding.name];
        data.binding = binding;
        data.descriptors.resize(binding.binding->vkBinding().descriptorCount);
    }

    for(const auto& binding : gpiTemplate.storages())
    {
        auto& data = _storageData[binding.name];
        data.binding = binding;
        data.descriptors.resize(binding.binding->vkBinding().descriptorCount);
    }

    for(const auto& binding : gpiTemplate.textures())
    {
        auto& data = _textureData[binding.name];
        data.binding = binding;
        data.descriptors.resize(binding.binding->vkBinding().descriptorCount);
    }

    for(const auto& binding : gpiTemplate.images())
    {
        auto& data = _imageData[binding.name];
        data.binding = binding;
        data.descriptors.resize(binding.binding->vkBinding().descriptorCount);
    }
}

GpuProgramInterface::~GpuProgramInterface()
{
    PILS_ASSERT(false, "Not implemented!");
}

void GpuProgramInterface::setBindless(const std::string& name, const GpuBindlessTexture& resource, uint32_t arrayIndex)
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

    if (descriptors[arrayIndex].descriptor != resource.handle().descriptor)
    {
        _isDirty = true;
        descriptors[arrayIndex] = resource.handle();
    }
}

void GpuProgramInterface::setConstant(const std::string& name, const GpuConstantResource& resource, uint32_t arrayIndex)
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

    if (descriptors[arrayIndex].descriptor != resource.handle().descriptor)
    {
        _isDirty = true;
        descriptors[arrayIndex] = resource.handle();
    }
}

void GpuProgramInterface::setStorage(const std::string& name, const GpuStorageResource& resource, uint32_t arrayIndex)
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

    if (descriptors[arrayIndex].descriptor != resource.handle().descriptor)
    {
        _isDirty = true;
        descriptors[arrayIndex] = resource.handle();
    }
}

void GpuProgramInterface::setTexture(const std::string& name, const GpuTextureResource& resource, uint32_t arrayIndex)
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

    if (descriptors[arrayIndex].descriptor != resource.handle().descriptor)
    {
        _isDirty = true;
        descriptors[arrayIndex] = resource.handle();
    }
}

void GpuProgramInterface::setTexture(const std::string& name, const GpuImageResource& resource, uint32_t arrayIndex)
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

    if (descriptors[arrayIndex].descriptor != resource.handle().descriptor)
    {
        _isDirty = true;

        GpuTextureResourceHandle handle;
        handle.descriptor = resource.handle().descriptor;
        descriptors[arrayIndex] = handle;
    }
}

void GpuProgramInterface::setImage(const std::string& name, const GpuImageResource& resource, uint32_t arrayIndex)
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

    if (descriptors[arrayIndex].descriptor != resource.handle().descriptor)
    {
        _isDirty = true;
        descriptors[arrayIndex] = resource.handle();
    }
}

bool GpuProgramInterface::compileAndSet(const GpuDevice& gpuDevice, const GraphicProgram& program)
{
    bool isCompatible = (uint32_t(program.type()) & uint32_t(_programType)) == uint32_t(program.type());
    PILS_ASSERT(isCompatible, "This program interface cannot be used with program ", program.name());

    bool ok = isCompatible;

    if (_isDirty)
    {
        recycleDescriptorSet(gpuDevice);

        std::vector<pils::gpu::Descriptor> pilsDescriptors;
        pilsDescriptors.reserve(32);

        auto write = [&](auto descTypeData) -> void
        {
            for(const auto& data : descTypeData)
            {
                const auto& descriptors = data.second.descriptors;
                pilsDescriptors.reserve(descriptors.size());

                for (const auto& desc : descriptors)
                    pilsDescriptors.push_back(*desc.descriptor);

                _cachedDescriptorSet->write(*data.second.binding.binding, pilsDescriptors.data(), 0, pilsDescriptors.size());
            }

            pilsDescriptors.clear();
        };

        write(_bindlessData);
        write(_constantData);
        write(_storageData);
        write(_textureData);
        write(_imageData);

        _cachedDescriptorSet->flushWrites();
        _isDirty = false;
    }

    gpuDevice.commandList().bindDescriptorSet(
        toPipelineType(program.type()),
        program.layout(),
        *_cachedDescriptorSet);

    return ok;
}

void GpuProgramInterface::recycleDescriptorSet(const GpuDevice& gpuDevice)
{
    const pils::gpu::Device& device = gpuDevice.context().device();

    for (std::size_t i = 0; i < _recycledDesciptorSets.size()-1; ++i)
    {
        if (_recycledDesciptorSets[i].second->isSignaled())
        {
            std::swap(_cachedDescriptorSet, _recycledDesciptorSets[i].first);
            _recycledDesciptorSets[i].second = gpuDevice.frameFence();
        }
    }

    // No free recycled set found, create a new one
    _recycledDesciptorSets.push_back({_cachedDescriptorSet, gpuDevice.frameFence()});
    _cachedDescriptorSet.reset(new pils::gpu::DescriptorSet(device, gpuDevice.descriptorPool(), *_layout, _setIndex));
}

}

#endif // UNISIM_GRAPHIC_BACKEND_VK
