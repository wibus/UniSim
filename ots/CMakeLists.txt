# GLEW
set(GLEW_INCLUDE_DIRS ots/glew/include)
set(GLEW_LIBRARIES glew)

file(GLOB GLEW_SRC
     "glew/src/*.c"
)

include_directories(glew/include)
add_library(glew
    ${GLEW_SRC}
)


# GLFW
add_subdirectory(glfw)
set(GLFW_LIBRARIES glfw)


# GLM
set(GLM_INCLUDE_DIRS)
set(GLM_LIBRARIES)


#libjpeg
add_subdirectory(libjpeg)
set(LIBJPEG_INCLUDE_DIRS ots/libjpeg)
set(LIBJPEG_LIBRARIES libjpeg)

#libpng
set(PNG_SHARED ON CACHE BOOL "" FORCE)
set(PNG_STATIC OFF CACHE BOOL "" FORCE)
set(PNG_EXECUTABLES OFF CACHE BOOL "" FORCE) # we only want lib
set(PNG_TESTS OFF CACHE BOOL "" FORCE) # we only want lib
set(SKIP_INSTALL_ALL OFF CACHE BOOL "" FORCE) # we only want lib
add_subdirectory(libpng)
set(LIBPNG_INCLUDE_DIRS ots/libpng)
set(LIBPNG_LIBRARIES png)

## OTS ##
set(OTS_INCLUDE_DIRS
    ots
    ${GLEW_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIRS}
    ${LIBJPEG_INCLUDE_DIRS}
    ${LIBPNG_INCLUDE_DIRS}
    PARENT_SCOPE
)

set(OTS_LIBRARIES
    ${OPENGL_LIBRARIES} 
    ${GLEW_LIBRARIES}
    ${GLFW_LIBRARIES}
    ${GLM_LIBRARIES}
    ${LIBJPEG_LIBRARIES}
    ${LIBPNG_LIBRARIES}
    PARENT_SCOPE
)
