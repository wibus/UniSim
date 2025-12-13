#ifndef GPUPROGRAMINTERFACE_H
#define GPUPROGRAMINTERFACE_H

#include <map>
#include <memory>


#ifdef UNISIM_GRAPHIC_BACKEND_GL
#include "gpuprograminterface_gl.h"
#endif // UNISIM_GRAPHIC_BACKEND_GL

#ifdef UNISIM_GRAPHIC_BACKEND_VK
#include "gpuprograminterface_vk.h"
#endif // UNISIM_GRAPHIC_BACKEND_VK


namespace unisim
{

class GraphicProgram;


class GpuProgramInterface
{
public:
    GpuProgramInterface(const std::shared_ptr<GraphicProgram>& program);

    const std::shared_ptr<GraphicProgram>& program() const { return _program; }

    bool isValid() const;

    bool declareConstant(const std::string& name);
    bool declareStorage(const std::string& name);
    bool declareTexture(const std::string& name);
    bool declareImage(const std::string& name);
    bool compile();

    GpuProgramConstantBindPoint getConstantBindPoint(const std::string& name) const;
    GpuProgramStorageBindPoint  getStorageBindPoint(const std::string& name) const;
    GpuProgramTextureBindPoint  getTextureBindPoint(const std::string& name) const;
    GpuProgramImageBindPoint    getImageBindPoint(const std::string& name) const;

private:
    bool _isCompiled;
    bool _declFailed;

    std::shared_ptr<GraphicProgram> _program;

    GpuProgramConstantBindPoint _nextConstantBindPoint;
    std::map<std::string, GpuProgramConstantBindPoint> _constantBindPoints;

    GpuProgramStorageBindPoint _nextStorageBindPoint;
    std::map<std::string, GpuProgramStorageBindPoint> _storageBindPoints;

    GpuProgramTextureBindPoint _nextTextureBindPoint;
    std::map<std::string, GpuProgramTextureBindPoint> _textureBindPoints;

    GpuProgramImageBindPoint _nextImageBindPoint;
    std::map<std::string, GpuProgramImageBindPoint> _imageBindPoints;
};

typedef std::shared_ptr<GpuProgramInterface> GpuProgramInterfacePtr;

}

#endif // GPUPROGRAMINTERFACE_H
