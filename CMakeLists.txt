
cmake_minimum_required(VERSION 3.9)

# Proeject name - should not contain whitespace 
project(UniSim_CPP)

#========== Global Configurations =============#
#----------------------------------------------#

# Set the C++ standard for all targets (It sets the flags
# (-std=c++11, -std=c++14 ...) on Clang or GCC. and /std=c++17 on MSVC
# OPTIONAL:
#---------------------------------
set(CMAKE_CXX_STANDARD 17)     
set(CMAKE_VERBOSE_MAKEFILE ON)

#========== Targets Configurations ============#

# Build an executable (Unix-like OSes generates ./openglDemo1, on
# Windows ./opengDemo1.exe)
# ..........................................

find_package( OpenGL REQUIRED )

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(ots)
add_subdirectory(textures)
add_subdirectory(PilsCore/PilsCore)

set(UNISIM_INCLUDE_DIRS
    ${OTS_INCLUDE_DIRS}
    ${PILS_CORE_INCLUDE_DIRS})
message( STATUS "UniSim Includes ${UNISIM_INCLUDE_DIRS}" )

set(UNISIM_LIBRARIES
    ${GLEW_LIBRARIES}
    ${OTS_LIBRARIES}
    ${PILS_CORE_LIBRARIES}
    PilsCore)
message( STATUS "UniSim Libraries ${UNISIM_LIBRARIES}" )


include_directories(${UNISIM_INCLUDE_DIRS})

# System files
set(SystemFiles
    system/input.h
    system/input.cpp
    system/profiler.h
    system/profiler.cpp
    system/random.h
    system/random.cpp
    system/units.h
)

# Resource files
set(ResourceFiles
    resource/body.h
    resource/body.cpp
    resource/material.h
    resource/material.cpp
    resource/primitive.h
    resource/primitive.cpp
    resource/texture.h
    resource/texture.cpp
)

# Graphic files
set(GraphicFiles
    graphic/gpuresource.h
    graphic/gpuresource.cpp
    graphic/graphic.h
    graphic/graphic.cpp
    graphic/graphic_gl.h
    graphic/graphic_gl.cpp
    graphic/graphic_vk.h
    graphic/graphic_vk.cpp
    graphic/pathtracer.h
    graphic/pathtracer.cpp
)

# Engine files
set(EngineFiles
    engine/bvh.h
    engine/bvh.cpp
    engine/enginetask.h
    engine/enginetask.cpp
    engine/grading.h
    engine/grading.cpp
    engine/graphictask.h
    engine/graphictask.cpp
    engine/gravity.h
    engine/gravity.cpp
    engine/light.h
    engine/light.cpp
    engine/materialdatabase.h
    engine/materialdatabase.cpp
    engine/camera.h
    engine/camera.cpp
    engine/instance.h
    engine/instance.cpp
    engine/scene.h
    engine/scene.cpp
    engine/project.h
    engine/project.cpp
    engine/sky.h
    engine/sky.cpp
    engine/terrain.h
    engine/terrain.cpp
    engine/tracer.h
    engine/tracer.cpp
    engine/ui.h
    engine/ui.cpp
)

# Shaders
set(ShaderFiles
    shaders/fullscreen.vert
    shaders/pathtrace.glsl
    shaders/colorgrade.frag
    shaders/physicalsky.glsl
    shaders/sphericalsky.glsl
    shaders/moonlight.glsl
    shaders/common/constants.glsl
    shaders/common/data.glsl
    shaders/common/inputs.glsl
    shaders/common/signatures.glsl
    shaders/common/utils.glsl
    shaders/common/intersection.glsl
)

## Projects ##

# Solar System
set(SolarSystemFiles
    projects/solar/solarsystemproject.h
    projects/solar/solarsystemproject.cpp
    projects/solar/solarsystemscene.h
    projects/solar/solarsystemscene.cpp
    projects/solar/solarsystemcamera.h
    projects/solar/solarsystemcamera.cpp)

# Path Tracer
set(PathTracerSystemFiles
    projects/pathtracer/pathtracerproject.h
    projects/pathtracer/pathtracerproject.cpp
    projects/pathtracer/pathtracerscene.h
    projects/pathtracer/pathtracerscene.cpp
    projects/pathtracer/pathtracercamera.h
    projects/pathtracer/pathtracercamera.cpp)

# tests
set(PilsCoreTest
    PilsCore/test/gpu_tests.cpp)

add_executable(UniSim
    main.cpp
    universe.h
    universe.cpp
    ${SystemFiles}
    ${ResourceFiles}
    ${GraphicFiles}
    ${EngineFiles}
    ${ShaderFiles}
    ${SolarSystemFiles}
    ${PathTracerSystemFiles}
    ${PilsCoreTest})

# Set executable dependency libraries
target_link_libraries(UniSim ${UNISIM_LIBRARIES})
target_compile_definitions(UniSim PUBLIC UNISIM_GRAPHIC_BACKEND_GL)
#target_compile_definitions(UniSim PUBLIC UNISIM_GRAPHIC_BACKEND_VK)

# Add target to run executable 
add_custom_target(run-ex1
    COMMAND UniSim
    DEPENDS UniSim
    WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
)
