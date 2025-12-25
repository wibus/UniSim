#include "gpuprograminterface.h"

#include <PilsCore/Utils/Assert.h>

#include "graphic.h"


namespace unisim
{

// Constant //

GpuProgramConstantBindPoint GpuProgramConstantBindPoint::first()
{
    return {0};
}

GpuProgramConstantBindPoint GpuProgramConstantBindPoint::invalid()
{
    return {GL_INVALID_INDEX};
}

GpuProgramConstantBindPoint GpuProgramConstantBindPoint::next(const GpuProgramConstantBindPoint& current)
{
    return {current.bindPoint + 1};
}


// Storage //

GpuProgramStorageBindPoint GpuProgramStorageBindPoint::first()
{
    return {0};
}

GpuProgramStorageBindPoint GpuProgramStorageBindPoint::invalid()
{
    return {GL_INVALID_INDEX};
}

GpuProgramStorageBindPoint GpuProgramStorageBindPoint::next(const GpuProgramStorageBindPoint& current)
{
    return {current.bindPoint + 1};
}


// Texture //

GpuProgramTextureBindPoint GpuProgramTextureBindPoint::first()
{
    return {0};
}

GpuProgramTextureBindPoint GpuProgramTextureBindPoint::invalid()
{
    return {GL_INVALID_INDEX};
}

GpuProgramTextureBindPoint GpuProgramTextureBindPoint::next(const GpuProgramTextureBindPoint& current)
{
    return {current.bindPoint + 1};
}


// Image //

GpuProgramImageBindPoint GpuProgramImageBindPoint::first()
{
    return {0};
}

GpuProgramImageBindPoint GpuProgramImageBindPoint::invalid()
{
    return {GL_INVALID_INDEX};
}

GpuProgramImageBindPoint GpuProgramImageBindPoint::next(const GpuProgramImageBindPoint& current)
{
    return {current.bindPoint + 1};
}


// Interface //
CompiledGpuProgramInterface::CompiledGpuProgramInterface()
    : _isValid(true)
{}

bool CompiledGpuProgramInterface::set(const GraphicProgram& program, const GpuProgramConstantBindPoint& bindPoint, const GpuProgramConstantInput& input)
{
    if (!_isValid)
        return false;

    PILS_ASSERT(_constantBindPoints.find(input.name) == _constantBindPoints.end(), "Constant buffer already declared: ", input.name);

    GLuint location = glGetProgramResourceIndex(program.handle(), GL_UNIFORM_BLOCK, input.name.c_str());

    if(location == GL_INVALID_ENUM)
    {
        PILS_ERROR("Invalid constant interface type\n");
        _isValid = false;
        return false;
    }

    if(location == GL_INVALID_INDEX)
    {
        PILS_ERROR("Could not find constant block named ", input.name);
        _isValid = false;
        return false;
    }

    glUniformBlockBinding(program.handle(), location, bindPoint.bindPoint);
    _constantBindPoints[input.name] = bindPoint;

    return true;
}

bool CompiledGpuProgramInterface::set(const GraphicProgram& program, const GpuProgramStorageBindPoint& bindPoint, const GpuProgramStorageInput& input)
{
    if (!_isValid)
        return false;

    PILS_ASSERT(_storageBindPoints.find(input.name) == _storageBindPoints.end(), "Storage buffer already declared: ", input.name);

    GLuint location = glGetProgramResourceIndex(program.handle(), GL_SHADER_STORAGE_BLOCK, input.name.c_str());

    if(location == GL_INVALID_ENUM)
    {
        PILS_ERROR("Invalid storage interface type\n");
        _isValid = false;
        return false;
    }

    if(location == GL_INVALID_INDEX)
    {
        PILS_ERROR("Could not find storage block named ", input.name);
        _isValid = false;
        return false;
    }

    glShaderStorageBlockBinding(program.handle(), location, bindPoint.bindPoint);
    _storageBindPoints[input.name] = bindPoint;

    return true;
}

bool CompiledGpuProgramInterface::set(const GraphicProgram& program, const GpuProgramTextureBindPoint& bindPoint, const GpuProgramTextureInput& input)
{
    if (!_isValid)
        return false;

    PILS_ASSERT(_textureBindPoints.find(input.name) == _textureBindPoints.end(), "Texture already declared: ", input.name);

    GLuint location = glGetUniformLocation(program.handle(), input.name.c_str());

    if(location == GL_INVALID_ENUM)
    {
        PILS_ERROR("Invalid enum while trying to read uniform location\n");
        _isValid = false;
        return false;
    }

    if(location == GL_INVALID_INDEX)
    {
        PILS_ERROR("Could not find texture named ", input.name);
        _isValid = false;
        return false;
    }

    glProgramUniform1i(program.handle(), location, bindPoint.bindPoint);
    _textureBindPoints[input.name] = bindPoint;

    return true;
}

bool CompiledGpuProgramInterface::set(const GraphicProgram& program, const GpuProgramImageBindPoint& bindPoint, const GpuProgramImageInput& input)
{
    if (!_isValid)
        return false;

    PILS_ASSERT(_imageBindPoints.find(input.name) == _imageBindPoints.end(), "Image already declared: ", input.name);

    GLuint location = glGetUniformLocation(program.handle(), input.name.c_str());

    if(location == GL_INVALID_ENUM)
    {
        PILS_ERROR("Invalid enum while trying to read uniform location\n");
        _isValid = true;
        return false;
    }

    if(location == GL_INVALID_INDEX)
    {
        PILS_ERROR("Could not find image named ", input.name);
        _isValid = true;
        return false;
    }

    glProgramUniform1i(program.handle(), location, bindPoint.bindPoint);
    _imageBindPoints[input.name] = bindPoint;

    return true;
}

}
