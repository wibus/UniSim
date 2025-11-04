#ifndef GRAPHIC_H
#define GRAPHIC_H

#include <string>
#include <vector>
#include <memory>

#ifdef UNISIM_GRAPHIC_BACKEND_GL
#include "graphic_gl.h"
#endif // UNISIM_GRAPHIC_BACKEND_GL

#ifdef UNISIM_GRAPHIC_BACKEND_VK
#include "graphic_vk.h"
#endif // UNISIM_GRAPHIC_BACKEND_VK

namespace unisim
{

enum class ShaderType {Vertex, Fragment, Compute};


class GraphicShader
{
    GraphicShader(const GraphicShader&) = delete;

public:
    GraphicShader(const std::string& name, GraphicShaderHandle&& handle);
    ~GraphicShader();

    std::string name() const { return _name; }

    const GraphicShaderHandle& handle() const { return *_handle; }

private:
    std::string _name;
    std::unique_ptr<GraphicShaderHandle> _handle;
};


class GraphicProgram
{
    GraphicProgram(const GraphicProgram&) = delete;

    friend class GraphicTask;
    GraphicProgram(const std::string& name);
public:
    ~GraphicProgram();

    bool isValid() const { return _handle.get() != nullptr; }

    std::string name() const { return _name; }

    const GraphicProgramHandle& handle() const { return *_handle; }

    void reset(GraphicProgramHandle&& programHandle, const std::vector<std::shared_ptr<GraphicShader>>& shaders);

private:
    std::string _name;
    std::unique_ptr<GraphicProgramHandle> _handle;
    std::vector<std::shared_ptr<GraphicShader>> _shaders;
};

using GraphicProgramPtr = std::shared_ptr<GraphicProgram>;

class GraphicProgramScope
{
public:
    GraphicProgramScope(const GraphicProgram& program);
    ~GraphicProgramScope();
};

extern const std::string GLSL_VERSION__HEADER;
extern std::string g_GlslExtensions;

std::string loadSource(const std::string& fileName);

bool generateShader(
    std::shared_ptr<GraphicShader>& shader,
    ShaderType shaderType,
    const std::string& shaderName,
    const std::vector<std::string>& srcs,
    const std::vector<std::string>& defines);

bool generateVertexShader(
    std::shared_ptr<GraphicShader>& shader,
    const std::string& fileName,
    const std::vector<std::string>& defines);

bool generateFragmentShader(
    std::shared_ptr<GraphicShader>& shader,
    const std::string& fileName,
    const std::vector<std::string>& defines);

bool generateComputerShader(
    std::shared_ptr<GraphicShader>& shader,
    const std::string& fileName,
    const std::vector<std::string>& defines);

bool generateGraphicProgram(
    GraphicProgram& program,
    const std::string& vertexFileName,
    const std::string& fragmentFileName,
    const std::vector<std::string>& defines = {});

bool generateComputeProgram(
    GraphicProgram& program,
    const std::string& computeFileName,
    const std::vector<std::string>& defines = {});

bool generateComputeProgram(
    GraphicProgram& program,
    const std::string programName,
    const std::vector<std::shared_ptr<GraphicShader>>& shaders);

}

#endif // GRAPHIC_H
