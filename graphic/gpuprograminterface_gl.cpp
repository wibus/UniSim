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

GpuProgramTextureUnit GpuProgramTextureUnit::first()
{
    return {0};
}

GpuProgramTextureUnit GpuProgramTextureUnit::invalid()
{
    return {GL_INVALID_INDEX};
}

GpuProgramTextureUnit GpuProgramTextureUnit::next(const GpuProgramTextureUnit& current)
{
    return {current.unit + 1};
}


// Image //

GpuProgramImageUnit GpuProgramImageUnit::first()
{
    return {0};
}

GpuProgramImageUnit GpuProgramImageUnit::invalid()
{
    return {GL_INVALID_INDEX};
}

GpuProgramImageUnit GpuProgramImageUnit::next(const GpuProgramImageUnit& current)
{
    return {current.unit + 1};
}


// Interface //

bool GpuProgramInterface::declareConstant(const std::string& blockName)
{
    if (_declFailed)
        return false;

    PILS_ASSERT(_uboBindPoints.find(blockName) == _uboBindPoints.end(), "Constant buffer already declared: ", blockName);

    GLuint location = glGetProgramResourceIndex(_program->handle(), GL_UNIFORM_BLOCK, blockName.c_str());

    if(location == GL_INVALID_ENUM)
    {
        PILS_ERROR("Invalid constant interface type\n");
        _declFailed = true;
        return false;
    }

    if(location == GL_INVALID_INDEX)
    {
        PILS_ERROR("Could not find constant block named ", blockName);
        _declFailed = true;
        return false;
    }

    glUniformBlockBinding(_program->handle(), location, _nextUboBindPoint.bindPoint);
    _uboBindPoints[blockName] = _nextUboBindPoint;
    _nextUboBindPoint = GpuProgramConstantBindPoint::next(_nextUboBindPoint);

    return true;
}

bool GpuProgramInterface::declareStorage(const std::string& blockName)
{
    if (_declFailed)
        return false;

    PILS_ASSERT(_ssboBindPoints.find(blockName) == _ssboBindPoints.end(), "Storage buffer already declared: ", blockName);

    GLuint location = glGetProgramResourceIndex(_program->handle(), GL_SHADER_STORAGE_BLOCK, blockName.c_str());

    if(location == GL_INVALID_ENUM)
    {
        PILS_ERROR("Invalid storage interface type\n");
        _declFailed = true;
        return false;
    }

    if(location == GL_INVALID_INDEX)
    {
        PILS_ERROR("Could not find storage block named ", blockName);
        _declFailed = true;
        return false;
    }

    glShaderStorageBlockBinding(_program->handle(), location, _nextSsboBindPoint.bindPoint);
    _ssboBindPoints[blockName] = _nextSsboBindPoint;
    _nextSsboBindPoint = GpuProgramStorageBindPoint::next(_nextSsboBindPoint);

    return true;
}

}
