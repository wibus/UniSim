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

    bool declareConstant(const std::string& blockName);
    bool declareStorage(const std::string& blockName);
    bool compile();

    GpuProgramConstantBindPoint getConstantBindPoint(const std::string& blockName) const;
    GpuProgramStorageBindPoint getStorageBindPoint(const std::string& blockName) const;

    GpuProgramTextureUnit grabTextureUnit();
    void resetTextureUnits();

private:
    bool _isCompiled;
    bool _declFailed;

    std::shared_ptr<GraphicProgram> _program;
    GpuProgramTextureUnit _nextTextureUnit;

    GpuProgramConstantBindPoint _nextUboBindPoint;
    std::map<std::string, GpuProgramConstantBindPoint> _uboBindPoints;

    GpuProgramStorageBindPoint _nextSsboBindPoint;
    std::map<std::string, GpuProgramStorageBindPoint> _ssboBindPoints;
};

}

#endif // GPUPROGRAMINTERFACE_H
