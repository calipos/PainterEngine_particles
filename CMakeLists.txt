cmake_minimum_required(VERSION 3.10)
project(PainterEngine)

set(CMAKE_BUILD_TYPE Relaese)
#SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC  ") 
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11") 
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -w  ") 
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp  ") 
 
message("hello")

include_directories(../..)
 

aux_source_directory(../../core painterengine_build_core)
add_library (core ${painterengine_build_core})
#FILE (GLOB core_head  CONFIGURE_DEPENDS  ../../core/*.h)
#target_sources(core PRIVATE ${core_head})
#message(${core_head})
 
aux_source_directory(../../kernel painterengine_build_kernel)
add_library (kernel ${painterengine_build_kernel})

aux_source_directory(../../architecture painterengine_build_architecture)
add_library (architecture ${painterengine_build_architecture})

aux_source_directory(../../project app_src)
add_library(app ${app_src})

aux_source_directory(../../platform/windows painterengine_build_platform)
add_executable (platform ${painterengine_build_platform})
target_include_directories(platform PRIVATE ../../platform/windows)
target_include_directories(platform PRIVATE ../../project)
#/SUBSYSTEM:WINDOWS
target_link_libraries(platform core) 
target_link_libraries(platform kernel) 
target_link_libraries(platform architecture) 
target_link_libraries(platform app)   
target_link_options(platform PRIVATE /SUBSYSTEM:WINDOWS)