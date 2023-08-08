#ifndef UI_H
#define UI_H

#include "engine.h"
#include "graphic.h"


namespace unisim
{

class Scene;
class Camera;
class ResourceManager;


class UiEngineTask : public EngineTask
{
public:
    UiEngineTask(bool showUi = false);
    virtual ~UiEngineTask();

    void show();
    void hide();
    bool isShown() const { return _showUi; }

    virtual void update(EngineContext& context);

protected:
    bool _showUi;
};


class UiGraphicTask : public GraphicTask
{
public:
    UiGraphicTask();

    void render(GraphicContext& context) override;
};

}

#endif // UI_H
