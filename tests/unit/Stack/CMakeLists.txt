add_gtest(StackTests
  SOURCES stack_test.cpp
  LIBRARIES
    fibheap::Stack
    project_options
    project_sanitizers
    project_coverage
)
