cmake_minimum_required(VERSION 3.14)
project(TaskDecomposer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(TaskDecomposer
    main.cpp
    RequirementParser.cpp
    TaskDecompositionEngine.cpp
    RequirementParser.hpp
    TaskDecompositionEngine.hpp
)

# 如果使用 MSVC，确保正确处理 UTF-8
if(MSVC)
    target_compile_options(TaskDecomposer PRIVATE /utf-8)
endif()

# 安装
install(TARGETS TaskDecomposer DESTINATION bin)
