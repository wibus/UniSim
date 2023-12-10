cmake_minimum_required(VERSION 3.2)

project(imgui)

include_directories(../glfw/include)

add_library(imgui
    imgui_demo.cpp
    imgui_internal.h
    imstb_rectpack.h
    imconfig.h
    imgui_draw.cpp
    imgui_tables.cpp
    imstb_textedit.h
    imgui.cpp
    imgui.h
    imgui_widgets.cpp
    imstb_truetype.h
    imgui_impl_glfw.cpp
    imgui_impl_glfw.h
    imgui_impl_opengl3.cpp
    imgui_impl_opengl3.h
    imgui_impl_opengl3_loader.h
)
