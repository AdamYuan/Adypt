cmake_minimum_required(VERSION 3.7)
project(Adypt)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_BUILD_PARALLEL_LEVEL 8)

include_directories(dep)
add_definitions(-DSTB_IMAGE_WRITE_IMPLEMENTATION -DGLM_FORCE_SWIZZLE)

find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

file(GLOB SOURCE_FILES src/main.cpp src/BVH/*.* src/Util/*.* src/Tracer/*.*)

add_executable(
		Adypt
		dep/mygl3/texture.cpp
		dep/GL/gl3w.c
		${SOURCE_FILES})

target_link_libraries(Adypt ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS} ${OPENGL_LIBRARIES} glfw)
