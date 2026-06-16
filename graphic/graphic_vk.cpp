#ifdef UNISIM_GRAPHIC_BACKEND_VK

#include <iostream>

#include <PilsCore/Utils/Assert.h>

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

GraphicShader::GraphicShader(const std::string& name, GraphicShaderHandle&& handle)
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

GraphicProgram::GraphicProgram(
    const std::string& name,
    GraphicProgramHandle&& programHandle,
    const std::vector<std::shared_ptr<GraphicShader>>& shaders)
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
    GraphicProgramPtr& program,
    const std::string& name,
    const std::string& computeFileName,
    const std::vector<std::string>& defines)
{
    PILS_ASSERT(false, "Not implemented!");
    return true;
}

bool generateComputeProgram(
    GraphicProgramPtr& program,
    const std::string& name,
    const std::vector<std::shared_ptr<GraphicShader>>& shaders)
{
    PILS_ASSERT(false, "Not implemented!");
    return true;
}

}

#endif // UNISIM_GRAPHIC_BACKEND_VK
