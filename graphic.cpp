#include "graphic.h"

#include <fstream>
#include <algorithm>

#include "light.h"
#include "scene.h"
#include "sky.h"
#include "terrain.h"
#include "primitive.h"
#include "material.h"
#include "bvh.h"
#include "grading.h"
#include "tracer.h"
#include "ui.h"
#include "profiler.h"


namespace unisim
{

DefineProfilePointGpu(Clear);


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
    {
        glDeleteShader(shaderId);
        shaderId = 0;
        return false;
    }
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


GraphicProgram::GraphicProgram(const std::string& name) :
    _name(name),
    _programId(0),
    _shaders()
{
    std::cout << "Creating program '" << _name << "'\n";
}

GraphicProgram::~GraphicProgram()
{
    std::cout << "Destroying program '" << _name << "'\n";

    for(GLuint shader : _shaders)
        glDeleteShader(shader);

    glDeleteProgram(_programId);
}

void GraphicProgram::reset(GLuint programId, const std::vector<GLuint>& shaders)
{
    std::cout << "Resetting program '" << _name << "'\n";

    for(GLuint shader : _shaders)
        glDeleteShader(shader);

    glDeleteProgram(_programId);

    _programId = programId;
    _shaders = shaders;
}


PathTracerModule::PathTracerModule(const std::string& name) :
    _name(name),
    _shaderId(0)
{
    std::cout << "Creating path tracer module '" << _name << "'\n";
}

PathTracerModule::~PathTracerModule()
{
    std::cout << "Destroying path tracer module '" << _name << "'\n";

    glDeleteShader(_shaderId);
}

void PathTracerModule::reset(GLuint shaderId)
{
    std::cout << "Resetting path tracer module '" << _name << "'\n";

    glDeleteShader(_shaderId);

    _shaderId = shaderId;
}


GraphicTask::GraphicTask(const std::string& name) :
    _name(name)
{
}

GraphicTask::~GraphicTask()
{
}

std::shared_ptr<GraphicProgram> GraphicTask::registerProgram(const std::string& name)
{
    _programs.emplace_back(new GraphicProgram(name));
    return _programs.back();
}

std::shared_ptr<PathTracerModule> GraphicTask::registerPathTracerModule(const std::string& name)
{
    _modules.emplace_back(new PathTracerModule(name));
    return _modules.back();
}

std::vector<GLuint> GraphicTask::pathTracerModules() const
{
    std::vector<GLuint> shaders;
    for(const auto& module : _modules)
        shaders.push_back(module->shaderId());
    return shaders;
}

bool GraphicTask::generateGraphicProgram(
        GraphicProgram& program,
        const std::string& vertexFileName,
        const std::string& fragmentFileName,
        const std::vector<std::string>& defines)
{
    GLuint vertexId = 0;
    if(!generateVertexShader(vertexId, vertexFileName, defines))
        return false;

    GLuint fragmentId = 0;
    if(!generateFragmentShader(fragmentId, fragmentFileName, defines))
        return false;

    GLuint programId = glCreateProgram();
    glAttachShader(programId, vertexId);
    glAttachShader(programId, fragmentId);
    glLinkProgram(programId);

    if(!validateProgram(programId, vertexFileName + " - " + fragmentFileName))
    {
        glDeleteProgram(programId);
        programId = 0;

        glDeleteShader(vertexId);
        vertexId = 0;

        glDeleteShader(fragmentId);
        fragmentId = 0;

        return false;
    }

    program.reset(programId, {vertexId, fragmentId});

    return true;
}

bool GraphicTask::generateComputeProgram(
        GraphicProgram& program,
        const std::string& computeFileName,
        const std::vector<std::string>& defines)
{
    GLuint shaderId = 0;
    if(!generateComputerShader(shaderId, computeFileName, defines))
        return false;

    GLuint programId = glCreateProgram();

    glAttachShader(programId, shaderId);
    glLinkProgram(programId);

    if(!validateProgram(programId, computeFileName))
    {
        glDeleteProgram(programId);
        programId = 0;

        glDeleteShader(shaderId);
        shaderId = 0;

        return false;
    }

    program.reset(programId, {shaderId});

    return true;
}

bool GraphicTask::generateComputeProgram(
        GraphicProgram& program,
        const std::string programName,
        const std::vector<GLuint>& shaders)
{
    GLuint programId = glCreateProgram();

    for(GLuint shader : shaders)
        glAttachShader(programId, shader);

    glLinkProgram(programId);

    if(!validateProgram(programId, programName))
    {
        glDeleteProgram(programId);
        programId = 0;

        return false;
    }

    program.reset(programId, {});

    return true;
}

bool GraphicTask::addPathTracerModule(
        PathTracerModule& module,
        const GraphicSettings& settings,
        const std::string& computeFileName,
        const std::vector<std::string>& defines)
{
    std::vector<std::string> allDefines = defines;

    if(settings.unbiased)
        allDefines.push_back("UNBIASED");

    for(int t = 0; t < Primitive::Type_Count; ++t)
    {
        std::string upperName = Primitive::Type_Names[t];
        std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);
        allDefines.push_back("PRIMITIVE_TYPE_" + upperName + " " + std::to_string(t));
    }

    std::vector<std::string> allSrcs = g_PathTracerCommonSrcs;
    allSrcs.push_back(loadSource(computeFileName));

    GLuint shaderId = 0;
    if(!generateShader(shaderId, GL_COMPUTE_SHADER, computeFileName, allSrcs, allDefines))
        return false;

    module.reset(shaderId);

    return true;
}


ClearSwapChain::ClearSwapChain() :
    GraphicTask("Clear")
{

}

void ClearSwapChain::render(GraphicContext& context)
{
    ProfileGpu(Clear);

    const Viewport& viewport = context.camera.viewport();
    glViewport(0, 0, viewport.width, viewport.height);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}


GraphicTaskGraph::GraphicTaskGraph()
{
    _settings.unbiased = false;
}

bool GraphicTaskGraph::initialize(const Scene& scene, const Camera& camera)
{
    _materials.reset(new MaterialDatabase());

    createTaskGraph(scene);

    GraphicContext context = {scene, camera, _resources, *_materials, _settings};

    for(const auto& task : _tasks)
    {
        task->registerDynamicResources(context);
    }

    _resources.initialize();

    g_GlslExtensions += "#extension GL_ARB_bindless_texture : require\n";

    g_PathTracerCommonSrcs.clear();
    g_PathTracerCommonSrcs.push_back(loadSource("shaders/common/constants.glsl"));
    g_PathTracerCommonSrcs.push_back(loadSource("shaders/common/data.glsl"));
    g_PathTracerCommonSrcs.push_back(loadSource("shaders/common/inputs.glsl"));
    g_PathTracerCommonSrcs.push_back(loadSource("shaders/common/signatures.glsl"));

    for(const auto& task : _tasks)
    {
        bool ok = task->definePathTracerModules(context);

        if(!ok)
        {
            std::cerr << "'" << task->name() << "' path tracer module definition failed\n";
            return false;
        }

        _resources.registerPathTracerProvider(std::dynamic_pointer_cast<PathTracerProvider>(task));
    }

    for(const auto& task : _tasks)
    {
        bool ok = task->defineShaders(context);

        if(!ok)
        {
            std::cerr << "'" << task->name() << "' shader definition failed\n";
            return false;
        }
    }

    for(const auto& task : _tasks)
    {
        bool ok = task->defineResources(context);

        if(!ok)
        {
            std::cerr << "'" << task->name() << "' resource definition failed\n";
            return false;
        }
    }

    return true;
}

bool GraphicTaskGraph::reloadShaders(const Scene& scene, const Camera& camera)
{
    g_PathTracerCommonSrcs.clear();
    g_PathTracerCommonSrcs.push_back(loadSource("shaders/common/constants.glsl"));
    g_PathTracerCommonSrcs.push_back(loadSource("shaders/common/data.glsl"));
    g_PathTracerCommonSrcs.push_back(loadSource("shaders/common/inputs.glsl"));
    g_PathTracerCommonSrcs.push_back(loadSource("shaders/common/signatures.glsl"));

    GraphicContext context = {scene, camera, _resources, *_materials, _settings};

    for(const auto& task : _tasks)
    {
        bool ok = task->definePathTracerModules(context);

        if(!ok)
        {
            std::cerr << "'" << task->name() << "' path tracer module definition failed\n";
            return false;
        }

        _resources.registerPathTracerProvider(std::dynamic_pointer_cast<PathTracerProvider>(task));
    }

    for(const auto& task : _tasks)
    {
        bool ok = task->defineShaders(context);

        if(!ok)
        {
            std::cerr << "'" << task->name() << "' shader definition failed\n";
            return false;
        }
    }

    return true;
}

void GraphicTaskGraph::execute(const Scene& scene, const Camera& camera)
{
    GraphicContext context = {scene, camera, _resources, *_materials, _settings};

    for(const auto& task : _tasks)
    {
        task->update(context);
    }

    for(const auto& task : _tasks)
    {
        task->render(context);
    }
}

void GraphicTaskGraph::createTaskGraph(const Scene& scene)
{
    _tasks.clear();

    addTask(_materials);
    addTask(std::shared_ptr<GraphicTask>(new BVH()));
    addTask(scene.sky()->graphicTask());
    addTask(scene.terrain()->graphicTask());
    addTask(std::shared_ptr<GraphicTask>(new Lighting()));
    addTask(std::shared_ptr<GraphicTask>(new PathTracer()));
    addTask(std::shared_ptr<GraphicTask>(new ClearSwapChain()));
    addTask(std::shared_ptr<GraphicTask>(new ColorGrading()));
    addTask(std::shared_ptr<GraphicTask>(new Ui()));
}

void GraphicTaskGraph::addTask(const std::shared_ptr<GraphicTask>& task)
{
    _tasks.push_back(task);
}

}
