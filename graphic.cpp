#include "graphic.h"

#include <fstream>

#include "radiation.h"
#include "scene.h"
#include "sky.h"
#include "terrain.h"


namespace unisim
{

const std::string GLSL_VERSION__HEADER = "#version 440\n";

std::string g_GlslExtensions;

std::vector<std::string> g_PathTracerCommonSrcs;


std::string loadSource(const std::string& fileName)
{
    std::ifstream t(fileName);

    if(!t)
    {
        std::cerr << "Source file not found: " << fileName << std::endl;
        return std::string();
    }

    std::string str((std::istreambuf_iterator<char>(t)),
                     std::istreambuf_iterator<char>());
    return str;
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
    printf("Compiling shader %s\n" , name.c_str());

    glCompileShader(shaderId);

    // check for compile errors
    int params = -1;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &params);

    if (GL_TRUE != params)
    {
        fprintf(stderr, "ERROR: GL shader index %i did not compile\n", shaderId);
        printShaderInfoLog(shaderId);
        return false; // or exit or something
    }

    return true;
}

void printProgrammeInfoLog(GLuint programId)
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
    printf("Validating program %s\n" , name.c_str());

    int linkStatus = -1;
    glGetProgramiv(programId, GL_LINK_STATUS, &linkStatus);
    printf("program %i GL_LINK_STATUS = %i\n", programId, linkStatus);
    if (GL_TRUE != linkStatus)
    {
        printProgrammeInfoLog(programId);
        return false;
    }

    glValidateProgram(programId);

    int validationStatus = -1;
    glGetProgramiv(programId, GL_VALIDATE_STATUS, &validationStatus);
    printf("program %i GL_VALIDATE_STATUS = %i\n", programId, validationStatus);
    if (GL_TRUE != validationStatus)
    {
        printProgrammeInfoLog(programId);
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
        GLuint& shaderId,
        GLenum shaderType,
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

    shaderId = glCreateShader(shaderType);
    glShaderSource(shaderId, shaderSrc.size(), &shaderSrc[0], NULL);
    if(!compileShader(shaderId, shaderName))
        return false;
    else
        return true;
}

bool generateVertexShader(
        GLuint& shaderId,
        const std::string& fileName,
        const std::vector<std::string>& defines)
{
    std::string src = loadSource(fileName);
    return generateShader(shaderId, GL_VERTEX_SHADER, fileName, {src}, defines);
}

bool generateFragmentShader(
        GLuint& shaderId,
        const std::string& fileName,
        const std::vector<std::string>& defines)
{
    std::string src = loadSource(fileName);
    return generateShader(shaderId, GL_FRAGMENT_SHADER, fileName, {src}, defines);
}

bool generateComputerShader(
        GLuint& shaderId,
        const std::string& fileName,
        const std::vector<std::string>& defines)
{
    std::string src = loadSource(fileName);
    return generateShader(shaderId, GL_COMPUTE_SHADER, fileName, {src}, defines);
}

bool generateGraphicProgram(
        GLuint& programId,
        const std::string& vertexFileName,
        const std::string& fragmentFileName,
        const std::vector<std::string>& defines)
{
    GLuint vs = 0;
    if(!generateVertexShader(vs, vertexFileName, defines))
        return false;

    GLuint fs = 0;
    if(!generateFragmentShader(fs, fragmentFileName, defines))
        return false;

    programId = glCreateProgram();
    glAttachShader(programId, fs);
    glAttachShader(programId, vs);
    glLinkProgram(programId);

    if(!validateProgram(programId, vertexFileName + " - " + fragmentFileName))
        return false;

    return true;
}

bool generateComputeProgram(
        GLuint& programId,
        const std::string& computeFileName,
        const std::vector<std::string>& defines)
{
    GLuint cs = 0;
    if(!generateComputerShader(cs, computeFileName, defines))
        return false;

    programId = glCreateProgram();

    glAttachShader(programId, cs);
    glLinkProgram(programId);

    if(!validateProgram(programId, computeFileName))
        return false;

    return true;
}

bool generateComputeProgram(
        GLuint& programId,
        const std::string programName,
        const std::vector<GLuint>& shaders)
{
    programId = glCreateProgram();

    for(GLuint shader : shaders)
        glAttachShader(programId, shader);

    glLinkProgram(programId);

    if(!validateProgram(programId, programName))
        return false;

    return true;
}

GraphicTask::GraphicTask(const std::string& name) :
    _name(name)
{
}

GraphicTask::~GraphicTask()
{
}

bool GraphicTask::addPathTracerModule(GLuint shaderId)
{
    _pathTracerModules.push_back(shaderId);
    return true;
}

bool GraphicTask::addPathTracerModule(
        GLuint& shaderId,
        const GraphicSettings& settings,
        const std::string& computeFileName,
        const std::vector<std::string>& defines)
{
    std::vector<std::string> allDefines = defines;

    if(settings.unbiased)
        allDefines.push_back("UNBIASED");

    allDefines.push_back("PRIMITIVE_TYPE_SPHERE  0");
    allDefines.push_back("PRIMITIVE_TYPE_TERRAIN 1");

    std::vector<std::string> allSrcs = g_PathTracerCommonSrcs;
    allSrcs.push_back(loadSource(computeFileName));

    bool success = generateShader(shaderId, GL_COMPUTE_SHADER, computeFileName, allSrcs, allDefines);

    if(success)
        _pathTracerModules.push_back(shaderId);

    return success;
}


GraphicTaskGraph::GraphicTaskGraph()
{
    _settings.unbiased = false;
}

bool GraphicTaskGraph::initialize(const Scene& scene, const Camera& camera)
{
    createTaskGraph(scene, camera);

    GraphicContext context = {scene, camera, _resources, _settings};

    for(const auto& task : _tasks)
    {
        task->registerDynamicResources(context);
    }

    _resources.initialize();

    g_GlslExtensions += "#extension GL_ARB_bindless_texture : require\n";

    g_PathTracerCommonSrcs.push_back(loadSource("shaders/common/constants.glsl"));
    g_PathTracerCommonSrcs.push_back(loadSource("shaders/common/data.glsl"));
    g_PathTracerCommonSrcs.push_back(loadSource("shaders/common/inputs.glsl"));
    g_PathTracerCommonSrcs.push_back(loadSource("shaders/common/signatures.glsl"));

    for(const auto& task : _tasks)
    {
        bool ok = task->definePathTracerModules(context);

        if(!ok)
        {
            return false;
        }

        _resources.registerPathTracerProvider(std::dynamic_pointer_cast<PathTracerProvider>(task));
    }

    for(const auto& task : _tasks)
    {
        bool ok = task->defineResources(context);

        if(!ok)
        {
            return false;
        }
    }

    return true;
}

void GraphicTaskGraph::execute(const Scene& scene, const Camera& camera)
{
    GraphicContext context = {scene, camera, _resources, _settings};

    for(const auto& task : _tasks)
    {
        task->update(context);
    }

    for(const auto& task : _tasks)
    {
        task->render(context);
    }
}

void GraphicTaskGraph::createTaskGraph(const Scene& scene, const Camera& camera)
{
    addTask(scene.sky()->graphicTask());
    addTask(scene.terrain()->graphicTask());
    addTask(std::make_shared<Radiation>());
}

void GraphicTaskGraph::addTask(const std::shared_ptr<GraphicTask>& task)
{
    _tasks.push_back(task);
}

}
