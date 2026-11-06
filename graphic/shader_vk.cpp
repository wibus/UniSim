#ifdef UNISIM_GRAPHIC_BACKEND_VK

#include <shaderc/shaderc.hpp>

#include <PilsCore/Utils/Assert.h>

#include <PilsCore/Gpu/Enums.h>

#include "shader.h"


namespace unisim
{

ShaderHandle::ShaderHandle(ShaderHandle&& other)
{
    PILS_ASSERT(false, "Not implemented!");
}

ShaderHandle::~ShaderHandle()
{
    PILS_ASSERT(false, "Not implemented!");
}

Shader::Shader(const std::string& name, ShaderType shaderType, ShaderHandle&& handle)
{
    PILS_ASSERT(false, "Not implemented!");
}

Shader::~Shader()
{
    PILS_ASSERT(false, "Not implemented!");
}


ProgramHandle::ProgramHandle(ProgramHandle&& other)
{
    PILS_ASSERT(false, "Not implemented!");
}

ProgramHandle::~ProgramHandle()
{
    PILS_ASSERT(false, "Not implemented!");
}

pils::gpu::PipelineType toPipelineType(ProgramType programType)
{
    if ((uint32_t(programType) & uint32_t(ProgramType::Graphics)) == uint32_t(ProgramType::Graphics))
        return pils::gpu::PipelineType::Graphics;

    if ((uint32_t(programType) & uint32_t(ProgramType::Compute)) == uint32_t(ProgramType::Compute))
        return pils::gpu::PipelineType::Compute;

    PILS_FATAL("Program type is unkown or does not correspont to an uncombined (unique) type: ", uint32_t(programType));

    return pils::gpu::PipelineType::Graphics;
}

pils::gpu::ShaderStage toShaderStage(ProgramType programType)
{
    uint32_t stageBits = 0x0;

    if ((uint32_t(programType) & uint32_t(ProgramType::Graphics)) != 0)
        stageBits |= uint32_t(pils::gpu::ShaderStage::AllGraphics);

    if ((uint32_t(programType) & uint32_t(ProgramType::Compute)) != 0)
        stageBits |= uint32_t(pils::gpu::ShaderStage::Compute);

    PILS_ASSERT(stageBits != 0x0, "Count not find corresponding shader stage(s) for program type: ", uint32_t(programType));

    return pils::gpu::ShaderStage(stageBits);
}


Program::Program(
        const GpuDevice& gpuDevice,
        const ProgramInterfaceTemplate& piTemplate,
        ProgramType type,
        const std::string& name,
        ProgramHandle&& programHandle,
        const std::vector<std::shared_ptr<Shader>>& shaders) :
        _type(type),
        _name(name)
{
    PILS_ASSERT(false, "Not implemented!");
}

Program::~Program()
{
    PILS_ASSERT(false, "Not implemented!");

}


ProgramScope::ProgramScope(const Program& program)
{
    PILS_ASSERT(false, "Not implemented!");
}

ProgramScope::~ProgramScope()
{
    PILS_ASSERT(false, "Not implemented!");
}


ShaderBuilder::ShaderBuilder()
{
}

ShaderBuilder::~ShaderBuilder()
{
}

bool ShaderBuilder::generateShader(
    std::shared_ptr<Shader>& shader,
    ShaderType shaderType,
    const std::string& shaderName,
    const std::vector<std::string>& srcs,
    const std::vector<std::string>& defines)
{
    PILS_ASSERT(false, "Not implemented!");
    return true;
}

bool ShaderBuilder::generateGraphicProgram(
    const GpuDevice& gpuDevice,
    const ProgramInterfaceTemplate& piTemplate,
    ProgramPtr& program,
    const std::string& name,
    const std::string& vertexFileName,
    const std::string& fragmentFileName,
    const std::vector<std::string>& defines)
{
    PILS_ASSERT(false, "Not implemented!");
    return true;
}

bool ShaderBuilder::generateComputeProgram(
    const GpuDevice& gpuDevice,
    const ProgramInterfaceTemplate& piTemplate,
    ProgramPtr& program,
    const std::string& name,
    const std::string& computeFileName,
    const std::vector<std::string>& defines)
{
    PILS_ASSERT(false, "Not implemented!");
    return true;
}

bool ShaderBuilder::generateComputeProgram(
    const GpuDevice& gpuDevice,
    const ProgramInterfaceTemplate& piTemplate,
    ProgramPtr& program,
    const std::string& name,
    const std::vector<std::shared_ptr<Shader>>& shaders)
{
    PILS_ASSERT(false, "Not implemented!");
    return true;
}

}

#endif // UNISIM_GRAPHIC_BACKEND_VK
