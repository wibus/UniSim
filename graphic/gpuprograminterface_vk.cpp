#ifdef UNISIM_GRAPHIC_BACKEND_VK

#include "gpuprograminterface.h"

#include <PilsCore/Utils/Assert.h>

#include "graphic.h"


namespace unisim
{

// Constant //

GpuProgramConstantBindPoint GpuProgramConstantBindPoint::first()
{
    PILS_ASSERT(false, "Not implemented!");
    return {};
}

GpuProgramConstantBindPoint GpuProgramConstantBindPoint::invalid()
{
    PILS_ASSERT(false, "Not implemented!");
    return {};
}

GpuProgramConstantBindPoint GpuProgramConstantBindPoint::next(const GpuProgramConstantBindPoint& current)
{
    PILS_ASSERT(false, "Not implemented!");
    return {};
}


// Storage //

GpuProgramStorageBindPoint GpuProgramStorageBindPoint::first()
{
    PILS_ASSERT(false, "Not implemented!");
    return {};
}

GpuProgramStorageBindPoint GpuProgramStorageBindPoint::invalid()
{
    PILS_ASSERT(false, "Not implemented!");
    return {};
}

GpuProgramStorageBindPoint GpuProgramStorageBindPoint::next(const GpuProgramStorageBindPoint& current)
{
    PILS_ASSERT(false, "Not implemented!");
    return {};
}


// Texture //

GpuProgramTextureBindPoint GpuProgramTextureBindPoint::first()
{
    PILS_ASSERT(false, "Not implemented!");
    return {};
}

GpuProgramTextureBindPoint GpuProgramTextureBindPoint::invalid()
{
    PILS_ASSERT(false, "Not implemented!");
    return {};
}

GpuProgramTextureBindPoint GpuProgramTextureBindPoint::next(const GpuProgramTextureBindPoint& current)
{
    PILS_ASSERT(false, "Not implemented!");
    return {};
}


// Image //

GpuProgramImageBindPoint GpuProgramImageBindPoint::first()
{
    PILS_ASSERT(false, "Not implemented!");
    return {};
}

GpuProgramImageBindPoint GpuProgramImageBindPoint::invalid()
{
    PILS_ASSERT(false, "Not implemented!");
    return {};
}

GpuProgramImageBindPoint GpuProgramImageBindPoint::next(const GpuProgramImageBindPoint& current)
{
    PILS_ASSERT(false, "Not implemented!");
    return {};
}


// Interface //
CompiledGpuProgramInterface::CompiledGpuProgramInterface()
    : _isValid(true)
{}

bool CompiledGpuProgramInterface::set(const GraphicProgram& program, const GpuProgramConstantBindPoint& bindPoint, const GpuProgramConstantInput& input)
{
    PILS_ASSERT(false, "Not implemented!");

    if (!_isValid)
        return false;

    PILS_ASSERT(_constantBindPoints.find(input.name) == _constantBindPoints.end(), "Constant buffer already declared: ", input.name);

    return true;
}

bool CompiledGpuProgramInterface::set(const GraphicProgram& program, const GpuProgramStorageBindPoint& bindPoint, const GpuProgramStorageInput& input)
{
    PILS_ASSERT(false, "Not implemented!");

    if (!_isValid)
        return false;

    PILS_ASSERT(_storageBindPoints.find(input.name) == _storageBindPoints.end(), "Storage buffer already declared: ", input.name);

    return true;
}

bool CompiledGpuProgramInterface::set(const GraphicProgram& program, const GpuProgramTextureBindPoint& bindPoint, const GpuProgramTextureInput& input)
{
    PILS_ASSERT(false, "Not implemented!");

    if (!_isValid)
        return false;

    PILS_ASSERT(_textureBindPoints.find(input.name) == _textureBindPoints.end(), "Texture already declared: ", input.name);

    return true;
}

bool CompiledGpuProgramInterface::set(const GraphicProgram& program, const GpuProgramImageBindPoint& bindPoint, const GpuProgramImageInput& input)
{
    PILS_ASSERT(false, "Not implemented!");

    if (!_isValid)
        return false;

    PILS_ASSERT(_imageBindPoints.find(input.name) == _imageBindPoints.end(), "Image already declared: ", input.name);

    return true;
}

}

#endif // UNISIM_GRAPHIC_BACKEND_VK
