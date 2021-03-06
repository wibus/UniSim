
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

include_directories(${OTS_INCLUDE_DIRS})

message( STATUS "UniSim Includes ${GLEW_INCLUDE_DIRS}" )
message( STATUS "UniSim Libraries ${GLEW_LIBRARIES}" )

add_executable(UniSim
    main.cpp
    units.h
    universe.h
    universe.cpp
    gravity.h
    gravity.cpp
    radiation.h
    radiation.cpp
    material.h
    material.cpp
    camera.h
    camera.cpp
    body.h
    body.cpp
    shaders/fullscreen.vert
    shaders/pathtrace.frag)

# Set executable dependency libraries
target_link_libraries(UniSim
    ${OTS_LIBRARIES})

# Add target to run executable 
add_custom_target(run-ex1
    COMMAND UniSim
    DEPENDS UniSim
    WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
)
