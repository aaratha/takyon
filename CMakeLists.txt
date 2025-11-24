
cmake_minimum_required(VERSION 3.25)
project(takyon VERSION 0.1 LANGUAGES C CXX)

# -----------------------------
# C++ standard
# -----------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -----------------------------
# Project sources
# -----------------------------
file(GLOB_RECURSE PROJECT_SRC "${PROJECT_SOURCE_DIR}/src/*.cpp")

# -----------------------------
# External sources
# -----------------------------
# Miniaudio wrapper
set(EXTERNAL_SRC
    "${PROJECT_SOURCE_DIR}/external/miniaudio_wrapper.cpp"
)

# -----------------------------
# Lua
# -----------------------------

# Lua
file(GLOB LUA_SRC "${CMAKE_CURRENT_SOURCE_DIR}/external/lua/*.c")
add_library(lua STATIC ${LUA_SRC})
set_target_properties(lua PROPERTIES LINKER_LANGUAGE C)
target_include_directories(lua PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/external/lua")

# Miniaudio
add_library(miniaudio STATIC external/miniaudio/miniaudio.c)
set_target_properties(miniaudio PROPERTIES LINKER_LANGUAGE C)
target_include_directories(miniaudio PUBLIC external/miniaudio)

# linenoise
add_library(linenoise STATIC external/linenoise/linenoise.c)
set_target_properties(linenoise PROPERTIES LINKER_LANGUAGE C)
target_include_directories(linenoise PUBLIC external/linenoise)



# -----------------------------
# Executable
# -----------------------------
add_executable(${PROJECT_NAME} ${PROJECT_SRC} ${EXTERNAL_SRC})

# -----------------------------
# Include directories
# -----------------------------
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/external/miniaudio
        ${PROGECT_SOURCE_DIR}/external/linenoise
)

# -----------------------------
# Link libraries
# -----------------------------
target_link_libraries(${PROJECT_NAME} PRIVATE miniaudio lua linenoise)
