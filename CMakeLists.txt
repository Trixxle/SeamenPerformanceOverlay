cmake_minimum_required(VERSION 4.0)

project(SeamenPerformanceOverlay)
set(TARGET_NAME SeamenPerformanceOverlay)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules/")
set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin")

# Set CMake to run moc automatically as needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
# Set CMake to handle *.ui generation to avoid using QT6_WRAP_UI() macro.
set(CMAKE_AUTOUIC ON)
# Add the build dir to include paths to find UI's headers.
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Where CMake searches for .ui files
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_SOURCE_DIR})

add_executable(SeamenPerformanceOverlay
        resources-nonbin/SeamenPerformanceOverlay.rc
        src/main.cpp
        src/userinterface/dashboardui.cpp
        src/userinterface/dashboardui.h
        src/userinterface/dashboardui.ui
        src/steamvrcontrol/steamvrlogic.cpp
        src/steamvrcontrol/steamvrlogic.h
        src/frameHandler.cpp
        src/frameHandler.h
        src/userinterface/panicdashboard.cpp
        src/userinterface/panicdashboard.h
        src/userinterface/panicdashboard.ui
        src/systemResourcesHandler.cpp
        src/systemResourcesHandler.h
        src/systemResourcesHandler.h
        resources-nonbin/customIcons.qrc
        src/userSettings.cpp
        src/userSettings.h
)

# IMPORTANT: Change this path to where you have your Qt libraries installed
# Configuring Qt6 path
set(CMAKE_PREFIX_PATH "C:/Qt/6.11.0/mingw_64/")

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
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/"
                COMMAND ${CMAKE_COMMAND} -E make_directory
                "${OUTPUT_DIR}/plugins/platforms/")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/"
                "${OUTPUT_DIR}/plugins/platforms/")
    endif ()
    foreach (QT_LIB Core Gui Widgets OpenGL Charts OpenGLWidgets)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/bin/Qt6${QT_LIB}${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    endforeach (QT_LIB)
endif ()

find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        OpenGL
        Charts
        Multimedia
        REQUIRED)

# Configuring OpenVR location
set(OpenVR_ROOT ${CMAKE_SOURCE_DIR}/thirdparty/openvr)
find_package(OpenVR REQUIRED)

# Configuring Steamworks API location
set(Steamworks_ROOT ${CMAKE_SOURCE_DIR}/thirdparty/steamworks)
find_package(SteamworksSDK REQUIRED)

target_include_directories(SeamenPerformanceOverlay PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(SeamenPerformanceOverlay PUBLIC cxx_std_23)

target_link_libraries(SeamenPerformanceOverlay
        PRIVATE
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::OpenGL
        Qt::Charts
        Qt::OpenGLWidgets
        Qt::Multimedia
        OpenVR::API
        dxgi
        SteamworksSDK::API
        pdh
)

# Make all warnings errors etc.
target_compile_options(SeamenPerformanceOverlay PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        #$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic -Werror>
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wextra -Wpedantic -Werror>
)

target_compile_definitions(SeamenPerformanceOverlay PRIVATE
        $<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT>
)

set_target_properties(SeamenPerformanceOverlay PROPERTIES
        WIN32_EXECUTABLE TRUE
        MACOSX_BUNDLE TRUE
        COMPILE_FLAGS "${QT_FLAGS}"
)

file(COPY ${OpenVR_BINARY} DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${SteamworksSDK_BINARY} DESTINATION ${CMAKE_BINARY_DIR})

add_custom_command(
        TARGET SeamenPerformanceOverlay POST_BUILD
        COMMENT "Copying Executable and DLLs"
        COMMAND ${CMAKE_COMMAND} -E copy -t ${OUTPUT_DIR} $<TARGET_FILE:SeamenPerformanceOverlay>
        COMMAND ${CMAKE_COMMAND} -E copy -t ${OUTPUT_DIR} $<TARGET_RUNTIME_DLLS:SeamenPerformanceOverlay>
        COMMAND_EXPAND_LISTS
)

add_custom_command(
        TARGET SeamenPerformanceOverlay POST_BUILD
        COMMENT "Copying resources to bin"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources"
        "${OUTPUT_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources"
        "$<TARGET_FILE_DIR:SeamenPerformanceOverlay>"
)

if (MINGW)
    get_filename_component(MINGW_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)

    set(MINGW_RUNTIME_DLLS
            "libgcc_s_seh-1.dll"
            "libstdc++-6.dll"
            "libwinpthread-1.dll"
    )

    foreach(MINGW_DLL IN LISTS MINGW_RUNTIME_DLLS)
        add_custom_command(
                TARGET SeamenPerformanceOverlay POST_BUILD
                COMMENT "Copying MinGW runtime DLL: ${MINGW_DLL}"
                # Copy to the CMake target build directory (to sit alongside Qt DLLs)
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${MINGW_BIN_DIR}/${MINGW_DLL}"
                "$<TARGET_FILE_DIR:SeamenPerformanceOverlay>"
                # Copy to your custom output bin directory
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${MINGW_BIN_DIR}/${MINGW_DLL}"
                "${OUTPUT_DIR}"
        )
    endforeach()
endif ()

if (WIN32)
    target_sources(SeamenPerformanceOverlay PRIVATE "resources-nonbin/SeamenPerformanceOverlay.rc")
endif()
