#ifdef UNISIM_GRAPHIC_BACKEND_GL

#include "view.h"

#include "graphic.h"


namespace unisim
{

void View::resetViewportNative()
{

}

void View::resizeViewportNative()
{

}

void View::setViewportNative() const
{
    glViewport(0, 0, _viewport.width, _viewport.height);
}

}

#endif // UNISIM_GRAPHIC_BACKEND_GL