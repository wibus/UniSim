#ifndef SHADER_H
#define SHADER_H

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

class ProgramInterfaceTemplate;

enum class ShaderType {Vertex, Fragment, Compute};


class ShaderHandle
{
protected:
    ShaderHandle(const ShaderHandle&) = delete;

public:
    ShaderHandle(ShaderHandle&& other);
    ~ShaderHandle();

#ifdef UNISIM_GRAPHIC_BACKEND_GL
    ShaderHandle(GLuint shaderId);
    operator GLuint() const { return _shaderId; }
#endif // UNISIM_GRAPHIC_BACKEND_GL

private:
#ifdef UNISIM_GRAPHIC_BACKEND_GL
    GLuint _shaderId;
#endif // UNISIM_GRAPHIC_BACKEND_GL
};

class ProgramHandle
{
protected:
    ProgramHandle(const ProgramHandle&) = delete;

public:
    ProgramHandle(ProgramHandle&& other);
    ~ProgramHandle();

#ifdef UNISIM_GRAPHIC_BACKEND_GL
    ProgramHandle(GLuint programId);
    operator GLuint() const { return _programId; }
#endif // UNISIM_GRAPHIC_BACKEND_GL

private:
#ifdef UNISIM_GRAPHIC_BACKEND_GL
    GLuint _programId;
#endif // UNISIM_GRAPHIC_BACKEND_GL
};


class Shader
{
    Shader(const Shader&) = delete;

public:
    Shader(const std::string& name, ShaderType shaderType, ShaderHandle&& handle);
    ~Shader();

    std::string name() const { return _name; }
    ShaderType shaderType() const { return _shaderType; }

    const ShaderHandle& handle() const { return *_handle; }

private:
    std::string _name;
    ShaderType _shaderType;
    std::unique_ptr<ShaderHandle> _handle;
};

using ShaderPtr = std::shared_ptr<Shader>;


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


class Program
{
    Program(const Program&) = delete;

public:
    Program(
        const GpuDevice& gpuDevice,
        const ProgramInterfaceTemplate& piTemplate,
        ProgramType type,
        const std::string& name,
        ProgramHandle&& programHandle,
        const std::vector<std::shared_ptr<Shader>>& shaders);
    ~Program();

    bool isValid() const { return _handle.get() != nullptr; }

    ProgramType type() const { return _type; }
    std::string name() const { return _name; }

    const ProgramHandle& handle() const { return *_handle; }

#ifdef UNISIM_GRAPHIC_BACKEND_VK
    const pils::gpu::PipelineLayout& layout() const { return *_layout; }
#endif // UNISIM_GRAPHIC_BACKEND_VK

private:
    ProgramType _type;
    std::string _name;
    std::unique_ptr<ProgramHandle> _handle;
    std::vector<std::shared_ptr<Shader>> _shaders;

#ifdef UNISIM_GRAPHIC_BACKEND_VK
    std::shared_ptr<pils::gpu::PipelineLayout> _layout;
#endif // UNISIM_GRAPHIC_BACKEND_VK
};

using ProgramPtr = std::shared_ptr<Program>;


class ProgramScope
{
public:
    ProgramScope(const Program& program);
    ~ProgramScope();
};

extern const std::string GLSL_VERSION__HEADER;
extern std::string g_GlslExtensions;

class ShaderBuilder
{
public:
    ShaderBuilder();
    ~ShaderBuilder();

    std::string loadSource(const std::string& fileName);

    bool generateShader(
        std::shared_ptr<Shader>& shader,
        ShaderType shaderType,
        const std::string& shaderName,
        const std::vector<std::string>& srcs,
        const std::vector<std::string>& defines);

    bool generateVertexShader(
        std::shared_ptr<Shader>& shader,
        const std::string& fileName,
        const std::vector<std::string>& defines);

    bool generateFragmentShader(
        std::shared_ptr<Shader>& shader,
        const std::string& fileName,
        const std::vector<std::string>& defines);

    bool generateComputerShader(
        std::shared_ptr<Shader>& shader,
        const std::string& fileName,
        const std::vector<std::string>& defines);

    bool generateGraphicProgram(
        const GpuDevice& gpuDevice,
        const ProgramInterfaceTemplate& piTemplate,
        ProgramPtr& program,
        const std::string& name,
        const std::string& vertexFileName,
        const std::string& fragmentFileName,
        const std::vector<std::string>& defines = {});

    bool generateComputeProgram(
        const GpuDevice& gpuDevice,
        const ProgramInterfaceTemplate& piTemplate,
        ProgramPtr& program,
        const std::string& name,
        const std::string& computeFileName,
        const std::vector<std::string>& defines = {});

    bool generateComputeProgram(
        const GpuDevice& gpuDevice,
        const ProgramInterfaceTemplate& piTemplate,
        ProgramPtr& program,
        const std::string& name,
        const std::vector<std::shared_ptr<Shader>>& shaders);

private:
};

}

#endif // SHADER_H
