cmake_minimum_required(VERSION 3.20)

project(G4Cosmic LANGUAGES CXX)

# =============================================================================
# C++ standard
# =============================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =============================================================================
# Geant4
#
# UI/visualization is enabled by default because the existing application
# supports interactive Geant4 sessions. Turn it off for headless CI, clusters,
# or minimal Geant4 installations:
#
#   cmake -S . -B build -DG4COSMIC_ENABLE_UIVIS=OFF
# =============================================================================

option(G4COSMIC_ENABLE_UIVIS "Build interactive Geant4 UI and visualization support" ON)

if(G4COSMIC_ENABLE_UIVIS)
    find_package(Geant4 REQUIRED ui_all vis_all)
else()
    find_package(Geant4 REQUIRED)
endif()

include(${Geant4_USE_FILE})

# =============================================================================
# Geometry data
#
# Stage 1 keeps the existing detector geometry unchanged. If this directory is
# used inside the original WarpTrack-style repository layout, CMake regenerates
# DetectorGeometryGenerated.hh from ../geometry. If only this uploaded simulation
# tree is available, CMake falls back to the generated header included in
# generated/.
# =============================================================================

set(G4COSMIC_GENERATED_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
)

set(G4COSMIC_GEOMETRY_JSON
    "${CMAKE_CURRENT_SOURCE_DIR}/../geometry/detector_geometry.json"
)

set(G4COSMIC_GEOMETRY_GENERATOR
    "${CMAKE_CURRENT_SOURCE_DIR}/../geometry/generate_cpp_geometry.py"
)

set(G4COSMIC_VENDORED_GEOMETRY
    "${CMAKE_CURRENT_SOURCE_DIR}/generated/DetectorGeometryGenerated.hh"
)

set(G4COSMIC_GENERATED_GEOMETRY
    "${G4COSMIC_GENERATED_DIR}/DetectorGeometryGenerated.hh"
)

set(G4COSMIC_GEOMETRY_IS_GENERATED OFF)

if(EXISTS "${G4COSMIC_GEOMETRY_JSON}" AND EXISTS "${G4COSMIC_GEOMETRY_GENERATOR}")
    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    file(MAKE_DIRECTORY "${G4COSMIC_GENERATED_DIR}")

    add_custom_command(
        OUTPUT
            "${G4COSMIC_GENERATED_GEOMETRY}"

        COMMAND
            "${Python3_EXECUTABLE}"
            "${G4COSMIC_GEOMETRY_GENERATOR}"
            "${G4COSMIC_GEOMETRY_JSON}"
            "${G4COSMIC_GENERATED_GEOMETRY}"

        DEPENDS
            "${G4COSMIC_GEOMETRY_JSON}"
            "${G4COSMIC_GEOMETRY_GENERATOR}"

        COMMENT
            "Generating C++ detector geometry from shared JSON"

        VERBATIM
    )

    add_custom_target(
        generate_detector_geometry
        DEPENDS
            "${G4COSMIC_GENERATED_GEOMETRY}"
    )

    set(G4COSMIC_GEOMETRY_INCLUDE_DIR "${G4COSMIC_GENERATED_DIR}")
    set(G4COSMIC_GEOMETRY_IS_GENERATED ON)
else()
    if(NOT EXISTS "${G4COSMIC_VENDORED_GEOMETRY}")
        message(FATAL_ERROR
            "Detector geometry was not found. Expected either:\n"
            "  ${G4COSMIC_GEOMETRY_JSON}\n"
            "  ${G4COSMIC_GEOMETRY_GENERATOR}\n"
            "or the fallback generated header:\n"
            "  ${G4COSMIC_VENDORED_GEOMETRY}"
        )
    endif()

    message(WARNING
        "Using vendored generated detector geometry:\n"
        "  ${G4COSMIC_VENDORED_GEOMETRY}\n"
        "The original geometry JSON/generator were not included in the uploaded archive."
    )

    set(G4COSMIC_GENERATED_GEOMETRY "${G4COSMIC_VENDORED_GEOMETRY}")
    set(G4COSMIC_GEOMETRY_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/generated")
endif()

# =============================================================================
# CRY
# =============================================================================

set(CRY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/external/cry" CACHE PATH "Path to the CRY source tree")

if(NOT EXISTS "${CRY_ROOT}/src/CRYGenerator.cc")
    message(FATAL_ERROR
        "CRY was not found at:\n"
        "  ${CRY_ROOT}\n\n"
        "Install CRY into external/cry with one of:\n"
        "  powershell -ExecutionPolicy Bypass -File .\\scripts\\install_cry.ps1\n"
        "  ./scripts/install_cry.sh\n\n"
        "Or set CRY_ROOT to an existing CRY source tree."
    )
endif()

if(NOT EXISTS "${CRY_ROOT}/data")
    message(FATAL_ERROR
        "CRY data directory was not found at:\n"
        "  ${CRY_ROOT}/data"
    )
endif()

file(GLOB CRY_SOURCES CONFIGURE_DEPENDS
    "${CRY_ROOT}/src/*.cc"
)

add_library(
    cry
    STATIC
    ${CRY_SOURCES}
)

target_include_directories(
    cry
    PUBLIC
        "${CRY_ROOT}/src"
)

target_compile_features(
    cry
    PUBLIC
        cxx_std_17
)

if(MSVC)
    target_compile_definitions(
        cry
        PRIVATE
            _USE_MATH_DEFINES
            _CRT_SECURE_NO_WARNINGS
    )

    target_compile_options(
        cry
        PRIVATE
            /Zc:__cplusplus
    )
endif()

# =============================================================================
# G4Cosmic simulation sources
# =============================================================================

file(GLOB G4COSMIC_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
)

file(GLOB_RECURSE G4COSMIC_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hh"
)

add_executable(
    g4cosmic
    ${G4COSMIC_SOURCES}
    ${G4COSMIC_HEADERS}
    "${G4COSMIC_GENERATED_GEOMETRY}"
)

if(G4COSMIC_GEOMETRY_IS_GENERATED)
    add_dependencies(
        g4cosmic
        generate_detector_geometry
    )
endif()

# =============================================================================
# Include directories
# =============================================================================

target_include_directories(
    g4cosmic
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${G4COSMIC_GEOMETRY_INCLUDE_DIR}"
        "${CRY_ROOT}/src"
)

if(G4COSMIC_ENABLE_UIVIS)
    target_compile_definitions(
        g4cosmic
        PRIVATE
            G4COSMIC_ENABLE_UIVIS=1
    )
endif()

# =============================================================================
# Link libraries
# =============================================================================

target_link_libraries(
    g4cosmic
    PRIVATE
        cry
        ${Geant4_LIBRARIES}
)

# =============================================================================
# MSVC options
# =============================================================================

if(MSVC)
    target_compile_options(
        g4cosmic
        PRIVATE
            /Zc:__cplusplus
    )

    target_compile_definitions(
        g4cosmic
        PRIVATE
            _USE_MATH_DEFINES
    )
endif()

# =============================================================================
# CRY runtime paths
# =============================================================================

file(TO_CMAKE_PATH
    "${CRY_ROOT}/data"
    CRY_DATA_DIR_NORMALIZED
)

file(TO_CMAKE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/cry/cry_setup.txt"
    CRY_SETUP_FILE_NORMALIZED
)

target_compile_definitions(
    g4cosmic
    PRIVATE
        G4COSMIC_CRY_DATA_DIR="${CRY_DATA_DIR_NORMALIZED}"
        G4COSMIC_CRY_SETUP_FILE="${CRY_SETUP_FILE_NORMALIZED}"
)

# =============================================================================
# Copy macros beside the executable
# =============================================================================

add_custom_command(
    TARGET g4cosmic
    POST_BUILD

    COMMAND
        ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/macros"
        "$<TARGET_FILE_DIR:g4cosmic>/macros"

    COMMENT
        "Copying Geant4 macros"
)

# =============================================================================
# Copy CRY setup beside the executable
# =============================================================================

add_custom_command(
    TARGET g4cosmic
    POST_BUILD

    COMMAND
        ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:g4cosmic>/cry"

    COMMAND
        ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/cry/cry_setup.txt"
        "$<TARGET_FILE_DIR:g4cosmic>/cry/cry_setup.txt"

    COMMENT
        "Copying CRY configuration"
)

# =============================================================================
# Status
# =============================================================================

message(STATUS "G4Cosmic source:      ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "Detector geometry:   ${G4COSMIC_GENERATED_GEOMETRY}")
message(STATUS "CRY root:            ${CRY_ROOT}")
message(STATUS "CRY data:            ${CRY_ROOT}/data")
message(STATUS "Geant4 version:      ${Geant4_VERSION}")
message(STATUS "UI/Vis enabled:      ${G4COSMIC_ENABLE_UIVIS}")
