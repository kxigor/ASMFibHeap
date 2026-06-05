add_gtest(FibHeapTests
  SOURCES fibheap_test.cpp
  LIBRARIES
    fibheap::FibHeap
    project_options
    project_sanitizers
    project_coverage
)
