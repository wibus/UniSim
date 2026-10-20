#ifdef UNISIM_GRAPHIC_BACKEND_GL

#include "gpuprograminterface.h"

#include <PilsCore/Utils/Assert.h>

#include "graphic.h"
#include "gpudevice.h"
#include "gpuresource.h"


namespace unisim
{

// Template //
GpuProgramInterfaceTemplate::GpuProgramInterfaceTemplate(ProgramType programType) :
    _programType(programType),
    _nextBindlessStorageBinding(0),
    _nextConstantBinding(0),
    _nextTextureBinding(0),
    _nextImageBinding(0)
{
}

GpuProgramInterfaceTemplate::GpuProgramInterfaceTemplate(ProgramType programType, const GpuProgramInterfaceTemplate& parent) :
    _programType(programType),
    _nextBindlessStorageBinding(parent._nextBindlessStorageBinding),
    _nextConstantBinding(parent._nextConstantBinding),
    _nextTextureBinding(parent._nextTextureBinding),
    _nextImageBinding(parent._nextImageBinding)
{
}

void GpuProgramInterfaceTemplate::declareBindless(const std::string& name, uint32_t descriptorCount)
{
    _bindless.push_back({name, _nextBindlessStorageBinding++, descriptorCount});
}

void GpuProgramInterfaceTemplate::declareConstant(const std::string& name, uint32_t descriptorCount)
{
    PILS_ASSERT(descriptorCount == 1, "OpenGL backend does not support constance buffer arrays");
    _constants.push_back({name, _nextConstantBinding++, descriptorCount});
}

void GpuProgramInterfaceTemplate::declareStorage(const std::string& name, uint32_t descriptorCount)
{
    PILS_ASSERT(descriptorCount == 1, "OpenGL backend does not support storage buffer arrays");
    _storages.push_back({name, _nextBindlessStorageBinding++, descriptorCount});
}

void GpuProgramInterfaceTemplate::declareTexture(const std::string& name, uint32_t descriptorCount)
{
    PILS_ASSERT(descriptorCount == 1, "OpenGL backend does not support texture arrays");
    _textures.push_back({name, _nextTextureBinding++, descriptorCount});
}

void GpuProgramInterfaceTemplate::declareImage(const std::string& name, uint32_t descriptorCount)
{
    PILS_ASSERT(descriptorCount == 1, "OpenGL backend does not support image arrays");
    _images.push_back({name, _nextImageBinding++, descriptorCount});
}


// Interface //

bool GpuProgramInterface::setBindlessBindPoint(const GraphicProgram& program, const GpuProgramBinding& binding)
{
    PILS_ASSERT(binding.bindPoint != -1, binding.name, " has an innvalid bind point index ", binding.bindPoint);
    PILS_ASSERT(_bindlessData.find(binding.name) != _bindlessData.end(), "Unknown bindless buffer: ", binding.name);

    GLuint location = glGetProgramResourceIndex(program.handle(), GL_SHADER_STORAGE_BLOCK, binding.name.c_str());

    if(location == GL_INVALID_ENUM)
    {
        PILS_ERROR("Invalid program resource type\n");
        return false;
    }

    if(location == GL_INVALID_INDEX)
    {
        PILS_ERROR("Could not find bindless block named ", binding.name);
        return false;
    }

    glShaderStorageBlockBinding(program.handle(), location, binding.bindPoint);

    return true;
}

bool GpuProgramInterface::setConstantBindPoint(const GraphicProgram& program, const GpuProgramBinding& binding)
{
    PILS_ASSERT(binding.bindPoint != -1, binding.name, " has an innvalid bind point index ", binding.bindPoint);
    PILS_ASSERT(_constantData.find(binding.name) != _constantData.end(), "Unkown constant buffer : ", binding.name);

    GLuint location = glGetProgramResourceIndex(program.handle(), GL_UNIFORM_BLOCK, binding.name.c_str());

    if(location == GL_INVALID_ENUM)
    {
        PILS_ERROR("Invalid program resource type\n");
        return false;
    }

    if(location == GL_INVALID_INDEX)
    {
        PILS_ERROR("Could not find constant block named ", binding.name);
        return false;
    }

    glUniformBlockBinding(program.handle(), location, binding.bindPoint);

    return true;
}

bool GpuProgramInterface::setStorageBindPoint(const GraphicProgram& program, const GpuProgramBinding& binding)
{
    PILS_ASSERT(binding.bindPoint != -1, binding.name, " has an innvalid bind point index ", binding.bindPoint);
    PILS_ASSERT(_storageData.find(binding.name) != _storageData.end(), "Unkown storage buffer: ", binding.name);

    GLuint location = glGetProgramResourceIndex(program.handle(), GL_SHADER_STORAGE_BLOCK, binding.name.c_str());

    if(location == GL_INVALID_ENUM)
    {
        PILS_ERROR("Invalid program resource type\n");
        return false;
    }

    if(location == GL_INVALID_INDEX)
    {
        PILS_ERROR("Could not find storage block named ", binding.name);
        return false;
    }

    glShaderStorageBlockBinding(program.handle(), location, binding.bindPoint);

    return true;
}

bool GpuProgramInterface::setTextureBindPoint(const GraphicProgram& program, const GpuProgramBinding& binding)
{
    PILS_ASSERT(binding.bindPoint != -1, binding.name, " has an innvalid bind point index ", binding.bindPoint);
    PILS_ASSERT(_textureData.find(binding.name) != _textureData.end(), "Unkown texture: ", binding.name);

    GLuint location = glGetUniformLocation(program.handle(), binding.name.c_str());

    if(location == GL_INVALID_ENUM)
    {
        PILS_ERROR("Invalid enum while trying to read uniform location\n");
        return false;
    }

    if(location == GL_INVALID_INDEX)
    {
        PILS_ERROR("Could not find texture named ", binding.name);
        return false;
    }

    glProgramUniform1i(program.handle(), location, binding.bindPoint);

    return true;
}

bool GpuProgramInterface::setImageBindPoint(const GraphicProgram& program,  const GpuProgramBinding& binding)
{
    PILS_ASSERT(binding.bindPoint != -1, binding.name, " has an innvalid bind point index ", binding.bindPoint);
    PILS_ASSERT(_imageData.find(binding.name) != _imageData.end(), "Unkown image: ", binding.name);

    GLuint location = glGetUniformLocation(program.handle(), binding.name.c_str());

    if(location == GL_INVALID_ENUM)
    {
        PILS_ERROR("Invalid enum while trying to read uniform location\n");
        return false;
    }

    if(location == GL_INVALID_INDEX)
    {
        PILS_ERROR("Could not find image named ", binding.name);
        return false;
    }

    glProgramUniform1i(program.handle(), location, binding.bindPoint);

    return true;
}

struct GpuBindlessSlot
{
    GLuint64 handle{0};
    GLuint64 padding{0};
};

GpuProgramInterface::GpuProgramInterface(const GpuDevice& gpuDevice, const GpuProgramInterfaceTemplate& gpiTemplate) :
    _programType(gpiTemplate.programType())
{
    for(const auto& binding : gpiTemplate.bindless())
    {
        auto& data = _bindlessData[binding.name];
        data.binding = binding;
        data.descriptors.resize(binding.descriptorCount);

        GLuint storageBufferId;
        glGenBuffers(1, &storageBufferId);

        GLsizei dataSize = sizeof(GpuBindlessSlot) * data.descriptors.size();
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, storageBufferId);
        glBufferData(GL_SHADER_STORAGE_BUFFER, dataSize, nullptr, GL_STATIC_DRAW);

        _bindlessBacking[binding.name] = {true, storageBufferId};
    }

    for(const auto& binding : gpiTemplate.constants())
    {
        auto& data = _constantData[binding.name];
        data.binding = binding;
        data.descriptors.resize(1);
    }

    for(const auto& binding : gpiTemplate.storages())
    {
        auto& data = _storageData[binding.name];
        data.binding = binding;
        data.descriptors.resize(1);
    }

    for(const auto& binding : gpiTemplate.textures())
    {
        auto& data = _textureData[binding.name];
        data.binding = binding;
        data.descriptors.resize(1);
    }

    for(const auto& binding : gpiTemplate.images())
    {
        auto& data = _imageData[binding.name];
        data.binding = binding;
        data.descriptors.resize(1);
    }
}

GpuProgramInterface::~GpuProgramInterface()
{
    for (const auto& [name, backing] : _bindlessBacking)
    {
        glDeleteBuffers(1, &backing.bufferId);
    }
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

    PILS_ASSERT(resource.handle().handle != 0, "Writing invalid descriptor");
    descriptors[arrayIndex] = resource.handle();
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

    PILS_ASSERT(resource.handle().bufferId != 0, "Writing invalid descriptor");
    descriptors[arrayIndex] = resource.handle();
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

    PILS_ASSERT(resource.handle().bufferId != 0, "Writing invalid descriptor");
    descriptors[arrayIndex] = resource.handle();
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

    PILS_ASSERT(resource.handle().texId != 0, "Writing invalid descriptor");
    descriptors[arrayIndex] = resource.handle();
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

    GpuTextureResourceHandle handle;
    handle.texId = resource.handle().texId;
    handle.dimension = resource.handle().dimension;
    PILS_ASSERT(handle.texId != 0, "Writing invalid descriptor");
    descriptors[arrayIndex] = handle;
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

    PILS_ASSERT(resource.handle().texId != 0, "Writing invalid descriptor");
    descriptors[arrayIndex] = resource.handle();
}

bool GpuProgramInterface::compileAndSet(const GpuDevice& gpuDevice, const GraphicProgram& program)
{
    bool ok = true;

    for(const auto& data : _bindlessData)
    {
        const auto& descriptors = data.second.descriptors;

        auto backingIt = _bindlessBacking.find(data.first);
        PILS_ASSERT(backingIt != _bindlessBacking.end(), "Not bindless backing buffer declared for: ", data.first);

        if (backingIt->second.isDirty)
        {
            std::vector<GpuBindlessSlot> bindlessDescriptorData;
            bindlessDescriptorData.reserve(descriptors.size());
            for (const GpuBindlessTextureResourceHandle& handle : descriptors)
                bindlessDescriptorData.emplace_back(GpuBindlessSlot{handle.handle, 0ULL});

            GLsizei dataSize = sizeof(GpuBindlessSlot) * descriptors.size();
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, backingIt->second.bufferId);
            glBufferData(GL_SHADER_STORAGE_BUFFER, dataSize, bindlessDescriptorData.data(), GL_STATIC_DRAW);

            backingIt->second.isDirty = false;
        }

        const auto& binding = data.second.binding;
        ok = ok && setBindlessBindPoint(program, binding);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding.bindPoint, backingIt->second.bufferId);
    }

    for(const auto& data : _constantData)
    {
        const auto& descriptors = data.second.descriptors;
        const auto& handle = descriptors[0];

        const auto& binding = data.second.binding;
        ok = ok && setConstantBindPoint(program, binding);
        glBindBufferBase(GL_UNIFORM_BUFFER, binding.bindPoint, handle.bufferId);
    }

    for(const auto& data : _storageData)
    {
        const auto& descriptors = data.second.descriptors;
        const auto& handle = descriptors[0];

        const auto& binding = data.second.binding;
        ok = ok && setStorageBindPoint(program, binding);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding.bindPoint, handle.bufferId);
    }

    for(const auto& data : _textureData)
    {
        const auto& descriptors = data.second.descriptors;
        const auto& handle = descriptors[0];

        const auto& binding = data.second.binding;
        ok = ok && setTextureBindPoint(program, binding);
        glActiveTexture(GL_TEXTURE0 + binding.bindPoint);
        glBindTexture(handle.dimension, handle.texId);
    }

    for(const auto& data : _imageData)
    {
        const auto& descriptors = data.second.descriptors;
        const auto& handle = descriptors[0];

        const auto& binding = data.second.binding;
        ok = ok && setImageBindPoint(program, binding);
        glActiveTexture(GL_TEXTURE0 + binding.bindPoint);
        glBindImageTexture(binding.bindPoint, handle.texId, 0, handle.dimension == GL_TEXTURE_3D, 0, GL_WRITE_ONLY, handle.internalFormat);
    }

    PILS_ASSERT(ok, "Program interface not compatible with program ", program.name());

    return ok;
}

}

#endif // UNISIM_GRAPHIC_BACKEND_GL
