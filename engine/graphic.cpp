#include "graphic.h"

#include <fstream>
#include <iostream>


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


PathTracerModule::PathTracerModule(const std::string& name) :
    _name(name)
{
    std::cout << "Creating path tracer module '" << _name << "'" << std::endl;
}

PathTracerModule::PathTracerModule(const std::string& name, const std::shared_ptr<GraphicShader>& shader) :
    _name(name),
    _shader(shader)
{
    std::cout << "Creating path tracer module '" << _name << "' from shader '" << shader->name() << "'" << std::endl;
}

PathTracerModule::~PathTracerModule()
{
    std::cout << "Destroying path tracer module '" << _name << "'" << std::endl;
}

void PathTracerModule::reset(const std::shared_ptr<GraphicShader>& shader)
{
    std::cout << "Resetting path tracer module '" << _name << "'" << std::endl;

    _shader = shader;
}

}
