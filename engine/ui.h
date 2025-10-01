#ifndef UI_H
#define UI_H

#include "engine.h"
#include "taskgraph.h"


namespace unisim
{

class Ui : public GraphicTask
{
public:
    Ui();
    
    void render(Context& context) override;
};

}

#endif // UI_H
