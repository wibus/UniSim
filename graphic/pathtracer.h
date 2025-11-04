#ifndef PATHTRACER_H
#define PATHTRACER_H

#include <map>
#include <vector>
#include <memory>
#include <string>

#include "graphic.h"


namespace unisim
{

class GraphicProgram;
class GraphicContext;

extern std::vector<std::string> g_PathTracerCommonSrcs;


class PathTracerInterface
{
public:
    PathTracerInterface(const std::shared_ptr<GraphicProgram>& program);

    const std::shared_ptr<GraphicProgram>& program() const { return _program; }

    bool declareUbo(const std::string& blockName);
    bool declareSsbo(const std::string& blockName);

    GLuint getUboBindPoint(const std::string& blockName) const;
    GLuint getSsboBindPoint(const std::string& blockName) const;

    GLuint grabTextureUnit();
    void resetTextureUnits();

    bool isValid() const;

private:
    std::shared_ptr<GraphicProgram> _program;
    GLuint _nextTextureUnit;

    GLuint _nextUboBindPoint;
    std::map<std::string, GLuint> _uboBindPoints;

    GLuint _nextSsboBindPoint;
    std::map<std::string, GLuint> _ssboBindPoints;
};


class PathTracerModule
{
    PathTracerModule(const PathTracerModule&) = delete;

    friend class GraphicTask;
    PathTracerModule(const std::string& name);

public:
    PathTracerModule(const std::string& name, const std::shared_ptr<GraphicShader>& shader);
    ~PathTracerModule();

    std::shared_ptr<GraphicShader> shader() const { return _shader; }

    std::string name() const { return _name; }

    void reset(const std::shared_ptr<GraphicShader>& shader);

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

    virtual std::vector<std::shared_ptr<PathTracerModule>> pathTracerModules() const;

    virtual bool definePathTracerModules(GraphicContext& context);

    virtual void setPathTracerResources(GraphicContext& context, PathTracerInterface& interface) const;

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
