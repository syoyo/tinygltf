
include_directories(${CMAKE_SOURCE_DIR})
add_executable(create_triangle_gltf create_triangle_gltf.cpp)
target_compile_options(create_triangle_gltf PUBLIC -Wall)
target_link_libraries(create_triangle_gltf )