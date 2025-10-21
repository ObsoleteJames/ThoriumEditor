cmake_minimum_required(VERSION 3.20.0)
project(TH_BUILDTOOL)

include_directories(src F:/MyProjects/VS_Projects/ThoriumEngine/src/Util/include)

add_subdirectory(F:/MyProjects/VS_Projects/ThoriumEngine/src/Util build)

add_executable(BuildTool src/main.cpp src/Build.cpp)
set_property(TARGET BuildTool PROPERTY CXX_STANDARD 17)

target_link_libraries(BuildTool Util)
