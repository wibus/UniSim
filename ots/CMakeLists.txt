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


## OTS ##
set(OTS_INCLUDE_DIRS
    ots
    ${GLEW_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIRS}
    ${LIBJPEG_INCLUDE_DIRS}
    PARENT_SCOPE
)

set(OTS_LIBRARIES
    ${OPENGL_LIBRARIES} 
    ${GLEW_LIBRARIES}
    ${GLFW_LIBRARIES}
    ${GLM_LIBRARIES}
    ${LIBJPEG_LIBRARIES}
    PARENT_SCOPE
)
