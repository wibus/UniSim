#ifndef PATHTRACER_H
#define PATHTRACER_H

#include <vector>
#include <memory>
#include <string>

#include "graphic.h"
#include "gpuprograminterface.h"


namespace unisim
{

class GraphicProgram;
class GraphicContext;

extern std::vector<std::string> g_PathTracerCommonSrcs;


class PathTracerInterface : public GpuProgramInterface
{
public:
    PathTracerInterface(const std::shared_ptr<GraphicProgram>& program);

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


class PathTracerProvider
{
public:
    PathTracerProvider();
    virtual ~PathTracerProvider();

    virtual bool definePathTracerModules(GraphicContext& context, std::vector<std::shared_ptr<PathTracerModule>>& modules);

    virtual bool definePathTracerInterface(GraphicContext& context, PathTracerInterface& interface);

    virtual void bindPathTracerResources(GraphicContext& context, PathTracerInterface& interface) const;

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
    uint64_t _hash;
};

}

#endif // PATHTRACER_H
