
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

# Engine files
set(EngineFile
    units.h
    universe.h
    universe.cpp
    input.h
    input.cpp
    grading.h
    grading.cpp
    gravity.h
    gravity.cpp
    engine.h
    engine.cpp
    graphic.h
    graphic.cpp
    light.h
    light.cpp
    material.h
    material.cpp
    camera.h
    camera.cpp
    instance.h
    instance.cpp
    body.h
    body.cpp
    primitive.h
    primitive.cpp
    bvh.h
    bvh.cpp
    scene.h
    scene.cpp
    profiler.h
    profiler.cpp
    project.h
    project.cpp
    random.h
    random.cpp
    resource.h
    resource.cpp
    sky.h
    sky.cpp
    terrain.h
    terrain.cpp
    tracer.h
    tracer.cpp
    ui.h
    ui.cpp
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
    solar/solarsystemproject.h
    solar/solarsystemproject.cpp
    solar/solarsystemscene.h
    solar/solarsystemscene.cpp
    solar/solarsystemcamera.h
    solar/solarsystemcamera.cpp)

# Path Tracer
set(PathTracerSystemFiles
    pathtracer/pathtracerproject.h
    pathtracer/pathtracerproject.cpp
    pathtracer/pathtracerscene.h
    pathtracer/pathtracerscene.cpp
    pathtracer/pathtracercamera.h
    pathtracer/pathtracercamera.cpp)

# tests
set(PilsCoreTest
    PilsCore/test/gpu_tests.cpp)

add_executable(UniSim
    main.cpp
    ${EngineFile}
    ${ShaderFiles}
    ${SolarSystemFiles}
    ${PathTracerSystemFiles}
    ${PilsCoreTest})

# Set executable dependency libraries
target_link_libraries(UniSim ${UNISIM_LIBRARIES})

# Add target to run executable 
add_custom_target(run-ex1
    COMMAND UniSim
    DEPENDS UniSim
    WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
)
