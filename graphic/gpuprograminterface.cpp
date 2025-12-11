#include "gpuprograminterface.h"

#include <PilsCore/Utils/Assert.h>
#include <PilsCore/Utils/Logger.h>

#include "graphic.h"


namespace unisim
{

GpuProgramInterface::GpuProgramInterface(const std::shared_ptr<GraphicProgram>& program) :
    _isCompiled(false),
    _declFailed(false),
    _program(program),
    _nextConstantBindPoint(GpuProgramConstantBindPoint::first()),
    _nextStorageBindPoint(GpuProgramStorageBindPoint::first()),
    _nextTextureBindPoint(GpuProgramTextureBindPoint::first()),
    _nextImageBindPoint(GpuProgramImageBindPoint::first())
{
}

bool GpuProgramInterface::compile()
{
    if (_isCompiled)
        return !_declFailed;

    _isCompiled = true;

    PILS_ASSERT(!_declFailed, "Could not compile GPI for ", _program->name());

    if (_declFailed)
    {
        _program.reset();

        return false;
    }

    PILS_INFO("GPI compilation succeeded for ", _program->name());

    return true;
}

GpuProgramConstantBindPoint GpuProgramInterface::getConstantBindPoint(const std::string& name) const
{
    auto it = _constantBindPoints.find(name);
    PILS_ASSERT(it != _constantBindPoints.end(), "Could not find constant buffer named ", name);
    if(it != _constantBindPoints.end())
        return it->second;

    return GpuProgramConstantBindPoint::invalid();
}

GpuProgramStorageBindPoint GpuProgramInterface::getStorageBindPoint(const std::string& name) const
{
    auto it = _storageBindPoints.find(name);
    PILS_ASSERT(it != _storageBindPoints.end(), "Could not find storage buffer named ", name);
    if(it != _storageBindPoints.end())
        return it->second;

    return GpuProgramStorageBindPoint::invalid();
}

GpuProgramTextureBindPoint GpuProgramInterface::getTextureBindPoint(const std::string& name) const
{
    auto it = _textureBindPoints.find(name);
    PILS_ASSERT(it != _textureBindPoints.end(), "Could not find texture named ", name);
    if(it != _textureBindPoints.end())
        return it->second;

    return GpuProgramTextureBindPoint::invalid();
}

GpuProgramImageBindPoint GpuProgramInterface::getImageBindPoint(const std::string& name) const
{
    auto it = _imageBindPoints.find(name);
    PILS_ASSERT(it != _imageBindPoints.end(), "Could not find image named ", name);
    if(it != _imageBindPoints.end())
        return it->second;

    return GpuProgramImageBindPoint::invalid();
}

bool GpuProgramInterface::isValid() const
{
    return _program && _program->isValid() && _isCompiled;
}

}
