#ifndef PATHTRACERPROVIDER_H
#define PATHTRACERPROVIDER_H

#include <vector>
#include <memory>
#include <string>

#include "../graphic/graphic.h"
#include "../graphic/gpuprograminterface.h"

#include "../taskgraph/graphictask.h"


namespace unisim
{

class GraphicProgram;
class GraphicContext;
class GraphicSettings;

extern std::vector<std::string> g_PathTracerCommonSrcs;


class PathTracerInterface : public GpuProgramInterface
{
public:
    PathTracerInterface();

private:
};


class PathTracerModule
{
    PathTracerModule(const PathTracerModule&) = delete;

public:
    PathTracerModule(const std::string& name, const std::shared_ptr<GraphicShader>& shader);
    ~PathTracerModule();

    std::shared_ptr<GraphicShader> shader() const { return _shader; }

    std::string name() const { return _name; }

private:
    std::string _name;
    std::shared_ptr<GraphicShader> _shader;
};

using PathTracerModulePtr = std::shared_ptr<PathTracerModule>;


class PathTracerProviderTask : public GraphicTask
{
public:
    PathTracerProviderTask(const std::string& name);
    virtual ~PathTracerProviderTask();

    virtual bool definePathTracerModules(
        GraphicContext& context,
        std::vector<std::shared_ptr<PathTracerModule>>& modules);

    virtual bool definePathTracerInterface(
        GraphicContext& context,
        PathTracerInterface& interface);

    virtual void bindPathTracerResources(
        GraphicContext& context,
        CompiledGpuProgramInterface& compiledGpi) const;

    uint64_t hash() const { return _hash; }

    template<typename T>
    static uint64_t hashVal(const T& data, uint64_t seed)
    {
        return combineHashes(seed, std::hash<std::string_view>()(
                                       std::string_view((char*)&data, sizeof (T))));
    }

    template<typename T>
    static uint64_t hashVec(const std::vector<T>& data, uint64_t seed)
    {
        return combineHashes(seed, std::hash<std::string_view>()(
                                       std::string_view((char*)data.data(), sizeof (T) * data.size())));
    }

    static uint64_t combineHashes(uint64_t h1, uint64_t h2)
    {
        return h1 ^ (h2 + 0x9e3779b9 + (h1<<6) + (h1>>2));
    }

protected:
    bool addPathTracerModule(
        std::vector<std::shared_ptr<PathTracerModule>>& modules,
        const std::string& name,
        const GraphicSettings& settings,
        const std::string& computeFileName,
        const std::vector<std::string>& defines = {});

    std::string _name;
    uint64_t _hash;
};

using PathTracerProviderTaskPtr = std::shared_ptr<PathTracerProviderTask>;

}

#endif // PATHTRACERPROVIDER_H
