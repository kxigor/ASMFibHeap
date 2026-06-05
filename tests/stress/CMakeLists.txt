# The stress suite runs both the C and the NASM heap at scale, so it requires
# the assembly backend.
if(NOT ASMFIBHEAP_BUILD_ASM)
  message(STATUS "Stress tests skipped: ASMFIBHEAP_BUILD_ASM is OFF.")
  return()
endif()

add_gtest(StressTests
  SOURCES stress_test.cpp
  LIBRARIES
    fibheap::FibHeap
    fibheap::ASM_FibHeap
    project_options
    project_sanitizers
    project_coverage
  PROPERTIES TIMEOUT 300
)
