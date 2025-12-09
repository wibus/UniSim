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
    _nextUboBindPoint(GpuProgramConstantBindPoint::first()),
    _nextSsboBindPoint(GpuProgramStorageBindPoint::first()),
    _nextTextureUnit(GpuProgramTextureUnit::first())
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

GpuProgramConstantBindPoint GpuProgramInterface::getConstantBindPoint(const std::string& blockName) const
{
    auto it = _uboBindPoints.find(blockName);
    PILS_ASSERT(it != _uboBindPoints.end(), "Could not find constant buffer named ", blockName);
    if(it != _uboBindPoints.end())
        return it->second;

    return GpuProgramConstantBindPoint::invalid();
}

GpuProgramStorageBindPoint GpuProgramInterface::getStorageBindPoint(const std::string& blockName) const
{
    auto it = _ssboBindPoints.find(blockName);
    PILS_ASSERT(it != _ssboBindPoints.end(), "Could not find storage buffer named ", blockName);
    if(it != _ssboBindPoints.end())
        return it->second;

    return GpuProgramStorageBindPoint::invalid();
}

GpuProgramTextureUnit GpuProgramInterface::grabTextureUnit()
{
    GpuProgramTextureUnit texUnit = _nextTextureUnit;
    _nextTextureUnit = GpuProgramTextureUnit::next(_nextTextureUnit);
    return texUnit;
}

void GpuProgramInterface::resetTextureUnits()
{
    _nextTextureUnit = GpuProgramTextureUnit::first();
}

bool GpuProgramInterface::isValid() const
{
    return _program && _program->isValid() && _isCompiled;
}

}
