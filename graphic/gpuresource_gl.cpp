#include "gpuresource.h"

#include "../resource/texture.h"


namespace unisim
{

// TEXTURE //

GpuTextureResource::GpuTextureResource(ResourceId id, Definition def) :
    GpuResource(id)
{
    _handle.reset(new GpuTextureResourceHandle());

    glGenTextures(1, &_handle->texId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _handle->texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    GLint internalFormat = 0;
    GLenum type = 0;
    switch(def.texture.format)
    {
    case TextureFormat::UNORM8 :
        internalFormat = def.texture.numComponents == 3 ? GL_RGB8 : GL_RGBA8;
        type = GL_UNSIGNED_BYTE;
        break;
    case TextureFormat::Float32:
        internalFormat = GL_RGBA32F;
        type = GL_FLOAT;
        break;
    }

    GLenum format = def.texture.numComponents == 3 ? GL_RGB : GL_RGBA;

    glTexImage2D(
        GL_TEXTURE_2D,
        0, // mip level
        internalFormat,
        def.texture.width,
        def.texture.height,
        0, // border
        format,
        type,
        &def.texture.data.front());

    // For ImGui
    Texture& texNoConst = const_cast<Texture&>(def.texture);
    texNoConst.handle = _handle->texId;

    glBindTexture(GL_TEXTURE_2D, 0);
}

GpuTextureResource::~GpuTextureResource()
{
    glDeleteTextures(1, &_handle->texId);
}


// IMAGE //
GpuImageResource::GpuImageResource(ResourceId id, Definition def) :
    GpuResource(id)
{
    _handle.reset(new GpuImageResourceHandle());
    _handle->format = def.format;

    glGenTextures(1, &_handle->texId);

    glBindTexture(GL_TEXTURE_2D, _handle->texId);
    glTexStorage2D(GL_TEXTURE_2D, 1, def.format, def.width, def.height);
    glBindTexture(GL_TEXTURE_2D, 0);
}

GpuImageResource::~GpuImageResource()
{
    glDeleteTextures(1, &_handle->texId);
}


// BINDLESS //
GpuBindlessResource::GpuBindlessResource(ResourceId id, Definition def) :
    GpuResource(id)
{
    _handle.reset(new GpuBindlessResourceHandle());

    GLenum format = GL_RGBA8;
    if(def.texture != nullptr)
    {
        switch(def.texture->format)
        {
        case TextureFormat::UNORM8:
            format = def.texture->numComponents == 3 ? GL_RGB8 : GL_RGBA8;
            break;
        case TextureFormat::Float32:
            format = def.texture->numComponents == 3 ? GL_RGB32F : GL_RGBA32F;
            break;
        }
    }

    _handle->handle = glGetImageHandleARB(def.textureResource.handle().texId, 0, GL_FALSE, 0, format);
    glMakeImageHandleResidentARB(_handle->handle, GL_READ_ONLY);
}

GpuBindlessResource::~GpuBindlessResource()
{
    glMakeImageHandleNonResidentARB(_handle->handle);
}


// BINDLESS DESCRIPTOR //
GpuBindlessTextureDescriptor::GpuBindlessTextureDescriptor(const unisim::GpuBindlessResourceHandle& bindless) :
    texture(bindless.handle),
    padding(0)
{

}


// STORAGE //
GpuStorageResource::GpuStorageResource(ResourceId id, Definition def) :
    GpuResource(id)
{
    _handle.reset(new GpuStorageResourceHandle());

    glGenBuffers(1, &_handle->bufferId);
    update(def);
}

GpuStorageResource::~GpuStorageResource()
{
    glDeleteBuffers(1, &_handle->bufferId);
}

void GpuStorageResource::update(const Definition& def) const
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _handle->bufferId);
    GLsizei dataSize = def.elemSize * def.elemCount;
    glBufferData(GL_SHADER_STORAGE_BUFFER, dataSize, def.data, GL_STATIC_DRAW);
}


// CONSTANT //
GpuConstantResource::GpuConstantResource(ResourceId id, Definition def) :
    GpuResource(id)
{
    _handle.reset(new GpuConstantResourceHandle());

    glGenBuffers(1, &_handle->bufferId);
    update(def);
}

GpuConstantResource::~GpuConstantResource()
{
    glDeleteBuffers(1, &_handle->bufferId);
}

void GpuConstantResource::update(const Definition& def) const
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _handle->bufferId);
    glBindBuffer(GL_UNIFORM_BUFFER, _handle->bufferId);
    glBufferData(GL_UNIFORM_BUFFER, def.size, def.data, GL_STATIC_DRAW);
}


// VERTEX ARRAY //
GpuGeometryResource::GpuGeometryResource(ResourceId id, Definition def) :
    GpuResource(id)
{
    _handle.reset(new GpuGeometryResourceHandle());

    glGenBuffers(1, &_handle->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _handle->vbo);
    glBufferData(GL_ARRAY_BUFFER, def.vertices.size() * sizeof(decltype(def.vertices[0])), &def.vertices[0], GL_STATIC_DRAW);

    glGenVertexArrays(1, &_handle->vao);
    glBindVertexArray(_handle->vao);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, _handle->vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
}

GpuGeometryResource::~GpuGeometryResource()
{
    glDeleteBuffers(1, &_handle->vao);
    glDeleteBuffers(1, &_handle->vbo);
}

}
