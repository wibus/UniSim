#include "graphic.h"

#include <fstream>

#include "radiation.h"
#include "scene.h"
#include "sky.h"


namespace unisim
{


std::string loadSource(const std::string& fileName)
{
    std::ifstream t(fileName);
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

bool generateVertexShader(GLuint& shaderId, const std::string& fileName)
{
    std::string vertexSrc = loadSource(fileName);
    const char* vertexSrcChar = vertexSrc.data();
    shaderId = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(shaderId, 1, &vertexSrcChar, NULL);
    if(!compileShader(shaderId, fileName))
        return false;
    else
        return true;
}

bool generateFragmentShader(GLuint& shaderId, const std::string& fileName)
{
    std::string fragmentSrc = loadSource(fileName);
    const char* fragmentSrcChar = fragmentSrc.data();
    shaderId = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(shaderId, 1, &fragmentSrcChar, NULL);
    if(!compileShader(shaderId, fileName))
        return false;
    else
        return true;
}

bool generateComputerShader(GLuint& shaderId, const std::string& fileName)
{
    std::string computeSrc = loadSource(fileName);
    const char* computeSrcChar = computeSrc.data();
    shaderId = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shaderId, 1, &computeSrcChar, NULL);
    if(!compileShader(shaderId, fileName))
        return false;
    else
        return true;
}

bool generateGraphicProgram(GLuint& programId, const std::string& vertexFileName, const std::string& fragmentFileName)
{
    GLuint vs = 0;
    if(!generateVertexShader(vs, vertexFileName))
        return false;

    GLuint fs = 0;
    if(!generateFragmentShader(fs, fragmentFileName))
        return false;

    programId = glCreateProgram();
    glAttachShader(programId, fs);
    glAttachShader(programId, vs);
    glLinkProgram(programId);

    if(!validateProgram(programId, vertexFileName + " - " + fragmentFileName))
        return false;

    return true;
}

bool generateComputeProgram(GLuint& programId, const std::string& computeFileName, const std::vector<GLuint>& shaders)
{
    GLuint cs = 0;
    if(!generateComputerShader(cs, computeFileName))
        return false;

    programId = glCreateProgram();

    glAttachShader(programId, cs);
    for(GLuint shader : shaders)
        glAttachShader(programId, shader);

    glLinkProgram(programId);

    if(!validateProgram(programId, computeFileName))
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


GraphicTaskGraph::GraphicTaskGraph()
{

}

bool GraphicTaskGraph::initialize(const Scene& scene, const Camera& camera)
{
    createTaskGraph(scene, camera);

    GraphicContext context = {scene, camera, _resources};

    for(const auto& task : _tasks)
    {
        task->registerDynamicResources(context);
    }

    _resources.initialize();

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
    GraphicContext context = {scene, camera, _resources};

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
    addTask(std::make_shared<Radiation>());
}

void GraphicTaskGraph::addTask(const std::shared_ptr<GraphicTask>& task)
{
    _tasks.push_back(task);
}

}
