cmake_minimum_required(VERSION 3.7)
project(Adypt)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_BUILD_PARALLEL_LEVEL 8)

include_directories(dep)
add_definitions(-DGLM_FORCE_SWIZZLE)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

file(GLOB SOURCE_FILES src/*.* src/BVH/*.* src/Util/*.* src/Tracer/*.*
		dep/imgui/*.* dep/GL/gl3w.c)

add_executable(
		Adypt
		${SOURCE_FILES})

target_link_libraries(Adypt ${CMAKE_DL_LIBS} ${OPENGL_opengl_LIBRARY} glfw)
