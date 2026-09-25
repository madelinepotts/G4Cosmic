cmake_minimum_required(VERSION 3.20)

project(G4Cosmic VERSION 0.8.4 LANGUAGES CXX)

# =============================================================================
# C++ standard
# =============================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# =============================================================================
# Build metadata
# =============================================================================

set(G4COSMIC_GIT_COMMIT "unknown")
set(G4COSMIC_GIT_BRANCH "unknown")
set(G4COSMIC_GIT_TAG "")
set(G4COSMIC_GIT_DIRTY "0")

find_package(Git QUIET)
if(Git_FOUND)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --short=12 HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE G4COSMIC_GIT_COMMIT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE G4COSMIC_GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" describe --tags --exact-match
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE G4COSMIC_GIT_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" status --porcelain
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE G4COSMIC_GIT_STATUS
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(NOT "${G4COSMIC_GIT_STATUS}" STREQUAL "")
        set(G4COSMIC_GIT_DIRTY "1")
    endif()
endif()

# =============================================================================
# Geant4
# =============================================================================

option(G4COSMIC_ENABLE_UIVIS "Build interactive Geant4 UI and visualization support" ON)
option(G4COSMIC_BUILD_EXAMPLES "Build bundled example applications" ON)
option(G4COSMIC_BUILD_DOCS "Add a Doxygen documentation target when Doxygen is available" OFF)

if(G4COSMIC_ENABLE_UIVIS)
    find_package(Geant4 REQUIRED ui_all vis_all)
else()
    find_package(Geant4 REQUIRED)
endif()

include(${Geant4_USE_FILE})

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/G4Cosmic/BuildInfo.hh.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/G4Cosmic/BuildInfo.hh"
    @ONLY
)


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
    src/CORSIKAPrimaryGenerator.cc
    src/CRYPrimaryGenerator.cc
    src/EventAction.cc
    src/FrameworkDetectorConstruction.cc
    src/GenericSensitiveDetector.cc
    src/GunPrimaryGenerator.cc
    src/OutputConfig.cc
    src/OutputMessenger.cc
    src/JsonWriter.cc
    src/PrimaryGenerator.cc
    src/PrimaryGeneratorAction.cc
    src/RunAction.cc
    src/RunMetadata.cc
    src/SamplePrimaryGenerator.cc
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
        "${CMAKE_CURRENT_BINARY_DIR}/generated"
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
set(G4COSMIC_CRY_SETUP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/config/cry/cry_setup.txt" CACHE FILEPATH "Path to the G4Cosmic CRY setup macro")
file(TO_CMAKE_PATH "${G4COSMIC_CRY_SETUP_FILE}" CRY_SETUP_FILE_NORMALIZED)

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
                "${G4COSMIC_CRY_SETUP_FILE}"
                "$<TARGET_FILE_DIR:${target_name}>/cry/cry_setup.txt"
        COMMENT "Copying CRY configuration for ${target_name}"
    )

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/corsika")
        add_custom_command(
            TARGET ${target_name}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${CMAKE_CURRENT_SOURCE_DIR}/examples/corsika"
                    "$<TARGET_FILE_DIR:${target_name}>/examples/corsika"
            COMMENT "Copying CORSIKA example input files for ${target_name}"
        )
    endif()
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
# Doxygen documentation
# =============================================================================

if(G4COSMIC_BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(G4COSMIC_DOXYGEN_INPUT
            "${CMAKE_CURRENT_SOURCE_DIR}/include"
            "${CMAKE_CURRENT_SOURCE_DIR}/src"
            "${CMAKE_CURRENT_SOURCE_DIR}/docs/doxygen"
        )
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/docs/doxygen/Doxyfile.in"
            "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
            @ONLY
        )
        add_custom_target(g4cosmic_docs
            COMMAND "${DOXYGEN_EXECUTABLE}" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            COMMENT "Generating G4Cosmic Doxygen documentation"
            VERBATIM
        )
    else()
        message(WARNING "G4COSMIC_BUILD_DOCS=ON, but Doxygen was not found. Documentation target will not be added.")
    endif()
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
