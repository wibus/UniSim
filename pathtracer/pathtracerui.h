#ifndef PATH_TRACER_UI_H
#define PATH_TRACER_UI_H

#include "../ui.h"


namespace unisim
{

class PathTracerUi : public Ui
{
public:
    PathTracerUi();
    virtual ~PathTracerUi();

    void render(Scene& scene) override;

private:
};

}

#endif // PATH_TRACER_UI_H
