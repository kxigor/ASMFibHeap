# Interactive demo for the assembly Fibonacci heap. Renders the heap to
# fib_heap.png via the Graph exporter after each change.
add_executable(asm_fib_heap main.cpp)

target_link_libraries(asm_fib_heap PRIVATE
  fibheap::ASM_FibHeap
  fibheap::Graph
  project_options
  project_sanitizers
  project_coverage
)
