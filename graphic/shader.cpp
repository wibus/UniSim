#include "shader.h"

#include <fstream>
#include <iostream>


namespace unisim
{

const std::string GLSL_VERSION__HEADER = "#version 440\n";
std::string g_GlslExtensions;


std::string ShaderBuilder::loadSource(const std::string& fileName)
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

bool ShaderBuilder::generateVertexShader(
    std::shared_ptr<Shader>& shader,
    const std::string& fileName,
    const std::vector<std::string>& defines)
{
    std::string src = loadSource(fileName);
    return generateShader(shader, ShaderType::Vertex, fileName, {src}, defines);
}

bool ShaderBuilder::generateFragmentShader(
    std::shared_ptr<Shader>& shader,
    const std::string& fileName,
    const std::vector<std::string>& defines)
{
    std::string src = loadSource(fileName);
    return generateShader(shader, ShaderType::Fragment, fileName, {src}, defines);
}

bool ShaderBuilder::generateComputerShader(
    std::shared_ptr<Shader>& shader,
    const std::string& fileName,
    const std::vector<std::string>& defines)
{
    std::string src = loadSource(fileName);
    return generateShader(shader, ShaderType::Compute, fileName, {src}, defines);
}

}
