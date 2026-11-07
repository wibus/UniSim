#include "shader.h"

#include <fstream>
#include <iostream>

#include <PilsCore/Utils/Logger.h>


namespace unisim
{

const std::string GLSL_VERSION__HEADER = "#version 440\n";
std::string g_GlslExtensions;


ShaderSource::ShaderSource(const std::string& uri) :
    _uri(uri)
{

}

bool ShaderSource::load(const std::string& fileName)
{
    std::ifstream t(fileName);

    if(!t)
    {
        PILS_ERROR("Source file not found: ", fileName);
        return false;
    }

    _source = std::string((std::istreambuf_iterator<char>(t)),
                           std::istreambuf_iterator<char>());

    return true;
}

void ShaderSource::set(const std::string& source)
{
    _source = source;
}


bool ShaderBuilder::generateVertexShader(
    std::shared_ptr<Shader>& shader,
    const std::string& fileName,
    const std::vector<std::string>& defines)
{
    ShaderSourcePtr src = ShaderSourcePtr(new ShaderSource(fileName));

    if (!src->load(fileName))
        return false;

    return generateShader(shader, ShaderType::Vertex, fileName, {src}, defines);
}

bool ShaderBuilder::generateFragmentShader(
    std::shared_ptr<Shader>& shader,
    const std::string& fileName,
    const std::vector<std::string>& defines)
{
    ShaderSourcePtr src = ShaderSourcePtr(new ShaderSource(fileName));

    if (!src->load(fileName))
        return false;

    return generateShader(shader, ShaderType::Fragment, fileName, {src}, defines);
}

bool ShaderBuilder::generateComputerShader(
    std::shared_ptr<Shader>& shader,
    const std::string& fileName,
    const std::vector<std::string>& defines)
{
    ShaderSourcePtr src = ShaderSourcePtr(new ShaderSource(fileName));

    if (!src->load(fileName))
        return false;

    return generateShader(shader, ShaderType::Compute, fileName, {src}, defines);
}

const char* ShaderBuilder::defineLine(std::vector<std::string>& directives, int lineId, const std::string& srcName)
{
    directives.push_back("#line " + std::to_string(lineId) + " \"" + srcName + "\"\n");
    return directives.back().c_str();
}

}
