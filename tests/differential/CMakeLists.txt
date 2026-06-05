# The differential suite cross-checks the C and NASM implementations, so it
# can only be built when the assembly backend is present.
if(NOT ASMFIBHEAP_BUILD_ASM)
  message(STATUS "Differential tests skipped: ASMFIBHEAP_BUILD_ASM is OFF.")
  return()
endif()

add_gtest(DifferentialTests
  SOURCES diff_test.cpp
  LIBRARIES
    fibheap::FibHeap
    fibheap::ASM_FibHeap
    project_options
    project_sanitizers
    project_coverage
  PROPERTIES TIMEOUT 120
)
