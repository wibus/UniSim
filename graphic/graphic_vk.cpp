#ifdef UNISIM_GRAPHIC_BACKEND_VK

#include <iostream>

#include <PilsCore/Utils/Assert.h>

#include <PilsCore/Gpu/Enums.h>

#include "graphic.h"


namespace unisim
{

GraphicShaderHandle::GraphicShaderHandle(GraphicShaderHandle&& other)
{
    PILS_ASSERT(false, "Not implemented!");
}

GraphicShaderHandle::~GraphicShaderHandle()
{
    PILS_ASSERT(false, "Not implemented!");
}

GraphicShader::GraphicShader(const std::string& name, ShaderType shaderType, GraphicShaderHandle&& handle)
{
    PILS_ASSERT(false, "Not implemented!");
}

GraphicShader::~GraphicShader()
{
    PILS_ASSERT(false, "Not implemented!");
}


GraphicProgramHandle::GraphicProgramHandle(GraphicProgramHandle&& other)
{
    PILS_ASSERT(false, "Not implemented!");
}

GraphicProgramHandle::~GraphicProgramHandle()
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

GraphicProgram::GraphicProgram(
        const GpuDevice& gpuDevice,
        const GpuProgramInterfaceTemplate& gpiTemplate,
        ProgramType type,
        const std::string& name,
        GraphicProgramHandle&& programHandle,
        const std::vector<std::shared_ptr<GraphicShader>>& shaders) :
        _type(type),
        _name(name)
{
    PILS_ASSERT(false, "Not implemented!");
}

GraphicProgram::~GraphicProgram()
{
    PILS_ASSERT(false, "Not implemented!");

}

GraphicProgramScope::GraphicProgramScope(const GraphicProgram& program)
{
    PILS_ASSERT(false, "Not implemented!");
}

GraphicProgramScope::~GraphicProgramScope()
{
    PILS_ASSERT(false, "Not implemented!");
}

bool generateShader(
    std::shared_ptr<GraphicShader>& shader,
    ShaderType shaderType,
    const std::string& shaderName,
    const std::vector<std::string>& srcs,
    const std::vector<std::string>& defines)
{
    PILS_ASSERT(false, "Not implemented!");
    return true;
}

bool generateGraphicProgram(
    const GpuDevice& gpuDevice,
    const GpuProgramInterfaceTemplate& gpiTemplate,
    GraphicProgramPtr& program,
    const std::string& name,
    const std::string& vertexFileName,
    const std::string& fragmentFileName,
    const std::vector<std::string>& defines)
{
    PILS_ASSERT(false, "Not implemented!");
    return true;
}

bool generateComputeProgram(
    const GpuDevice& gpuDevice,
    const GpuProgramInterfaceTemplate& gpiTemplate,
    GraphicProgramPtr& program,
    const std::string& name,
    const std::string& computeFileName,
    const std::vector<std::string>& defines)
{
    PILS_ASSERT(false, "Not implemented!");
    return true;
}

bool generateComputeProgram(
    const GpuDevice& gpuDevice,
    const GpuProgramInterfaceTemplate& gpiTemplate,
    GraphicProgramPtr& program,
    const std::string& name,
    const std::vector<std::shared_ptr<GraphicShader>>& shaders)
{
    PILS_ASSERT(false, "Not implemented!");
    return true;
}

}

#endif // UNISIM_GRAPHIC_BACKEND_VK
