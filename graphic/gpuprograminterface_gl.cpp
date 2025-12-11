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

bool GpuProgramInterface::declareConstant(const std::string& name)
{
    if (_declFailed)
        return false;

    PILS_ASSERT(_constantBindPoints.find(name) == _constantBindPoints.end(), "Constant buffer already declared: ", name);

    GLuint location = glGetProgramResourceIndex(_program->handle(), GL_UNIFORM_BLOCK, name.c_str());

    if(location == GL_INVALID_ENUM)
    {
        PILS_ERROR("Invalid constant interface type\n");
        _declFailed = true;
        return false;
    }

    if(location == GL_INVALID_INDEX)
    {
        PILS_ERROR("Could not find constant block named ", name);
        _declFailed = true;
        return false;
    }

    glUniformBlockBinding(_program->handle(), location, _nextConstantBindPoint.bindPoint);
    _constantBindPoints[name] = _nextConstantBindPoint;
    _nextConstantBindPoint = GpuProgramConstantBindPoint::next(_nextConstantBindPoint);

    return true;
}

bool GpuProgramInterface::declareStorage(const std::string& name)
{
    if (_declFailed)
        return false;

    PILS_ASSERT(_storageBindPoints.find(name) == _storageBindPoints.end(), "Storage buffer already declared: ", name);

    GLuint location = glGetProgramResourceIndex(_program->handle(), GL_SHADER_STORAGE_BLOCK, name.c_str());

    if(location == GL_INVALID_ENUM)
    {
        PILS_ERROR("Invalid storage interface type\n");
        _declFailed = true;
        return false;
    }

    if(location == GL_INVALID_INDEX)
    {
        PILS_ERROR("Could not find storage block named ", name);
        _declFailed = true;
        return false;
    }

    glShaderStorageBlockBinding(_program->handle(), location, _nextStorageBindPoint.bindPoint);
    _storageBindPoints[name] = _nextStorageBindPoint;
    _nextStorageBindPoint = GpuProgramStorageBindPoint::next(_nextStorageBindPoint);

    return true;
}

bool GpuProgramInterface::declareTexture(const std::string& name)
{
    if (_declFailed)
        return false;

    PILS_ASSERT(_textureBindPoints.find(name) == _textureBindPoints.end(), "Texture already declared: ", name);

    GLuint location = glGetUniformLocation(_program->handle(), name.c_str());

    if(location == GL_INVALID_ENUM)
    {
        PILS_ERROR("Invalid enum while trying to read uniform location\n");
        _declFailed = true;
        return false;
    }

    if(location == GL_INVALID_INDEX)
    {
        PILS_ERROR("Could not find texture named ", name);
        _declFailed = true;
        return false;
    }

    glProgramUniform1i(_program->handle(), location, _nextTextureBindPoint.bindPoint);
    _textureBindPoints[name] = _nextTextureBindPoint;
    _nextTextureBindPoint = GpuProgramTextureBindPoint::next(_nextTextureBindPoint);

    return true;
}

bool GpuProgramInterface::declareImage(const std::string& name)
{
    if (_declFailed)
        return false;

    PILS_ASSERT(_imageBindPoints.find(name) == _imageBindPoints.end(), "Image already declared: ", name);

    GLuint location = glGetUniformLocation(_program->handle(), name.c_str());

    if(location == GL_INVALID_ENUM)
    {
        PILS_ERROR("Invalid enum while trying to read uniform location\n");
        _declFailed = true;
        return false;
    }

    if(location == GL_INVALID_INDEX)
    {
        PILS_ERROR("Could not find image named ", name);
        _declFailed = true;
        return false;
    }

    glProgramUniform1i(_program->handle(), location, _nextImageBindPoint.bindPoint);
    _imageBindPoints[name] = _nextImageBindPoint;
    _nextImageBindPoint = GpuProgramImageBindPoint::next(_nextImageBindPoint);

    return true;
}

}
