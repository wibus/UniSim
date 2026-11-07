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


ShaderBuilder::ShaderBuilder() :
    _compiler(new shaderc::Compiler()),
    _options(new shaderc::CompileOptions())
{
    _options->SetOptimizationLevel(shaderc_optimization_level_performance);

    g_GlslExtensions = "";
    g_GlslExtensions += "#extension GL_ARB_bindless_texture : require\n";
    g_GlslExtensions += "#extension GL_GOOGLE_cpp_style_line_directive : enable\n";
}

ShaderBuilder::~ShaderBuilder()
{
}

 shaderc_shader_kind toShaderc(ShaderType type)
 {
    switch(type)
    {
    case ShaderType::Vertex : return shaderc_vertex_shader;
    case ShaderType::Fragment : return shaderc_fragment_shader;
    case ShaderType::Compute : return shaderc_compute_shader;
    default:
        PILS_FATAL("Unkown shader type: ", int(type));
        return shaderc_compute_shader;
    }
 }

bool ShaderBuilder::generateShader(
    std::shared_ptr<Shader>& shader,
    ShaderType shaderType,
    const std::string& shaderName,
    const std::vector<ShaderSourcePtr>& sources,
    const std::vector<std::string>& defines)
{
    std::vector<std::string> lineDirectives;
    lineDirectives.reserve(1 + sources.size());

    std::string shaderSrc;
    shaderSrc += GLSL_VERSION__HEADER;
    shaderSrc += g_GlslExtensions;

    std::string definitionBloc;
    if(!defines.empty())
    {
        for(const auto& define : defines)
            definitionBloc += "#define " + define + "\n";
        shaderSrc += defineLine(lineDirectives, 1, "DEFINES");
        shaderSrc += definitionBloc.c_str();
    }

    const char* LINE_RETURN = "\n";
    for(const auto& source : sources)
    {
        if (!source)
        {
            PILS_ERROR("One of the shader source file provided is null. Aborting shader creation.");
            return false;
        }

        if (source->source().empty())
        {
            PILS_WARN("One of the shader source is empty: ", source->uri());
            continue;
        }

        shaderSrc += defineLine(lineDirectives, 1, source->uri());
        shaderSrc += source->source();
        shaderSrc += LINE_RETURN;
    }

    shaderc::SpvCompilationResult result = _compiler->CompileGlslToSpv(
        shaderSrc,
        toShaderc(shaderType),
        shaderName.c_str(),
        *_options);

    shaderc_compilation_status status = result.GetCompilationStatus();

    if (status != shaderc_compilation_status_success)
    {
        if (status == shaderc_compilation_status_compilation_error)
            PILS_ERROR("Could not compile shader '", shaderName, "' \n", result.GetErrorMessage());
        else if (result.GetCompilationStatus() == shaderc_compilation_status_internal_error)
            PILS_FATAL("shaderc experienced an internal error while compiling shader: ", shaderName);
        else
            PILS_ERROR("Unexpected error while compiling shader '", shaderName, "'\n", result.GetErrorMessage());

        return false;
    }

    PILS_DEBUG("Successfully compiled shader: ", shaderName);

    // Create vulkan shader
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
        const std::vector<ShaderSourcePtr>& sources,
        const std::vector<std::string>& defines)
{
    PILS_ASSERT(false, "Not implemented!");
    return true;
}

}

#endif // UNISIM_GRAPHIC_BACKEND_VK
