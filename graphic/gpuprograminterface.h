#ifndef GPUPROGRAMINTERFACE_H
#define GPUPROGRAMINTERFACE_H

#include <map>
#include <memory>
#include <vector>
#include <string>


#ifdef UNISIM_GRAPHIC_BACKEND_GL
#include "gpuprograminterface_gl.h"
#endif // UNISIM_GRAPHIC_BACKEND_GL

#ifdef UNISIM_GRAPHIC_BACKEND_VK
#include "gpuprograminterface_vk.h"
#endif // UNISIM_GRAPHIC_BACKEND_VK


namespace unisim
{

class GraphicProgram;

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


class CompiledGpuProgramInterface
{
public:
    CompiledGpuProgramInterface();

    bool isValid() const { return _isValid; }

    GpuProgramConstantBindPoint getConstantBindPoint(const std::string& name) const;
    GpuProgramStorageBindPoint  getStorageBindPoint(const std::string& name) const;
    GpuProgramTextureBindPoint  getTextureBindPoint(const std::string& name) const;
    GpuProgramImageBindPoint    getImageBindPoint(const std::string& name) const;

private:
    friend class GpuProgramInterface;

    bool set(const GraphicProgram& program, const GpuProgramConstantBindPoint& bindPoint, const GpuProgramConstantInput& input);
    bool set(const GraphicProgram& program, const GpuProgramStorageBindPoint& bindPoint, const GpuProgramStorageInput& input);
    bool set(const GraphicProgram& program, const GpuProgramTextureBindPoint& bindPoint, const GpuProgramTextureInput& input);
    bool set(const GraphicProgram& program, const GpuProgramImageBindPoint& bindPoint, const GpuProgramImageInput& input);

    bool _isValid;
    std::map<std::string, GpuProgramConstantBindPoint> _constantBindPoints;
    std::map<std::string, GpuProgramStorageBindPoint> _storageBindPoints;
    std::map<std::string, GpuProgramTextureBindPoint> _textureBindPoints;
    std::map<std::string, GpuProgramImageBindPoint> _imageBindPoints;
};

class GpuProgramInterface
{
public:
    GpuProgramInterface();

    bool declareConstant(const GpuProgramConstantInput& input);
    bool declareStorage(const GpuProgramStorageInput& input);
    bool declareTexture(const GpuProgramTextureInput& input);
    bool declareImage(const GpuProgramImageInput& input);

    bool compile(CompiledGpuProgramInterface& compiledGpi, const GraphicProgram& program);

private:
    std::vector<GpuProgramConstantInput> _constants;
    std::vector<GpuProgramStorageInput> _storages;
    std::vector<GpuProgramTextureInput> _textures;
    std::vector<GpuProgramImageInput> _images;
};

typedef std::shared_ptr<GpuProgramInterface> GpuProgramInterfacePtr;

}

#endif // GPUPROGRAMINTERFACE_H
