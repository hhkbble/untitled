cmake_minimum_required(VERSION 3.30)
project(untitled)

if (!MSVC)
    message(FATAL_ERROR "Only MSVC is supported")
endif()

set(CMAKE_CXX_STANDARD 23)
include(FetchContent)

FetchContent_Declare(minhook
        GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
        GIT_TAG        v1.3.4
        GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(minhook)

set(build_static_lib ON CACHE BOOL "" FORCE)
FetchContent_Declare(easyloggingpp
        GIT_REPOSITORY https://github.com/abumq/easyloggingpp.git
        GIT_TAG        v9.97.1
        GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(easyloggingpp)
target_compile_definitions(easyloggingpp PUBLIC ELPP_THREAD_SAFE)
FetchContent_GetProperties(easyloggingpp)

FetchContent_Declare(ReShade
        GIT_REPOSITORY https://github.com/crosire/reshade.git
        GIT_TAG     main
        GIT_SHALLOW TRUE
        GIT_SUBMODULES "deps/imgui"
        GIT_SUBMODULES_RECURSE OFF
)
FetchContent_MakeAvailable(ReShade)

add_library(untitled SHARED
        addon.cpp
        hook.cpp
        Dumper-7/SDK/Basic.cpp
        Dumper-7/SDK/CoreUObject_functions.cpp
        Dumper-7/SDK/Engine_functions.cpp
)
target_compile_options(untitled PUBLIC "/Zc:preprocessor")
target_compile_definitions(untitled PRIVATE IMGUI_DISABLE_INCLUDE_IMCONFIG_H)
target_include_directories(untitled PRIVATE
        "Dumper-7"
        "${reshade_SOURCE_DIR}/include"
        "${reshade_SOURCE_DIR}/deps/imgui"
        ${easyloggingpp_SOURCE_DIR}/src
)
target_link_libraries(untitled PRIVATE
        minhook
        easyloggingpp
)
