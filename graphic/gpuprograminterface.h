#ifndef GPUPROGRAMINTERFACE_H
#define GPUPROGRAMINTERFACE_H

#include <map>
#include <memory>
#include <vector>
#include <string>


#ifdef UNISIM_GRAPHIC_BACKEND_GL
#include <GL/glew.h>
#endif // UNISIM_GRAPHIC_BACKEND_GL


namespace unisim
{

class GraphicProgram;


struct GpuProgramBindlessInput
{
    std::string name;
};

struct GpuProgramConstantInput
{
    std::string name;
};

struct GpuProgramStorageInput
{
    std::string name;
};

struct GpuProgramTextureInput
{
    std::string name;
};

struct GpuProgramImageInput
{
    std::string name;
};


struct GpuProgramBindlessBindPoint
{
    static GpuProgramBindlessBindPoint first();
    static GpuProgramBindlessBindPoint invalid();
    static GpuProgramBindlessBindPoint next(const GpuProgramBindlessBindPoint& current);

#ifdef UNISIM_GRAPHIC_BACKEND_GL
    GLuint bindPoint;
#endif // UNISIM_GRAPHIC_BACKEND_GL
};

struct GpuProgramConstantBindPoint
{
    static GpuProgramConstantBindPoint first();
    static GpuProgramConstantBindPoint invalid();
    static GpuProgramConstantBindPoint next(const GpuProgramConstantBindPoint& current);

#ifdef UNISIM_GRAPHIC_BACKEND_GL
    GLuint bindPoint;
#endif // UNISIM_GRAPHIC_BACKEND_GL
};


struct GpuProgramStorageBindPoint
{

    static GpuProgramStorageBindPoint first();
    static GpuProgramStorageBindPoint invalid();
    static GpuProgramStorageBindPoint next(const GpuProgramStorageBindPoint& current);

#ifdef UNISIM_GRAPHIC_BACKEND_GL
    GLuint bindPoint;
#endif // UNISIM_GRAPHIC_BACKEND_GL
};

struct GpuProgramTextureBindPoint
{
    static GpuProgramTextureBindPoint first();
    static GpuProgramTextureBindPoint invalid();
    static GpuProgramTextureBindPoint next(const GpuProgramTextureBindPoint& current);

#ifdef UNISIM_GRAPHIC_BACKEND_GL
    GLuint bindPoint;
#endif // UNISIM_GRAPHIC_BACKEND_GL
};

struct GpuProgramImageBindPoint
{
    static GpuProgramImageBindPoint first();
    static GpuProgramImageBindPoint invalid();
    static GpuProgramImageBindPoint next(const GpuProgramImageBindPoint& current);

#ifdef UNISIM_GRAPHIC_BACKEND_GL
    GLuint bindPoint;
#endif // UNISIM_GRAPHIC_BACKEND_GL
};


class CompiledGpuProgramInterface
{
public:
    CompiledGpuProgramInterface();

    bool isValid() const { return _isValid; }

    GpuProgramBindlessBindPoint getBindlessBindPoint(const std::string& name) const;
    GpuProgramConstantBindPoint getConstantBindPoint(const std::string& name) const;
    GpuProgramStorageBindPoint  getStorageBindPoint(const std::string& name) const;
    GpuProgramTextureBindPoint  getTextureBindPoint(const std::string& name) const;
    GpuProgramImageBindPoint    getImageBindPoint(const std::string& name) const;

private:
    friend class GpuProgramInterface;

    bool set(const GraphicProgram& program, const GpuProgramBindlessBindPoint& bindPoint,   const GpuProgramBindlessInput& input);
    bool set(const GraphicProgram& program, const GpuProgramConstantBindPoint& bindPoint,   const GpuProgramConstantInput& input);
    bool set(const GraphicProgram& program, const GpuProgramStorageBindPoint& bindPoint,    const GpuProgramStorageInput& input);
    bool set(const GraphicProgram& program, const GpuProgramTextureBindPoint& bindPoint,    const GpuProgramTextureInput& input);
    bool set(const GraphicProgram& program, const GpuProgramImageBindPoint& bindPoint,      const GpuProgramImageInput& input);

    bool _isValid;
    std::map<std::string, GpuProgramBindlessBindPoint> _bindlessBindPoints;
    std::map<std::string, GpuProgramConstantBindPoint> _constantBindPoints;
    std::map<std::string, GpuProgramStorageBindPoint> _storageBindPoints;
    std::map<std::string, GpuProgramTextureBindPoint> _textureBindPoints;
    std::map<std::string, GpuProgramImageBindPoint> _imageBindPoints;
};

class GpuProgramInterface
{
public:
    GpuProgramInterface();

    bool declareBindless(const GpuProgramBindlessInput& input);
    bool declareConstant(const GpuProgramConstantInput& input);
    bool declareStorage(const GpuProgramStorageInput& input);
    bool declareTexture(const GpuProgramTextureInput& input);
    bool declareImage(const GpuProgramImageInput& input);


    bool compile(CompiledGpuProgramInterface& compiledGpi, const GraphicProgram& program);

private:
    std::vector<GpuProgramBindlessInput> _bindless;
    std::vector<GpuProgramConstantInput> _constants;
    std::vector<GpuProgramStorageInput> _storages;
    std::vector<GpuProgramTextureInput> _textures;
    std::vector<GpuProgramImageInput> _images;
};

typedef std::shared_ptr<GpuProgramInterface> GpuProgramInterfacePtr;

}

#endif // GPUPROGRAMINTERFACE_H
