add_gtest(GraphTests
  SOURCES graph_test.cpp
  LIBRARIES
    fibheap::Graph
    fibheap::FibHeap
    project_options
    project_sanitizers
    project_coverage
)
