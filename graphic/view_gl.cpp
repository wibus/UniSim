#include "view.h"

#include "graphic_gl.h"


namespace unisim
{

void View::setViewportNative() const
{
    glViewport(0, 0, _viewport.width, _viewport.height);
}

}
