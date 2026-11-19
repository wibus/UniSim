#ifndef SHADER_H
#define SHADER_H

#include "graphic/gpudevice.h"
#include <string>
#include <vector>
#include <memory>
#include <map>

#ifdef UNISIM_GRAPHIC_BACKEND_GL
#include <GL/glew.h>
#endif // UNISIM_GRAPHIC_BACKEND_GL

#ifdef UNISIM_GRAPHIC_BACKEND_VK
namespace shaderc
{
class Compiler;
class CompileOptions;
}

namespace pils
{
namespace gpu
{
enum class PipelineType;
enum class ShaderStage;
class PipelineLayout;
class PipelineStage;
class ShaderModule;
}
}
#endif // UNISIM_GRAPHIC_BACKEND_VK


namespace unisim
{

class View;
class RenderStates;
class ProgramInterfaceTemplate;

enum class ShaderType {Vertex, Fragment, Compute};


class ShaderSource
{
public:
    ShaderSource(const std::string& uri);

    bool load(const std::string& fileName);
    void set(const std::string& source);

    std::string uri() const { return _uri; }
    const std::string& source() const { return _source; }

private:
    std::string _uri;
    std::string _source;
};

typedef std::shared_ptr<ShaderSource> ShaderSourcePtr;


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

#ifdef UNISIM_GRAPHIC_BACKEND_VK
    ShaderHandle(const std::shared_ptr<pils::gpu::ShaderModule>& module);
    const std::shared_ptr<pils::gpu::ShaderModule>& shaderModule() const { return _module; };
#endif // UNISIM_GRAPHIC_BACKEND_VK

private:
#ifdef UNISIM_GRAPHIC_BACKEND_GL
    GLuint _shaderId;
#endif // UNISIM_GRAPHIC_BACKEND_GL

#ifdef UNISIM_GRAPHIC_BACKEND_VK
    std::shared_ptr<pils::gpu::ShaderModule> _module;
#endif // UNISIM_GRAPHIC_BACKEND_VK
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

#ifdef UNISIM_GRAPHIC_BACKEND_VK
    ProgramHandle(const std::shared_ptr<pils::gpu::PipelineStage>& pipeline);
    const std::shared_ptr<pils::gpu::PipelineStage>& pipelineStage() const { return _pipeline; };
#endif // UNISIM_GRAPHIC_BACKEND_VK

private:
#ifdef UNISIM_GRAPHIC_BACKEND_GL
    GLuint _programId;
#endif // UNISIM_GRAPHIC_BACKEND_GL

#ifdef UNISIM_GRAPHIC_BACKEND_VK
    std::shared_ptr<pils::gpu::PipelineStage> _pipeline;
#endif // UNISIM_GRAPHIC_BACKEND_VK
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
    const pils::gpu::PipelineLayout& layout() const;
#endif // UNISIM_GRAPHIC_BACKEND_VK

private:
    ProgramType _type;
    std::string _name;
    std::unique_ptr<ProgramHandle> _handle;
    std::vector<std::shared_ptr<Shader>> _shaders;
};

using ProgramPtr = std::shared_ptr<Program>;


class ProgramScope
{
public:
    ProgramScope(const GpuDevice& gpuDevice, const Program& program);
    ~ProgramScope();
};


extern const std::string GLSL_VERSION__HEADER;
extern std::string g_GlslExtensions;

class ShaderBuilder
{
public:
    ShaderBuilder(const GpuDevice& device);
    ~ShaderBuilder();

    bool generateGraphicProgram(
        const GpuDevice& gpuDevice,
        const ProgramInterfaceTemplate& piTemplate,
        const RenderStates& renderStates,
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
        const std::vector<ShaderSourcePtr>& sources,
        const std::vector<std::string>& defines = {});

private:
    bool generateShader(
        std::shared_ptr<Shader>& shader,
        ShaderType shaderType,
        const std::string& shaderName,
        const std::vector<ShaderSourcePtr>& sources,
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

    const char* defineLine(std::vector<std::string>& directives, int lineId, const std::string& srcName);

    const GpuDevice& _device;

#ifdef UNISIM_GRAPHIC_BACKEND_VK
    typedef uint64_t ProgramHash;

    std::unique_ptr<shaderc::Compiler> _compiler;
    std::unique_ptr<shaderc::CompileOptions> _options;
    std::vector<ProgramPtr> _programs;
#endif // UNISIM_GRAPHIC_BACKEND_VK
};

}

#endif // SHADER_H
