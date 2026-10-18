cmake_minimum_required(VERSION 3.28)

project(FishingAssistant
    VERSION 0.0.1
    LANGUAGES CXX
)

include(FetchContent)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV 5.0 REQUIRED COMPONENTS
    core
    imgproc
    highgui
    imgcodecs
    dnn
)

set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG        yaml-cpp-0.9.0
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(yaml-cpp)

FetchContent_Declare(
    robot-cpp
    GIT_REPOSITORY https://github.com/dimonp/robot-cpp.git
    GIT_TAG        091a4f5d922e5a99aebc78404ea3e5aeb40d26cd
    GIT_SHALLOW    TRUE
    GIT_SUBMODULES_RECURSE TRUE
)

FetchContent_MakeAvailable(robot-cpp)

add_executable(fishing_assistant)

target_sources(fishing_assistant
    PRIVATE
        src/main.cpp
        src/config.cpp
        src/key_parser.cpp
        src/hotkeys.cpp
        src/display.cpp
        src/detector.cpp
        src/robot_context.cpp
        src/region_selector.cpp
        src/screen_capture.cpp
        src/fishing_action.cpp
        src/preview.cpp

    PRIVATE
        FILE_SET CXX_MODULES
        FILES
            src/config.cppm
            src/key_parser.cppm
            src/hotkeys.cppm
            src/display.cppm
            src/detector.cppm
            src/robot_context.cppm
            src/region_selector.cppm
            src/screen_capture.cppm
            src/controller.cppm
            src/fishing_action.cppm
            src/preview.cppm
)

target_compile_features(fishing_assistant PRIVATE cxx_std_23)

target_link_libraries(fishing_assistant
    PRIVATE
        ${OpenCV_LIBS}
        yaml-cpp::yaml-cpp
        robot::robot
)

if(MSVC)
    target_compile_options(fishing_assistant PRIVATE
        /W4
        /permissive-
    )
else()
    target_compile_options(fishing_assistant PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

# Installation directory
include(GNUInstallDirs)

install(TARGETS fishing_assistant
    RUNTIME_DEPENDENCY_SET fishing_runtime_deps
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(WIN32)
    install(RUNTIME_DEPENDENCY_SET fishing_runtime_deps
        DESTINATION ${CMAKE_INSTALL_BINDIR}

        # Do not copy Windows system DLLs
        PRE_EXCLUDE_REGEXES
            "api-ms-win-.*"
            "ext-ms-win-.*"

        POST_EXCLUDE_REGEXES
            ".*[Ww]indows[/\\\\]System32[/\\\\].*"
            ".*[Ww]indows[/\\\\]SysWOW64[/\\\\].*"
    )
endif()