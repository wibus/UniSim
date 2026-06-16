#ifndef GRAPHIC_VK_H
#define GRAPHIC_VK_H


namespace unisim
{

class GraphicShaderHandle
{
protected:
    GraphicShaderHandle(const GraphicShaderHandle&) = delete;

public:
    GraphicShaderHandle(GraphicShaderHandle&& other);
    ~GraphicShaderHandle();

private:
};

class GraphicProgramHandle
{
protected:
    GraphicProgramHandle(const GraphicProgramHandle&) = delete;

public:
    GraphicProgramHandle(GraphicProgramHandle&& other);
    ~GraphicProgramHandle();

private:
};

}

#endif // GRAPHIC_VK_H
