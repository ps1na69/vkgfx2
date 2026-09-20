cmake_minimum_required(VERSION 3.31)

project(vkgfx2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


add_subdirectory(vendor/SDL)
add_subdirectory(engine)
add_subdirectory(sandbox)