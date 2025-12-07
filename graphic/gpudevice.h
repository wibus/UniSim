#ifndef GPUDEVICE_H
#define GPUDEVICE_H

#ifdef UNISIM_GRAPHIC_BACKEND_GL
#include "gpudevice_gl.h"
#endif // UNISIM_GRAPHIC_BACKEND_GL

#ifdef UNISIM_GRAPHIC_BACKEND_VK
#include "gpudevice_vk.h"
#endif // UNISIM_GRAPHIC_BACKEND_VK


namespace unisim
{

}

#endif // GPUDEVICE_H
