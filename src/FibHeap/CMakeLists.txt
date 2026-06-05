add_library(FibHeap STATIC FibHeap.c FibHeap.h)
add_library(fibheap::FibHeap ALIAS FibHeap)

target_include_directories(FibHeap PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
)
target_link_libraries(FibHeap
  PUBLIC  fibheap::Stack
  PRIVATE project_options project_sanitizers project_coverage
)
