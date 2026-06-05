add_library(Graph STATIC Graph.c Graph.h)
add_library(fibheap::Graph ALIAS Graph)

target_include_directories(Graph PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
)
target_link_libraries(Graph
  PUBLIC  fibheap::FibHeap
  PRIVATE project_options project_sanitizers project_coverage
)
