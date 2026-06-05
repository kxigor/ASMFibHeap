# cmake/ProjectCoverage.cmake

# ========================================
# 1. CODE COVERAGE ARCHITECTURE SETUP
# ========================================
# This module defines the architecture necessary for measuring code coverage
# using GCC/Clang (GCOV/LCOV format).
# 
# It creates the 'project_coverage' INTERFACE library to conditionally apply
# coverage flags only when the ASMFIBHEAP_ENABLE_COVERAGE option is ON. This allows
# linking against 'project_coverage' without affecting the build when coverage
# is disabled (the library becomes a no-op).

include_guard()

# Defines an interface target to carry coverage properties.
add_library(project_coverage INTERFACE)

# ========================================
# 2. CONDITIONAL FLAG APPLICATION & TOOLS
# ========================================
if(ASMFIBHEAP_ENABLE_COVERAGE)
  # Use the standard GCC/Clang flag which enables both instrumentation and 
  # branch coverage features (-fprofile-arcs, -ftest-coverage).
  set(COVERAGE_FLAGS "--coverage")

  # Apply compilation flags:
  # The flags are applied to both C and C++ for GNU/Clang. ASM (NASM) has no
  # gcov instrumentation; runtime support libs come through the C/C++ link.
  target_compile_options(project_coverage INTERFACE
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:${COVERAGE_FLAGS}>
    $<$<COMPILE_LANG_AND_ID:C,GNU,Clang>:${COVERAGE_FLAGS}>
  )

  # Apply linking flags (required to link GCOV libraries):
  target_link_options(project_coverage INTERFACE
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:${COVERAGE_FLAGS}>
    $<$<COMPILE_LANG_AND_ID:C,GNU,Clang>:${COVERAGE_FLAGS}>
  )

  # ========================================
  # 3. REPORT TOOL DISCOVERY
  # ========================================
  # gcovr is preferred: lcov 2.0 mis-parses the .gcda format emitted by
  # recent GCC (14+), silently reporting bogus rates, whereas gcovr stays in
  # lock-step with the toolchain. We accept either a gcovr on PATH or the
  # `python3 -m gcovr` module form, then fall back to lcov only if neither is
  # available.
  set(GCOVR_COMMAND "")
  find_program(GCOVR_EXECUTABLE NAMES gcovr)
  if(GCOVR_EXECUTABLE)
    set(GCOVR_COMMAND ${GCOVR_EXECUTABLE})
  else()
    find_program(PYTHON3_EXECUTABLE NAMES python3)
    if(PYTHON3_EXECUTABLE)
      execute_process(
        COMMAND ${PYTHON3_EXECUTABLE} -m gcovr --version
        RESULT_VARIABLE _gcovr_module_rc
        OUTPUT_QUIET ERROR_QUIET)
      if(_gcovr_module_rc EQUAL 0)
        set(GCOVR_COMMAND ${PYTHON3_EXECUTABLE} -m gcovr)
      endif()
    endif()
  endif()

  set(COVERAGE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/coverage_report")
  # The library set under test is selected by gcovr.cfg's `filter` entries
  # (kept in a config file so their regex metacharacters never hit the shell).
  set(GCOVR_CONFIG "${PROJECT_SOURCE_DIR}/gcovr.cfg")

  # ========================================
  # 4. CUSTOM `coverage` TARGET
  # ========================================
  if(GCOVR_COMMAND)
    message(STATUS "Coverage reporting via gcovr: ${GCOVR_COMMAND}")
    add_custom_target(coverage
      COMMAND ${CMAKE_COMMAND} -E make_directory ${COVERAGE_OUTPUT_DIR}
      COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --test-dir ${CMAKE_BINARY_DIR}
      COMMAND ${GCOVR_COMMAND}
              --root ${PROJECT_SOURCE_DIR}
              --config ${GCOVR_CONFIG}
              ${CMAKE_BINARY_DIR}
              --txt
              --html-details ${COVERAGE_OUTPUT_DIR}/index.html
              --print-summary
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      COMMENT "Running tests and generating gcovr coverage report in ${COVERAGE_OUTPUT_DIR}"
      USES_TERMINAL
    )

    # Same report, but fail the build if line coverage drops below 100%.
    # `assert()` and allocation-failure branches are intentionally excluded
    # from the gate — they are uncoverable without fault injection — so the
    # bar is line coverage only.
    add_custom_target(coverage-check
      COMMAND ${CMAKE_COMMAND} -E make_directory ${COVERAGE_OUTPUT_DIR}
      COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --test-dir ${CMAKE_BINARY_DIR}
      COMMAND ${GCOVR_COMMAND}
              --root ${PROJECT_SOURCE_DIR}
              --config ${GCOVR_CONFIG}
              ${CMAKE_BINARY_DIR}
              --txt
              --print-summary
              --fail-under-line 100
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      COMMENT "Verifying 100% line coverage of the core libraries"
      USES_TERMINAL
    )
  else()
    # Fallback: lcov/genhtml. Known to be unreliable on GCC >= 14; the
    # --ignore-errors flags keep it from aborting outright.
    find_program(LCOV_EXECUTABLE NAMES lcov)
    find_program(GENHTML_EXECUTABLE NAMES genhtml)
    if(LCOV_EXECUTABLE AND GENHTML_EXECUTABLE)
      message(STATUS "gcovr not found; falling back to lcov (may misreport on recent GCC).")
      set(LCOV_INFO_FILE "${COVERAGE_OUTPUT_DIR}/coverage.info")
      set(_lcov_ignore --ignore-errors mismatch,gcov,source,negative,unused,empty)
      add_custom_target(coverage
        COMMAND ${CMAKE_COMMAND} -E make_directory ${COVERAGE_OUTPUT_DIR}
        COMMAND ${LCOV_EXECUTABLE} --zerocounters --directory ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --test-dir ${CMAKE_BINARY_DIR}
        COMMAND ${LCOV_EXECUTABLE} --capture --directory ${CMAKE_BINARY_DIR}
                --output-file ${LCOV_INFO_FILE} ${_lcov_ignore}
        COMMAND ${LCOV_EXECUTABLE} --remove ${LCOV_INFO_FILE} '/usr/*' '*/tests/*'
                -o ${LCOV_INFO_FILE} ${_lcov_ignore}
        COMMAND ${GENHTML_EXECUTABLE} ${LCOV_INFO_FILE}
                --output-directory ${COVERAGE_OUTPUT_DIR} ${_lcov_ignore}
        COMMENT "Running tests and generating LCOV HTML report in ${COVERAGE_OUTPUT_DIR}"
        USES_TERMINAL
      )
    else()
      message(WARNING "Neither gcovr nor lcov/genhtml found. Coverage target skipped.")
    endif()
  endif()
endif()