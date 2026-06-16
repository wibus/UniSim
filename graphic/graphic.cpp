#include "graphic.h"

#include <fstream>
#include <iostream>


namespace unisim
{

const std::string GLSL_VERSION__HEADER = "#version 440\n";
std::string g_GlslExtensions;


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

}
