cmake_minimum_required(VERSION 3.20)

project(G4Cosmic VERSION 0.4.0 LANGUAGES CXX)

# =============================================================================
# C++ standard
# =============================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =============================================================================
# Geant4
#
# UI/visualization is enabled by default because the example application supports
# interactive Geant4 sessions. Turn it off for headless CI, clusters, or minimal
# Geant4 installations:
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
# CRY
#
# This follows the original WarpTrack pattern: install CRY under external/cry
# with scripts/install_cry.ps1 or scripts/install_cry.sh, then CMake builds the
# CRY sources directly.
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

add_library(cry STATIC ${CRY_SOURCES})

target_include_directories(cry PUBLIC "${CRY_ROOT}/src")

target_compile_features(cry PUBLIC cxx_std_17)

if(MSVC)
    target_compile_definitions(cry PRIVATE _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)
    target_compile_options(cry PRIVATE /Zc:__cplusplus)
endif()

# =============================================================================
# WarpTrack example detector geometry
#
# Stage 3 removes the old JSON/code-generation path. The generated geometry
# header is now treated as a normal source artifact owned by the WarpTrack
# example. Later detector projects will provide their own normal Geant4 C++
# geometry rather than using this example header.
# =============================================================================

set(G4COSMIC_WARPTRACK_GEOMETRY_HEADER
    "${CMAKE_CURRENT_SOURCE_DIR}/examples/WarpTrack/generated/DetectorGeometryGenerated.hh"
)

if(NOT EXISTS "${G4COSMIC_WARPTRACK_GEOMETRY_HEADER}")
    message(FATAL_ERROR
        "The WarpTrack example geometry header was not found:\n"
        "  ${G4COSMIC_WARPTRACK_GEOMETRY_HEADER}"
    )
endif()

# =============================================================================
# G4Cosmic executable
#
# The current top-level executable runs the WarpTrack example detector. The
# framework pieces live under include/G4Cosmic and src/*.cc. The detector-specific
# geometry and sensitive-detector code live under examples/WarpTrack.
# =============================================================================

set(G4COSMIC_CORE_SOURCES
    src/ActionInitialization.cc
    src/Application.cc
    src/EventAction.cc
    src/FrameworkDetectorConstruction.cc
    src/GenericSensitiveDetector.cc
    src/OutputConfig.cc
    src/OutputMessenger.cc
    src/PrimaryGeneratorAction.cc
    src/RunAction.cc
    src/SteppingAction.cc
    src/TrackingAction.cc
)

set(G4COSMIC_WARPTRACK_EXAMPLE_SOURCES
    examples/WarpTrack/src/WarpTrackDetectorConstruction.cc
    examples/WarpTrack/src/WarpTrackSensitiveDetector.cc
)

file(GLOB_RECURSE G4COSMIC_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/examples/WarpTrack/include/*.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/examples/WarpTrack/generated/*.hh"
)

add_executable(
    g4cosmic
    src/main.cc
    ${G4COSMIC_CORE_SOURCES}
    ${G4COSMIC_WARPTRACK_EXAMPLE_SOURCES}
    ${G4COSMIC_HEADERS}
)

# =============================================================================
# Include directories
# =============================================================================

target_include_directories(
    g4cosmic
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/examples/WarpTrack/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/examples/WarpTrack/generated"
        "${CRY_ROOT}/src"
)

if(G4COSMIC_ENABLE_UIVIS)
    target_compile_definitions(g4cosmic PRIVATE G4COSMIC_ENABLE_UIVIS=1)
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
# Compiler options
# =============================================================================

if(MSVC)
    target_compile_options(g4cosmic PRIVATE /Zc:__cplusplus)
    target_compile_definitions(g4cosmic PRIVATE _USE_MATH_DEFINES)
endif()

# =============================================================================
# CRY runtime paths
# =============================================================================

file(TO_CMAKE_PATH "${CRY_ROOT}/data" CRY_DATA_DIR_NORMALIZED)
file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cry/cry_setup.txt" CRY_SETUP_FILE_NORMALIZED)

target_compile_definitions(
    g4cosmic
    PRIVATE
        G4COSMIC_CRY_DATA_DIR="${CRY_DATA_DIR_NORMALIZED}"
        G4COSMIC_CRY_SETUP_FILE="${CRY_SETUP_FILE_NORMALIZED}"
)

# =============================================================================
# Copy runtime files beside the executable
# =============================================================================

add_custom_command(
    TARGET g4cosmic
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/macros"
            "$<TARGET_FILE_DIR:g4cosmic>/macros"
    COMMENT "Copying Geant4 macros"
)

add_custom_command(
    TARGET g4cosmic
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:g4cosmic>/cry"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/cry/cry_setup.txt"
            "$<TARGET_FILE_DIR:g4cosmic>/cry/cry_setup.txt"
    COMMENT "Copying CRY configuration"
)

# =============================================================================
# Status
# =============================================================================

message(STATUS "G4Cosmic source:           ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "WarpTrack example header:  ${G4COSMIC_WARPTRACK_GEOMETRY_HEADER}")
message(STATUS "CRY root:                  ${CRY_ROOT}")
message(STATUS "CRY data:                  ${CRY_ROOT}/data")
message(STATUS "Geant4 version:            ${Geant4_VERSION}")
message(STATUS "UI/Vis enabled:            ${G4COSMIC_ENABLE_UIVIS}")
