#ifndef GRAPHIC_H
#define GRAPHIC_H

#include "graphic/gpudevice.h"
#include <string>
#include <vector>
#include <memory>

#ifdef UNISIM_GRAPHIC_BACKEND_GL
#include <GL/glew.h>
#endif // UNISIM_GRAPHIC_BACKEND_GL

#ifdef UNISIM_GRAPHIC_BACKEND_VK
namespace pils
{
namespace gpu
{
enum class PipelineType;
enum class ShaderStage;
class PipelineLayout;
}
}
#endif // UNISIM_GRAPHIC_BACKEND_VK


namespace unisim
{

class GpuProgramInterfaceTemplate;

enum class ShaderType {Vertex, Fragment, Compute};


class GraphicShaderHandle
{
protected:
    GraphicShaderHandle(const GraphicShaderHandle&) = delete;

public:
    GraphicShaderHandle(GraphicShaderHandle&& other);
    ~GraphicShaderHandle();

#ifdef UNISIM_GRAPHIC_BACKEND_GL
    GraphicShaderHandle(GLuint shaderId);
    operator GLuint() const { return _shaderId; }
#endif // UNISIM_GRAPHIC_BACKEND_GL

private:
#ifdef UNISIM_GRAPHIC_BACKEND_GL
    GLuint _shaderId;
#endif // UNISIM_GRAPHIC_BACKEND_GL
};

class GraphicProgramHandle
{
protected:
    GraphicProgramHandle(const GraphicProgramHandle&) = delete;

public:
    GraphicProgramHandle(GraphicProgramHandle&& other);
    ~GraphicProgramHandle();

#ifdef UNISIM_GRAPHIC_BACKEND_GL
    GraphicProgramHandle(GLuint programId);
    operator GLuint() const { return _programId; }
#endif // UNISIM_GRAPHIC_BACKEND_GL

private:
#ifdef UNISIM_GRAPHIC_BACKEND_GL
    GLuint _programId;
#endif // UNISIM_GRAPHIC_BACKEND_GL
};


class GraphicShader
{
    GraphicShader(const GraphicShader&) = delete;

public:
    GraphicShader(const std::string& name, ShaderType shaderType, GraphicShaderHandle&& handle);
    ~GraphicShader();

    std::string name() const { return _name; }
    ShaderType shaderType() const { return _shaderType; }

    const GraphicShaderHandle& handle() const { return *_handle; }

private:
    std::string _name;
    ShaderType _shaderType;
    std::unique_ptr<GraphicShaderHandle> _handle;
};

using GraphicShaderPtr = std::shared_ptr<GraphicShader>;


enum class ProgramType
{
    Graphics        = 1 << 0,
    Compute         = 1 << 1,
    All             = Graphics | Compute
};

#ifdef UNISIM_GRAPHIC_BACKEND_VK
pils::gpu::PipelineType toPipelineType(ProgramType programType);
pils::gpu::ShaderStage toShaderStage(ProgramType programType);
#endif // UNISIM_GRAPHIC_BACKEND_VK


class GraphicProgram
{
    GraphicProgram(const GraphicProgram&) = delete;

public:
    GraphicProgram(
        const GpuDevice& gpuDevice,
        const GpuProgramInterfaceTemplate& gpiTemplate,
        ProgramType type,
        const std::string& name,
        GraphicProgramHandle&& programHandle,
        const std::vector<std::shared_ptr<GraphicShader>>& shaders);
    ~GraphicProgram();

    bool isValid() const { return _handle.get() != nullptr; }

    ProgramType type() const { return _type; }
    std::string name() const { return _name; }

    const GraphicProgramHandle& handle() const { return *_handle; }

#ifdef UNISIM_GRAPHIC_BACKEND_VK
    const pils::gpu::PipelineLayout& layout() const { return *_layout; }
#endif // UNISIM_GRAPHIC_BACKEND_VK

private:
    ProgramType _type;
    std::string _name;
    std::unique_ptr<GraphicProgramHandle> _handle;
    std::vector<std::shared_ptr<GraphicShader>> _shaders;

#ifdef UNISIM_GRAPHIC_BACKEND_VK
    std::shared_ptr<pils::gpu::PipelineLayout> _layout;
#endif // UNISIM_GRAPHIC_BACKEND_VK
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
    const GpuDevice& gpuDevice,
    const GpuProgramInterfaceTemplate& gpiTemplate,
    GraphicProgramPtr& program,
    const std::string& name,
    const std::string& vertexFileName,
    const std::string& fragmentFileName,
    const std::vector<std::string>& defines = {});

bool generateComputeProgram(
    const GpuDevice& gpuDevice,
    const GpuProgramInterfaceTemplate& gpiTemplate,
    GraphicProgramPtr& program,
    const std::string& name,
    const std::string& computeFileName,
    const std::vector<std::string>& defines = {});

bool generateComputeProgram(
    const GpuDevice& gpuDevice,
    const GpuProgramInterfaceTemplate& gpiTemplate,
    GraphicProgramPtr& program,
    const std::string& name,
    const std::vector<std::shared_ptr<GraphicShader>>& shaders);

}

#endif // GRAPHIC_H
