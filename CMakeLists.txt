cmake_minimum_required(VERSION 3.20)

project(libLLKA LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(EIGEN_INCLUDE_DIR "" CACHE PATH "Path to the Eigen library files")
set(BUILD_STATIC_LIBRARY ON CACHE BOOL "Build static library")
set(BUILD_SHARED_LIBRARY OFF CACHE BOOL "Build shared library")
set(USE_ADDRESS_SANITIZER OFF CACHE BOOL "Use AddressSanitizer (supported only with recent Clang and GCC)")
set(BUILD_TESTING ON CACHE BOOL "Build tests")
set(BUILD_EXAMPLES ON CACHE BOOL "Build examples")
set(BUILD_PYTHON_BINDINGS OFF CACHE BOOL "Build Python bindings")

if (BUILD_STATIC_LIBRARY AND BUILD_SHARED_LIBRARY)
    message(
        FATAL_ERROR
        "It is not allowed to build both Shared and Static variant of the library simultaneously.
Create separate build configurations if you need to build both variants of this library."
    )
endif()

set(LIBLLKA_GLOBAL_DEFINITIONS -D_USE_MATH_DEFINES)
set(LIBLLKA_PLATFORM_DEFINITIONS "")

if (BUILD_SHARED_LIBRARY)
    set("LLKA_DLL_BUILD" "1")
endif()
if (BUILD_STATIC_LIBRARY)
    set("LLKA_STATIC_BUILD" "1")
endif()

if (WIN32)
    set("LLKA_PLATFORM_WIN32" "1")

    set(LIBLLKA_PLATFORM_DEFINITIONS "${LIBLLKA_PLATFORM_DEFINITIONS}" -DWIN32_LEAN_AND_MEAN -DNOMINMAX)

    if (MINGW OR MSYS)
        if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0.0)
            message(FATAL_ERROR "MinGW 7.0.0 or newer is required to build libLLKA")
        endif ()
        add_compile_options(-Wall -Wextra -pedantic -Wno-unused-function -fvisibility=hidden -g -msse2)
        if (CMAKE_BUILD_TYPE STREQUAL "Release")
            add_compile_options(-O2)
        endif ()

        if (USE_ADDRESS_SANITIZER)
            add_compile_options(-fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined -fsanitize=leak -fstack-protector-strong)
            add_link_options(-fsanitize=address -fsanitize=undefined -fsanitize=leak)
        endif ()

        if (MINGW)
            set(LIBLLKA_PLATFORM_DEFINITIONS -DLLKA_COMPILER_MINGW)
            set(LLKA_COMPILER_MINGW 1)
        else ()
            set(LIBLLKA_PLATFORM_DEFINITIONS -DLLKA_COMPILER_MSYS)
            set(LLKA_COMPILER_MSYS 1)
        endif ()

        set(DEFAULT_SYMVER_LINK "-Wl,--default-symver")
    elseif (MSVC)
        set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;MinSizeRel")

        set(CMAKE_CXX_FLAGS_RELEASE "/GL")
        set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "/INCREMENTAL:NO /LTCG")
        set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "/INCREMENTAL:NO /LTCG")

        set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/GL")
        set(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO "/debug /INCREMENTAL:NO /LTCG")
        set(CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO "/debug /INCREMENTAL:NO /LTCG")
        set(LIBLLKA_PLATFORM_DEFINITIONS "${LIBLLKA_PLATFORM_DEFINITIONS}" -DLLKA_COMPILER_MSVC -DLLKA_HAVE_SAD_LADY_COMPILER -DUNICODE -D_UNICODE)
        set(LLKA_COMPILER_MSVC 1)
        set(LLKA_HAVE_SAD_LADY_COMPILER 1)
        set(DEFAULT_SYMVER_LINK " ")
    else()
        message(FATAL_ERROR "Compiler not recognized or not supported")
    endif()
elseif (UNIX)
    set("LLKA_PLATFORM_UNIX" "1")

    if (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        add_compile_options(-msse2)
        add_definitions("-DLLKA_USE_SIMD_X86")
    endif ()

    if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR
        ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
        add_compile_options(-pipe -Wall -Wextra -pedantic -fvisibility=hidden -g)
        if (CMAKE_BUILD_TYPE STREQUAL "Release")
            if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" AND
                CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 9.1.0 OR
                CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 9.1.0)
                add_compile_options(-flto)
            endif ()
            add_compile_options(-O2)
        elseif (CMAKE_BUILD_TYPE STREQUAL "Profile")
            add_compile_options(-O2 -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer)
        endif ()

        if (USE_ADDRESS_SANITIZER)
            add_compile_options(-fno-omit-frame-pointer -mno-omit-leaf-frame-pointer -fsanitize=address -fsanitize=undefined -fsanitize=leak -fstack-protector-strong)
            add_link_options(-fsanitize=address -fsanitize=undefined -fsanitize=leak)
        endif ()

        if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
            add_compile_options(-fdiagnostics-color=always)
        elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
            add_compile_options(-fcolor-diagnostics)
        endif ()

        set(LIBLLKA_PLATFORM_DEFINITIONS -DLLKA_COMPILER_GCC_LIKE)
        set(DEFAULT_SYMVER_LINK "-Wl,--default-symver")
        set(LLKA_COMPILER_GCC_LIKE 1)
    endif ()
else()
    message(FATAL_ERROR "Platform not recognized or not supported")
endif()

if ((NOT BUILD_STATIC_LIBRARY) AND (NOT BUILD_SHARED_LIBRARY))
    message(FATAL "Neither static nor shared library is set to be built. There is nothing to do.")
endif ()

include_directories(
    ${INCLUDE_DIRECTORIES}
    include
    src
    "${CMAKE_CURRENT_BINARY_DIR}"
    SYSTEM ${EIGEN_INCLUDE_DIR}
)

set(
    libLLKA_SRCS
    "src/util/elementaries.cpp"
    "src/util/geometry.cpp"
    "src/util/geometry_cpp.cpp"
    "src/util/printers.cpp"
    "src/classification.cpp"
    "src/extend.cpp"
    "src/extract.cpp"
    "src/measurements.cpp"
    "src/ntc.cpp"
    "src/ntc_bones.cpp"
    "src/ntc_constants.cpp"
    "src/nucleotide.cpp"
    "src/llka.cpp"
    "src/resource_loaders.cpp"
    "src/structure.cpp"
    "src/structure_util.cpp"
    "src/superposition.cpp"
    "src/tracing.cpp"
    "src/llka_cpp.cpp"
)

set(
    libLLKA_PUB_HDRS
    "include/llka_classification.h"
    "include/llka_cpp.h"
    "include/llka_main.h"
    "include/llka_measurements.h"
    "include/llka_module.h"
    "include/llka_ntc.h"
    "include/llka_nucleotide.h"
    "include/llka_resource_loaders.h"
    "include/llka_structure.h"
    "include/llka_superposition.h"
    "include/llka_tracing.h"
    "include/llka_util.h"
)

if (DNATCO_EXTRAS)
    set(
        libLLKA_SRCS
        ${libLLKA_SRCS}
        "dnatco_extras/src/dnatco.cpp"
    )
endif ()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/include/llka_config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/llka_config.h")

set(LLKA_TARGETS "")

if (BUILD_STATIC_LIBRARY)
    add_library(libLLKA_STATIC STATIC ${libLLKA_SRCS})
    set_target_properties(
        libLLKA_STATIC
        PROPERTIES OUTPUT_NAME LLKA
        LINK_FLAGS ${DEFAULT_SYMVER_LINK}
    )
    target_compile_definitions(libLLKA_STATIC PRIVATE ${LIBLLKA_GLOBAL_DEFINITIONS} ${LIBLLKA_PLATFORM_DEFINITIONS})
    target_link_libraries(libLLKA_STATIC ${LLKA_EXTRA_LINK_LIBS})

    set(LLKA_TARGETS ${LLKA_TARGETS} libLLKA_STATIC)
endif ()

if (BUILD_SHARED_LIBRARY)
    add_library(libLLKA_SHARED SHARED ${libLLKA_SRCS})
    set_target_properties(
        libLLKA_SHARED
        PROPERTIES VERSION 0.1
                    SOVERSION 0.1
                    OUTPUT_NAME LLKA
                    LINK_FLAGS ${DEFAULT_SYMVER_LINK}
    )
    target_compile_definitions(libLLKA_SHARED PRIVATE ${LIBLLKA_GLOBAL_DEFINITIONS} ${LIBLLKA_PLATFORM_DEFINITIONS})
    target_link_libraries(libLLKA_SHARED ${LLKA_EXTRA_LINK_LIBS})

    set(LLKA_TARGETS ${LLKA_TARGETS} libLLKA_SHARED)
endif ()

if (UNIX)
    include(GNUInstallDirs)
else ()
    set(CMAKE_INSTALL_LIBDIR "lib")
    set(CMAKE_INSTALL_INCLUDEDIR "include")
    set(CMAKE_INSTALL_BINDIR "bin")
endif ()

install(
    TARGETS ${LLKA_TARGETS} EXPORT libLLKA_targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(
    EXPORT libLLKA_targets DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/LLKA"
)

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/llka_config.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/LLKA"
)
install(
    FILES ${libLLKA_PUB_HDRS}
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/LLKA"
)
if (DNATCO_EXTRAS)
    install(
        FILES "${CMAKE_CURRENT_SOURCE_DIR}/dnatco_extras/include/dnatco.h"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/LLKA/"
    )
endif ()

if (BUILD_TESTING)
    include(CTest)
    add_subdirectory("tests")
endif ()

if (BUILD_EXAMPLES)
    add_subdirectory("examples")
endif ()

if (BUILD_PYTHON_BINDINGS AND NOT EMSCRIPTEN)
    add_subdirectory("python")
endif ()

if (WIN32)
    set(LIBLLKA_CMAKE_DIR cmake)
else ()
    set(LIBLLKA_CMAKE_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/LLKA/")
endif ()

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/LLKAConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/LLKAConfigForInstall.cmake"
    INSTALL_DESTINATION "${LIBLLKA_CMAKE_DIR}"
    PATH_VARS
        CMAKE_INSTALL_PREFIX
        CMAKE_INSTALL_INCLUDEDIR
        CMAKE_INSTALL_LIBDIR
        CMAKE_INSTALL_BINDIR
)
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/cmake/LLKAConfigForInstall.cmake"
    DESTINATION "${LIBLLKA_CMAKE_DIR}"
    RENAME LLKAConfig.cmake
)

if (WIN32)
elseif (APPLE)
else ()
    set(PKGCONFIG_PLATFORM_LIBS "-lm")
endif ()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/pkgconfig/llka.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/pkgconfig/llka.pc" @ONLY
)
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/cmake/pkgconfig/llka.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig/"
)
