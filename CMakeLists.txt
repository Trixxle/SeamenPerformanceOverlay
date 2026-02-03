cmake_minimum_required(VERSION 4.0)

project(PerformanceVR)

# Set CMake to run moc automatically has needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
# Set CMake to handle *.ui generation to avoid using QT6_WRAP_UI() macro.
set(CMAKE_AUTOUIC ON)
# Add the build dir to include paths to find UI's headers.
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Where CMake searches for .ui files
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_SOURCE_DIR})

set(TARGET_NAME PerformanceVR)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules/")
set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin")

add_executable(PerformanceVR
        src/main.cpp
        src/userinterface/dashboardui.cpp
        src/userinterface/dashboardui.h
        src/userinterface/dashboardui.ui
)

# IMPORTANT: Change this path to where you have your Qt libraries installed
# Configuring Qt6 path
set(CMAKE_PREFIX_PATH "C:/Qt/6.10.1/mingw_64/")

#foreach(module Core Gui Widgets)
#    find_package(Qt6 REQUIRED COMPONENTS ${module})
#    list(APPEND QT_INCLUDE_DIRS "${Qt6${module}_INCLUDE_DIRS}")
#    list(APPEND QT_LIBRARIES    "${Qt6${module}_LIBRARIES}")
#    list(APPEND QT_FLAGS        "${Qt6${module}_EXECUTABLE_COMPILE_FLAGS}")
#    list(APPEND QT_DEFINITIONS  "${Qt6${module}_DEFINITIONS}")
#endforeach()
# Correct QT_FLAGS having semicolons, which break the compile step on POSIX
#list(REMOVE_DUPLICATES QT_FLAGS)

find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        REQUIRED)

set_target_properties(PerformanceVR PROPERTIES
        WIN32_EXECUTABLE TRUE
        MACOSX_BUNDLE TRUE
        COMPILE_FLAGS "${QT_FLAGS}"
)

# Configuring OpenVR location
set(OpenVR_ROOT ${CMAKE_SOURCE_DIR}/thirdparty/OpenVR)
find_package(OpenVR REQUIRED)

target_include_directories(PerformanceVR PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(PerformanceVR PUBLIC cxx_std_23)

target_link_libraries(PerformanceVR
        PRIVATE
        Qt::Core
        Qt::Gui
        Qt::Widgets
        OpenVR::API
)

# Make all warnings errors etc.
target_compile_options(PerformanceVR PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic -Werror>
)

# Find Qt6 platforms specific DLLs to build with
if (WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(DEBUG_SUFFIX)
    if (MSVC AND CMAKE_BUILD_TYPE MATCHES "Debug")
        set(DEBUG_SUFFIX "d")
    endif ()
    set(QT_INSTALL_PATH "${CMAKE_PREFIX_PATH}")
    if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
        set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
            set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        endif ()
    endif ()
    if (EXISTS "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
    endif ()
    foreach (QT_LIB Core Gui Widgets)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/bin/Qt6${QT_LIB}${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    endforeach (QT_LIB)
endif ()

add_custom_command(
        TARGET PerformanceVR POST_BUILD
        COMMENT "Copying Executable and DLLs"
        COMMAND ${CMAKE_COMMAND} -E copy -t ${OUTPUT_DIR} $<TARGET_FILE:PerformanceVR>
        COMMAND ${CMAKE_COMMAND} -E copy -t ${OUTPUT_DIR} $<TARGET_RUNTIME_DLLS:PerformanceVR>
        COMMAND_EXPAND_LISTS
)
