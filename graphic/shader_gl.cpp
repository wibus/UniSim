#ifdef UNISIM_GRAPHIC_BACKEND_GL

#include <PilsCore/Utils/Logger.h>

#include "shader.h"


namespace unisim
{

ShaderHandle::ShaderHandle(ShaderHandle&& other) :
    _shaderId(other._shaderId)
{
    other._shaderId = 0;
}

ShaderHandle::ShaderHandle(GLuint shaderId) :
    _shaderId(shaderId)
{
    PILS_DEBUG("Creating shader handle '", _shaderId, "'");
}

ShaderHandle::~ShaderHandle()
{
    if (_shaderId != 0)
    {
        PILS_DEBUG("Destroying shader handle '", _shaderId, "'");
        glDeleteShader(_shaderId);
    }
}

Shader::Shader(const std::string& name, ShaderType shaderType, ShaderHandle&& handle) :
    _name(name),
    _shaderType(shaderType),
    _handle(new ShaderHandle(std::move(handle)))
{

}

Shader::~Shader()
{

}


ProgramHandle::ProgramHandle(ProgramHandle&& other) :
    _programId(other._programId)
{
    other._programId = 0;
}

ProgramHandle::ProgramHandle(GLuint programId) :
    _programId(programId)
{
    PILS_DEBUG("Creating program handle '", _programId, "'");
}

ProgramHandle::~ProgramHandle()
{
    if (_programId != 0)
    {
        PILS_DEBUG("Destroying program handle '", _programId, "'");
        glDeleteProgram(_programId);
    }
}

Program::Program(
        const GpuDevice& gpuDevice,
        const ProgramInterfaceTemplate& piTemplate,
        ProgramType type,
        const std::string& name,
        ProgramHandle&& programHandle,
        const std::vector<std::shared_ptr<Shader>>& shaders) :
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

ProgramScope::ProgramScope(const Program& program)
{
    if(program.isValid())
        glUseProgram(program.handle());
}

ProgramScope::~ProgramScope()
{
    glUseProgram(0);
}



void printShaderInfoLog(GLuint shaderId)
{
    int max_length = 2048;
    int actual_length = 0;
    char shader_log[2048];
    glGetShaderInfoLog(shaderId, max_length, &actual_length, shader_log);
    PILS_INFO("shader info log for GL index ", shaderId, ":");
    PILS_INFO(std::string(shader_log));
}

bool compileShader(GLuint shaderId, const std::string& name)
{
    PILS_DEBUG("Compiling shader '", name.c_str(), "'");

    glCompileShader(shaderId);

    // check for compile errors
    int params = -1;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &params);

    if (GL_TRUE != params)
    {
        PILS_FATAL("ERROR: GL shader index ", shaderId, " did not compile");
        printShaderInfoLog(shaderId);
        return false; // or exit or something
    }

    return true;
}

void printProgramInfoLog(GLuint programId)
{
    int max_length = 2048;
    int actual_length = 0;
    char program_log[2048];
    glGetProgramInfoLog(programId, max_length, &actual_length, program_log);
    PILS_INFO("program info log for GL index ", programId, ":");
    PILS_INFO(std::string(program_log));
}

bool validateProgram(GLuint programId, const std::string& name)
{
    PILS_DEBUG("Validating program '", name.c_str(), "'");

    int linkStatus = -1;
    glGetProgramiv(programId, GL_LINK_STATUS, &linkStatus);
    if (GL_TRUE != linkStatus)
    {
        PILS_FATAL("Program '", name, "' linking failed");
        printProgramInfoLog(programId);
        return false;
    }

    glValidateProgram(programId);

    int validationStatus = -1;
    glGetProgramiv(programId, GL_VALIDATE_STATUS, &validationStatus);
    if (GL_TRUE != validationStatus)
    {
        PILS_FATAL("Program '", name, "' validation failed");
        printProgramInfoLog(programId);
        return false;
    }

    PILS_INFO("Program '", name, "' successfully validated");

    return true;
}

const char* defineLine(std::vector<std::string>& directives, int lineId)
{
    int sourceId = int(directives.size()) + 1;
    directives.push_back("#line " + std::to_string(lineId) + " " + std::to_string(sourceId) + "\n");
    return directives.back().c_str();
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
    std::vector<std::string> srcUris;

    std::vector<std::string> lineDirectives;
    lineDirectives.reserve(1 + srcs.size());

    std::vector<const GLchar*> shaderSrc;
    shaderSrc.push_back(GLSL_VERSION__HEADER.c_str());
    shaderSrc.push_back(g_GlslExtensions.c_str());

    std::string definitionBloc;
    if(!defines.empty())
    {
        for(const auto& define : defines)
            definitionBloc += "#define " + define + "\n";
        srcUris.push_back("DEFINES");
        shaderSrc.push_back(defineLine(lineDirectives, 1));
        shaderSrc.push_back(definitionBloc.c_str());
    }

    const char* LINE_RETURN = "\n";
    for(std::size_t s = 0; s < srcs.size(); ++s)
    {
        const std::string& src = srcs[s];
        srcUris.push_back("srcs " + std::to_string(s));
        shaderSrc.push_back(defineLine(lineDirectives, 1));
        shaderSrc.push_back(src.c_str());
        shaderSrc.push_back(LINE_RETURN);
    }

    GLuint shaderId = 0;
    switch(shaderType)
    {
    case ShaderType::Vertex:   shaderId = glCreateShader(GL_VERTEX_SHADER);   break;
    case ShaderType::Fragment: shaderId = glCreateShader(GL_FRAGMENT_SHADER); break;
    case ShaderType::Compute:  shaderId = glCreateShader(GL_COMPUTE_SHADER);  break;
    default: PILS_FATAL("Invalid shader type"); return false;
    }

    glShaderSource(shaderId, shaderSrc.size(), &shaderSrc[0], NULL);
    if(!compileShader(shaderId, shaderName))
    {
        glDeleteShader(shaderId);
        return false;
    }
    else
    {
        shader.reset(new Shader(shaderName, shaderType, ShaderHandle(shaderId)));
        return true;
    }
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
    std::shared_ptr<Shader> vertex;
    if(!generateVertexShader(vertex, vertexFileName, defines))
        return false;

    std::shared_ptr<Shader> fragment;
    if(!generateFragmentShader(fragment, fragmentFileName, defines))
        return false;

    GLuint programId = glCreateProgram();
    glAttachShader(programId, vertex->handle());
    glAttachShader(programId, fragment->handle());
    glLinkProgram(programId);

    if(!validateProgram(programId, vertexFileName + " - " + fragmentFileName))
    {
        glDeleteProgram(programId);
        vertex.reset();
        fragment.reset();

        return false;
    }

    program.reset(
        new Program(
            gpuDevice,
            piTemplate,
            ProgramType::Graphics,
            name,
            std::move(ProgramHandle(programId)),
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

    GLuint programId = glCreateProgram();

    glAttachShader(programId, compute->handle());
    glLinkProgram(programId);

    if(!validateProgram(programId, computeFileName))
    {
        glDeleteProgram(programId);
        compute.reset();

        return false;
    }

    program.reset(
        new Program(
            gpuDevice,
            piTemplate,
            ProgramType::Compute,
            name,
            std::move(ProgramHandle(programId)),
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
    const std::vector<std::shared_ptr<Shader>>& shaders)
{
    GLuint programId = glCreateProgram();

    for(const auto& shader : shaders)
        glAttachShader(programId, shader->handle());

    glLinkProgram(programId);

    if(!validateProgram(programId, name))
    {
        glDeleteProgram(programId);
        return false;
    }

    program.reset(
        new Program(
            gpuDevice,
            piTemplate,
            ProgramType::Compute,
            name,
            std::move(ProgramHandle(programId)),
            shaders
        )
    );

    return true;
}

}

#endif // UNISIM_GRAPHIC_BACKEND_GL
