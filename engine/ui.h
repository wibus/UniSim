#ifndef UI_H
#define UI_H

#include "taskgraph/graphictask.h"


namespace unisim
{

class Ui : public GraphicTask
{
public:
    Ui();
    
    void render(GraphicContext& context) override;
};

}

#endif // UI_H
