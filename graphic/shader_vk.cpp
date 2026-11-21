#ifdef UNISIM_GRAPHIC_BACKEND_VK

#include <shaderc/shaderc.hpp>

#include <PilsCore/Utils/Assert.h>

#include <PilsCore/Gpu/Enums.h>
#include <PilsCore/Gpu/Context.h>
#include <PilsCore/Gpu/Pipeline.h>
#include <PilsCore/Gpu/Pass.h>
#include <PilsCore/Gpu/Command.h>

#include "shader.h"
#include "programinterface.h"
#include "renderstates.h"


namespace unisim
{

ShaderHandle::ShaderHandle(ShaderHandle&& other) :
    _module(std::move(other._module))
{
}

ShaderHandle::ShaderHandle(const std::shared_ptr<pils::gpu::ShaderModule>& module) :
    _module(module)
{
}

ShaderHandle::~ShaderHandle()
{
}

Shader::Shader(const std::string& name, ShaderType shaderType, ShaderHandle&& handle) :
    _name(name),
    _shaderType(shaderType),
    _handle(new ShaderHandle(std::move(handle)))
{
    PILS_DEBUG("Creating shader '", _name, "'");
}

Shader::~Shader()
{
    PILS_DEBUG("Destroying shader '", _name, "'");
}


ProgramHandle::ProgramHandle(ProgramHandle&& other) :
    _pipeline(std::move(other._pipeline))
{
}

ProgramHandle::ProgramHandle(const std::shared_ptr<pils::gpu::PipelineStage>& pipeline) :
    _pipeline(pipeline)
{

}

ProgramHandle::~ProgramHandle()
{
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

pils::gpu::ShaderStage toShaderStage(ShaderType type)
{
    switch(type)
    {
    case ShaderType::Vertex : return pils::gpu::ShaderStage::Vertex;
    case ShaderType::Fragment : return pils::gpu::ShaderStage::Fragment;
    case ShaderType::Compute : return pils::gpu::ShaderStage::Compute;
    default:
        PILS_FATAL("Unkown shader type", int(type));
        return pils::gpu::ShaderStage::Compute;
    }
}


Program::Program(
    ProgramType type,
    const std::string& name,
    ProgramHandle&& programHandle,
    const std::vector<std::shared_ptr<Shader>>& shaders) :
    _type(type),
    _name(name),
    _handle(new ProgramHandle(std::move(programHandle))),
    _shaders(shaders)
{
    PILS_DEBUG("Creating program '", _name, "'");
}

Program::~Program()
{
    PILS_DEBUG("Destroying program '", _name, "'");
}

const pils::gpu::PipelineLayout& Program::layout() const
{
    return *_handle->pipelineStage()->layout();
}


ProgramScope::ProgramScope(const GpuDevice& gpuDevice, const Program& program)
{
    pils::gpu::CommandList& cmd = gpuDevice.commandList();
    cmd.bindPipeline(*program.handle().pipelineStage());
}

ProgramScope::~ProgramScope()
{
    // Apparently no way to bind a null pipeline
}


ShaderBuilder::ShaderBuilder(const GpuDevice& device) :
    _device(device),
    _compiler(new shaderc::Compiler()),
    _options(new shaderc::CompileOptions())
{
    _options->SetOptimizationLevel(shaderc_optimization_level_performance);

    g_GlslExtensions = "#define API_VULKAN\n";
    g_GlslExtensions += "#extension GL_GOOGLE_cpp_style_line_directive : enable\n";
    g_GlslExtensions += "#extension GL_EXT_nonuniform_qualifier : require\n";
    g_GlslExtensions += "#extension GL_EXT_shader_image_load_formatted : require\n";
    g_GlslExtensions += "#extension GL_AMD_shader_trinary_minmax : disable\n";
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
            PILS_ERROR("Could not compile shader '", shaderName, "':\n", result.GetErrorMessage());
        else if (result.GetCompilationStatus() == shaderc_compilation_status_internal_error)
            PILS_FATAL("shaderc experienced an internal error while compiling shader:\n", shaderName);
        else
            PILS_ERROR("Unexpected error while compiling shader '", shaderName, ":'\n", result.GetErrorMessage());

        return false;
    }

    PILS_DEBUG("Successfully compiled shader: ", shaderName);

    // Create vulkan shader
    std::vector<uint32_t> spirv = {result.cbegin(), result.cend()};

    std::shared_ptr<pils::gpu::ShaderModule> module(
        new pils::gpu::ShaderModule(
            _device.context().device(),
            toShaderStage(shaderType),
            spirv));

    if (!module->isValid())
    {
        PILS_ERROR("Created an invalid shader module:", shaderName);
        return false;
    }

    shader.reset(
        new Shader(
            shaderName,
            shaderType,
            std::move(ShaderHandle(module))
        )
    );

    return true;
}

bool ShaderBuilder::generateGraphicProgram(
    const GpuDevice& gpuDevice,
    const ProgramInterfaceTemplate& piTemplate,
    const RenderStates& renderStates,
    ProgramPtr& program,
    const std::string& name,
    const std::string& vertexFileName,
    const std::string& fragmentFileName,
    const std::vector<std::string>& defines)
{
    std::shared_ptr<Shader> vertex;
    if(!generateVertexShader(vertex, vertexFileName, defines))
        return false;

    std::shared_ptr<Shader> fragment;
    if(!generateFragmentShader(fragment, fragmentFileName, defines))
        return false;

    std::shared_ptr<pils::gpu::PipelineLayout> pipelineLayout = piTemplate.computePipelineLayout(gpuDevice);

    std::shared_ptr<pils::gpu::PipelineStage> pipeline(
        new pils::gpu::GraphicsPipeline(
            _device.context().device(),
            pipelineLayout,
            vertex->handle().shaderModule(),
            fragment->handle().shaderModule(),
            renderStates.vertexState(),
            renderStates.topologyState(),
            renderStates.viewportState(),
            renderStates.rasterizationState(),
            renderStates.multisamplingState(),
            renderStates.blendState(),
            renderStates.dynamicStates(),
            renderStates.renderPass()));

    if (!pipeline->isValid())
    {
        PILS_ERROR("Created an invalid compute pipeline: ", name);
        return false;
    }

    program.reset(
        new Program(
            ProgramType::Graphics,
            name,
            std::move(ProgramHandle(pipeline)),
            {vertex, fragment}
        )
    );
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
    std::shared_ptr<Shader> compute;
    if(!generateComputerShader(compute, computeFileName, defines))
        return false;

    std::shared_ptr<pils::gpu::PipelineStage> pipeline(
        new pils::gpu::ComputePipeline(
            _device.context().device(),
            piTemplate.computePipelineLayout(_device),
            compute->handle().shaderModule()));

    if (!pipeline->isValid())
    {
        PILS_ERROR("Created an invalid compute pipeline: ", name);
        return false;
    }

    program.reset(
        new Program(
            ProgramType::Compute,
            name,
            std::move(ProgramHandle(pipeline)),
            {compute}
        )
    );

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
    std::shared_ptr<Shader> compute;
    if(!generateShader(compute, ShaderType::Compute, name, sources, defines))
        return false;

    std::shared_ptr<pils::gpu::PipelineStage> pipeline(
        new pils::gpu::ComputePipeline(
            _device.context().device(),
            piTemplate.computePipelineLayout(_device),
            compute->handle().shaderModule()));

    if (!pipeline->isValid())
    {
        PILS_ERROR("Created an invalid compute pipeline: ", name);
        return false;
    }

    program.reset(
        new Program(
            ProgramType::Compute,
            name,
            std::move(ProgramHandle(pipeline)),
            {compute}
        )
    );

    return true;
}

}

#endif // UNISIM_GRAPHIC_BACKEND_VK
