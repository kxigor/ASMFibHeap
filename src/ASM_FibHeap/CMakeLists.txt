# NASM ELF64 build. The asm calls extern stackCtor/Dtor/Push/Pop/Top/Size
# from libStack, so consumers must link against fibheap::Stack as well.
add_library(ASM_FibHeap STATIC ASM_FibHeap.asm ASM_FibHeap.h)
add_library(fibheap::ASM_FibHeap ALIAS ASM_FibHeap)

target_include_directories(ASM_FibHeap PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
)
target_link_libraries(ASM_FibHeap PUBLIC fibheap::Stack)
