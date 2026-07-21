cmake_minimum_required(VERSION 3.25)

project(
    rafedit
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# ------------------------------------------------------------
# SDL3
# ------------------------------------------------------------

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.28
    GIT_SHALLOW TRUE
)

#set(SDL_SHARED OFF CACHE BOOL "" FORCE)
#set(SDL_STATIC ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(SDL3)

# ------------------------------------------------------------
# SDL3_ttf
# ------------------------------------------------------------

set(SDLTTF_VENDORED ON CACHE BOOL "" FORCE)
set(SDLTTF_HARFBUZZ OFF CACHE BOOL "" FORCE)
set(SDLTTF_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDLTTF_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    SDL3_ttf
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
    GIT_TAG release-3.2.2
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(SDL3_ttf)

# ------------------------------------------------------------
# Aplicación
# ------------------------------------------------------------

add_executable(${PROJECT_NAME}
    src/main.cpp
    
    src/Application.cpp
    src/Application.h

    src/editor/Editor.h
    src/editor/Editor.cpp
    src/editor/Cursor.h
    src/editor/Cursor.cpp
    src/editor/TextBuffer.h
    src/editor/TextBuffer.cpp
    src/editor/TextPosition.h
    src/editor/Viewport.h
    src/editor/Viewport.cpp
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        SDL3::SDL3
        SDL3_ttf::SDL3_ttf
)

target_compile_features(${PROJECT_NAME}
    PRIVATE
        cxx_std_20
)

target_compile_options(${PROJECT_NAME}
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
)

# ------------------------------------------------------------
# Copiar las DLL junto al ejecutable
# ------------------------------------------------------------

add_custom_command(
    TARGET rafedit
    POST_BUILD

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL3::SDL3>
        $<TARGET_FILE_DIR:rafedit>

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL3_ttf::SDL3_ttf>
        $<TARGET_FILE_DIR:rafedit>

    COMMENT "Copiando DLL de SDL3 y SDL3_ttf"
)

# ------------------------------------------------------------
# Copiar recursos junto al ejecutable
# ------------------------------------------------------------

add_custom_command(
    TARGET rafedit
    POST_BUILD

    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:rafedit>/assets"

    COMMENT "Copiando recursos de Rafedit"
)