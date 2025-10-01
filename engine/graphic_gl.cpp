#ifdef UNISIM_GRAPHIC_BACKEND_GL

#include <iostream>

#include "graphic.h"


namespace unisim
{


GraphicShaderHandle::GraphicShaderHandle(GraphicShaderHandle&& other) :
    _shaderId(other._shaderId)
{
    other._shaderId = 0;
}

GraphicShaderHandle::GraphicShaderHandle(GLuint shaderId) :
    _shaderId(shaderId)
{
    std::cout << "Creating shader handle '" << _shaderId << "'" << std::endl;
}

GraphicShaderHandle::~GraphicShaderHandle()
{
    if (_shaderId != 0)
    {
        std::cout << "Destroying shader handle '" << _shaderId << "'" << std::endl;
        glDeleteShader(_shaderId);
    }
}

GraphicShader::GraphicShader(const std::string& name, GraphicShaderHandle&& handle) :
    _name(name),
    _handle(new GraphicShaderHandle(std::move(handle)))
{

}

GraphicShader::~GraphicShader()
{

}


GraphicProgramHandle::GraphicProgramHandle(GraphicProgramHandle&& other) :
    _programId(other._programId)
{
    other._programId = 0;
}

GraphicProgramHandle::GraphicProgramHandle(GLuint programId) :
    _programId(programId)
{
    std::cout << "Creating program handle '" << _programId << "'" << std::endl;
}

GraphicProgramHandle::~GraphicProgramHandle()
{
    if (_programId != 0)
    {
        std::cout << "Destroying program handle '" << _programId << "'" << std::endl;
        glDeleteProgram(_programId);
    }
}

GraphicProgram::GraphicProgram(const std::string& name) :
    _name(name)
{
    std::cout << "Creating program '" << _name << "'" << std::endl;
}

GraphicProgram::~GraphicProgram()
{
    std::cout << "Destroying program '" << _name << "'" << std::endl;

}

void GraphicProgram::reset(GraphicProgramHandle&& programHandle, const std::vector<std::shared_ptr<GraphicShader>>& shaders)
{
    std::cout << "Resetting program '" << _name << "'" << std::endl;

    _shaders.clear();
    _handle.reset();

    _handle.reset(new GraphicProgramHandle(std::move(programHandle)));
    _shaders = shaders;
}

GraphicProgramScope::GraphicProgramScope(const GraphicProgram& program)
{
    if(program.isValid())
        glUseProgram(program.handle());
}

GraphicProgramScope::~GraphicProgramScope()
{
    glUseProgram(0);
}



void printShaderInfoLog(GLuint shaderId)
{
    int max_length = 2048;
    int actual_length = 0;
    char shader_log[2048];
    glGetShaderInfoLog(shaderId, max_length, &actual_length, shader_log);
    std::cout << "shader info log for GL index " << shaderId << ":" << std::endl;
    std::cout << shader_log << std::endl;
}

bool compileShader(GLuint shaderId, const std::string& name)
{
    std::cout << "Compiling shader '" << name.c_str() << "'" << std::endl;

    glCompileShader(shaderId);

    // check for compile errors
    int params = -1;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &params);

    if (GL_TRUE != params)
    {
        std::cerr << "ERROR: GL shader index " << shaderId << " did not compile" << std::endl;
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
    std::cout << "program info log for GL index " << programId << ":" << std::endl;
    std::cout << program_log << std::endl;
}

bool validateProgram(GLuint programId, const std::string& name)
{
    std::cout << "Validating program '" << name.c_str() << "'" << std::endl;

    int linkStatus = -1;
    glGetProgramiv(programId, GL_LINK_STATUS, &linkStatus);
    std::cout << "program " << programId << " GL_LINK_STATUS = " << (linkStatus ? "SUCCESS" : "FAILURE") << std::endl;
    if (GL_TRUE != linkStatus)
    {
        printProgramInfoLog(programId);
        return false;
    }

    glValidateProgram(programId);

    int validationStatus = -1;
    glGetProgramiv(programId, GL_VALIDATE_STATUS, &validationStatus);
    std::cout << "program " << programId << " GL_VALIDATE_STATUS = " << (validationStatus ? "SUCCESS" : "FAILURE") << std::endl;
    if (GL_TRUE != validationStatus)
    {
        printProgramInfoLog(programId);
        return false;
    }

    return true;
}

const char* defineLine(std::vector<std::string>& directives, int lineId)
{
    int sourceId = int(directives.size()) + 1;
    directives.push_back("#line " + std::to_string(lineId) + " " + std::to_string(sourceId) + "\n");
    return directives.back().c_str();
}

bool generateShader(
    std::shared_ptr<GraphicShader>& shader,
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
    default: std::cerr << "Invalid shader type" << std::endl; return false;
    }

    glShaderSource(shaderId, shaderSrc.size(), &shaderSrc[0], NULL);
    if(!compileShader(shaderId, shaderName))
    {
        glDeleteShader(shaderId);
        return false;
    }
    else
    {
        shader.reset(new GraphicShader(shaderName, GraphicShaderHandle(shaderId)));
        return true;
    }
}

bool generateVertexShader(
    std::shared_ptr<GraphicShader>& shader,
    const std::string& fileName,
    const std::vector<std::string>& defines)
{
    std::string src = loadSource(fileName);
    return generateShader(shader, ShaderType::Vertex, fileName, {src}, defines);
}

bool generateFragmentShader(
    std::shared_ptr<GraphicShader>& shader,
    const std::string& fileName,
    const std::vector<std::string>& defines)
{
    std::string src = loadSource(fileName);
    return generateShader(shader, ShaderType::Fragment, fileName, {src}, defines);
}

bool generateComputerShader(
    std::shared_ptr<GraphicShader>& shader,
    const std::string& fileName,
    const std::vector<std::string>& defines)
{
    std::string src = loadSource(fileName);
    return generateShader(shader, ShaderType::Compute, fileName, {src}, defines);
}

bool generateGraphicProgram(
    GraphicProgram& program,
    const std::string& vertexFileName,
    const std::string& fragmentFileName,
    const std::vector<std::string>& defines)
{
    std::shared_ptr<GraphicShader> vertex;
    if(!generateVertexShader(vertex, vertexFileName, defines))
        return false;

    std::shared_ptr<GraphicShader> fragment;
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

    program.reset(std::move(GraphicProgramHandle(programId)), {vertex, fragment});

    return true;
}

bool generateComputeProgram(
    GraphicProgram& program,
    const std::string& computeFileName,
    const std::vector<std::string>& defines)
{
    std::shared_ptr<GraphicShader> compute;
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

    program.reset(std::move(GraphicProgramHandle(programId)), {compute});

    return true;
}

bool generateComputeProgram(
    GraphicProgram& program,
    const std::string programName,
    const std::vector<std::shared_ptr<PathTracerModule>>& pathTracerModules)
{
    GLuint programId = glCreateProgram();

    for(const auto& module : pathTracerModules)
        glAttachShader(programId, module->shader()->handle());

    glLinkProgram(programId);

    if(!validateProgram(programId, programName))
    {
        glDeleteProgram(programId);
        return false;
    }

    program.reset(programId, {});

    return true;
}

}

#endif // UNISIM_GRAPHIC_BACKEND_GL
