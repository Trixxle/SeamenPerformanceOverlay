cmake_minimum_required(VERSION 3.26)

project(PerformanceVR)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin")

# Configuring OpenVR location
set(OpenVR_ROOT ${CMAKE_SOURCE_DIR}/thirdparty/OpenVR)
find_package(OpenVR REQUIRED)

add_executable(performance_overlay
        src/main.cpp
)

target_include_directories(performance_overlay PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(performance_overlay PUBLIC cxx_std_23)

target_link_libraries(performance_overlay
        PRIVATE
        OpenVR::API
)

target_compile_options(performance_overlay PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic -Werror>
)
