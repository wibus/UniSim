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
class Context;
class PathTracerModule;


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


class PathTracerProvider
{
public:
    PathTracerProvider();
    virtual ~PathTracerProvider();

    virtual std::vector<std::shared_ptr<PathTracerModule>> pathTracerModules() const;

    virtual bool definePathTracerModules(Context& context);

    virtual void setPathTracerResources(Context& context, PathTracerInterface& interface) const;

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
