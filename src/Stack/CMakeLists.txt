add_library(Stack STATIC Stack.c Stack.h)
add_library(fibheap::Stack ALIAS Stack)

target_include_directories(Stack PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
)
target_link_libraries(Stack PRIVATE project_options project_sanitizers project_coverage)
