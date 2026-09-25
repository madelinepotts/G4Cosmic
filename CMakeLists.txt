cmake_minimum_required(VERSION 3.20)

project(G4Cosmic VERSION 0.5.0 LANGUAGES CXX)

# =============================================================================
# C++ standard
# =============================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =============================================================================
# Geant4
# =============================================================================

option(G4COSMIC_ENABLE_UIVIS "Build interactive Geant4 UI and visualization support" ON)
option(G4COSMIC_BUILD_EXAMPLES "Build bundled example applications" ON)

if(G4COSMIC_ENABLE_UIVIS)
    find_package(Geant4 REQUIRED ui_all vis_all)
else()
    find_package(Geant4 REQUIRED)
endif()

include(${Geant4_USE_FILE})

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

add_library(cry STATIC ${CRY_SOURCES})

target_include_directories(cry PUBLIC "${CRY_ROOT}/src")
target_compile_features(cry PUBLIC cxx_std_17)

if(MSVC)
    target_compile_definitions(cry PRIVATE _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)
    target_compile_options(cry PRIVATE /Zc:__cplusplus)
endif()

# =============================================================================
# G4Cosmic framework library
# =============================================================================

set(G4COSMIC_CORE_SOURCES
    src/ActionInitialization.cc
    src/Application.cc
    src/FrameworkDetectorConstruction.cc
    src/GenericSensitiveDetector.cc
    src/OutputConfig.cc
    src/OutputMessenger.cc
    src/PrimaryGeneratorAction.cc
    src/RunAction.cc
    src/TrackingAction.cc
)

file(GLOB_RECURSE G4COSMIC_CORE_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hh"
)

add_library(g4cosmic_core STATIC
    ${G4COSMIC_CORE_SOURCES}
    ${G4COSMIC_CORE_HEADERS}
)

add_library(G4Cosmic::core ALIAS g4cosmic_core)

target_include_directories(g4cosmic_core
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CRY_ROOT}/src"
)

target_link_libraries(g4cosmic_core
    PUBLIC
        cry
        ${Geant4_LIBRARIES}
)

target_compile_features(g4cosmic_core PUBLIC cxx_std_17)

if(G4COSMIC_ENABLE_UIVIS)
    target_compile_definitions(g4cosmic_core PUBLIC G4COSMIC_ENABLE_UIVIS=1)
endif()

if(MSVC)
    target_compile_options(g4cosmic_core PRIVATE /Zc:__cplusplus)
    target_compile_definitions(g4cosmic_core PRIVATE _USE_MATH_DEFINES)
endif()

file(TO_CMAKE_PATH "${CRY_ROOT}/data" CRY_DATA_DIR_NORMALIZED)
file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cry/cry_setup.txt" CRY_SETUP_FILE_NORMALIZED)

target_compile_definitions(g4cosmic_core
    PRIVATE
        G4COSMIC_CRY_DATA_DIR="${CRY_DATA_DIR_NORMALIZED}"
        G4COSMIC_CRY_SETUP_FILE="${CRY_SETUP_FILE_NORMALIZED}"
)

# =============================================================================
# Shared runtime-file copy helper
# =============================================================================

function(g4cosmic_copy_runtime_files target_name)
    add_custom_command(
        TARGET ${target_name}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/macros"
                "$<TARGET_FILE_DIR:${target_name}>/macros"
        COMMENT "Copying Geant4 macros for ${target_name}"
    )

    add_custom_command(
        TARGET ${target_name}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${target_name}>/cry"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/cry/cry_setup.txt"
                "$<TARGET_FILE_DIR:${target_name}>/cry/cry_setup.txt"
        COMMENT "Copying CRY configuration for ${target_name}"
    )
endfunction()

# =============================================================================
# Examples
# =============================================================================

if(G4COSMIC_BUILD_EXAMPLES)
    set(G4COSMIC_WARPTRACK_GEOMETRY_HEADER
        "${CMAKE_CURRENT_SOURCE_DIR}/examples/WarpTrack/generated/DetectorGeometryGenerated.hh"
    )

    if(NOT EXISTS "${G4COSMIC_WARPTRACK_GEOMETRY_HEADER}")
        message(FATAL_ERROR
            "The WarpTrack example geometry header was not found:\n"
            "  ${G4COSMIC_WARPTRACK_GEOMETRY_HEADER}"
        )
    endif()

    add_executable(g4cosmic
        examples/WarpTrack/src/main.cc
        examples/WarpTrack/src/WarpTrackDetectorConstruction.cc
        examples/WarpTrack/src/WarpTrackSensitiveDetector.cc
        examples/WarpTrack/include/WarpTrackDetectorConstruction.hh
        examples/WarpTrack/include/WarpTrackSensitiveDetector.hh
        examples/WarpTrack/generated/DetectorGeometryGenerated.hh
    )

    target_include_directories(g4cosmic
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/examples/WarpTrack/include"
            "${CMAKE_CURRENT_SOURCE_DIR}/examples/WarpTrack/generated"
    )

    target_link_libraries(g4cosmic PRIVATE G4Cosmic::core)

    if(MSVC)
        target_compile_options(g4cosmic PRIVATE /Zc:__cplusplus)
    endif()

    g4cosmic_copy_runtime_files(g4cosmic)

    add_executable(g4cosmic_basic
        examples/BasicDetector/src/main.cc
        examples/BasicDetector/src/BasicDetectorConstruction.cc
        examples/BasicDetector/include/BasicDetectorConstruction.hh
    )

    target_include_directories(g4cosmic_basic
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/examples/BasicDetector/include"
    )

    target_link_libraries(g4cosmic_basic PRIVATE G4Cosmic::core)

    if(MSVC)
        target_compile_options(g4cosmic_basic PRIVATE /Zc:__cplusplus)
    endif()

    g4cosmic_copy_runtime_files(g4cosmic_basic)
endif()

# =============================================================================
# Status
# =============================================================================

message(STATUS "G4Cosmic source:           ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "CRY root:                  ${CRY_ROOT}")
message(STATUS "CRY data:                  ${CRY_ROOT}/data")
message(STATUS "Geant4 version:            ${Geant4_VERSION}")
if(G4COSMIC_BUILD_EXAMPLES)
    message(STATUS "Examples:                  WarpTrack, BasicDetector")
endif()
