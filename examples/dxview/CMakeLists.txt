cmake_minimum_required(VERSION 3.5)
project(dxview)

find_package(glfw3 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

add_executable(dxview
    src/Viewer.h
    src/Viewer.cc
    src/dxview.cc
)

target_include_directories(dxview
PRIVATE
    ../../
)

target_compile_definitions(dxview
PRIVATE
    DXVIEW_SWAP_CHAIN_BUFFER_COUNT=3
    DXVIEW_RES_DIR=L"${PROJECT_SOURCE_DIR}/res"
)

target_link_libraries(dxview
PRIVATE
    dxgi
    d3dcompiler
    d3d12
    glfw
    spdlog::spdlog
)

set_target_properties(dxview
PROPERTIES
    CXX_STANDARD 17
)