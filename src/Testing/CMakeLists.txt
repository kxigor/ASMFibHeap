# Legacy testing harness. Contains testing() and ASM_testing() entry points
# that the main.cpp driver calls. Not a unit-test framework.
add_library(FibHeapTesting STATIC
  FibHeapTesting/FibHeapTesting.cpp
  FibHeapTesting/FibHeapTesting.hpp
  GlobalHeaderTesting.h
  ImprovedVector/ImprovedVector.hpp
)
add_library(fibheap::testing ALIAS FibHeapTesting)

target_include_directories(FibHeapTesting PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/Testing>
)
target_link_libraries(FibHeapTesting
  PUBLIC
    fibheap::FibHeap
    fibheap::Graph
    fibheap::Stack
    fibheap::ASM_FibHeap
  PRIVATE
    project_options
    project_sanitizers
    project_coverage
)
