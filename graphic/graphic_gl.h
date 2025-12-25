#ifndef GRAPHIC_GL_H
#define GRAPHIC_GL_H

#include <GL/glew.h>


namespace unisim
{

class GraphicShaderHandle
{
protected:
    GraphicShaderHandle(const GraphicShaderHandle&) = delete;

public:
    GraphicShaderHandle(GraphicShaderHandle&& other);
    GraphicShaderHandle(GLuint shaderId, bool releaseOnDestroy = true);
    ~GraphicShaderHandle();

    operator GLuint() const { return _shaderId; }

private:
    GLuint _shaderId;
    bool _releaseOnDestroy;
};

class GraphicProgramHandle
{
protected:
    GraphicProgramHandle(const GraphicProgramHandle&) = delete;

public:
    GraphicProgramHandle(GraphicProgramHandle&& other);
    GraphicProgramHandle(GLuint programId);
    ~GraphicProgramHandle();

    operator GLuint() const { return _programId; }

private:
    GLuint _programId;
};

}


#endif // GRAPHIC_GL_H
