#include "gpuprograminterface.h"

#include <PilsCore/Utils/Assert.h>
#include <PilsCore/Utils/Logger.h>

#include "graphic.h"


namespace unisim
{

GpuProgramConstantBindPoint CompiledGpuProgramInterface::getConstantBindPoint(const std::string& name) const
{
    auto it = _constantBindPoints.find(name);
    PILS_ASSERT(it != _constantBindPoints.end(), "Could not find constant buffer named ", name);
    if(it != _constantBindPoints.end())
        return it->second;

    return GpuProgramConstantBindPoint::invalid();
}

GpuProgramStorageBindPoint CompiledGpuProgramInterface::getStorageBindPoint(const std::string& name) const
{
    auto it = _storageBindPoints.find(name);
    PILS_ASSERT(it != _storageBindPoints.end(), "Could not find storage buffer named ", name);
    if(it != _storageBindPoints.end())
        return it->second;

    return GpuProgramStorageBindPoint::invalid();
}

GpuProgramTextureBindPoint CompiledGpuProgramInterface::getTextureBindPoint(const std::string& name) const
{
    auto it = _textureBindPoints.find(name);
    PILS_ASSERT(it != _textureBindPoints.end(), "Could not find texture named ", name);
    if(it != _textureBindPoints.end())
        return it->second;

    return GpuProgramTextureBindPoint::invalid();
}

GpuProgramImageBindPoint CompiledGpuProgramInterface::getImageBindPoint(const std::string& name) const
{
    auto it = _imageBindPoints.find(name);
    PILS_ASSERT(it != _imageBindPoints.end(), "Could not find image named ", name);
    if(it != _imageBindPoints.end())
        return it->second;

    return GpuProgramImageBindPoint::invalid();
}


GpuProgramInterface::GpuProgramInterface()
{
}

bool GpuProgramInterface::declareConstant(const GpuProgramConstantInput& input)
{
    _constants.push_back(input);
    return true;
}

bool GpuProgramInterface::declareStorage(const GpuProgramStorageInput& input)
{
    _storages.push_back(input);
    return true;
}

bool GpuProgramInterface::declareTexture(const GpuProgramTextureInput& input)
{
    _textures.push_back(input);
    return true;
}

bool GpuProgramInterface::declareImage(const GpuProgramImageInput& input)
{
    _images.push_back(input);
    return true;
}

bool GpuProgramInterface::compile(CompiledGpuProgramInterface& compiledGpi, const GraphicProgram& program)
{
    bool ok = true;

    GpuProgramConstantBindPoint nextConstantBindPoint = GpuProgramConstantBindPoint::first();
    for(const auto& input : _constants)
    {
        ok = ok && compiledGpi.set(program, nextConstantBindPoint, input);
        nextConstantBindPoint = GpuProgramConstantBindPoint::next(nextConstantBindPoint);
    }

    GpuProgramStorageBindPoint nextStorageBindPoint = GpuProgramStorageBindPoint::first();
    for(const auto& input : _storages)
    {
        ok = ok && compiledGpi.set(program, nextStorageBindPoint, input);
        nextStorageBindPoint = GpuProgramStorageBindPoint::next(nextStorageBindPoint);
    }

    GpuProgramTextureBindPoint nextTextureBindPoint = GpuProgramTextureBindPoint::first();
    for(const auto& input : _textures)
    {
        ok = ok && compiledGpi.set(program, nextTextureBindPoint, input);
        nextTextureBindPoint = GpuProgramTextureBindPoint::next(nextTextureBindPoint);
    }

    GpuProgramImageBindPoint nextImageBindPoint = GpuProgramImageBindPoint::first();
    for(const auto& input : _images)
    {
        ok = ok && compiledGpi.set(program, nextImageBindPoint, input);
        nextImageBindPoint = GpuProgramImageBindPoint::next(nextImageBindPoint);
    }

    PILS_ASSERT(ok, "GPU Program Interface compilation failed for program ", program.name());

    return ok;
}

}
