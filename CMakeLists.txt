cmake_minimum_required(VERSION 3.25)

project(ASMFibHeap
  VERSION 0.1.0
  LANGUAGES C CXX
)

# =========================================================
# 0. STANDALONE vs SUBPROJECT
# =========================================================
if(PROJECT_IS_TOP_LEVEL)
  set(ASMFIBHEAP_DEV_DEFAULT ON)
else()
  set(ASMFIBHEAP_DEV_DEFAULT OFF)
endif()

# =========================================================
# 1. OPTIONAL ASSEMBLER BACKEND
# =========================================================
# The NASM-based FibHeap is optional — pure-C build still works on machines
# without NASM. ASMFIBHEAP_BUILD_ASM defaults to ON only when NASM is found.

find_program(NASM_EXECUTABLE nasm)
if(NASM_EXECUTABLE)
  set(ASMFIBHEAP_HAVE_ASM_DEFAULT ON)
else()
  set(ASMFIBHEAP_HAVE_ASM_DEFAULT OFF)
endif()

option(ASMFIBHEAP_BUILD_ASM "Build the NASM-based FibHeap (requires nasm)" ${ASMFIBHEAP_HAVE_ASM_DEFAULT})

if(ASMFIBHEAP_BUILD_ASM)
  if(NOT NASM_EXECUTABLE)
    message(FATAL_ERROR
      "ASMFIBHEAP_BUILD_ASM=ON but `nasm` is not on PATH. "
      "Install it (sudo dnf install nasm / sudo apt install nasm) or pass "
      "-DASMFIBHEAP_BUILD_ASM=OFF.")
  endif()
  enable_language(ASM_NASM)
  set(CMAKE_ASM_NASM_FLAGS "${CMAKE_ASM_NASM_FLAGS} -f elf64")
endif()

# =========================================================
# 2. OPTIONS
# =========================================================
option(ASMFIBHEAP_BUILD_APPS       "Build the demo and test-driver apps"  ${ASMFIBHEAP_DEV_DEFAULT})
option(ASMFIBHEAP_BUILD_TESTS      "Build the GTest unit/diff/stress suite" ${ASMFIBHEAP_DEV_DEFAULT})
option(ASMFIBHEAP_ENABLE_COVERAGE  "Enable gcov/lcov coverage targets"    OFF)

# Apps and the FibHeapTesting harness reference symbols from ASM_FibHeap
# unconditionally, so they can't link without the asm backend yet.
if(NOT ASMFIBHEAP_BUILD_ASM)
  set(ASMFIBHEAP_BUILD_APPS OFF CACHE BOOL "Disabled: requires ASMFIBHEAP_BUILD_ASM=ON (install nasm)" FORCE)
  set(ASMFIBHEAP_BUILD_TESTS OFF CACHE BOOL "Disabled: requires ASMFIBHEAP_BUILD_ASM=ON (install nasm)" FORCE)
endif()

# =========================================================
# 3. TOP-LEVEL-ONLY GLOBAL SETTINGS
# =========================================================
if(PROJECT_IS_TOP_LEVEL)
  if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
  endif()
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
  set(CMAKE_C_STANDARD 11)
  set(CMAKE_C_STANDARD_REQUIRED ON)
  set(CMAKE_C_EXTENSIONS OFF)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)

  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
endif()

# =========================================================
# 4. SHARED INTERFACE TARGETS
# =========================================================
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ProjectOptions)
include(ProjectSanitizers)
include(ProjectCoverage)

# =========================================================
# 5. LIBRARIES + APPS
# =========================================================
add_subdirectory(src)

if(ASMFIBHEAP_BUILD_APPS)
  add_subdirectory(apps)
endif()

if(ASMFIBHEAP_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
